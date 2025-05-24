/*!
 * \file src/fanout.c
 *
 * \brief Actions for creating a fanout for a BGA package.
 * 
 * \author Copyright (C) 2025 Bert Timmerman <bert.timmerman@xs4all.nl>
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
 *
 * Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 * Contact addresses for paper mail and Email:
 * Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 * haceaton@aplcomm.jhuapl.edu
 */


#include "fanout.h"


/*!
 * \brief Via dimensions lookup table.
 *
 *| BGA Pin Pitch | VIA Name    | Pad Size | Hole Size | Plane Clearance | Solder Mask | Thermal ID | Thermal OD | Thermal Spoke Width | Trace & Space width | Number of Traces |
 *|---------------|-------------|----------|-----------|-----------------|-------------|------------|------------|---------------------|---------------------|------------------|
 *| 1.27          | VIA63-30-85 | 0.635    | 0.30      | 0.85            | 0.00        | None       | None       | None                | 0.125               | 2                |
 *| 1.00          | VIA50-25-70 | 0.55     | 0.25      | 0.70            | 0.00        | None       | None       | None                | 0.10                | 2                |
 *| 0.80          | VIA50-25-70 | 0.50     | 0.25      | 0.70            | 0.00        | None       | None       | None                | 0.10                | 1                |
 *| 0.75          | VIA40-15-60 | 0.40     | 0.15      | 0.60            | 0.00        | None       | None       | None                | 0.10                | 1                |
 *| 0.65          | Via-in-Pad  | 0.45     | 0.15      | 0.55            | 0.65        | 0.45       | 0.55       | 0.10                | 0.075               | 1                |
 *| 0.50          | Via-in-Pad  | 0.275    | 0.125     | 0.40            | 0.50        | 0.30       | 0.40       | 0.10                | 0.05                | 1                |
 *| 0.40          | Via-in-Pad  | 0.25     | 0.125     | 0.35            | 0.25        | 0.25       | 0.35       | 0.075               | 0.05                | 1                |
 *
 * The value "None" in the table above is implemented as a negative
 * value in the array below.
 *
 * All dimensions in mm.
 *
char fanout_via_sizes_lookup_table []=
{
  {1.27, "VIA63-30-85", 0.635, 0.30,  0.85, 0.00, -1.0,  -1.0,  -1.0,   0.125, 2},
  {1.00, "VIA50-25-70", 0.55,  0.25,  0.70, 0.00, -1.0,  -1.0,  -1.0,   0.10,  2},
  {0.80, "VIA50-25-70", 0.50,  0.25,  0.70, 0.00, -1.0,  -1.0,  -1.0,   0.10,  1},
  {0.75, "VIA40-15-60", 0.40,  0.15,  0.60, 0.00, -1.0,  -1.0,  -1.0,   0.10,  1},
  {0.65, "Via-in-Pad",  0.45,  0.15,  0.55, 0.65,  0.45,  0.55,  0.10,  0.075, 1},
  {0.50, "Via-in-Pad",  0.275, 0.125, 0.40, 0.50,  0.30,  0.40,  0.10,  0.05,  1},
  {0.40, "Via-in-Pad",  0.25,  0.125, 0.35, 0.25,  0.25,  0.35,  0.075, 0.05,  1},
}
*/

/*!
 * \brief Allocate memory for a \c FanOutStruct.
 *
 * Fill the memory contents with zeros.
 *
 * \return exit with \c EXIT_FAILURE when no memory was allocated,
 * a pointer to the allocated memory when successful.
 */
FanOutStruct *
fanout_new ()
{
  FanOutStruct *retval = NULL;
  size_t size;

  size = sizeof (FanOutStruct);

  /* avoid malloc of 0 bytes */
  if (size == 0) size = 1;
  retval = malloc (size);

  if (retval == NULL)
  {
    fprintf (stderr,
      (_("Critical error in %s () could not allocate memory.\n")),
      __FUNCTION__);
    exit (EXIT_FAILURE);
  }
  else
  {
    memset (retval, 0, size);
  }

  return (retval);
}


