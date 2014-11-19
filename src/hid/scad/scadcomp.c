/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *
 *  OpenSCAD export HID
 *  This code is based on the GERBER and VRML export HID
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>

#include <time.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "misc.h"
#include "error.h"
#include "buffer.h"
#include "mirror.h"
#include "create.h"

#include "hid.h"
#include "../hidint.h"
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"
#include "hid/common/hidinit.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#include "scad.h"

/* model types for scad_export_model */
#define SCAD_STANDARD	0	/* primary model */
#define SCAD_OVERLAY	1


static char *include_files_list;
static int include_files_size, include_files_bufsize, include_file_maxlength;

static void
scad_init_include_files (void)
{
  include_files_list = NULL;
  include_files_size = 0;
  include_files_bufsize = 0;
  include_file_maxlength = 0;
}

static void
scad_free_include_files (void)
{
  if (include_files_list)
    free (include_files_list);
}

static void
scad_add_include_file (char *include_file)
{

  int ln = strlen (include_file);
  int new_length = ln + include_files_size + 1;
  int ptr;
  char *bf;

/* quietly assumimng, that include file name is shorter than 2048 characters */
  if (!include_files_list
      || (include_files_list != NULL && new_length > include_files_bufsize))
    {
      bf = malloc (2048 + include_files_bufsize);
      if (bf)
	{
	  include_files_list = bf;
	  include_files_size = 0;
	  include_files_bufsize = 2048;
	}
      else
	{
	  Message
	    (" Error: Cannot allocate memory for component included files.\n");
	}
    }

/* Check, if the file is already in list */
  ptr = 0;
  while (ptr < include_files_size)
    {
      if (strcmp (include_files_list + ptr, include_file) == 0)
	{
	  return;
	}
      ptr += strlen (include_files_list + ptr) + 1;
    }
  strcpy (include_files_list + ptr, include_file);
  include_files_size = new_length;
  if (ln > include_file_maxlength)
    {
      include_file_maxlength = ln;
    }
}

static void
scad_export_include_files (void)
{
  int ptr;
  char *fullname;
  char line[2048];
  int l;
  FILE *f;

  if (!include_files_list)
    return;


  l =
    strlen (pcblibdir) + 1 + strlen (MODELBASE) + 1 + strlen (SCADBASE) + 1 +
    include_file_maxlength + 1;
  if ((fullname = (char *) malloc (l * sizeof (char))) == NULL)
    {
      Message
	(" Error: Cannot allocate memory for component included files.\n");
      return;
    }
  sprintf (fullname, "%s%s%s%s%s%s", pcblibdir, PCB_DIR_SEPARATOR_S,
	   MODELBASE, PCB_DIR_SEPARATOR_S, SCADBASE, PCB_DIR_SEPARATOR_S);

  l = strlen (fullname);	/* index to be used to append include file names */

  fprintf (scad_output,
	   "/***************************************************/\n");
  fprintf (scad_output,
	   "/*                                                 */\n");
  fprintf (scad_output,
	   "/* Embedded include files                          */\n");
  fprintf (scad_output,
	   "/*                                                 */\n");
  fprintf (scad_output,
	   "/***************************************************/\n");

  ptr = 0;
  while (ptr < include_files_size)
    {
      strcpy (fullname + l, include_files_list + ptr);
      /* printf ("[%s] @ %s\n", include_files_list + ptr, fullname); */
      f = fopen (fullname, "r");
      if (f)
	{
	  while (fgets (line, sizeof (line), f))
	    {
	      fputs (line, scad_output);
	    }
	  fclose (f);
	}
      else
	{
	  fprintf (scad_output, "include <%s>\n", include_files_list + ptr);
	}

      ptr += strlen (include_files_list + ptr) + 1;
    }
  fprintf (scad_output, "\n");

}


/***********************************************************************
*
* Export of the components
*
***********************************************************************/
static void
scad_imported_model_name (char *model, char *name, int size, bool simple)
{

  sprintf (name, "%s%s%s%s", model, (simple) ? "-" : "",
	   (simple) ? SCADSIMPLEMODELS : "", SCAD_STL_EXT);
}

static void
scad_close_model (FILE * f)
{
  if (f)
    {
      fclose (f);
    }
}

