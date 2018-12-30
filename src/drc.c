/*!
 * \file src/drc.c
 *
 * \brief DRC related structures and functions
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
#include "drc.h"

#include "error.h" /* Message */
#include "object_list.h"

/* This list keeps track of DRCViolations */
object_list * drc_violation_list = 0;

DrcViolationType *
pcb_drc_violation_new (const char *title,
                        const char *explanation,
                        Coord x, Coord y,
                        Angle angle,
                        bool have_measured,
                        Coord measured_value,
                        Coord required_value,
                        int object_count,
                        long int *object_id_list,
                        int *object_type_list)
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
  violation->object_count = object_count;
  violation->object_id_list = object_id_list;
  violation->object_type_list = object_type_list;

  return violation;
}

void
pcb_drc_violation_free (DrcViolationType *violation)
{
  free (violation->title);
  free (violation->explanation);
  free (violation);
}

void
pcb_drc_violation_clear (void * v)
{
  DrcViolationType * violation = (DrcViolationType *) v;
  free (violation->title);
  free (violation->explanation);
  free (violation->object_id_list);
  free (violation->object_type_list);
}

void pcb_drc_violation_copy(void * d, void * s)
{
  DrcViolationType * dest = (DrcViolationType *)d;
  DrcViolationType * src = (DrcViolationType *)s;
  memcpy(dest, src, sizeof(DrcViolationType));
  dest->title = strdup(src->title);
  dest->explanation = strdup(src->explanation);
  dest->object_id_list =
      (long int *)malloc(dest->object_count * sizeof(long int));
  memcpy(dest->object_id_list, src->object_id_list,
         dest->object_count * sizeof(long int));
  dest->object_type_list = (int *)malloc(dest->object_count * sizeof(int));
  memcpy(dest->object_type_list, src->object_type_list,
         dest->object_count * sizeof(int));
}

object_operations drc_violation_ops = {
  .clear_object = &pcb_drc_violation_clear,
  .copy_object = &pcb_drc_violation_copy
};

void
pcb_drc_violation_print(FILE* fp, DrcViolationType * violation)
{
  int i = 0;
  if (fp == NULL) fp = stdout;
  fprintf(fp, "title: %s\n", violation->title);
  fprintf(fp, "explanation: %s\n", violation->explanation);
  fprintf(fp, "location: (x, y) = (%ld, %ld), angle = %f\n",
                  violation->x, violation->y, violation->angle);
  fprintf(fp, "have_measured: %s\n",
                  violation->have_measured ? "true":"false");
  fprintf(fp, "measured value: %ld\n", violation->measured_value);
  fprintf(fp, "required value: %ld\n", violation->required_value);
  fprintf(fp, "object count: %d\n", violation->object_count);
  fprintf(fp, "object IDs: ");
  for (i = 0; i < violation->object_count; i++)
    fprintf(fp, "%ld ", violation->object_id_list[i]);
  fprintf(fp, "\n");
  fprintf(fp, "object types: ");
  for (i = 0; i < violation->object_count; i++)
    fprintf(fp, "%d ", violation->object_type_list[i]);
  fprintf(fp, "\n");
}

static const char drc_report_syntax[] = N_("DRCReport([Output file])");
static const char drc_report_help[] = 
            N_("Write the DRC violation data from the last DRC to a file.");

static int
ActionDRCReport (int argc, char **argv, Coord x, Coord y)
{
  int i=0, len=0;
  FILE * fp;
  char starliner[81];
  char buffer[80];
  
  if (!drc_violation_list)
  {
	Message("DRCReport: Must run DRC check first.\n");
	return 0;
  }

  memset(starliner, '*', 80);
  starliner[80] = '\0';
  if (argc == 1) fp = fopen(argv[0], "w");
  else fp = stdout;
  
  for (i=0; i < drc_violation_list->count; i++){
    len = sprintf(buffer, "Violation %d", i);
    fprintf(fp, "%s\n%*s\n%s\n", starliner, 40+len/2, buffer, starliner);
    pcb_drc_violation_print(fp,
           (DrcViolationType*) object_list_get_item(drc_violation_list,i));
    fprintf(fp, "\n");
  }
  if (argc == 1) fclose(fp);
  return 0;
}

HID_Action drc_action_list[] = {
  {"DRCReport", 0, ActionDRCReport, drc_report_help, drc_report_syntax}
};

REGISTER_ACTIONS (drc_action_list)