/*!
 * \brief Free the allocated memory for a \c FanOutStruct and all it's
 * data fields.
 *
 * \return \c EXIT_SUCCESS when done, or \c EXIT_FAILURE when an error
 * occurred.
 */
static int
fanout_free
(
        FanOutStruct *fanout
                /*!< A pointer to the memory occupied by the
                 * \c FanOutStruct. */
)
{
  if (fanout == NULL)
  {
    fprintf (stderr,
      (_("Error in %s () a NULL pointer was passed.\n")),
      __FUNCTION__);
    return (EXIT_FAILURE);
  }

  free (fanout->refdes);
  free (fanout->footprint_name);
  free (fanout->element_name);
  free (fanout->exceptions);
  free (fanout->layer_stack);
  free (fanout->recognised_layer_names);
  free (fanout->netlist);
  free (fanout->pin_out_information);
  free (fanout->routing_style);
  free (fanout->style);
  free (fanout);
  fanout = NULL;

  return (EXIT_SUCCESS);
}


static const char fanout_syntax[] = N_("Fanout(refdes)");

static const char fanout_help[] = N_("Invoke the Fanout action.");


/* %start-doc actions Fanout

The @code{Fanout()} action creates a Fanout for a given refdes
(BGA only) on the pcb.

@emph{Warning:} Please back up your layout prior to invoking @code{Fanout()}.

@emph{Warning Warning:} Yes, you should really back up your layout prior
to invoking @code{Fanout()}.

@emph{Warning:} for the moment, @code{Fanout} is handling metric
dimensions only (sorry, it's hard coded).

@emph{Example:} @code{Fanout(U1)} will fanout the BGA element with
refdes U1, according to the existing netlist within the loaded pcb file.

If the loaded pcb file has no netlist, a netlist could be imported
before running @code{Fanout()}.

The element needs to contain any and all of the required attribute
attached prior to invoking @code{Fanout()}.

Information on valid attribute key/value combinations is given below.

@emph{Example:} Attribute("fanout_style" "FANOUT_THROUGH_VIA_IN_PAD")

@code{Fanout} requires the following attributes present in the element:
@itemize @bullet
@item "fanout_style" (enumerated, for valid values see below)
@item "number_of_columns" (integer)
@item "number_of_rows" (integer)
@item "pad_diameter" (double)
@item "pad_clearance" (double)
@item "pad_soldermask_clearance" (double)
@item "courtyard_length" (double)
@item "courtyard_width" (double)
@end itemize

Valid values for @code{fanout_style} are:
@itemize @bullet
@item FANOUT_THROUGH_VIA_IN_PAD,
@item FANOUT_BLIND_VIA_IN_PAD, (not yet implemented)
@item FANOUT_MICRO_VIA_IN_PAD, (not yet implemented)
@item FANOUT_QUADRANT_DOG_BONE_THROUGH_VIA, (not yet implemented)
@item FANOUT_QUADRANT_DOG_BONE_BLIND_VIA, (not yet implemented)
@item FANOUT_QUADRANT_DOG_BONE_MICRO_VIA, (not yet implemented)
@end itemize

If required, @code{Fanout()} will run escape traces until the "courtyard"
dimensions provided.

If no "keepout" dimensions are provided @code{Fanout} will limit itself
to extending traces no more than two times the pitch distance of the BGA
balls from the outermost rows and columns.

Any pad name that is recognised will be connected to a plane on a layer
with a familiar name: eg. "VCC", "vcc", "VCCIO", "GND", "DGND", "gnd",
etc. 

%end-doc */