static FILE *
scad_open_model (char *model, char *first_line, int size, bool simple)
{
  int l;
  FILE *f = NULL;
  char *cmd;

  l =
    strlen (pcblibdir) + 1 + strlen (MODELBASE) + 1 + strlen (SCADBASE) + 1 +
    strlen (SCADSIMPLEMODELS) + 1 + strlen (model) + 1 + strlen (SCAD_EXT);
  if ((cmd = (char *) malloc (l * sizeof (char))) != NULL)
    {
      sprintf (cmd, "%s%s%s%s%s%s%s%s%s%s", pcblibdir, PCB_DIR_SEPARATOR_S,
	       MODELBASE, PCB_DIR_SEPARATOR_S, SCADBASE, PCB_DIR_SEPARATOR_S,
	       model, (simple) ? "-" : "", (simple) ? SCADSIMPLEMODELS : "",
	       SCAD_EXT);

      f = fopen (cmd, "r");
#if 0
      if (!f)
	{
	  sprintf (cmd, "%s%s%s%s%s%s%s%s%s%s", pcblibdir,
		   PCB_DIR_SEPARATOR_S, MODELBASE, PCB_DIR_SEPARATOR_S,
		   SCADBASE, PCB_DIR_SEPARATOR_S,
		   (simple) ? SCADSIMPLEMODELS : "",
		   (simple) ? PCB_DIR_SEPARATOR_S : "", model, SCAD_EXT);
	  f = fopen (cmd, "r");
	}
#endif
    }

  if (cmd)
    free (cmd);

  if (f && fgets (first_line, size, f))
    return f;

  if (f)
    scad_close_model (f);

  return NULL;

}

static void
scad_process_line (char *line)
{
  char *s0, *s1, *s2;

  if ((s0 = strstr (line, "include")) != NULL
      && (s1 = strchr (s0, '<')) != NULL && (s2 = strchr (s1, '>')) != NULL)
    {
      *s2 = 0;
      scad_add_include_file (s1 + 1);
    }
  else
    {
      fputs (line, scad_output);
    }
}


static int
scad_parse_coord_triplet (char *s, Coord * ox, Coord * oy, Coord * oz)
{
  Coord xx = 0, yy = 0, zz = 0;
  int n = 0, ln = 0;
  char val[32];

  while (sscanf (s, "%30s%n", val, &ln) >= 1)
    {
      switch (n)
	{
	case 0:
	  xx = GetValueEx (val, NULL, NULL, NULL, "mm");
	  break;
	case 1:
	  yy = GetValueEx (val, NULL, NULL, NULL, "mm");
	  break;
	case 2:
	  zz = GetValueEx (val, NULL, NULL, NULL, "mm");
	  break;
	}
      s = s + ln;
      n++;
    }
  if (n == 3)
    {
      *ox = xx;
      *oy = yy;
      *oz = zz;
      return true;
    }
  else
    {
      return false;
    }
}

static int
scad_parse_float_triplet (char *s, float *ox, float *oy, float *oz)
{
  float xx = 0, yy = 0, zz = 0;

  if (sscanf (s, "%f %f %f", &xx, &yy, &zz) == 3)
    {
      *ox = xx;
      *oy = yy;
      *oz = zz;
      return true;
    }
  else
    {
      return false;
    }
}

