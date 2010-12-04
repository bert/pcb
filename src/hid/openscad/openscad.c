/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */


/*!
 * \file openscad.c
 * \author Copyright 2010 Bert Timmerman <bert.timmerman@xs4all.nl>
 * \brief Exporter for use with OpenSCAD 3D-modelling.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.\n
 * \n
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.\n
 * \n
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.\n
 *
 * Prints a file in a format which includes data used by OpenSCAD to
 * place 3D-models of parts in a 3D-model of a Printed Circuit Board
 * (PCB) thus creating a 3D representation of a Printed Circuit Assembly
 * (PCA).\n
 * 
 * This exporter re-used the BOM HID code (partially) as a template.\n
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "global.h"
#include "data.h"
#include "error.h"
#include "misc.h"

#include "hid.h"
#include "../hidint.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif


RCSID ("$Id$");


static HID_Attribute openscad_options[] =
{
    {
        "openscadfile",
        "OpenSCAD output file",
        HID_String,
        0, 0, {0, 0, 0}, 0, 0
    },
#define HA_openscad_file 0
    {
        "openscad-in-mm",
        "OpenSCAD dimensions in mm instead of mils",
        HID_Boolean,
        0, 0, {0, 0, 0}, 0, 0
    },
#define HA_openscad_mm 1
};


#define NUM_OPTIONS (sizeof(openscad_options)/sizeof(openscad_options[0]))


static HID_Attr_Val openscad_values[NUM_OPTIONS];
static char *openscad_filename;
static int openscad_dim_type;


typedef struct _StringList
{
  char *str;
  struct _StringList *next;
} StringList;


typedef struct _OpenscadList
{
  char *modelname;
  char *value;
  int num;
  StringList *refdes;
  struct _OpenscadList *next;
} OpenscadList;


static HID_Attribute *
openscad_get_export_options (int *n)
{
    static char *last_openscad_filename = 0;
    if (PCB)
    {
        derive_default_filename
        (
            PCB->Filename,
            &openscad_options[HA_openscad_file ],
            ".scad",
            &last_openscad_filename
        );
    }
    if (n)
        *n = NUM_OPTIONS;
    return openscad_options;
}


/*! 
 * \brief Copy over in to out with some character conversions.
 *
 * Go all the way to the end to get the terminating \0.
 *
 * \return pointer to a cleaned string.
 */
static char *
openscad_clean_string (char *in)
{
    char *out;
    int i;
    if ((out = malloc ((strlen (in) + 1) * sizeof (char))) == NULL)
    {
        fprintf (stderr, "Error: malloc() failed in openscad_clean_string()\n");
        exit (1);
    }
    for (i = 0; i <= strlen (in); i++)
    {
        switch (in[i])
        {
            case '"':
                out[i] = '\'';
                break;
            default:
                out[i] = in[i];
        }
    }
    return out;
}


/*!
 * \brief Copy over in to out until a digit [0..9]  or underscore is found.
 *
 * \return pointer to a string containing the package type.
 */
static char *
openscad_get_package_type_string (char *in)
{
    char *out;
    int i;
    if ((out = malloc ((strlen (in) + 1) * sizeof (char))) == NULL)
    {
        fprintf (stderr, "Error: malloc() failed in openscad_get_package_type_string()\n");
        exit (1);
    }
    for (i = 0; i <= strlen (in); i++)
    {
        if (isdigit (in[i]))
        {
            return (out);
        }
        else if (in[i] == '_')
        {
            return (out);
        }
        else
        {
            out[i] = in[i];
        }
    }
    return out;
}


/*!
 * \brief Get the basename including suffix from the absolute filename.
 *
 * \return pointer to a string containing the basename including suffix.
 */
char *
openscad_get_basename (char *pathname)
{
    char *fname = NULL;

    if (pathname)
    {
        fname = strrchr (pathname, '/') + 1;
    }
    return fname;
}


/*!
 * \brief Strip the string \c suffix from the string \c name.
 *
 * \return pointer to a string containing the \c name without \c suffix.
 */