static int
fanout (int argc, char **argv, Coord x, Coord y)
{
  NetListType *fanout_nets;
  FanOutStruct *fanout;
  int element_count;
  PinType *fanout_via;
  Cardinal buried_from;
  Cardinal buried_to;


  /* Let's start finding out what argument(s) we got. */
  if (argc < 1)
  {
    fprintf (stderr, "Error: too few arguments, use: Fanout(refdes)\n");
    return (EXIT_FAILURE);
  }

  if (argc > 1)
  {
    fprintf (stderr, "Error: too many arguments, use: Fanout(refdes)\n");
    return (EXIT_FAILURE);
  }

  /* Allocate memory for the fanout struct. */
  fanout = fanout_new ();

  if (fanout)
  {
    fprintf (stderr, "Info: created a fanout struct at 0x%" PRIXPTR "\n", (uintptr_t) &fanout);
  }
  else
  {
    fprintf (stderr, "Error: received no pointer to a new fanout struct.\n");
  }

  element_count = 0;

  /* Let's see if we can find the element with the refdes given in arg[0]. */
  ELEMENT_LOOP (PCB->Data);
  {
    if (strcmp (argv[0], NAMEONPCB_NAME(element)) == 0)
    {
      /* fprintf ("0x%" PRIXPTR "\n", (uintptr_t) element); */
      fprintf (stderr, "Info: found element 0x%" PRIXPTR "\n", (uintptr_t) element);
      fanout->found_element = element;
      element_count++;
    }
  }
  END_LOOP;

  /* Handle common deviations. */
  if (element_count > 1)
  {
    fprintf (stderr, "Error: more than 1 element found with an identical refdes found, please rename !\n");
    fprintf (stderr, "Error: number of element(s) with an identical refdes found %i .\n", element_count);
    return (EXIT_FAILURE);
  }

  if (element_count == 0)
  {
    fprintf (stderr, "Error: couldn't find element %s.\n", argv[0]);
    return (EXIT_FAILURE);
  }

  /* Find out what unit for dimensions is used. */
  fanout->units = strdup ((char*) AttributeGet (fanout->found_element, "footprint_units"));

  if (strcmp (fanout->units, "mil") == 0)
  {
    fprintf (stderr, "Error: no, we're not going to do imperial dimensions on Fanout().\n");
    return (EXIT_FAILURE);
  }

  /* Find out what fanout style to use. */
  fanout->style = strdup ((char*) AttributeGet (fanout->found_element, "fanout_style"));

  if (fanout->style == NULL)
  {
    fanout->style = strdup ("");
  }

  if ((strcmp (fanout->style, "FANOUT_NO_STYLE") == 0) || (strcmp (fanout->style, "") == 0))
  {
    fprintf (stderr, "Error: no fanout style was defined.\n");
    return (EXIT_FAILURE);
  }

  /* Find out what the number of columns is. */
  errno = 0;
  fanout->num_columns = atoi ((char*) AttributeGet (fanout->found_element, "number_of_columns"));

  if (errno)
  {
    fprintf (stderr, "Error: something went wrong with converting an attribute value (number_of_columns).\n");
    return (EXIT_FAILURE);
  }

  if (fanout->num_columns <= 0)
  {
    fprintf (stderr, "Error: element contains no columns.\n");
    return (EXIT_FAILURE);
  }

  /* Find out what the number of rows is. */
  errno = 0;
  fanout->num_rows = atoi ((char*) AttributeGet (fanout->found_element, "number_of_rows"));

  if (errno)
  {
    fprintf (stderr, "Error: something went wrong with converting an attribute value (number_of_rows).\n");
    return (EXIT_FAILURE);
  }

  if (fanout->num_rows <= 0)
  {
    fprintf (stderr, "Error: element contains no rows.\n");
    return (EXIT_FAILURE);
  }

  /* Find out what the pad diameter is. */
  errno = 0;
  fanout->pad_diameter = strtod ((char*) AttributeGet (fanout->found_element, "pad_diameter"), NULL);

  if (errno)
  {
    fprintf (stderr, "Error: something went wrong with converting an attribute value (pad_diameter).\n");
    return (EXIT_FAILURE);
  }

  if (fanout->pad_diameter <= 0)
  {
    fprintf (stderr, "Error: element contains pads with zero or smaller diameter.\n");
    return (EXIT_FAILURE);
  }

  fprintf (stderr, "Info: found fanout pad diameter attribute: %f .\n", fanout->pad_diameter);

  /* Find the smallest value of the annulus size used in the PCB and the
   * found pad diameter attribute in the element (in mm). */
  fanout->via_annulus_size = MIN (fanout->pad_diameter, (COORD_TO_MM (PCB->minRing)));
  fanout->via_annulus_size = MIN (fanout->via_annulus_size, (COORD_TO_MM (Settings.ViaThickness)));

  /* Find out what the pad clearance is. */
  errno = 0;
  fanout->pad_clearance = strtod ((char*) AttributeGet (fanout->found_element, "pad_clearance"), NULL);

  if (errno)
  {
    fprintf (stderr, "Error: something went wrong with converting an attribute value (clearance).\n");
    return (EXIT_FAILURE);
  }

  if (fanout->pad_clearance <= 0)
  {
    fprintf (stderr, "Warning: element contains pads with zero or smaller clearance. This may lead to shorts.\n");
  }

  fprintf (stderr, "Info: found fanout pad clearance attribute: %f .\n", fanout->pad_clearance);

  /* Find out what the via drill size is. */
  errno = 0;
  fanout->via_drill_size = strtod ((char*) AttributeGet (fanout->found_element, "via_drill_size"), NULL);

  if (errno)
  {
    fprintf (stderr, "Error: something went wrong with converting an attribute value (via_drill_size).\n");
    return (EXIT_FAILURE);
  }

  if (fanout->via_drill_size <= 0)
  {
    fprintf (stderr, "Error: element contains a zero or smaller via drill size.\n");
    return (EXIT_FAILURE);
  }

  fprintf (stderr, "Info: found fanout via drill size attribute: %f\n", fanout->via_drill_size);

  /*! \todo Use the smallest drill size the fab house allows without
   * increasing board costs. Get the minmum drill size from a vendor
   * file, if any. */


  /* Find the smallest value of the used drill size in the pcb (in mm).*/

  fanout->via_drill_size = MIN (fanout->via_drill_size, FANOUT_VIA_MIN_DRILL_SIZE);
  fanout->via_drill_size = MIN (fanout->via_drill_size, (COORD_TO_MM (Settings.ViaDrillingHole)));

  /* Find out what the pad soldermask clearance is. */
  errno = 0;
  fanout->pad_soldermask_clearance = strtod ((char*) AttributeGet (fanout->found_element, "pad_soldermask_clearance"), NULL);

  if (errno)
  {
    fprintf (stderr, "Error: something went wrong with converting an attribute value (soldermask clearance).\n");
    return (EXIT_FAILURE);
  }

  if (fanout->pad_soldermask_clearance <= 0.0)
  {
    fprintf (stderr, "Warning: element contains pads with zero or smaller soldermask clearance. This may lead to bad connections.\n");
  }

  fprintf (stderr, "Info: found fanout pad soldermask clearance attribute: %f\n", fanout->pad_soldermask_clearance);

  /* Find out what the courtyard length is. */
  errno = 0;
  fanout->courtyard_length = strtod ((char*) AttributeGet (fanout->found_element, "courtyard_length"), NULL);

  if (errno)
  {
    fprintf (stderr, "Error: something went wrong with converting an attribute value (courtyard length).\n");
    return (EXIT_FAILURE);
  }

  if (fanout->courtyard_length <= 0.0)
  {
    fprintf (stderr, "Warning: element contains a courtyard with zero or smaller length.\n");
  }

  fprintf (stderr, "Info: found fanout courtyard length attribute: %f\n", fanout->courtyard_length);

  /* Find out what the courtyard width is. */
  errno = 0;
  fanout->courtyard_width = strtod ((char*) AttributeGet (fanout->found_element, "courtyard_width"), NULL);

  if (errno)
  {
    fprintf (stderr, "Error: something went wrong with converting an attribute value (courtyard width).\n");
    return (EXIT_FAILURE);
  }

  if (fanout->courtyard_width <= 0.0)
  {
    fprintf (stderr, "Warning: element contains a courtyard with zero or smaller width.\n");
  }

  fprintf (stderr, "Info: found fanout courtyard width attribute: %f\n", fanout->courtyard_width);

  /* Find out what the pad exceptions are. */
  fanout->exceptions = strdup ((char*) AttributeGet (fanout->found_element, "pin_pad_exceptions"));

  fprintf (stderr, "Info: found fanout pad exceptions attribute: %s\n", fanout->exceptions);

  /*! \todo Check parameters of current routing style and test whether
   * these match for the element. */

  /*! \todo Procure a netlist from the layout if available. */
//  fanout_nets = ProcNetlist (&PCB->NetlistLib);

//  if (!fanout_nets)
//  {
//    fprintf (stderr, "Warning: can't use the netlist because no netlist is provided.\n");
//  }
//  else
//  {
//    NET_LOOP (PCB->Data);
//    {
      /*! \todo Do something with the nets in the netlist provided. */
//    }
//    END_LOOP;
//  }


  /* Do something with the pads in the found element. */


  if (strcmp (fanout->style, "FANOUT_THROUGH_VIA_IN_PAD") == 0)
  {
    /*! \todo Avoid putting through vias in the two outer most rows and columns. */

    /*! \todo Try to find out the "last" copper layer. */

    /* Test whether the found_element is on TOP or BOTTOM side. */
    if (ON_SIDE (fanout->found_element, TOP_SIDE))
    {
      fprintf (stderr, "Info: found element on TOP_SIDE.\n");
      buried_from = 0;
      buried_to = max_copper_layer;
    }
    else
    {
      fprintf (stderr, "Info: found element on BOTTOM_SIDE.\n");
      buried_from = max_copper_layer;
      buried_to = 0;
    }

    PAD_LOOP (fanout->found_element);
    {
      if (pad->Name == NULL)
      {
        pad->Name = strdup ("");
      }

    /* Take the smallest value of the pad size used in the PCB and the found pad thickness. */
    fanout->via_annulus_size = MIN (pad->Thickness, PCB->minRing);

      /* Create a new via. */
      fanout_via = CreateNewViaEx (PCB->Data,
        pad->Point1.X, /* already in coords. */
        pad->Point1.Y, /* already in coords. */
        MM_TO_COORD (fanout->pad_diameter),
        MM_TO_COORD (fanout->pad_clearance),
        MM_TO_COORD (fanout->pad_soldermask_clearance),
        MM_TO_COORD (fanout->via_drill_size),
        pad->Name,
        fanout->via_flags,
        buried_from,
        buried_to);

      if (!fanout_via)
      {
        fprintf (stderr, "Warning: failed to create a through via in pad number: %s with pad name: %s\n", pad->Number, pad->Name);
        return EXIT_FAILURE;
      }
      else
      {
        fprintf (stderr, "Info: created a through via in pad number: %s with pad name: %s\n", pad->Number, pad->Name);
      }

      DrawVia (fanout_via);
      AddObjectToCreateUndoList (VIA_TYPE, fanout_via, fanout_via, fanout_via);

      /*! \todo Add code for traces to courtyard boundaries. */

    }
    END_LOOP; /* PAD_LOOP */
  }


  /* Clean up. */
  fanout_free (fanout);

//  fclose (netlist_fp);

  /* Tell the user we're done. */
  fprintf (stderr, "Info: Successful end of the Fanout action.\n");

  return EXIT_SUCCESS;
}


static HID_Action
fanout_action_list[] =
{
  {"fanout", NULL, fanout, NULL, NULL}
};


REGISTER_ACTIONS (fanout_action_list)


void
hid_fanout_init ()
{
  register_fanout_action_list ();
}


/* EOF */
