#include "Otter/Util/LinkedList.h"

void linked_list_push(void* data, LinkedList** head)
{
  LinkedList* newNode = malloc(sizeof(LinkedList));
  if (newNode == NULL)
  {
    fprintf(stderr, "WARN: Unable to allocate a new linked list node.");
    return;
  }

  newNode->data = data;
  newNode->next = *head;
  *head         = newNode;
}

void linked_list_clear(LinkedListCb callback, LinkedList** head, void* userData)
{
  LinkedList* node = *head;
  while (node != NULL)
  {
    callback(node->data, userData);
    LinkedList* destroyNode = node;
    node                    = node->next;
    free(destroyNode);
  }
  *head = NULL;
}

void linked_list_for_each(
    LinkedListCb callback, LinkedList* head, void* userData)
{
  while (head != NULL)
  {
    callback(head->data, userData);
    head = head->next;
  }
}
