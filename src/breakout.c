/*!
 * \file src/breakout.c
 *
 * \brief Breakout for BGA packages.
 *
 * \author Bert Timmerman <bert.timmerman@xs4all.nl>
 *
 * \note All calculations are done in mm.
 *
 * Definitions:\n
 *
 * <dl>
 *   <dt>Breakout</dt>
 *   <dd>The combination of fanouts and escape traces, which allow
 *       routing out of the BGA pad array to the perimeter of the device
 *       prior to general routing of the PCB.</dd>
 *   <dt>Fanout Pattern</dt>
 *   <dd>When adding fanouts to the BGA, enabling routing on the inner
 *       layer, the pattern may vary considerably depending on the layer
 *       stackup, via model, and pin density. The pattern may range from
 *       simple quadrant-matrix to a set of complex alternating
 *       arrangements. Using the appropriate fanout patttern will make a
 *       significant difference on the success of breakouk and routing
 *       of the BGA.</dd>
 *   <dt>Escape Route</dt>
 *   <dd>A series of traces from fanout (via) to perimeter.</dd>
 *   <dt>Perimeter</dt>
 *   <dd>Outline of the package. Sometimes the perimeter extends a short
 *       distance to stay clear of vias which are part of the breakout.</dd>
 *   <dt>Through Via</dt>
 *   <dd>A via that extends from the top to the bottom of the board.</dd>
 *   <dt>Blind Via</dt>
 *   <dd>A via that begins on an outer layer and ends on an inner layer.</dd>
 *   <dt>Buried Via</dt>
 *   <dd>This type of via starts and ends on an inner layer.</dd>
 *   <dt>Micro Via</dt>
 *   <dd>This via has a hole diameter less than or equal to 0.15mm.</dd>
 *   <dt>Any Layer Via</dt>
 *   <dd>These are short micro-vias that individually span only a pair
 *       of layers and are stacked together to result in a span between
 *       any two layers.</dd>
 *   <dt>NSEW Breakout</dt>
 *   <dd>North, South, East, and West routing of the escape traces. This
 *       means the escape traces are routed in all four directions on
 *       the same layer.</dd>
 *   <dt>Layer Bsased Breakout</dt>
 *   <dd>The escape traces are routed in the direction of the layer bias
 *       as oppossed to NSEW routing. Escape traces are also routed in
 *       the direction of the connection according to the layer bias.</dd>
 *   <dt>Layer Stackup</dt>
 *   <dd>Early in the design process, the layer stackup will be defined.
 *       If the board has large and dense BGAs, High Density
 *       Interconnect (HDI) with a laminated core and buildup layers may
 *       be required. There are many different options using various
 *       materials and processes. Cost and reliability are usually the
 *       primary factors in determining the stackup and you will have to
 *       balance the tradeoff between layer count and fabrication
 *       processes to reach your goals.</dd>
 *   <dt>Via Models</dt>
 *   <dd>Within the context of any given layer stackup, you have many
 *       options regarding via models. The decision on which type of via
 *       to use (thru, laminated blind and buried or HDI micro vias)
 *       will likely be driven by the density of the board and the BGA
 *       packages. There are also options regarding vias inside pads and
 *       stacking that affect cost. In addition to this, board
 *       fabricators tend to focus on a limited set of processes thereby
 *       limiting your choice of vendors, depending on the technology
 *       you desire. From the design point of view, choosing the
 *       appropriate via models directly impacts the route-ability of
 *       the board.</dd>
 *   <dt>Design Rules</dt>
 *   <dd>PCB fabricators continue to find methods that allow for further
 *       miniaturization and increased reliability. The design rules
 *       have to balance the tradeoffs between cost, signal integrity,
 *       and route-ability.</dd>
 *   <dt>Signal Integrity</dt>
 *   <dd>Although the fabricators continue to improve their processes
 *       and produce reliable boards with smaller and features and
 *       clearances, maintaining signal integrity at high performance
 *       levels usually requires greater spacing between critical nets,
 *       especially to manage crosstalk effects at higher speeds. This
 *       conflict is exasperated with high pin-count and dense BGAs.
 *       Choosing appropriate layer stackups and via models will not
 *       only improve route-ability but signal integrity as well.</dd>
 *   <dt>Power Integrity</dt>
 *   <dd>Managing power distribution effectively for large pin-count
 *       BGAs is a challenge and is significantly impacted by the layer
 *       stackup. There are methods that can minimize the number
 *       decoupling capacitors required, thereby increasing the space
 *       available for signal routing.</dd>
 *   <dt></dt>
 *   <dd></dd>
 * </dl>
 *
 * Below follows a rough outline of what the execution of events should
 * look like:\n
 * \n
 * <ol type = "1">
 *   <li>Get a list of elements selected.\n
 * \n
 *   <li>For each selected element do:\n
 * \n
 *   <ol type = "A">
 *     <li>Check if the element is a BGA package (check for attributes).\n
 *         If that doesn't work start guessing around and ask the User
 *         for confirmation.\n
 * \n
 *     <li>Get the part description (check for attribute).\n
 * \n
 *     <li>Get the package length and width dimensions (check for
 *         attributes).\n
 * \n
 *     <li>Determine the bounding box for the breakout traces handover
 *         (typically something like the outline +100 mil).\n
 * \n
 *     <li>Get the number of rows (check for attributes).\n
 * \n
 *     <li>Get the number of columns (check for attributes).\n
 * \n
 *     <li>Get the vertical pitch (check for attributes).\n
 * \n
 *     <li>Get the horizontal pitch (check for attributes).\n
 * \n
 *     <li>Get the minimum/maximum trace width (check for attributes).\n
 * \n
 *     <li>Get the minimum/maximum required clearance.\n
 * \n
 *     <li>Get the "blind via allowed" flag (check for attributes).\n
 * \n
 *     <li>Get the "micro via allowed" flag (check for attributes).\n
 * \n
 *     <li>Get the "via in pad allowed" flag (check for attributes).\n
 * \n
 *     <li>Calculate the max number of traces possible between pads
 *         (routes per channel).\n
 * \n
 *     <ol type = "a">
 *       <li>For the number of horizontal routes in a channel:\n
 * <pre>
 *             (vert_pitch - pad_diam)
 * n_hor = -------------------------------
 *         (trace_width + trace_clearance)
 * </pre>
 *       <li>For the number of vertical routes in a channel:\n
 * <pre>
 *              (hor_pitch - pad_diam)
 * n_vert = -------------------------------
 *          (trace_width + trace_clearance)
 * </pre>
 *     </ol>
 *     <li>Calculate number of signal layers needed.\n
 *         A quick estimate is:\n
 * <pre>
 *                            number of signals
 * number of layers = ---------------------------------------
 *                    (routing channels x routes per channel)
 * </pre>
 *
 *         For the number of routing channels:\n
 * \n
 *         n_hor = number of columns - 1\n
 * \n
 *         n_vert = number of rows - 1\n
 * \n
 *     <li>Determine the layer Stack-up order.\n
 *         For example an 8-layer board for a Xilinx FG676 BGA could look
 *         like the Stack-up below:\n
 *     <ol>
 *       <li value = "1">ground (top)
 *       <li value = "2">signal
 *       <li value = "3">signal
 *       <li value = "4">plane
 *       <li value = "5">plane
 *       <li value = "6">signal
 *       <li value = "7">signal
 *       <li value = "8">ground (bottom)
 *     </ol>
 * \n
 *     <li>For every pad in the element do:\n
 *     <ol type = "a">
 *       <li>Get the pad diameter.
 *       <li>Get the pad copper clearance.
 *       <li>Get the pad label (or connectivity from the netlist).
 *       <li>Determine the quadrant in which the pad resides.
 *       <li>Determine wether the pad is an inner or outer pad.
 *       <ol type = "I">
 *         <li>Outer pad: dogbone has no via and has an escape trace.
 *         <li>Inner pad: dogbone has a via to a different layer and has
 *             an escape trace on that layer.
 *       </ol>
 *     </ol>
 *        and store those values in a linked list of struct DogboneType.
 * \n
 *     <li>Start drawing entities:\n
 *     <ol type = "a">
 *       <li>Create traces on the top layer.
 *       <li>Create dogbones for inner pads in all quadrants.
 *       <li>Create vias from the ground plane layer to the dogbones on the
 *           top layer (use blind vias if this is allowed).
 *       <li>Create vias from the power plane layer to the dogbones on the
 *           top layer (use blind vias if this is allowed).
 *       <li>Create traces on the intermediate copper layers.
 *       <li>Create vias from the intermediate copper layers to the top
 *           layer (use buried vias if this is allowed).
 *      </ol>
 *    </ol>
 * \n
 *   <li>Update the ratlines.
 * </ol>
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2018 PCB Contributors
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 *
 * <hr>
 */


