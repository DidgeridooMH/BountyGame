#include "Otter/Util/Heap.h"

#include "Otter/Util/Array/AutoArray.h"

void heap_create(Heap* heap)
{
  auto_array_create(&heap->contents, sizeof(HeapElement));
}

static size_t heap_left_index(size_t index)
{
  return 2 * index + 1;
}

static size_t heap_right_index(size_t index)
{
  return 2 * index + 2;
}

static size_t heap_parent_index(size_t index)
{
  return (index - 1) / 2;
}

static int heap_get_key(Heap* heap, size_t index)
{
  return ((HeapElement*) auto_array_get(&heap->contents, index))->key;
}

static void heap_swap(Heap* heap, size_t index1, size_t index2)
{
  HeapElement temp = *(HeapElement*) auto_array_get(&heap->contents, index1);
  *(HeapElement*) auto_array_get(&heap->contents, index1) =
      *(HeapElement*) auto_array_get(&heap->contents, index2);
  *(HeapElement*) auto_array_get(&heap->contents, index2) = temp;
}

void heap_push(Heap* heap, uint32_t key, float value)
{
  HeapElement* element = auto_array_allocate(&heap->contents);
  element->key         = key;
  element->value       = value;

  size_t current = heap->contents.size - 1;
  while (current > 0
         && heap_get_key(heap, current)
                > heap_get_key(heap, heap_parent_index(current)))
  {
    {
      heap_swap(heap, current, heap_parent_index(current));
      current = heap_parent_index(current);
    }
  }
}

static bool heap_is_leaf(Heap* heap, size_t index)
{
  return index >= heap->contents.size / 2 && index <= heap->contents.size;
}

static void heap_heapify(Heap* heap, size_t index)
{
  while (
      !heap_is_leaf(heap, index)
      && (heap_get_key(heap, index) < heap_get_key(heap, heap_left_index(index))
          || heap_get_key(heap, index)
                 < heap_get_key(heap, heap_right_index(index))))
  {
    if (heap_get_key(heap, heap_left_index(index))
        > heap_get_key(heap, heap_right_index(index)))
    {
      heap_swap(heap, index, heap_left_index(index));
      index = heap_left_index(index);
    }
    else
    {
      heap_swap(heap, index, heap_right_index(index));
      index = heap_right_index(index);
    }
  }
}

void heap_pop(Heap* heap)
{
  heap_swap(heap, 0, heap->contents.size - 1);
  auto_array_pop(&heap->contents);
  heap_heapify(heap, 0);
}

void heap_top(Heap* heap, uint32_t* key, float* value)
{
  HeapElement* top = auto_array_get(&heap->contents, 0);
  *key             = top->key;
  *value           = top->value;
}

void heap_destroy(Heap* heap)
{
  auto_array_destroy(&heap->contents);
}
