#include "Otter/Async/Scheduler.h"

typedef struct TaskData
{
  void* userData;
  enum TaskFlags flags;
  HANDLE completionHandle;
  TaskFunction function;
  struct TaskData* next;
} TaskData;

typedef struct ThreadData
{
  HANDLE functionReady;
  HANDLE endThread;
  HANDLE threadIdle;
  TaskData* taskData;
} ThreadData;

static HANDLE g_endOfProcess;
static HANDLE g_schedulerThread;
static CRITICAL_SECTION g_taskQueueLock;
static TaskData* g_taskQueueHead;
static TaskData* g_taskQueueTail;

static TaskData* task_scheduler_dequeue()
{
  TaskData* taskData = NULL;
  EnterCriticalSection(&g_taskQueueLock);
  if (g_taskQueueHead != NULL)
  {
    taskData        = g_taskQueueHead;
    g_taskQueueHead = g_taskQueueHead->next;
    if (g_taskQueueHead == NULL)
    {
      g_taskQueueTail = NULL;
    }
  }
  LeaveCriticalSection(&g_taskQueueLock);
  return taskData;
}

static DWORD WINAPI task_process(ThreadData* threadData)
{
  HANDLE eventHandles[] = {threadData->endThread, threadData->functionReady};
  while (true)
  {
    SetEvent(threadData->threadIdle);

    if (WaitForMultipleObjects(
            _countof(eventHandles), eventHandles, false, INFINITE)
        == WAIT_OBJECT_0)
    {
      break;
    }

    threadData->taskData->function(threadData->taskData->userData);
    if (threadData->taskData->completionHandle != NULL)
    {
      SetEvent(threadData->taskData->completionHandle);
    }

    if (threadData->taskData->flags & TASK_FLAGS_FREE_DATA_ON_COMPLETE)
    {
      free(threadData->taskData->userData);
    }

    free(threadData->taskData);
    threadData->taskData = NULL;
  }

  return 0;
}

static DWORD WINAPI task_scheduler(void* unused)
{
  HANDLE threadEvent[TASK_SCHEDULER_THREADS + 1] = {0};
  threadEvent[0]                                 = g_endOfProcess;

  ThreadData threadData[TASK_SCHEDULER_THREADS] = {0};
  HANDLE threadHandles[TASK_SCHEDULER_THREADS]  = {0};
  for (int i = 0; i < TASK_SCHEDULER_THREADS; i++)
  {
    threadData[i].functionReady = CreateEvent(NULL, false, false, NULL);
    threadData[i].endThread     = g_endOfProcess;
    threadData[i].threadIdle    = CreateEvent(NULL, true, false, NULL);

    threadEvent[i + 1] = threadData[i].threadIdle;
    threadHandles[i]   = CreateThread(NULL, 0,
          (LPTHREAD_START_ROUTINE) task_process, &threadData[i], 0, NULL);
  }

  while (true)
  {
    DWORD eventId = WaitForMultipleObjects(
        TASK_SCHEDULER_THREADS + 1, threadEvent, false, INFINITE);
    if (eventId == WAIT_OBJECT_0)
    {
      break;
    }

    TaskData* taskData = task_scheduler_dequeue();
    if (taskData != NULL)
    {
      threadData[eventId - 1].taskData = taskData;
      ResetEvent(threadEvent[eventId]);
      SetEvent(threadData[eventId - 1].functionReady);
    }
  }

  WaitForMultipleObjects(TASK_SCHEDULER_THREADS, threadHandles, true, 15000);

  for (int i = 0; i < TASK_SCHEDULER_THREADS; i++)
  {
    CloseHandle(threadData[i].functionReady);
    CloseHandle(threadData[i].threadIdle);
    CloseHandle(threadHandles[i]);
  }

  return 0;
}

void task_scheduler_init()
{
  InitializeCriticalSection(&g_taskQueueLock);
  g_endOfProcess    = CreateEvent(NULL, true, false, NULL);
  g_schedulerThread = CreateThread(NULL, 0, task_scheduler, NULL, 0, NULL);
}

void task_scheduler_destroy()
{
  SetEvent(g_endOfProcess);
  WaitForSingleObject(g_schedulerThread, 30000);
  CloseHandle(g_endOfProcess);
  CloseHandle(g_schedulerThread);
  DeleteCriticalSection(&g_taskQueueLock);
}

HANDLE task_scheduler_enqueue(
    TaskFunction function, void* data, enum TaskFlags flags)
{
  TaskData* taskData = malloc(sizeof(TaskData));
  if (taskData == NULL)
  {
    return NULL;
  }

  taskData->function         = function;
  taskData->userData         = data;
  taskData->flags            = flags;
  taskData->completionHandle = CreateEvent(NULL, true, false, NULL);
  taskData->next             = NULL;

  EnterCriticalSection(&g_taskQueueLock);
  if (g_taskQueueHead != NULL)
  {
    g_taskQueueTail->next = taskData;
    g_taskQueueTail       = taskData;
  }
  else
  {
    g_taskQueueHead = taskData;
    g_taskQueueTail = taskData;
  }
  LeaveCriticalSection(&g_taskQueueLock);

  return taskData->completionHandle;
}