static void
openscad_remove_suffix (char *name, const char *suffix)
{
  char *np;
  const char *sp;

  np = name + strlen (name);
  sp = suffix + strlen (suffix);

  while (np > name && sp > suffix)
    if (*--np != *--sp)
      return;
  if (np > name)
    *np = '\0';
}


/*!
 * \brief Figure out the rotation angle of the part (element).
 *
 * \return rotation angle.
 */
static double
openscad_xy_to_angle (double x, double y)
{
    double theta;
    if ((x > 0.0) && (y >= 0.0))
        theta = 180.0;
    else if ((x <= 0.0) && (y > 0.0))
        theta = 90.0;
    else if ((x < 0.0) && (y <= 0.0))
        theta = 0.0;
    else if ((x >= 0.0) && (y < 0.0))
        theta = 270.0;
    else
    {
        theta = 0.0;
        Message ("openscad_xy_to_angle(): unable to figure out angle of element\n"
            "     because the pin is at the centroid of the part.\n"
            "     This is a BUG!!!\n"
            "     Setting to %g degrees\n", theta);
    }
    return (theta);
}


static StringList *
openscad_string_insert (char *str, StringList * list)
{
    StringList *new;
    StringList *cur;
    if ((new = (StringList *) malloc (sizeof (StringList))) == NULL)
    {
        fprintf (stderr, "malloc() failed in openscad_string_insert()\n");
        exit (1);
    }
    new->next = NULL;
    new->str = strdup (str);
    if (list == NULL)
    return (new);
    cur = list;
    while (cur->next != NULL)
        cur = cur->next;
    cur->next = new;
    return (list);
}


static OpenscadList *
openscad_insert (char *refdes, char *modelname, char *value, OpenscadList * openscad)
{
    OpenscadList *new;
    OpenscadList *cur;
    OpenscadList *prev = NULL;
    if (openscad == NULL)
    {
        /* This is the first element so automatically create an entry. */
        if ((new = (OpenscadList *) malloc (sizeof (OpenscadList))) == NULL)
        {
            fprintf (stderr, "ERROR: malloc() failed in openscad_insert()\n");
            exit (1);
        }
        new->next = NULL;
        new->modelname = strdup (modelname);
        new->value = strdup (value);
        new->num = 1;
        new->refdes = openscad_string_insert (refdes, NULL);
        return (new);
    }
    /* Search and see if we already have used one of these components. */
    cur = openscad;
    while (cur != NULL)
    {
        if ((NSTRCMP (modelname, cur->modelname) == 0) &&
            (NSTRCMP (value, cur->value) == 0))
        {
            cur->num++;
            cur->refdes = openscad_string_insert (refdes, cur->refdes);
            break;
        }
        prev = cur;
        cur = cur->next;
    }
    if (cur == NULL)
    {
        if ((new = (OpenscadList *) malloc (sizeof (OpenscadList))) == NULL)
        {
            fprintf (stderr, "ERROR: malloc() failed in openscad_insert()\n");
            exit (1);
        }
        prev->next = new;
        new->next = NULL;
        new->modelname = strdup (modelname);
        new->value = strdup (value);
        new->num = 1;
        new->refdes = openscad_string_insert (refdes, NULL);
    }
    return (openscad);
}


/*!
 * \brief
 *
 * If \c fp is not NULL then print out the element list contained in
 * \c openscad .\n
 * Either way, free all memory which has been allocated for \c openscad.
 * 
 * \return 0 if successful.
 */
