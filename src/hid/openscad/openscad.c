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
 * \author Copyright 2010, 2011 Bert Timmerman <bert.timmerman@xs4all.nl>
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
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "global.h"
#include "data.h"
#include "error.h"
#include "misc.h"

#include "hid.h"
#include "../hidint.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

typedef struct {
    double x, y;
    double theta;
    char *str;
    int top;
} ElementInfo;

static HID_Attribute openscad_options[] =
{
/*
%start-doc options "OpenSCAD Export"
@ftable @code
@item --output-file <string>
Name of the OpenSCAD top model file.
@end ftable
%end-doc
*/
    {
        "output-file",
        "OpenSCAD output top model filename",
        HID_String,
        0, 0, {0, 0, 0}, 0, 0
    },
#define HA_openscad_file 0

/*
%start-doc options "OpenSCAD Export"
@ftable @code
@item --units <unit>
Unit of OpenSCAD dimensions. Defaults to mil.
@end ftable
%end-doc
*/
    {
        "units",
        "tick for OpenSCAD dimensions in mm instead of mils",
        HID_Boolean,
        0, 0, {0, 0, 0}, 0, 0
    },
#define HA_openscad_mm 1

/*
%start-doc options "OpenSCAD Export"
@ftable @code
@item --pcb-thickness <real>
Printed circuit board thickness.
@end ftable
%end-doc
*/
    {
        "pcb-thickness",
        "Printed circuit board thickness",
        HID_Real,
        0, 100, {0, 0, 62.00}, 0, 0
    },
#define HA_openscad_thickness 2

/*
%start-doc options "OpenSCAD Export"
@ftable @code
@item --scad-script <string>
Script to generate 'use\ncommand\n'.
@end ftable
%end-doc
*/
    {
        "scad-script",
        "Script",
        HID_String,
        0, 0, {0, 0, 0}, 0, 0
    },
#define HA_openscad_command 3
};


#define NUM_OPTIONS (sizeof(openscad_options)/sizeof(openscad_options[0]))


static HID_Attr_Val openscad_values[NUM_OPTIONS];
static char *openscad_filename;
static int openscad_dim_type;
static double openscad_pcb_thickness;
static char *openscad_command;

static double
openscad_to_unit(double v)
{
    return openscad_dim_type ? COORD_TO_MM (v) : COORD_TO_MIL (v);
}

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

static GList *
openscad_simple_outline(void)
{
    GList *points;
    static PointType A, B, C, D;

    D.X = B.X = openscad_to_unit (PCB->MaxWidth);
    D.Y = C.Y = openscad_to_unit (PCB->MaxHeight);

    points = g_list_append (NULL, &A);
    points = g_list_append (points, &B);
    points = g_list_append (points, &C);
    points = g_list_append (points, &D);

    return points;
}

static int
pointcmp(PointType *a, PointType *b)
{
    return a->X != b->X || a->Y != b->Y;
}

static GList *
openscad_trace_outline(void)
{
    LayerType *outline_layer;
    GList *unused;
    GList *sorted;
    PointType *start;
    PointType *next;
    LineType *line;
    GList *iter;
    int i;
    int progress;

    outline_layer = NULL;
    for (i = 0; i < max_copper_layer; i++)
    {
        LayerType *layer = PCB->Data->Layer + i;
        if (strcmp (layer->Name, "outline") == 0 ||
            strcmp (layer->Name, "route") == 0)
        {
            outline_layer = layer;
            break;
        }
    }

    if (!outline_layer || !outline_layer->Line)
        return openscad_simple_outline ();

    unused = g_list_copy(outline_layer->Line);
    line = unused->data;
    unused = g_list_delete_link(unused, unused);

    start = &line->Point1;
    next = &line->Point2;
    sorted = g_list_append(NULL, start);

    progress = 1;
    while (progress && unused && pointcmp(next, start))
    {
         progress = 0;
         for (iter = unused; iter; iter = g_list_next(iter))
         {
             line = iter->data;
             if (!pointcmp(&line->Point1, next) || !pointcmp(&line->Point2, next))
             {
                 sorted = g_list_append(sorted, next);
                 next = !pointcmp(&line->Point1, next) ? &line->Point2 : &line->Point1;
                 unused = g_list_delete_link(unused, iter);
                 progress = 1;
                 break;
             }
         }
    }

    if (!progress || unused)
    {
        g_list_free(unused);
        g_list_free(sorted);
        return openscad_simple_outline ();
    }

    return sorted;
}