#include <stdio.h>

#include "globalconst.h"
#include "global.h"
#include "error.h"
#include "hid.h"
#include "breakout.h"


static const char breakout_syntax[] = "Breakout(Selected|All)";

static const char breakout_help[] = "Create a breakout for BGA package type elements.";

/* %start-doc actions Breakout

@table @code

@item Selected
Creates a Breakout for the selected elements (single/multiple) .

@item All
Check all elements in the layout and test for BGA type elements.

Ask the User for confirmation.

@end table

Create a Breakout for every BGA type element.

%end-doc */


typedef enum
selections
{
  F_none,
  F_All,
  F_Selected,
};


/*!
 * \brief Primary routing directions.
 *
 * Directions for escape traces to the perimeter bounding box limits.
 */
typedef enum
pr_directions
{
  N,
  E,
  S,
  W
};


/*!
 * \brief Secondary routing directions.
 *
 * Directions for dogbone traces and vias.
 */
typedef enum
sec_directions
{
  NE,
  SE,
  SW,
  NW
};


/*!
 * \brief Struct containing all data for creating a single Dogbone.
 *
 * To be used as a single linked list.
 */
typedef struct
fanout_struct
{
  struct pad_st *pad;
    /*!< a pointer to the BGA pad of the element.\n
     * This gives an implicit link to the coordinates of the pad. */
  struct LayerType *outter_layer;
    /*!< layer for the fanout. */
  struct LayerType *inner_layer;
    /*!< destination layer for the fanout via. */
  int has_via;
    /*!< Flag if a via is required:\n
     * <ol>
     *   <li value = "0">No via required (outer pad).
     *   <li value = "1">Via required (inner pad).
     * </ol>
     */
  struct pin_st *via;
    /*!< a fanout via. */
  struct LineType *line;
    /*!< a trace connecting pad and via. */
  struct FanoutType *next;
    /*!< Pointer to the \c next FanoutType.\n
     * \c NULL in the last FanoutType. */
} FanoutType;