/************************************************************
* Export of the single model
* - adjusts the model position, rotation, scale
* - processes the model line-by-line and perfoprms variable expansion
************************************************************/
static void
scad_export_model (int model_type, ElementType * element, bool imported,
		   FILE * f, char *line, int size)
{
  char *model_rotation, *model_translate, *model_scale, *model_angle;
  Angle tmp_angle = (Angle) 0;
  Coord tx, ty, tz;
  float fx, fy, fz;

  int x = element->MarkX, y = element->MarkY;

  model_rotation =
    AttributeGetFromList (&(element->Attributes),
			  (model_type ==
			   SCAD_OVERLAY) ? "OpenSCAD::Overlay:rotate" :
			  "OpenSCAD::Model:rotate");
  model_scale =
    AttributeGetFromList (&(element->Attributes),
			  (model_type ==
			   SCAD_OVERLAY) ? "OpenSCAD::Overlay:scale" :
			  "OpenSCAD::Model:scale");
  model_translate =
    AttributeGetFromList (&(element->Attributes),
			  (model_type ==
			   SCAD_OVERLAY) ? "OpenSCAD::Overlay:translate" :
			  "OpenSCAD::Model:translate");

  if ((model_angle =
       AttributeGetFromList (&(element->Attributes),
			     "Footprint::RotationTracking")) != NULL)
    {
      sscanf (model_angle, "%lf", &tmp_angle);
    }

  if (model_translate
      && scad_parse_coord_triplet (model_translate, &tx, &ty, &tz))
    fprintf (scad_output, "translate ([%f, %f, %f]) ", scad_scale_coord (tx),
	     scad_scale_coord (ty), scad_scale_coord (tz));

  fprintf (scad_output, "translate ([%f, %f, %f]) ",
	   scad_scale_coord ((float) x), -scad_scale_coord ((float) y),
	   ((TEST_FLAG (ONSOLDERFLAG, (element))) ? -1. : 1.) *
	   (BOARD_THICKNESS / 2. + OUTER_COPPER_THICKNESS));

  /* rotate order: angle onsolder user-defined */
  if (tmp_angle != 0.)
    fprintf (scad_output, "rotate ([0, 0, %lf]) ",
	     (TEST_FLAG (ONSOLDERFLAG, (element))) ? -tmp_angle : tmp_angle);

  if (TEST_FLAG (ONSOLDERFLAG, (element)))
    fprintf (scad_output, "rotate([180.,0,0]) ");

  if (model_rotation
      && scad_parse_float_triplet (model_rotation, &fx, &fy, &fz))
    fprintf (scad_output, "rotate ([%f, %f, %f]) ", fx, fy, fz);

  if (model_scale && scad_parse_float_triplet (model_scale, &fx, &fy, &fz))
    fprintf (scad_output, "scale ([%f, %f, %f]) ", fx, fy, fz);

  if (imported)
    {
      fprintf (scad_output, "{ import(\"%s\"); }\n", line);

    }
  else
    {
      fprintf (scad_output, "{\n");

      /* Flush first line of text, already read in buffer */
      scad_process_line (line);

      while (fgets (line, size, f))
	{
	  scad_process_line (line);
	}
      fprintf (scad_output, "}\n");
    }
}



extern void FreeRotateBuffer (BufferType * Buffer, Angle angle);

static int
scad_calculate_bbox (ElementType * element, Angle angle, float *w, float *h,
		     float *ox, float *oy)
{
  return 0;

/*
  TODO: automatic calculation of bounding box

  BufferType element_buffer;

  element_buffer.Data = CreateNewBuffer ();

  -- Copy
  AddElementToBuffer (ElementType *Element)
  if (ON_SIDE(Element,(Settings.ShowBottomSide)?BOTTOM_SIDE:TOP_SIDE))
     MirrorElementCoordinates (element_buffer.Data, element, 0);

  ClearBuffer (&element_buffer);
*/
}

/************************************************************
* Export of the single element
* - identifies the model for the component - primary and overlay
* - exports both models
************************************************************/
static void
scad_export_bbox (ElementType * element)
{
  char *model_angle, *bbox;
  Angle tmp_angle = (Angle) 0;
  float w = 0., h = 0., t = 0., ox = 0., oy = 0.;
  int x = element->MarkX, y = element->MarkY;
  int n, ln;
  char val[32], *s;

  if ((bbox =
       AttributeGetFromList (&(element->Attributes),
			     "Footprint::BoundingBox")) == NULL)
    return;


  if ((model_angle =
       AttributeGetFromList (&(element->Attributes),
			     "Footprint::RotationTracking")) != NULL)
    {
      sscanf (model_angle, "%lf", &tmp_angle);
    }

  /* Parse values with units... */
  s = bbox;
  n = 0;
  while (sscanf (s, "%30s%n", val, &ln) >= 1)
    {
      switch (n)
	{
	case 0:
	  w = GetValueEx (val, NULL, NULL, NULL, "mm");
	  break;
	case 1:
	  h = GetValueEx (val, NULL, NULL, NULL, "mm");
	  break;
	case 2:
	  t = GetValueEx (val, NULL, NULL, NULL, "mm");
	  break;
	case 3:
	  ox = GetValueEx (val, NULL, NULL, NULL, "mm");
	  break;
	case 4:
	  oy = GetValueEx (val, NULL, NULL, NULL, "mm");
	  break;
	}
      s = s + ln;
      n++;
    }

  if (n == 3)
    {
      ox = 0.;
      oy = 0.;
    }
  else if (n == 1)
    {
      /* Try automatically calculate the bounding box */
      t = w;
      if (!scad_calculate_bbox (element, tmp_angle, &w, &h, &ox, &oy))
	return;
    }
  else if (n != 5)
    return;

  fprintf (scad_output, "translate ([%f, %f, %f]) ",
	   scad_scale_coord ((float) x), -scad_scale_coord ((float) y),
	   ((TEST_FLAG (ONSOLDERFLAG, (element))) ? -1. : 1.) *
	   (BOARD_THICKNESS / 2. + OUTER_COPPER_THICKNESS));

  if (tmp_angle != 0.)
    fprintf (scad_output, "rotate ([0, 0, %lf]) ",
	     (TEST_FLAG (ONSOLDERFLAG, (element))) ? -tmp_angle : tmp_angle);
  if (TEST_FLAG (ONSOLDERFLAG, (element)))
    fprintf (scad_output, "rotate([180.,0,0]) ");

  fprintf (scad_output, "{\n");

  fprintf (scad_output,
	   "translate ([%f, %f, %f]) color ([0.2, 0.2, 0.2]) cube ([%f,%f,%f],true);\n",
	   scad_scale_coord ((float) ox), scad_scale_coord ((float) oy),
	   scad_scale_coord ((float) t / 2.), scad_scale_coord ((float) w),
	   scad_scale_coord ((float) h), scad_scale_coord ((float) t));

  fprintf (scad_output, "}\n");

}

