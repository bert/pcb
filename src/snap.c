/*!
 * \file src/snap.c
 *
 * \brief Snapping functions.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2017 Charles Parker
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 *
 */

#include "snap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "global.h"
#include "search.h"

/*
 * Snap Type functions
 */

SnapSpecType *
snap_spec_new(char * name, int priority)
{
  SnapSpecType * snap = malloc(sizeof(SnapSpecType));
  snap->name = strdup(name);
  snap->priority = priority;
  snap->enabled = false;
  snap->search = NULL;
  snap->radius = 0;
  return snap;
}

void
snap_delete(SnapSpecType * snap)
{
  free(snap->name);
  free(snap);
}

/*
 * Snap List Type functions
 */

/*!
 * \brief Create a new snap list.
 *
 * Allocate and initialize memory for a snap list structure.
 *
 */
SnapListType *
snap_list_new(void)
{
  SnapListType * list = malloc(sizeof(SnapListType));
  list->n = 0;
  list->max = 0;
  list->snaps = NULL;
  return list;
}

/*!
 * \brief Delete a snap list.
 * 
 * Free the memory allocated to the snap list. This frees all of the snaps in 
 * the list, since we make copies of them.
 * 
 * TODO:
 * Are other objects allowed to keep pointers to snaps in the list? If so, then
 * when we free them here, the objects that have those pointers might try to 
 * access them after we've freed them. So, either we don't let things keep 
 * pointers, or we have to let the other objects own the snaps.
 *
 */
void
snap_list_delete(SnapListType * list)
{
  free(list->snaps);
  free(list);
}

/*!
 * \brief Add a snap to a snap list
 * 
 * When a snap is added to the list, a copy is made. The idea is to keep
 * them all together in memory since we're likely going to be iterating over 
 * the list.
 */

SnapSpecType *
snap_list_add_snap(SnapListType * list, SnapSpecType * snap)
{
  int i, in = list->n;
  
  if ((list == NULL) || (snap == NULL)) return NULL;
  
  // figure out where in the list to put the new snap
  for(i = 0; i < list->n; i++)
  {
    /* use the name to detect and reject duplicates */
    if (strcmp (list->snaps[i].name, snap->name)==0) return NULL;
    /* make a note of where to insert the entry */
    if (snap->priority > list->snaps[i].priority) in = i;
  }
  
  /* reallocate the memory to the new size */
  if (list->n + 1 > list->max)
  {
    list->snaps = (SnapSpecType *) realloc(list->snaps,
                                       (list->n + 1) * sizeof(SnapSpecType));
    list->max += 1;
  }
  /* shift everything after the new entry down */
  memmove(list->snaps + in + 1, list->snaps + in,
          (list->n - in)*sizeof(SnapSpecType));
  /* copy in the new entry */
  memcpy(list->snaps+in, snap, sizeof(SnapSpecType));
  /* increment the list counter */
  list->n += 1;
  return list->snaps+in;
}

int
snap_list_remove_snap_by_name(SnapListType * list, char * name)
{
  int i;
  
  if ((list == NULL) || (name == NULL)) return -1;
  
  for(i = 0; i < list->n; i++)
    if (strcmp (list->snaps[i].name, name)==0)
    {
      if (i+1 != list->n)
      /* item was not the last element in the list */
        memmove(list->snaps + i, list->snaps + i + 1,
                (list->n - i - 1)*sizeof(SnapSpecType));
      list->n -= 1;
    }
  return i;
}

SnapSpecType *
snap_list_find_snap_by_name(SnapListType * list, char * name)
{
  int i;
  
  if ((list == NULL) || (name == NULL)) return NULL;
  
  for(i = 0; i < list->n; i++)
    if (strcmp (list->snaps[i].name, name)==0) return list->snaps + i;
  return NULL;
}

int snap_list_list_snaps(SnapListType * snaps)
{
  int i;
  printf("List has %d snaps out of a maximum %d\n", snaps->n, snaps->max);
  for(i=0; i< snaps->n; i++)
    printf("\tsnap: %s (%d)\n", snaps->snaps[i].name, snaps->snaps[i].priority);
  return 0;
}

SnapType *
snap_list_search_snaps(SnapListType * list, Coord x, Coord y)
{
  int i;
  static SnapType snap;
  
  for (i=0; i < list->n; i++){
    if (!list->snaps[i].enabled) continue;
    snap = list->snaps[i].search(x, y, list->snaps[i].radius);
    snap.spec = list->snaps + i;
    if (snap.valid) return &snap;
  }
  return NULL;
}