static int
openscad_print (void)
{
    char utcTime[64];
    double x;
    double y;
    double theta = 0.0;
    double user_x;
    double user_y;
    double sumx, sumy;
    double pin1x = 0.0;
    double pin1y = 0.0;
    double pin1angle = 0.0;
    double pin2x = 0.0;
    double pin2y = 0.0;
    double pin2angle;
    int found_pin1;
    int found_pin2;
    int pin_cnt;
    time_t currenttime;
    FILE *fp;
    OpenscadList *openscad = NULL;
    char *name;
    char *package_type;
    char *modelname;
    char *value;
    double board_width;
    double board_height;
    double board_thickness;
    double drill_x;
    double drill_y;
    double drill_d;
    fp = fopen (openscad_filename, "w");
    if (!fp)
    {
        gui->log ("Cannot open file %s for writing\n", openscad_filename);
        return 1;
    }
    /* Create a portable timestamp. */
    currenttime = time (NULL);
    {
        /* Avoid gcc complaints. */
        const char *fmt = "%c UTC";
        strftime (utcTime, sizeof (utcTime), fmt, gmtime (&currenttime));
    }
    fprintf (fp, "/*!\n");
    fprintf (fp, " * \\file %s.scad\n", EMPTY (PCB->Name));
    fprintf (fp, " *\n");
    fprintf (fp, " * \\author Copyright %s.\n", pcb_author ());
    fprintf (fp, " *\n");
    fprintf (fp, " * \\brief PCB - OpenSCAD 3D-model exporter Version 1.0\n");
    fprintf (fp, " *\n");
    fprintf (fp, " * \\date %s\n", utcTime);
    fprintf (fp, " *\n");
    /* Write the license statement for footprints for the GPL version to file */
    fprintf (fp, " * This OpenSCAD 3D-model is free software; you may redistribute it\n");
    fprintf (fp, " * and/or modify it under the terms of the GNU General Public License\n");
    fprintf (fp, " * as published by the Free Software Foundation; either version 2 of the\n");
    fprintf (fp, " * License, or (at your option) any later version.\n");
    fprintf (fp, " * As a special exception, if you create a design which uses this\n");
    fprintf (fp, " * OpenSCAD 3D-model, and embed this 3D-model file or unaltered portions\n");
    fprintf (fp, " * of this OpenSCAD 3D-model into the design, this OpenSCAD 3D-model does\n");
    fprintf (fp, " * not by itself cause the resulting design to be covered by the GNU\n");
    fprintf (fp, " * General Public License.\n");
    fprintf (fp, " * This exception does not however invalidate any other reasons why\n");
    fprintf (fp, " * the design itself might be covered by the GNU General Public\n");
    fprintf (fp, " * License.\n");
    fprintf (fp, " * If you modify this OpenSCAD 3D-model, you may extend this exception\n");
    fprintf (fp, " * to your version of the OpenSCAD 3D-model, but you are not obligated\n");
    fprintf (fp, " * to do so.\n");
    fprintf (fp, " * If you do not wish to do so, delete this exception statement from\n");
    fprintf (fp, " * your version.\n");
    fprintf (fp, " * \n");
    if (openscad_dim_type)
    {
        /* Dimensions in mm. */
        fprintf (fp, " * All dimensions in mm. Angles in degrees.\n");
    }
    else
    {
        /* Dimensions in mil. */
        fprintf (fp, " * All dimensions in mils. Angles in degrees.\n");
    }
    fprintf (fp, " */\n");
    fprintf (fp, "\n");
    fprintf (fp, "\n");
    fprintf (fp, "include <COLORS.scad>\n");
    fprintf (fp, "include <CONST.scad>\n");
    fprintf (fp, "include <BOARD.scad>\n");
    fprintf (fp, "include <PIN_HOLE.scad>\n");
    fprintf (fp, "include <VIA_HOLE.scad>\n");
    fprintf (fp, "include <PACKAGES.scad>\n");
    fprintf (fp, "use <INSERT_PART_MODEL.scad>\n");
    fprintf (fp, "\n");
    fprintf (fp, "\n");
    fprintf (fp, "/* Uncomment the following line for an example. */\n");
    fprintf (fp, "%s ();\n", EMPTY (PCB->Name));
    fprintf (fp, "\n");
    fprintf (fp, "\n");
    fprintf (fp, "module %s ()\n", EMPTY (PCB->Name));
    fprintf (fp, "{\n");
    /* Lookup the board dimensions and create an entry in the OpenSCAD
     * file. */
    if (openscad_dim_type)
    {
        /* Dimensions in mm. */
        board_width = COOR_TO_MM * PCB->MaxWidth;
        board_height = COOR_TO_MM * PCB->MaxHeight;
        board_thickness = 1.6;
    }
    else
    {
        /* Dimensions in mil. */
        board_width = PCB->MaxWidth / 100;
        board_height = PCB->MaxHeight / 100;
        board_thickness = 1.6 * MM_TO_MIL;
    }
    fprintf (fp, "    /* Modelling a printed circuit board based on maximum dimensions. */\n");
    fprintf (fp, "    difference ()\n");
    fprintf (fp, "    {\n");

    fprintf (fp, "        BOARD (%.2f, %.2f, %.2f);\n", board_width, board_height, board_thickness);
    fprintf (fp, "        /* Now subtract some via holes. */\n");
    /* Now subtract some via holes. */
    VIA_LOOP (PCB->Data);
    {
        if (openscad_dim_type)
        {
            /* Dimensions in mm. */
            drill_x = COOR_TO_MM * via->X;
            drill_y = COOR_TO_MM * via->Y;
            drill_d = COOR_TO_MM * via->DrillingHole;
        }
        else
        {
            /* Dimensions in mil. */
            drill_x = via->X / 100;
            drill_y = via->Y / 100;
            drill_d = via->DrillingHole / 100;
        }
        fprintf (fp, "        VIA_HOLE (%.2f, %.2f, %.2f, %.2f);\n", drill_x, drill_y, drill_d, board_thickness);
    }
    END_LOOP; /* End of VIA_LOOP */
    /* Now subtract some pin holes. */
    fprintf (fp, "        /* Now subtract some pin holes. */\n");
    ALLPIN_LOOP (PCB->Data);
    {
        if (openscad_dim_type)
        {
            /* Dimensions in mm. */
            drill_x = COOR_TO_MM * pin->X;
            drill_y = COOR_TO_MM * pin->Y;
            drill_d = COOR_TO_MM * pin->DrillingHole;
        }
        else
        {
            /* Dimensions in mil. */
            drill_x = pin->X / 100;
            drill_y = pin->Y / 100;
            drill_d = pin->DrillingHole / 100;
        }
        fprintf (fp, "        PIN_HOLE (%.2f, %.2f, %.2f, %.2f);\n", drill_x, drill_y, drill_d, board_thickness);
    }
    ENDALL_LOOP; /* End of ALLPIN_LOOP */
    fprintf (fp, "    }\n");
    fprintf (fp, "\n");
    fprintf (fp, "    /* Now insert some element models.\n");
    fprintf (fp, "    INSERT_PART_MODEL (\"Package type\", \"Model name\", Tx, Ty, Rz, \"Side\", \"Value\"); // RefDes */\n");
    /*
     * For each element we calculate the centroid of the footprint.
     * In addition, we need to extract some notion of rotation.  
     * While here generate the OpenSCAD list.
     */
    ELEMENT_LOOP (PCB->Data);
    {
        /* Initialize our pin count and our totals for finding the
         * centriod. */
        pin_cnt = 0;
        sumx = 0.0;
        sumy = 0.0;
        found_pin1 = 0;
        found_pin2 = 0;
        /* Insert this component into the list of OpenSCAD models. */
        openscad = openscad_insert
        (
            EMPTY (NAMEONPCB_NAME (element)),
            EMPTY (DESCRIPTION_NAME (element)),
            EMPTY (VALUE_NAME (element)),
            openscad
        );
        /*
         * Iterate over the pins and pads keeping a running count of how
         * many pins/pads total and the sum of x and y coordinates.
         * 
         * While we're at it, store the location of pin/pad #1 and #2 if
         * we can find them.
         */
        PIN_LOOP (element);
        {
            sumx += (double) pin->X;
            sumy += (double) pin->Y;
            pin_cnt++;
            if (NSTRCMP (pin->Number, "1") == 0)
            {
                pin1x = (double) pin->X;
                pin1y = (double) pin->Y;
                pin1angle = 0.0; /* Pins have no notion of angle. */
                found_pin1 = 1;
            }
            else if (NSTRCMP (pin->Number, "2") == 0)
            {
                pin2x = (double) pin->X;
                pin2y = (double) pin->Y;
                pin2angle = 0.0; /* Pins have no notion of angle. */
                found_pin2 = 1;
            }
        }
        END_LOOP; /* End of PIN_LOOP. */
        PAD_LOOP (element);
        {
            sumx += (pad->Point1.X + pad->Point2.X) / 2.0;
            sumy += (pad->Point1.Y + pad->Point2.Y) / 2.0;
            pin_cnt++;
            if (NSTRCMP (pad->Number, "1") == 0)
            {
                pin1x = (double) (pad->Point1.X + pad->Point2.X) / 2.0;
                pin1y = (double) (pad->Point1.Y + pad->Point2.Y) / 2.0;
                /*
                 * NOTE: We swap the Y points because in PCB, the Y-axis
                 * is inverted, in PCB increasing Y moves down.
                 * We want to have the usual orthogonal Right Hand
                 * system where increasing Y moves coordinates up.
                 */
                pin1angle = (180.0 / M_PI) * atan2 (pad->Point1.Y - pad->Point2.Y,
                    pad->Point2.X - pad->Point1.X);
                found_pin1 = 1;
            }
            else if (NSTRCMP (pad->Number, "2") == 0)
            {
                pin2x = (double) (pad->Point1.X + pad->Point2.X) / 2.0;
                pin2y = (double) (pad->Point1.Y + pad->Point2.Y) / 2.0;
                pin2angle = (180.0 / M_PI) * atan2 (pad->Point1.Y - pad->Point2.Y,
                    pad->Point2.X - pad->Point1.X);
                found_pin2 = 1;
            }
        }
        END_LOOP; /* End of PAD_LOOP. */
        if (pin_cnt > 0)
        {
            x = sumx / (double) pin_cnt;
            y = sumy / (double) pin_cnt;
            if (found_pin1)
            {
                /* Recenter pin #1 onto the axis which cross at the part
                 * centroid. */
                pin1x -= x;
                pin1y -= y;
                pin1y = -1.0 * pin1y;
                /* If only 1 pin, use pin 1's angle. */
                if (pin_cnt == 1)
                {
                    theta = pin1angle;
                }
                else
                {
                    /* If pin #1 is at (0,0) use pin #2 for rotation. */
                    if ((pin1x == 0.0) && (pin1y == 0.0))
                    {
                        if (found_pin2)
                        {
                            theta = openscad_xy_to_angle (pin2x, pin2y);
                        }
                        else
                        {
                            Message
                            ("openscad_print(): unable to figure out angle of element\n"
                                "     %s because pin #1 is at the centroid of the part.\n"
                                "     and I could not find pin #2's location\n"
                                "     Setting to %g degrees\n",
                                UNKNOWN (NAMEONPCB_NAME (element)), theta);
                        }
                    }
                    else
                    {
                        theta = openscad_xy_to_angle (pin1x, pin1y);
                    }
                }
            }
            /* We did not find pin #1. */
            else
            {
                theta = 0.0;
                Message
                ("openscad_print(): unable to figure out angle because I could\n"
                    "     not find pin #1 of element %s\n"
                    "     Setting to %g degrees\n",
                    UNKNOWN (NAMEONPCB_NAME (element)),
                    theta);
            }
            name = openscad_clean_string (EMPTY (NAMEONPCB_NAME (element)));
            modelname = openscad_clean_string (EMPTY (DESCRIPTION_NAME (element)));
            value = openscad_clean_string (EMPTY (VALUE_NAME (element)));
            y = PCB->MaxHeight - y;
            if (openscad_dim_type)
            {
                /* Dimensions in mm. */
                user_x = 0.000254 * x;
                user_y = 0.000254 * y;
            }
            else
            {
                /* Dimensions in mils. */
                user_x = 0.01 * x;
                user_y = 0.01 * y;
            }
            /* Test for the occurrence of a ".fp" suffix in the model
             * name string and strip it from the model name string. */
            openscad_remove_suffix (modelname, "fp");
            /* Determine the package type based on the modelname. */
            package_type = openscad_get_package_type_string (modelname);
            /* Write part model data to file test for dimension type. */
            if (openscad_dim_type)
            {
            fprintf
            (
                fp,
                "    INSERT_PART_MODEL (\"%s\", \"%s\", %.2f, %.2f, %.2f, %s, \"%s\"); // refdes: %s\n",
                package_type,
                modelname,
#if 0
                (double) element->MarkX,
                (double) element->MarkY,
#else
                user_x,
                user_y,
#endif
                theta,
                FRONT (element) == 1 ? "\"top\"" : "\"bottom\"",
                value,
                name
            );
            }
            else
            {
            fprintf
            (
                fp,
                "    INSERT_PART_MODEL (\"%s\", \"%s\", %.2f, %.2f, %.2f, %s, \"%s\"); // refdes: %s\n",
                package_type,
                modelname,
#if 0
                (double) (element->MarkX / 100),
                (double) (element->MarkY / 100),
#else
                user_x,
                user_y,
#endif
                theta,
                FRONT (element) == 1 ? "\"top\"" : "\"bottom\"",
                value,
                name
            );
            }
            free (name);
            free (modelname);
            free (value);
        }
    }
    END_LOOP; /* End of ELEMENT_LOOP. */
    fprintf (fp, "}\n");
    fprintf (fp, "\n");
    fprintf (fp, "/* EOF */ \n");
    fprintf (fp, "\n");
    fclose (fp);
    free (openscad);
    return (0);
}


