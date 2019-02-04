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

object_list * 
object_list_new (int n, unsigned item_size)
{
  object_list * list = (object_list*) malloc(sizeof(object_list));
  list->item_size = item_size;
  list->size = n;
  list->count = 0;
  /* can I find a way to do this with expand? */
  list->items = malloc(n*sizeof(void*));
  memset(list->items, 0, list->size*sizeof(void*)); 
  list->data = malloc(n*item_size);
  list->ops = NULL;
  return list;
}

object_list * 
object_list_duplicate (object_list * list)
{
  object_list * new_list;
  int i;

  /* can't duplicate a null pointer*/
  if (!list) return 0;

  new_list = object_list_new(list->size, list->item_size);
  for (i=0; i< list->count; i++) 
    object_list_append(new_list, object_list_get_item(list, i));
  return new_list;
}

void 
object_list_delete (object_list * list)
{
  object_list_clear(list);
  free(list->items);
  free(list->data);
  free(list);
}

int 
object_list_clear (object_list * list)
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
    memset(list->data, 0, list->size*list->item_size);
  }
  memset(list->items, 0, list->size*sizeof(void*));
  list->count = 0;
  return 0; /* success */
}

int
object_list_expand (object_list * list, int n)
{
  void * new_data = realloc(list->data, (list->size + n)*list->item_size);
  void ** new_items = realloc(list->items, (list->size + n)*sizeof(void*));
  int i=0;
  DBG_MSG("Expanding list by %d\n", n);
  if (new_data) list->data = new_data;
  else printf("[object list] Could not reallocate data memory!\n");
  if (new_items) list->items = new_items;
  else printf("[object list] Could not reallocate item vector memory!\n");
  list->size += n;
  for (i=0; i < list->size; i++)
  {
    if (i < list->count) list->items[i] = object_list_position_pointer(list, i);
    else list->items[i] = 0;
  }
  return 0; /* success */
}

void 
object_list_describe (object_list * list)
{
  printf("object list at %p has %d / %d items of size %d bytes\n",
         list, list->count, list->size, list->item_size);
}

int 
object_list_insert (object_list * list, int n, void * item)
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

  /* Update the pointer list */  
  list->items[list->count] = object_list_position_pointer(list, list->count);

  /* increment the list count */
  list->count++;
  return 0; /* success */
}

int 
object_list_remove(object_list * list, int n)
{
  int nItemsToMove;
  void * nptr, *temp;
  if(n >= list->count) return -1; // object not in list
  
  /* move all items after the specified position up one position */
  nItemsToMove = list->count - n - 1;
  nptr = object_list_get_item(list, n);
  if (list->ops && list->ops->clear_object){
    DBG_MSG("Clearing with clear operation object\n");
    list->ops->clear_object(nptr);
  } 
  /* no point in a memcpy condition since we're about to overwrite it anyway */
  
  /* Copying to an intermediate location appears to be necessary for 32-bit
   * platforms.
   *
   * TODO: Some profiling should be done to see if it's faster to do the
   *       malloc/free move, or to execute a for loop an move one item at a
   *       time, which would not require a memory allocation.
   */
  temp = malloc(list->item_size*nItemsToMove);
  memcpy(temp, nptr+list->item_size, list->item_size*nItemsToMove);
  memcpy(nptr, temp, list->item_size*nItemsToMove);
  free(temp);
  
  /* decrement the list count */
  list->count--;

  /* Update the pointer list */
  list->items[list->count] = 0;
  return 0; /* success */
}

int 
object_list_append (object_list * list, void * item)
{
  DBG_MSG("Appending to list... item %d\n", list->count);
  return object_list_insert (list, list->count, item);
}

/* would be nice to allow for negative indexing */
void * 
object_list_get_item (object_list * list, int n)
{
  if (n >= list->count) return 0; /* not a valid item */
  return object_list_position_pointer (list, n);
}

static void * 
object_list_position_pointer (object_list * list, int n)
{
  if (n >= list->size) return 0; /* position not in list */
  return list->data + list->item_size*n;
}