/*!
 * \brief Struct containing all data for creating a single escape route.
 *
 * To be used as a single linked list.
 */
typedef struct
escape_route_struct
{
  Cardinal number_of_traces;
    /*!< number of traces in the escape route. */
  TraceType *traces;
    /*!< single linked list containing all traces forming an escape
     * route from fanout via, or pad, to the bounding box (perimeter). */
  struct EscapeRouteType *next;
    /*!< Pointer to the \c next EscapeRouteType.\n
     * \c NULL in the last EscapeRouteType. */
} EscapeRouteType;


/*!
 * \brief Struct containing all data for creating a fanout.
 *
 * To be used as a single linked list.
 */
typedef struct
breakout_struct
{
  struct ElementType *element;
    /*!< the element to be fanned out. */
  int via_mode;
    /*!< via mode (bit coded):
     * <ol>
     *   <li value = "0">No via mode specified, ask the User.
     *   <li value = "1">dogbone vias.
     *   <li value = "2">blind vias.
     *   <li value = "4">micro vias.
     *   <li value = "8">vias in pads.
     *   <li value = "16">aligned vias (create space for channels).
     * </ol> */
  struct BoxType *perimeter;
    /*!< bounding box for fanout traces handover. */
  struct RouteStyleType *route_style;
    /*!< routing style. */
  Cardinal number_of_fanouts;
    /*!< number of fanouts. */
  struct FanoutType *fanouts;
    /*!< single linked list containing all fanouts. */
  struct BreakoutType *next;
    /*!< Pointer to the \c next BreakoutType.\n
     * \c NULL in the last BreakoutType. */
} BreakoutType;


