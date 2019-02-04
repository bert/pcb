/*!
 * \file src/object_list.h
 *
 * \brief object_list header
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
 * <hr>
 *
 * This is a container class for keeping lists of any type of object. It uses
 * two allocations of memory. The first keeps track of the list info, e.g.
 * number of objects in the list, number of objects the list can hold, size of
 * each object, etc. The second allocation contains the object data themselves.
 *
 * When an object is added to the list, the list makes its own copy of the data.
 * The list does not take ownership of the original data, so, the caller is
 * still responsible for the original object. Providing an option where the list
 * takes ownership of the original object is perhaps an improvement for the
 * future.
 *
 * For complex objects, objects that don't contain all of their data in one
 * place, but have pointers to other areas of memory, it is necessary to create
 * object operation functions. At a minimum, the copy_object and clear_object
 * functions, as these are used to make the list's copy of the object and move
 * its copies around.
 *
 * To do:
 *  * Allow the list to store objects anywhere instead of forcing a
 *    contiguous memory allocation. 
 * 
 *    The list would store pointers to the objects which could reside
 *    anywhere in memory. This would remove a lot of the reallocating of
 *    memory since we'd basically have a list of pointers, which are much
 *    smaller.
 *
 *    This should be an option that can be specified, but not required.
 *
 *  * Allow the list to take ownership of objects.
 *
 *    The list would not allocate memory and copy the object in this case,
 *    but it would free the objects memory when removed from the list. 
 *
 *    A fun case would be the mixed case in which the list owned some objects 
 *    but not others. Let's not go there for a while.
 *
 */

#ifndef object_list_h
#define object_list_h

/*!
 * \brief Object operations structure.
 *
 * This is a virtuial function table of functions that operate on whatever type
 * of object the list happens to contain. They must be written for any complex
 * object type this class stores.
 */
typedef struct object_operations
{
  void (*copy_object)(void * a, void * b);
  /*!< Create a new copy of the object.
   * If the object contains its own memory allocations, then this function
   * needs to make a copy of those also.
   */

  void (*clear_object)(void * a);
  /*!< Clear an object.
   * This doesn't free the memory for the object itself, but, it should free any
   * memory that the object owns.
   */

  int (*compare_objects)(void *a, void *b);
  /*!< Compare objects
   * Return 0 if the objects are "equal", < 0 if the first object is "less"
   * than the second object and > 0 if the first object is "greater" than
   * the second object.
   */
} object_operations;

/*!
 * \brief Object list structure.
 *
 * This is the metadata structure. The actual data is stored in a block of
 * memory somewhere outside of this structure.
 */
typedef struct object_list
{
  int count; /*!< number of items in the list. */
  int size; /*!< number of items the list can hold. */
  int item_size; /*!< the size of the items the list contains. */
  void ** items; /*!< array of pointers to objects. */
  void * data; /*!< pointer to the memory where the objects are stored. */
  object_operations * ops; /*!< pointer to the function table of object ops. */
} object_list;

/*
 * Constructors, destructors, etc.
 */
/* Create a new object list with n items of size item_size */
object_list * object_list_new(int n, unsigned item_size);
/* Copy constructor, copies data too. */
object_list * object_list_duplicate(object_list * list);
/* Delete an object list */
void object_list_delete(object_list * list);


/*
 * Functions that change the list itself
 */
/* Delete the data in an object list */
int object_list_clear(object_list * list);
/* Make the object list bigger by n items */
int object_list_expand(object_list * list, int n);
/* Print the list information */
void object_list_describe(object_list * list);

/*
 * Functions that manipulate the data stored in the list
 */
/* Insert an object into the list at position n */
int object_list_insert(object_list * list, int n, void * item);
/* Remove the object at position n from the list */
int object_list_remove(object_list * list, int n);
/*  Add an objet to the end of the list */
int object_list_append(object_list * list, void * item);
/* Get a pointer to the object at position n in the list */
void * object_list_get_item(object_list * list, int n);
/* Search the list for something equal to item
 * Note: This returns the first match. 
 * */
void * object_list_find_item(object_list * list, void * item);


/*
 * Unit test
 */
#ifdef PCB_UNIT_TEST
void object_list_register_tests(void);
void object_list_test(void);
#endif /* PCB_UNIT_TEST */

#endif /* object_list_h */