static void
openscad_do_export (HID_Attr_Val * options)
{
    int i;
    if (!options)
    {
        openscad_get_export_options (0);
        for (i = 0; i < NUM_OPTIONS; i++)
            openscad_values[i] = openscad_options[i].default_val;
        options = openscad_values;
    }
    openscad_filename = options[HA_openscad_file].str_value;
    if (!openscad_filename)
    {
        openscad_filename = "pcb-out.scad";
    }
    openscad_dim_type = options[HA_openscad_mm].int_value;
    openscad_print ();
}

static void
openscad_parse_arguments (int *argc, char ***argv)
{
    hid_register_attributes (openscad_options,
        sizeof (openscad_options) / sizeof (openscad_options[0]));
    hid_parse_command_line (argc, argv);
}

HID openscad_hid = {
  sizeof (HID),
  "openscad",
  "Exports code for a OpenSCAD PCA 3D-model",
  0, 0, 1, 0, 0, 0,
  openscad_get_export_options,
  openscad_do_export,
  openscad_parse_arguments,
  0,	/* openscad_invalidate_lr */
  0,	/* openscad_invalidate_all */
  0,	/* openscad_set_layer */
  0,	/* openscad_make_gc */
  0,	/* openscad_destroy_gc */
  0,	/* openscad_use_mask */
  0,	/* openscad_set_color */
  0,	/* openscad_set_line_cap */
  0,	/* openscad_set_line_width */
  0,	/* openscad_set_draw_xor */
  0,	/* openscad_set_draw_faded */
  0,	/* openscad_set_line_cap_angle */
  0,	/* openscad_draw_line */
  0,	/* openscad_draw_arc */
  0,	/* openscad_draw_rect */
  0,	/* openscad_fill_circle */
  0,	/* openscad_fill_polygon */
  0,	/* openscad_fill_pcb_polygon */
  0,	/* openscad_thindraw_pcb_polygon */
  0,	/* openscad_fill_rect */
  0,	/* openscad_calibrate */
  0,	/* openscad_shift_is_pressed */
  0,	/* openscad_control_is_pressed */
  0,	/* openscad_mod1_is_pressed */
  0,	/* openscad_get_coords */
  0,	/* openscad_set_crosshair */
  0,	/* openscad_add_timer */
  0,	/* openscad_stop_timer */
  0,	/* openscad_watch_file */
  0,	/* openscad_unwatch_file */
  0,	/* openscad_add_block_hook */
  0,	/* openscad_stop_block_hook */
  0,	/* openscad_log */
  0,	/* openscad_logv */
  0,	/* openscad_confirm_dialog */
  0,	/* openscad_close_confirm_dialog */
  0,	/* openscad_report_dialog */
  0,	/* openscad_prompt_for */
  0,	/* openscad_fileselect */
  0,	/* openscad_attribute_dialog */
  0,	/* openscad_show_item */
  0,	/* openscad_beep */
  0,	/* openscad_progress */
  0,	/* openscad_drc_gui */
  0,	/* openscad_edit_attributes */
};

void
hid_openscad_init ()
{
  apply_default_hid (&openscad_hid, 0);
  hid_register_hid (&openscad_hid);
}


/* EOF */
