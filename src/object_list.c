/*
 * \file src/object_list.c
 *
 * \brief object_list core code
 *
 * See src/object_list.h for details.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2018 Charles Parker <parker.charles@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

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
  if (list->ops && list->ops->clear_object){
    DBG_MSG("Clearing with clear operation object\n");
    list->ops->clear_object(nptr);
  } 
  /* no point in a memcpy condition since we're about to overwrite it anyway */

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

/*******************************************************************************
                                    Tests
*******************************************************************************/
#ifdef PCB_UNIT_TEST
#include <glib.h>

void
object_list_register_tests(void)
{
  g_test_add_func("/object-list/test", object_list_test);
}

typedef struct somestruct {
    char string[16];
    int n;
} somestruct;

void print_somestruct(somestruct * s)
{
    if(s > 0) printf("%d, %s\n", s->n, s->string);
    else printf("(null)\n");
}

void copy_somestruct(void * a, void * b)
{
  somestruct *aa = (somestruct*)a, *bb = (somestruct*)b;
  memcpy(aa->string, bb->string, 16);
  aa->n = bb->n;
}

void clear_somestruct(void * a)
{
  somestruct *aa = (somestruct*)a;
  memset(aa->string, 0, 16);
  aa->n = 0;
}

object_operations somestruct_opts = {
  .copy_object = &copy_somestruct,
  .clear_object = &clear_somestruct
};

#define check_item(item, n) \
          g_assert_cmpmem(&item, ss_size, object_list_get_item(list, n), ss_size);

void object_list_test(void)
{
  int ss_size = sizeof(somestruct);
  somestruct a={"A", 1}, b={"B", 2}, c={"C", 3}, 
			 d={"D", 4}, e={"E", 5}, f={"F", 6};
  object_list * list;
 
  list = object_list_new(2, ss_size);
  g_assert_cmpint(list->size, ==, 2);
  g_assert_cmpint(list->count, ==, 0);
  g_assert_cmpint(list->item_size, ==, sizeof(somestruct));
  /* 0: (null), 1: (null)
   */
  
  /*
   * Insertion tests
   */
  
  /* Appending object 'c' to list */
  object_list_append(list, &c);
  /* 0: c, 1: (null) */
  check_item(c, 0);
  /* Keeping this long form as a reminder of how it's done */
  //g_assert_cmpmem(&c, ss_size, object_list_get_item(list, 0), ss_size);
  g_assert_cmpint(list->size, ==, 2);
  g_assert_cmpint(list->count, ==, 1);
  
  /* Inserting object 'a' at position 0 (initial position) */
  object_list_insert(list, 0, &a);
  /* 0: a, 1: c */
  check_item(a, 0);
  check_item(c, 1);
  g_assert_cmpint(list->size, ==, 2);
  g_assert_cmpint(list->count, ==, 2);
 
  /* Inserting object 'd' at position 2 (append position) */
  object_list_insert(list, 2, &d);
  /* 0: a, 1: c, 2: d */
  check_item(a, 0);
  check_item(c, 1);
  check_item(d, 2);
  g_assert_cmpint(list->size, ==, 3);
  g_assert_cmpint(list->count, ==, 3);

  /* Inserting object 'b' at position 1 (middle position) */
  object_list_insert(list, 1, &b);
  /* 0: a, 1: b, 2: c, 3: d */
  check_item(a, 0);
  check_item(b, 1);
  check_item(c, 2);
  check_item(d, 3);
  g_assert_cmpint(list->size, ==, 4);
  g_assert_cmpint(list->count, ==, 4);
 
  /* Inserting object 'f' at position 5 (past list, fails) */
  object_list_insert(list, 5, &f);
  /* 0: a, 1: b, 2: c, 3: d */
  check_item(a, 0);
  check_item(b, 1);
  check_item(c, 2);
  check_item(d, 3);
  g_assert_cmpint(list->size, ==, 4);
  g_assert_cmpint(list->count, ==, 4);

  /* Appending object 'e' and expanding */
  object_list_append(list, &e);
  /* 0: a, 1: b, 2: c, 3: d, 4: e*/
  check_item(a, 0);
  check_item(b, 1);
  check_item(c, 2);
  check_item(d, 3);
  check_item(e, 4);
  g_assert_cmpint(list->size, ==, 5);
  g_assert_cmpint(list->count, ==, 5);

  /*
   * Removal tests
   */
  
  /* Removing object at position 0 (initial position) */
  object_list_remove(list, 0);
  /* 0: b, 1: c, 2: d, 3: e, 4: (null) */
  check_item(b, 0);
  check_item(c, 1);
  check_item(d, 2);
  check_item(e, 3);
  g_assert_cmpint(list->size, ==, 5);
  g_assert_cmpint(list->count, ==, 4);

  /* Removing object at position 1 (middle position) */
  object_list_remove(list, 1);
  /* 0: b, 1: d, 2: e, 3: (null), 4: (null) */
  check_item(b, 0);
  check_item(d, 1);
  check_item(e, 2);
  g_assert_cmpint(list->size, ==, 5);
  g_assert_cmpint(list->count, ==, 3);

  /* Removing object at position 2 (final position) */
  object_list_remove(list, 2);
  /* 0: b, 1: d, 2: (null), 3: (null), 4: (null) */
  check_item(b, 0);
  check_item(d, 1);
  g_assert_cmpint(list->size, ==, 5);
  g_assert_cmpint(list->count, ==, 2);

  /* Removing object at position 2 (no item) */
  object_list_remove(list, 2);
  /* 0: b, 1: d, 2: (null), 3: (null), 4: (null) */
  check_item(b, 0);
  check_item(d, 1);
  g_assert_cmpint(list->size, ==, 5);
  g_assert_cmpint(list->count, ==, 2);

  /* Clearing list */
  object_list_clear(list);
  /* 0: (null), 1: (null), 2: (null), 3: (null), 4: (null) */
  g_assert_cmpint(list->size, ==, 5);
  g_assert_cmpint(list->count, ==, 0);

  /*
   * Now do these things with the object operations instead of memcopy
   */
  list->ops = &somestruct_opts;

  /* test use of copy_object */
  object_list_append(list, &c);
  object_list_insert(list, 0, &a);
  object_list_insert(list, 1, &b);
  /* 0: a, 1: b, 2: c, 3: (null), 4: (null) */
  check_item(a, 0);
  check_item(b, 1);
  check_item(c, 2);
  g_assert_cmpint(list->size, ==, 5);
  g_assert_cmpint(list->count, ==, 3);
  
  /* test use of clear_object */
  /* To do this properly, I would really need to have a pointer to allocated
   * memory in somestruct, which the clear_object function could free. I'd
   * then have to find a way to test whether or not that memory still
   * belonged to me. */
  object_list_remove(list, 1);
  /* 0: a, 1: c, 2: (null), 3: (null), 4: (null) */
  check_item(a, 0);
  check_item(c, 1);
  g_assert_cmpint(list->size, ==, 5);
  g_assert_cmpint(list->count, ==, 2);

  /* Clearing list */
  /* See earlier comment. The same applies here. */
  object_list_clear(list);
  /* 0: (null), 1: (null), 2: (null), 3: (null), 4: (null) */
  g_assert_cmpint(list->size, ==, 5);
  g_assert_cmpint(list->count, ==, 0);

  /* Deleting list */
  /* See earlier comment. The same applies here. */
  object_list_delete(list);
}

#endif /* PCB_UNIT_TEST */
