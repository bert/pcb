/*!
 * \file src/drc_violation.c
 *
 * \brief Class for storing data about DRC violations
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
 */

#include <stdio.h>

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "global.h" /* Coord */
#include "drc_violation.h"
#include "drc_object.h"

#include "data.h" /* PCB structure */
#include "error.h" /* Message */
#include "flags.h" /* SET_FLAG */
#include "misc.h" /* ChangeGroupVisibility*/
#include "search.h" /* SearchObjectByID */
#include "set.h" /* SetChangedFlag */
#include "undo.h" /* AddObjectToFlagUndoList */


/* ----------------------------------------------------------------------- *
 * DRC Violation Type
 * ----------------------------------------------------------------------- */

DrcViolationType *
pcb_drc_violation_new (const char *title,
                        const char *explanation,
                        Coord x, Coord y,
                        Angle angle,
                        bool have_measured,
                        Coord measured_value,
                        Coord required_value,
                        object_list * objects)
{
  DrcViolationType *violation = (DrcViolationType *)malloc (sizeof (DrcViolationType));

  violation->title = strdup (title);
  violation->explanation = strdup (explanation);
  violation->x = x;
  violation->y = y;
  violation->angle = angle;
  violation->have_measured = have_measured;
  violation->measured_value = measured_value;
  violation->required_value = required_value;
  if (objects)
    violation->objects = object_list_duplicate(objects);
  else
    violation->objects = object_list_new(2, sizeof(DrcViolationType));
  return violation;
}

void
pcb_drc_violation_clear (void * v)
{
  DrcViolationType * violation = (DrcViolationType *) v;
  free (violation->title);
  free (violation->explanation);
  if (violation->objects) object_list_delete(violation->objects);
}

void pcb_drc_violation_copy(void * d, void * s)
{
  DrcViolationType * dest = (DrcViolationType *)d;
  DrcViolationType * src = (DrcViolationType *)s;
  memcpy(dest, src, sizeof(DrcViolationType));
  dest->title = strdup(src->title);
  dest->explanation = strdup(src->explanation);
  dest->objects = object_list_duplicate(src->objects);
}

object_operations drc_violation_ops = {
  .clear_object = &pcb_drc_violation_clear,
  .copy_object = &pcb_drc_violation_copy
};

void
pcb_drc_violation_free (DrcViolationType *violation)
{
  pcb_drc_violation_clear(violation);
  free(violation);
}

void
pcb_drc_violation_print(FILE* fp, DrcViolationType * violation)
{
  int i = 0;
  DRCObject * obj;
  if (fp == NULL) fp = stdout;
  fprintf(fp, "title: %s\n", violation->title);
  fprintf(fp, "explanation: %s\n", violation->explanation);
  fprintf(fp, "location: (x, y) = (%ld, %ld), angle = %f\n",
                  violation->x, violation->y, violation->angle);
  fprintf(fp, "have_measured: %s\n",
                  violation->have_measured ? "true":"false");
  fprintf(fp, "measured value: %ld\n", violation->measured_value);
  fprintf(fp, "required value: %ld\n", violation->required_value);
  fprintf(fp, "object count: %d\n", violation->objects->count);
  fprintf(fp, "object IDs: ");
  if (violation->objects)
    for (i = 0; i < violation->objects->count; i++)
    {
      obj = object_list_get_item(violation->objects, i);
      fprintf(fp, "%ld ", obj->id);
    }
  fprintf(fp, "\n");
  fprintf(fp, "object types: ");
  if (violation->objects)
    for (i = 0; i < violation->objects->count; i++)
    {
      obj = object_list_get_item(violation->objects, i);
      fprintf(fp, "%d ", obj->type);
    }
  fprintf(fp, "\n");
}

void
set_flag_on_violating_objects (DrcViolationType * v, int f)
{
  void *p1, *p2, *p3;
  int obj_type, i;
  DRCObject *drcobj;

  /* If we get a null violation, or the violation has no objects, just
   * return. There are no objects for us to operate on. */
  if (!v || !v->objects || (v->objects->count == 0)) return;
  
  /* Flag the objects listed against this DRC violation */
  for (i = 0; i < v->objects->count; i++)
  {
    drcobj = object_list_get_item(v->objects, i);
    if (drcobj == 0) break;
    /* We don't need to do this to get the pointers, but, it's probably a
     * good idea to make sure that object still exists.
     */
    obj_type = SearchObjectByID(PCB->Data, &p1, &p2, &p3, 
                                  drcobj->id, drcobj->type);
    if (obj_type != drcobj->type)
    {
      Message (_("Object ID %i identified during DRC was not found, or "
                 "changed type. Stale DRC window?\n"), drcobj->id);
  	  continue;
    }
    AddObjectToFlagUndoList (obj_type, p1, p2, p3);
    SET_FLAG (f, (AnyObjectType *)p2);
    switch (drcobj->type)
    {
    case LINE_TYPE:
    case ARC_TYPE:
    case POLYGON_TYPE:
  	  ChangeGroupVisibility (GetLayerNumber (PCB->Data, (LayerType *) p1), true, true);
  	} /* switch (drcobj->type) */
  } /* for (violation->objects->count) */
  SetChangedFlag (true);

}
