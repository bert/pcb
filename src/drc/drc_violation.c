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
#include <string.h> /* strcmp */

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "global.h" /* Coord */
#include "drc_violation.h"
#include "drc_object.h"

#include "data.h" /* PCB structure */
#include "draw.h" /* Draw */
#include "error.h" /* Message */
#include "flags.h" /* SET_FLAG */
#include "misc.h" /* ChangeGroupVisibility*/
#include "pcb-printf.h"
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

void 
pcb_drc_violation_copy (void * d, void * s)
{
  DrcViolationType * dest = (DrcViolationType *)d;
  DrcViolationType * src = (DrcViolationType *)s;
  memcpy(dest, src, sizeof(DrcViolationType));
  dest->title = strdup(src->title);
  dest->explanation = strdup(src->explanation);
  dest->objects = object_list_duplicate(src->objects);
}

/* 
 * Compare two drc violations.
 * DRC violations are considered equal if they reference the same objects
 * and have the same title string. The order of the objects in the list does
 * not have to be the same.
 * */
int 
pcb_drc_violation_compare (void * a, void * b)
{
  DrcViolationType * A = (DrcViolationType*) a;
  DrcViolationType * B = (DrcViolationType*) b;
  DRCObject *oa, *ob;

  int i = 0, j = 0;
  int found = 0;

  /* Objects are the same, check the title strings to make sure the
   * violation is the same.
   * */
  if ( strcmp(A->title, B->title) != 0 ) return -1;

  /* Check that both violations have the same objects.
   * */
  /* Must have the same number */
  if (A->objects->count != B->objects->count) return -1;
  
  /* Loop over the objects in A, and try to find each in B. */
  for (i = 0; i < A->objects->count; i++)
  {
    found = 0;
    oa = (DRCObject*) object_list_get_item(A->objects, i);
    for (j = 0; j < B->objects->count; j++)
    {
      ob = object_list_get_item(B->objects, j);
      if (oa->id == ob->id)
      {
        found = 1;
        break;
      }
    }
    if (found == 0) 
      /* Didn't find oa in violation B */
      return -1;
  }

  /* Same titles, same objects.*/
  return 0;
}

object_operations drc_violation_ops = {
  .clear_object = &pcb_drc_violation_clear,
  .copy_object = &pcb_drc_violation_copy,
  .compare_objects = &pcb_drc_violation_compare
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

#define DRC_CONTINUE _("Press Next to continue DRC checking")
#define DRC_NEXT _("Next")
#define DRC_CANCEL _("Cancel")

/*!
 * \brief Present a dialog box to show the user a DRC violation
 */
int
pcb_drc_violation_prompt (DrcViolationType *violation)
{
  int r, serial;
  char msg[1024];


  r = pcb_snprintf(msg, 1024, _("%s\n%m+near %$mD\n%s"), violation->title,
                             Settings.grid_unit->allow,
                             violation->x, violation->y,
                             DRC_CONTINUE);

  CenterDisplay (violation->x, violation->y, true);
  ClearFlagOnAllObjects(FOUNDFLAG, true);
  set_flag_on_violating_objects(violation, FOUNDFLAG);
  serial = IncrementUndoSerialNumber();
  Draw();
  r = gui->confirm_dialog (msg, DRC_CANCEL, DRC_NEXT);
  /* If we're moving on to the next one, we want to undo our flag changes,
   * however, if the user has done some work first, we don't want to undo
   * that. If the serial number hasn't changed, no work was done. */
  if (r && SaveUndoSerialNumber() == serial) Undo(false);
  return r;
}
