#include "Window/GameWindow.h"

#include <stdio.h>
#include <winuser.h>
#include <xinput.h>

#include "Input/InputMap.h"
#include "Otter/Util/AutoArray.h"

#define PLAYER_RECT_SIZE 50

static LRESULT CALLBACK game_window_process(
    HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
  AutoArray* inputs = (AutoArray*) GetWindowLongPtr(window, GWLP_USERDATA);

  switch (message)
  {
  case WM_CREATE:
    break;
  case WM_KEYDOWN:
    if (((lParam >> 16) & KF_REPEAT) == 0)
    {
      InputEvent* event    = auto_array_allocate(inputs);
      event->source.source = INPUT_TYPE_KEYBOARD;
      event->source.index  = wParam;
      event->value         = 1.0f;
    }
    break;
  case WM_KEYUP:
    {
      InputEvent* event    = auto_array_allocate(inputs);
      event->source.source = INPUT_TYPE_KEYBOARD;
      event->source.index  = wParam;
      event->value         = 0.0f;
    }
    break;
  case WM_LBUTTONDOWN:
    {
      InputEvent* event    = auto_array_allocate(inputs);
      event->source.source = INPUT_TYPE_MOUSE;
      event->source.index  = 0;
      event->value         = 1.0f;
    }
    break;
  case WM_LBUTTONUP:
    {
      InputEvent* event    = auto_array_allocate(inputs);
      event->source.source = INPUT_TYPE_MOUSE;
      event->source.index  = 0;
      event->value         = 0.0f;
    }
    break;
  case WM_RBUTTONDOWN:
    {
      InputEvent* event    = auto_array_allocate(inputs);
      event->source.source = INPUT_TYPE_MOUSE;
      event->source.index  = 1;
      event->value         = 1.0f;
    }
    break;
  case WM_RBUTTONUP:
    {
      InputEvent* event    = auto_array_allocate(inputs);
      event->source.source = INPUT_TYPE_MOUSE;
      event->source.index  = 1;
      event->value         = 0.0f;
    }
    break;
  case WM_MBUTTONDOWN:
    {
      InputEvent* event    = auto_array_allocate(inputs);
      event->source.source = INPUT_TYPE_MOUSE;
      event->source.index  = 2;
      event->value         = 1.0f;
    }
    break;
  case WM_MBUTTONUP:
    {
      InputEvent* event    = auto_array_allocate(inputs);
      event->source.source = INPUT_TYPE_MOUSE;
      event->source.index  = 2;
      event->value         = 0.0f;
    }
    break;
  case WM_XBUTTONDOWN:
    if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
    {
      InputEvent* event    = auto_array_allocate(inputs);
      event->source.source = INPUT_TYPE_MOUSE;
      event->source.index  = 3;
      event->value         = 1.0f;
    }
    else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2)
    {
      InputEvent* event    = auto_array_allocate(inputs);
      event->source.source = INPUT_TYPE_MOUSE;
      event->source.index  = 4;
      event->value         = 1.0f;
    }
    break;
  case WM_XBUTTONUP:
    if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
    {
      InputEvent* event    = auto_array_allocate(inputs);
      event->source.source = INPUT_TYPE_MOUSE;
      event->source.index  = 3;
      event->value         = 0.0f;
    }
    else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2)
    {
      InputEvent* event    = auto_array_allocate(inputs);
      event->source.source = INPUT_TYPE_MOUSE;
      event->source.index  = 4;
      event->value         = 0.0f;
    }
    break;
  case WM_ACTIVATEAPP:
    XInputEnable(wParam);
    break;
  case WM_CLOSE:
    DestroyWindow(window);
    break;
  case WM_DESTROY:
    auto_array_destroy(inputs);
    free(inputs);
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProc(window, message, wParam, lParam);
  }

  return 0;
}

HWND game_window_create(int width, int height, enum WindowMode windowMode)
{
  static const wchar_t CLASS_NAME[] = L"SampleWindowClass";

  SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

  HINSTANCE instance = GetModuleHandle(NULL);

  WNDCLASS wc      = {0};
  wc.style         = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc   = &game_window_process;
  wc.hInstance     = instance;
  wc.lpszClassName = CLASS_NAME;
  wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  RegisterClass(&wc);

  int windowFlags     = WS_OVERLAPPED;
  int windowPositionX = 0;
  int windowPositionY = 0;
  switch (windowMode)
  {
  case WM_WINDOWED:
    windowFlags |= WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    windowPositionX = CW_USEDEFAULT;
    windowPositionY = CW_USEDEFAULT;
    break;
  case WM_BORDERLESS_WINDOWED:
    windowFlags |= WS_POPUP;
    break;
  }

  HWND window = CreateWindowEx(0, CLASS_NAME, L"Bounty Game", windowFlags,
      windowPositionX, windowPositionY, width, height, NULL, NULL, instance,
      NULL);
  if (window == NULL)
  {
    return NULL;
  }

  AutoArray* inputs = malloc(sizeof(AutoArray));
  if (inputs == NULL)
  {
    fprintf(stderr, "Unable to create buffer for inputs\n");
    DestroyWindow(window);
    return NULL;
  }
  auto_array_create(inputs, sizeof(InputEvent));

  SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR) inputs);

  ShowWindow(window, SW_SHOWDEFAULT);
  UpdateWindow(window);

  return window;
}

void game_window_destroy(HWND window)
{
  DestroyWindow(window);
}

bool game_window_process_message(HWND window)
{
  // Clear out old input events.
  auto_array_clear((AutoArray*) GetWindowLongPtr(window, GWLP_USERDATA));

  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.message == WM_QUIT;
}
 