static void
openscad_get_statement (ElementType *element, double x, double y, double theta, GList **use, GList **elements)
{
    int pipefd[2];
    pid_t cpid;
    FILE *fp;
    int status;
    char line[4096];
    char *strx;
    char *stry;
    char *strtheta;

    if (pipe (pipefd) < 0)
    {
        perror ("pipe");
        return;
    }

    cpid = fork ();
    switch (cpid) {
    case -1:
        close (pipefd[0]);
        close (pipefd[1]);
        return;
    case 0:
        close (pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        asprintf(&strx, "%f", x);
        asprintf(&stry, "%f", y);
        asprintf(&strtheta, "%f", theta);
        execl(openscad_command, openscad_command,
            EMPTY (DESCRIPTION_NAME (element)),
            EMPTY (NAMEONPCB_NAME (element)),
            EMPTY (VALUE_NAME (element)), NULL);
        _exit(EXIT_FAILURE);
    }
    close (pipefd[1]);
    fp = fdopen(pipefd[0], "r");

    line[0] = '\0';
    fgets (line, sizeof(line), fp);
    if (strlen (line) > 1)
    {
        GList *iter;
        line[strlen (line) - 1] = '\0';
        for (iter = *use; iter; iter = g_list_next(iter))
        {
            if (!strcmp (iter->data, line))
                break;
        }
        if (!iter)
            *use = g_list_append (*use, strdup (line));
    }

    line[0] = '\0';
    fgets (line, sizeof(line), fp);
    if (strlen (line) > 1)
    {
        ElementInfo *info = malloc(sizeof(ElementInfo));
        line[strlen (line) - 1] = '\0';
        info->x = x;
        info->y = y;
        info->theta = theta;
	info->top = FRONT (element) == 1;
        info->str = strdup (line);
        *elements = g_list_append (*elements, info);
    }

    fclose(fp);
    waitpid(cpid, &status, 0);
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
    time_t currenttime;
    FILE *fp;
    double board_thickness;
    GList *iter;
    GList *use = NULL;
    GList *elements = NULL;
    GList *sorted;

    /*
     * For each element we calculate the centroid of the footprint.
     * In addition, we need to extract some notion of rotation.
     * While here generate the OpenSCAD list.
     */
    ELEMENT_LOOP (PCB->Data);
    {
        /* Initialize our pin count and our totals for finding the
         * centriod. */
        double sumx, sumy;
        double pin1x = 0.0;
        double pin1y = 0.0;
        double pin1angle = 0.0;
        double pin2x = 0.0;
        double pin2y = 0.0;
        int found_pin1;
        int found_pin2;
        int pin_cnt;

        pin_cnt = 0;
        sumx = 0.0;
        sumy = 0.0;
        found_pin1 = 0;
        found_pin2 = 0;
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
                found_pin2 = 1;
            }
        }
        END_LOOP; /* End of PAD_LOOP. */
        if (pin_cnt > 0)
        {
            double x;
            double y;
            double user_x;
            double user_y;
            double theta;

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
            //y = PCB->MaxHeight - y;
            user_x = openscad_to_unit (x);
            user_y = openscad_to_unit (PCB->MaxWidth - y);

            openscad_get_statement (element, user_x, user_y, theta, &use, &elements);
        }
    }
    END_LOOP; /* End of ELEMENT_LOOP. */

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
    for (iter = use; iter; iter = g_list_next(iter))
    {
        fprintf (fp, "use <%s>\n", (char *) iter->data);
        free(iter->data);
    }
    g_list_free(use);
    fprintf (fp, "\n");
    fprintf (fp, "$fn = 16;\n");
    fprintf (fp, "\n");
    fprintf (fp, "\n");
    fprintf (fp, "COPPER = [0.88, 0.78, 0.5];\n");
    fprintf (fp, "FR4 = [0.7, 0.67, 0.6, 0.95];\n");
    fprintf (fp, "DRILL_HOLE = [1.0, 1.0, 1.0];\n");
    fprintf (fp, "\n");
    fprintf (fp, "\n");
    fprintf (fp, "module VIA_HOLE(x, y, diameter, depth) {\n");
    fprintf (fp, "    translate([x, y, -depth*.05]) {\n");
    fprintf (fp, "        color (COPPER)\n");
    fprintf (fp, "        cylinder(r = (diameter / 2), h = depth*1.1, center = false);\n");
    fprintf (fp, "    }\n");
    fprintf (fp, "}\n");
    fprintf (fp, "\n");
    fprintf (fp, "\n");
    fprintf (fp, "module PIN_HOLE(x, y, diameter, depth) {\n");
    fprintf (fp, "    translate([x, y, -depth*.05]) {\n");
    fprintf (fp, "        color (DRILL_HOLE)\n");
    fprintf (fp, "        cylinder(r = (diameter / 2), h = depth*1.1, center = false);\n");
    fprintf (fp, "    }\n");
    fprintf (fp, "}\n");
    fprintf (fp, "\n");
    fprintf (fp, "\n");
    fprintf (fp, "%s ();\n", EMPTY (PCB->Name));
    fprintf (fp, "\n");
    fprintf (fp, "\n");
    fprintf (fp, "module %s ()\n", EMPTY (PCB->Name));
    fprintf (fp, "{\n");
    /* Lookup the board dimensions and create an entry in the OpenSCAD
     * file. */
    board_thickness = openscad_pcb_thickness;
    fprintf (fp, "    /* Modelling a printed circuit board based on maximum dimensions. */\n");
    fprintf (fp, "    difference ()\n");
    fprintf (fp, "    {\n");

    fprintf (fp, "        color (FR4)\n");
    fprintf (fp, "        {\n");
    fprintf (fp, "            linear_extrude (height=%f)\n", board_thickness);
    fprintf (fp, "            polygon ([ ");
    sorted = openscad_trace_outline ();
    for (iter = sorted; iter; iter = g_list_next(iter))
    {
        PointType *p = iter->data;
        fprintf(fp, "[%f, %f]", openscad_to_unit (p->X), openscad_to_unit (PCB->MaxWidth - p->Y));
        if (g_list_next(iter))
            fprintf(fp, ", ");
    }
    g_list_free (sorted);
    fprintf (fp, " ]);\n");
    fprintf (fp, "        }\n");
    fprintf (fp, "        /* Now subtract some via holes. */\n");
    /* Now subtract some via holes. */
    VIA_LOOP (PCB->Data);
    {
        fprintf (fp, "        VIA_HOLE (%.2f, %.2f, %.2f, %.2f);\n",
                 openscad_to_unit (via->X),
                 openscad_to_unit (PCB->MaxWidth - via->Y),
                 openscad_to_unit (via->DrillingHole),
                 board_thickness);
    }
    END_LOOP; /* End of VIA_LOOP */
    /* Now subtract some pin holes. */
    fprintf (fp, "        /* Now subtract some pin holes. */\n");
    ALLPIN_LOOP (PCB->Data);
    {
        fprintf (fp, "        PIN_HOLE (%.2f, %.2f, %.2f, %.2f);\n",
                 openscad_to_unit (pin->X),
                 openscad_to_unit (PCB->MaxWidth - pin->Y),
                 openscad_to_unit (pin->DrillingHole),
                 board_thickness);
    }
    ENDALL_LOOP; /* End of ALLPIN_LOOP */
    fprintf (fp, "    }\n");
    fprintf (fp, "\n");
    fprintf (fp, "    /* Now insert some element models.*/\n");
    for (iter = elements; iter; iter = g_list_next(iter))
    {
        ElementInfo *info = iter->data;
        fprintf (fp, "    translate ([%f, %f, %f]) {\n",
            info->x, info->y, info->top ? board_thickness : 0);
        fprintf (fp, "        rotate ([0, %f, %f]) {\n",
            info->top ? 0.0 : 180.0, info->theta);
        fprintf (fp, "            %s\n", info->str);
        fprintf (fp, "        }\n");
        fprintf (fp, "    }\n");
        free(info->str);
        free(info);
    }
    g_list_free(elements);

    fprintf (fp, "}\n");
    fprintf (fp, "\n");
    fprintf (fp, "/* EOF */ \n");
    fprintf (fp, "\n");
    fclose (fp);
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
    /* Get the filename. */
    openscad_filename = options[HA_openscad_file].str_value;
    if (!openscad_filename)
    {
        openscad_filename = strdup ("pcb-out.scad");
    }
    /* Get the dimension type. */
    openscad_dim_type = options[HA_openscad_mm].int_value;
    /* Get the pcb thickness. */
    openscad_pcb_thickness = options[HA_openscad_thickness].real_value;

    openscad_command = options[HA_openscad_command].str_value;
    if (!openscad_command)
        return;

    /* Call the worker function which is creating the output files. */
    openscad_print ();
}


static void
openscad_parse_arguments (int *argc, char ***argv)
{
    hid_register_attributes (openscad_options,
        sizeof (openscad_options) / sizeof (openscad_options[0]));
    hid_parse_command_line (argc, argv);
}


HID openscad_hid;


void
hid_openscad_init ()
{
  memset (&openscad_hid, 0, sizeof (HID));
  common_nogui_init (&openscad_hid);
  openscad_hid.struct_size = sizeof (HID);
  openscad_hid.name = "openscad";
  openscad_hid.description = "OpenSCAD export";
  openscad_hid.exporter = 1;
  openscad_hid.get_export_options  = openscad_get_export_options;
  openscad_hid.do_export           = openscad_do_export;
  openscad_hid.parse_arguments     = openscad_parse_arguments;
  hid_register_hid (&openscad_hid);
}


/* EOF */