static void
scad_writeout_element (ElementType * element, char *name, int model_type,
		       bool imported, bool simple)
{
  FILE *f = NULL;
  char line[2048];

  if (imported)
    {
      scad_imported_model_name (name, line, sizeof (line), simple);
      scad_export_model (model_type, element, imported, f, line,
			 sizeof (line));
    }
  else
    {
      /* if model is defined, try to open it */
      f = scad_open_model (name, line, sizeof (line), simple);

      if (f)
	{
	  scad_export_model (model_type, element, imported, f, line,
			     sizeof (line));
	  scad_close_model (f);
	}
    }
}

/************************************************************
* Export of the single element
* - identifies the model for the component - primary and overlay
* - exports both models
************************************************************/
static void
scad_export_element (ElementType * element, bool simple)
{
  char *model_name, *s;
  bool imported_model;

  s = AttributeGetFromList (&(element->Attributes), "OpenSCAD::Model:type");
  imported_model = s && (strcmp (s, "STL") == 0);

  /* get model name from attibute */
  model_name =
    AttributeGetFromList (&(element->Attributes), "OpenSCAD::Model");

  if (model_name)
    {
      scad_writeout_element (element, model_name, SCAD_STANDARD,
			     imported_model, simple);
    }
  else
    {
      /* no model variable found, try model, based on footprint name attribute */
      model_name =
	AttributeGetFromList (&(element->Attributes), "Footprint::File");
      if (model_name)
	{
	  scad_writeout_element (element, model_name, SCAD_STANDARD,
				 imported_model, simple);
	}
      else
	{
	  /* still no model found, try model, based on description */
	  model_name = DESCRIPTION_NAME (element);
	  if (model_name)
	    {
	      scad_writeout_element (element, model_name, SCAD_STANDARD,
				     imported_model, simple);
	    }
	}
    }

  s = AttributeGetFromList (&(element->Attributes), "OpenSCAD::Overlay:type");
  imported_model = s && (strcmp (s, "STL") == 0);

  /* get overlay name from attibute */
  model_name =
    AttributeGetFromList (&(element->Attributes), "OpenSCAD::Overlay");

  if (model_name)
    {
      scad_writeout_element (element, model_name, SCAD_OVERLAY,
			     imported_model, simple);
    }

  return;
}

/************************************************************
* Main function for components export
* - initialize footprint and element database
* - loops through all components on the board and for each component:
*   - loads the footprint into temporary buffer and calculates anfgle
*   - export the element
************************************************************/
void
scad_process_components (int mode)
{

  scad_init_include_files ();


  fprintf (scad_output, "module all_components() {\n");


  ELEMENT_LOOP (PCB->Data);
  {
    if ((mode == SCAD_COMPONENT_SIMPLE) || (mode == SCAD_COMPONENT_REALISTIC))
      {
	scad_export_element (element,
			     (mode == SCAD_COMPONENT_SIMPLE) ? 1 : 0);
      }
    else
      {
	scad_export_bbox (element);
      }
  }

  END_LOOP;

  fprintf (scad_output, "}\n\n");

  scad_export_include_files ();

  scad_free_include_files ();

}
