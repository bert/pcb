//
//  object_list.c
//  
//
//  Created by Parker, Charles W. on 8/18/18.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//#include "global.h"
#if defined(DEBUG_OBJECT_LIST) //&& DEBUG > 0
#define DBG_MSG(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt, \
__FILE__, __LINE__, __func__, ##args)
#else
#define DBG_MSG(fmt, args...) /* Don't do anything in release builds */
#endif

#include "object_list.h"

static void * object_list_position_pointer(object_list * list, int n);

object_list * object_list_new(int n, unsigned item_size)
{
  object_list * list = (object_list*) malloc(sizeof(object_list));
  list->item_size = item_size;
  list->size = n;
  list->count = 0;
  /* can I find a way to do this with expand? */
  list->items = malloc(n*item_size);
  list->ops = NULL;
  return list;
}

void object_list_delete(object_list * list)
{
  object_list_clear(list);
  free(list);
}

int object_list_clear(object_list * list)
{
  int i=0;
  DBG_MSG("Clearing list\n");
  if (list->ops && list->ops->clear_object){
    DBG_MSG("Clearing with clear operation object\n");
    /* If there's a ops structure with a clear function, use it */
    for(i=0; i < list->count; i++){
      list->ops->clear_object(object_list_get_item(list, i));
    }
  } else {
    DBG_MSG("Clearing with memset\n");
    /* By default, just use memset to clear the item memory. */
    memset(list->items, 0, list->size*list->item_size);
  }
  list->count = 0;
  return 0; /* success */
}

int object_list_expand(object_list * list, int n)
{
  void * new_items = malloc((list->size + n)*list->item_size);
  DBG_MSG("Expanding list by %d\n", n);
  memcpy(new_items, list->items, list->size*list->item_size);
  list->size += n;
  free(list->items);
  list->items = new_items;
  return 0; /* success */
}

void object_list_describe(object_list * list)
{
  printf("object list at %p has %d / %d items of size %d bytes\n",
         list, list->count, list->size, list->item_size);
}

int object_list_insert(object_list * list, int n, void * item)
{
  void * nptr, * tmp;
  int nItemsToMove, result;
  if(n > list->count) return -1; // must be contiguous
  
  DBG_MSG("Inserting object at position %d\n", n);
  /* make sure we have enough room in the list for the new item */
  if (list->count == list->size){
    result = object_list_expand(list, 1);
    if (result < 0)
    {
      printf("Failed to expand list!\n");
      return -1;
    }
  }
  
  /* move the items to make room for the new one */
  nItemsToMove = list->count - n;
  nptr = object_list_position_pointer(list, n);
  if (nItemsToMove > 0)
  {
    /* copying forward, must use a temp */
    DBG_MSG("Need to move %d objects\n", nItemsToMove);
    tmp = malloc(list->item_size*nItemsToMove);
    memcpy(tmp, nptr, list->item_size*nItemsToMove);
    memcpy(nptr+list->item_size, tmp, list->item_size*nItemsToMove);
    free(tmp);
  }
  
  /* copy the data into the list */
  if (list->ops && list->ops->copy_object){
    DBG_MSG("Copying with copy operation object\n");
    list->ops->copy_object(nptr, item);
  } else {
    DBG_MSG("Copying with memcpy\n");
    memcpy(nptr, item, list->item_size);
  }
  
  /* increment the list count */
  list->count++;
  return 0; /* success */
}

int object_list_remove(object_list * list, int n)
{
  int nItemsToMove;
  void * nptr;
  if(n >= list->count) return -1; // object not in list
  
  /* move all items after the specified position up one position */
  nItemsToMove = list->count - n - 1;
  nptr = object_list_get_item(list, n);
  memcpy(nptr, nptr+list->item_size, list->item_size*nItemsToMove);
  
  /* decrement the list count */
  list->count--;
  return 0; /* success */
}

int object_list_append(object_list * list, void * item)
{
  DBG_MSG("Appending to list... item %d\n", list->count);
  return object_list_insert(list, list->count, item);
}

/* would be nice to allow for negative indexing */
void * object_list_get_item(object_list * list, int n)
{
  if (n >= list->count) return 0; /* not a valid item */
  return object_list_position_pointer(list, n);
}

static void * object_list_position_pointer(object_list * list, int n)
{
  if (n >= list->size) return 0; /* position not in list */
  return list->items + list->item_size*n;
}
