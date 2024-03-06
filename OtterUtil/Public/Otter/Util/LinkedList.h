#pragma once

#include "Otter/Util/export.h"

typedef struct LinkedList
{
  struct LinkedList* next;
  void* data;
} LinkedList;

typedef void (*LinkedListCb)(void* data, void* userData);

OTTERUTIL_API void linked_list_push(void* data, LinkedList** head);
OTTERUTIL_API void linked_list_clear(
    LinkedListCb callback, LinkedList** head, void* userData);
OTTERUTIL_API void linked_list_for_each(
    LinkedListCb callback, LinkedList* head, void* userData);