/*!
 * \brief Allocate memory for a \c FanoutType struct.
 *
 * Fill the memory contents with zeros.
 *
 * \return \c NULL when no memory was allocated, a pointer to the
 * allocated memory when succesful.
 */
FanoutType *
fanout_new ()
{
        FanoutType *fanout = NULL;
  size_t size;

  size = sizeof (FanoutType);
  /* avoid malloc of 0 bytes */
  if (size == 0) size = 1;
  if ((fanout = malloc (size)) == NULL)
  {
    fprintf (stderr,
      (_("Error in %s () could not allocate memory.\n")),
      __FUNCTION__);
    fanout = NULL;
  }
  else
  {
    memset (fanout, 0, size);
  }
  return (fanout);
}


/*!
 * \brief Allocate memory and initialize data fields in a \c FanoutType.
 * 
 * \return \c NULL when no memory was allocated, a pointer to the
 * allocated memory when succesful.
 */
FanoutType *
fanout_init
(
  FanoutType *fanout
    /*!< a pointer to the FanoutType. */
)
{
  /* Do some basic checks. */
  if (fanout == NULL)
  {
    fprintf (stderr,
      (_("Warning in %s () a NULL pointer was passed.\n")), __FUNCTION__);
                fanout = fanout_new ();
  }
  if (fanout == NULL)
  {
    fprintf (stderr,
      (_("Error in %s () could not allocate memory.\n")), __FUNCTION__);
    return (NULL);
  }
  /* Initialize members. */
  fanout-next = NULL;
  return (fanout);
}


/*!
 * \brief Allocate memory for a \c BreakoutType struct.
 *
 * Fill the memory contents with zeros.
 *
 * \return \c NULL when no memory was allocated, a pointer to the
 * allocated memory when succesful.
 */
BreakoutType *
breakout_new ()
{
        BreakoutType *breakout = NULL;
  size_t size;

  size = sizeof (BreakoutType);
  /* avoid malloc of 0 bytes */
  if (size == 0) size = 1;
  if ((breakout = malloc (size)) == NULL)
  {
    fprintf (stderr,
      (_("Error in %s () could not allocate memory.\n")),
      __FUNCTION__);
    breakout = NULL;
  }
  else
  {
    memset (breakout, 0, size);
  }
  return (breakout);
}


/*!
 * \brief Allocate memory and initialize data fields in a \c BreakoutType.
 * 
 * \return \c NULL when no memory was allocated, a pointer to the
 * allocated memory when succesful.
 */
BreakoutType *
breakout_init
(
  BreakoutType *fanout
    /*!< a pointer to the BreakoutType. */
)
{
  /* Do some basic checks. */
  if (breakout == NULL)
  {
    fprintf (stderr,
      (_("Warning in %s () a NULL pointer was passed.\n")), __FUNCTION__);
                breakout = breakout_new ();
  }
  if (breakout == NULL)
  {
    fprintf (stderr,
      (_("Error in %s () could not allocate memory.\n")), __FUNCTION__);
    return (NULL);
  }
  /* Initialize members. */
  breakout-next = NULL;
  return (breakout);
}


static int
get_function_ID (const char *s)
{
  int i;

  if (!s)
    return F_none;
  for (i = 0; i < ENTRIES (functions); ++i)
    if (functions[i] && strcasecmp (s, functions[i]) == 0)
      return i;
  return F_none;
}