void * object_list_find_item (object_list * list, void * item)
{
  void * list_item;
  int i;
  /* We have to have a compare operator to do the testing.
   * */
  if (list->ops == 0) return 0;
  if (list->ops->compare_objects == 0) return 0;

  for (i=0; i < list->count; i++)
  {
    list_item = object_list_get_item(list, i);
    if (list->ops->compare_objects(item, list_item) == 0) return list_item;
  }

  /* Item not in the list */
  return 0;
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

int compare_somestructs(void *a, void *b)
{
  somestruct *aa = (somestruct*)a, *bb = (somestruct*)b;
  if(strncmp(aa->string, bb->string, 16) != 0) return -1;
  if(aa->n != bb->n) return  1;
  return 0;
}

object_operations somestruct_opts = {
  .copy_object = &copy_somestruct,
  .clear_object = &clear_somestruct,
  .compare_objects = &compare_somestructs
};

/* 
 * check_item is used to validate that a particular item in the list matches
 * the data of the original item. As a side-effect it also tests
 * object_list_get_item.
 *
 * Note that this is a define rather than a function so that failures are
 * easier to locate.
 * */

#define check_item(l, item, n) \
          g_assert_cmpint(\
              compare_somestructs(&item, object_list_get_item(l, n)), \
              ==, 0); \
          g_assert_cmpint(\
              (gint64) object_list_get_item(l, n), ==, (gint64) l->items[n]);


void object_list_test(void)
{
  int ss_size = sizeof(somestruct);
  somestruct a={"A", 1}, b={"B", 2}, c={"C", 3}, 
			 d={"D", 4}, e={"E", 5}, f={"F", 6};
  somestruct x={"X", 24}, y={"Y", 25}, z={"Z", 26};
  somestruct * p;
  object_list *list, *dup_list;

  /*
   * First test our operations to make sure they work as advertised.
   */

  /* test clearing */
  clear_somestruct(&z);
  g_assert_cmpint(z.n, ==, 0);
  g_assert_cmpint(strlen(z.string), ==, 0);

  /* test copying */
  copy_somestruct(&z, &y);
  g_assert_cmpint(z.n, ==, y.n);
  g_assert_cmpint(strncmp(z.string, y.string, 16), ==, 0);

  /* test comparing */
  /* should be equal by previous test */
  g_assert_cmpint(compare_somestructs(&y, &z), ==, 0);
  /* should be unequal */
  g_assert_cmpint(compare_somestructs(&x, &z), !=, 0);

  /*
   * Okay, the operations are good to go. Now onto the object_list stuff.
   */

  list = object_list_new(2, ss_size);
  g_assert_cmpint(list->size, ==, 2);
  g_assert_cmpint(list->count, ==, 0);
  g_assert_cmpint(list->item_size, ==, sizeof(somestruct));
  /* 0: (null), 1: (null)
   */
  
  /*
   * Duplicate a null pointer
   */
  g_assert_cmpint((gint64) object_list_duplicate(0), ==, 0);

  /*
   * Duplicating an empty list
   */
  dup_list = object_list_duplicate(list);
  g_assert_cmpint(dup_list->size, ==, 2);
  g_assert_cmpint(dup_list->count, ==, 0);
  g_assert_cmpint(dup_list->item_size, ==, sizeof(somestruct));

  /*
   * Delete an empty list
   */
  object_list_delete(dup_list);

  /*
   * Insertion tests
   */
  
  /* Appending object 'c' to list */
  object_list_append(list, &c);
  /* 0: c, 1: (null) */
  check_item(list, c, 0);
  /* Keeping this long form as a reminder of how it's done */
  //g_assert_cmpmem(&c, ss_size, object_list_get_item(list, 0), ss_size);
  g_assert_cmpint(list->size, ==, 2);
  g_assert_cmpint(list->count, ==, 1);
  
  /* Inserting object 'a' at position 0 (initial position) */
  object_list_insert(list, 0, &a);
  /* 0: a, 1: c */
  check_item(list, a, 0);
  check_item(list, c, 1);
  g_assert_cmpint(list->size, ==, 2);
  g_assert_cmpint(list->count, ==, 2);
 
  /* Inserting object 'd' at position 2 (append position) */
  object_list_insert(list, 2, &d);
  /* 0: a, 1: c, 2: d */
  check_item(list, a, 0);
  check_item(list, c, 1);
  check_item(list, d, 2);
  g_assert_cmpint(list->size, ==, 3);
  g_assert_cmpint(list->count, ==, 3);

  /* Inserting object 'b' at position 1 (middle position) */
  object_list_insert(list, 1, &b);
  /* 0: a, 1: b, 2: c, 3: d */
  check_item(list, a, 0);
  check_item(list, b, 1);
  check_item(list, c, 2);
  check_item(list, d, 3);
  g_assert_cmpint(list->size, ==, 4);
  g_assert_cmpint(list->count, ==, 4);
 
  /* Inserting object 'f' at position 5 (past list, fails) */
  object_list_insert(list, 5, &f);
  /* 0: a, 1: b, 2: c, 3: d */
  check_item(list, a, 0);
  check_item(list, b, 1);
  check_item(list, c, 2);
  check_item(list, d, 3);
  g_assert_cmpint(list->size, ==, 4);
  g_assert_cmpint(list->count, ==, 4);

  /* Appending object 'e' and expanding */
  object_list_append(list, &e);
  /* 0: a, 1: b, 2: c, 3: d, 4: e*/
  check_item(list, a, 0);
  check_item(list, b, 1);
  check_item(list, c, 2);
  check_item(list, d, 3);
  check_item(list, e, 4);
  g_assert_cmpint(list->size, ==, 5);
  g_assert_cmpint(list->count, ==, 5);

  /*
   * Check that find item returns zero, since there are no object operations
   * yet.
   */

  g_assert_cmpint((gint64) object_list_find_item(list, &c), ==, 0);

  /*
   * Duplicate a list with stuff in it
   */ 

  dup_list = object_list_duplicate(list);
  /* 0: a, 1: b, 2: c, 3: d, 4: e*/
  check_item(dup_list, a, 0);
  check_item(dup_list, b, 1);
  check_item(dup_list, c, 2);
  check_item(dup_list, d, 3);
  check_item(dup_list, e, 4);
  g_assert_cmpint(dup_list->size, ==, 5);
  g_assert_cmpint(dup_list->count, ==, 5);

  /*
   * Delete a list with stuff in it
   */

  object_list_delete(dup_list);

  /*
   * Removal tests
   */
  
  /* Removing object at position 0 (initial position) */
  object_list_remove(list, 0);
  /* 0: b, 1: c, 2: d, 3: e, 4: (null) */
  check_item(list, b, 0);
  check_item(list, c, 1);
  check_item(list, d, 2);
  check_item(list, e, 3);
  g_assert_cmpint(list->size, ==, 5);
  g_assert_cmpint(list->count, ==, 4);

  /* Removing object at position 1 (middle position) */
  object_list_remove(list, 1);
  /* 0: b, 1: d, 2: e, 3: (null), 4: (null) */
  check_item(list, b, 0);
  check_item(list, d, 1);
  check_item(list, e, 2);
  g_assert_cmpint(list->size, ==, 5);
  g_assert_cmpint(list->count, ==, 3);

  /* Removing object at position 2 (final position) */
  object_list_remove(list, 2);
  /* 0: b, 1: d, 2: (null), 3: (null), 4: (null) */
  check_item(list, b, 0);
  check_item(list, d, 1);
  g_assert_cmpint(list->size, ==, 5);
  g_assert_cmpint(list->count, ==, 2);

  /* Removing object at position 2 (no item) */
  object_list_remove(list, 2);
  /* 0: b, 1: d, 2: (null), 3: (null), 4: (null) */
  check_item(list, b, 0);
  check_item(list, d, 1);
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
  check_item(list, a, 0);
  check_item(list, b, 1);
  check_item(list, c, 2);
  g_assert_cmpint(list->size, ==, 5);
  g_assert_cmpint(list->count, ==, 3);
  
  /* test use of clear_object */
  /* To do this properly, I would really need to have a pointer to allocated
   * memory in somestruct, which the clear_object function could free. I'd
   * then have to find a way to test whether or not that memory still
   * belonged to me. */
  object_list_remove(list, 1);
  /* 0: a, 1: c, 2: (null), 3: (null), 4: (null) */
  check_item(list, a, 0);
  check_item(list, c, 1);
  g_assert_cmpint(list->size, ==, 5);
  g_assert_cmpint(list->count, ==, 2);

  /*
   * Test object_list_find_item
   */

  /* Make the list a little larger. */
  object_list_append(list, &e);
  /* Include a duplicate in the list (we'll never find it). */
  object_list_append(list, &c);
  /* To test finding the last object, last object must be unique */
  object_list_append(list, &f);
  /* 0: a, 1: c, 2: e, 3: c, 4: f */

  /* Find the first item */
  p = object_list_find_item(list, &a);
  g_assert_cmpint (compare_somestructs (p, &a), ==, 0);
  g_assert_cmpint ((gint64) p, ==, (gint64) list->items[0]);

  /* Find a middle item */
  p = object_list_find_item(list, &c);
  g_assert_cmpint (compare_somestructs (p, &c), ==, 0);
  g_assert_cmpint ((gint64) p, ==, (gint64) list->items[1]);

  /* Find the last item */
  p = object_list_find_item(list, &f);
  g_assert_cmpint (compare_somestructs (p, &f), ==, 0);
  g_assert_cmpint ((gint64) p, ==, (gint64) list->items[4]);

  /* Find an item not in the list */
  p = object_list_find_item(list, &d);
  g_assert_cmpint ((gint64) p, ==, 0);


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