static int
ActionBreakout (int argc, char **argv, Coord x, Coord y)
{
  char *function = NULL;
  int fnid;
  int i = 0;
  char * element_type;
  int nsel = 0;
  FanoutType *fanout = NULL;
  FanoutType *fanout_iter = NULL;

  Message (_("Breakout plugin\nUnder Construction ;-)\n"));
  if ((argc == 2) && (strcmp (argv[0], "Echo") == 0))
  {
    Message (_("Echoing arguments: %s\n", argv[1]));
  }

  function = ARG (0);

#ifdef DEBUG
  printf ("Entered ActionBreakout, executing function %s\n", function);
#endif

  if (function)
  {
    fnid = get_function_ID (function);
    if (!(fnid == F_All || fnid == F_Selected))
      AFAIL (fanout);
  }
  else
  {
    AFAIL (fanout);
  }

  /* Initialize at least one fanout (first in the single linked list). */
  fanout = fanout_new ();
  fanout = fanout_init (fanout);

  /* Get a list of BGA package type selected elements. */
  if (fnid == F_Selected)
  {
    ELEMENT_LOOP (PCB->Data);
    {
      if (TEST_FLAG (SELECTEDFLAG, element))
      {
        if (nsel == 0)
        {
          fanout->element = element;
        }
        else
        {
          fanout_iter->element = element;
          fanout_iter->next = fanout_new ();
          fanout_iter->next = fanout_init (fanout_iter->next)
          fanout_iter->next = fanout_iter;
        }
        nsel++;
      }
      else
      {
        continue;
      }
    }
    END_LOOP;
    Message (_("Found %i selected elements.", nsel));

    if (nsel == 0)
    {
      Message (_("No elements selected\n"));
      return 0;
    }
  }

  /* Get a list of BGA package type selected elements. */
  if (fnid == F_All)
  {
    ELEMENT_LOOP (PCB->Data);
    {
      /* Check if the element has a BGA package attribute. */
      if (AttributeGet (element, "element_type::BGA"))
      {
        Message (_("Found element of type: %s\n", element_type));
      }

      /* Check if the element has a refdes. */
      if (EMPTY_STRING_P (NAMEONPCB_NAME (element)))
      {
        Message (_("Skipping element, no reference designator found\n"));
        continue;
      }
      else
      {
        /* Check if the reference designator string gives a clue for a
         * BGA type package element and ask for confirmation. */
      }

      /* Check if the element has a description. */
      if (EMPTY_STRING_P (DESCRIPTION_NAME (element)))
      {
        Message (_("Skipping element with reference designator %s, no footprint name found\n"),
          NAMEONPCB_NAME (element));
        continue;
      }
      else
      {
        /* Check if the description string gives a clue for a BGA type
         * package element and ask for confirmation. */
      }

      /* Check if the element has a value. */
      if (EMPTY_STRING_P (VALUE_NAME (element)))
      {
        Message (_("Skipping element with reference designator %s, no value found\n"),
          NAMEONPCB_NAME (element));
        continue;
      }
      else
      {
        /* Check if the value string gives a clue for a BGA type package
         * element and ask for confirmation. */
      }

      /* Check if the number of pads and configuration of pads gives
       * reason for the element under investigation to have a BGA type
       * package and ask for confirmation. */


    }
    END_LOOP;

  /* Get the part description (check for attribute). */

  /* Get the package length and width dimensions (check for attributes). */

  /* Determine the bounding box for the fanout traces handover
   * (typically something like the outline +100 mil). */


#ifdef DEBUG
  printf ("... Leaving ActionBreakout.\n");
#endif

  return 0;
}


static HID_Action breakout_action_list[] =
{
  {"Fanout", NULL, ActionBreakout, breakout_help, breakout_syntax}
};

REGISTER_ACTIONS (breakout_action_list)


void
pcb_plugin_init()
{
  printf("Loading plugin: breakout\n");
  register_breakout_action_list();
}


/* EOF */

