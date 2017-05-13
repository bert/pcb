/*!
 * \file src/layerflags.c
 *
 * \brief Functions for changing layer flags.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2007 DJ Delorie <dj@delorie.com>
 *
 * Copyright (C) 2015 Markus "Traumflug" Hitter <mah@jump-ing.de>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <ctype.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "globalconst.h"
#include "global.h"
#include "compat.h"
#include "data.h"
#include "error.h"
#include "hid.h"
#include "strflags.h"
#include "layerflags.h"


/*!
 * \brief These are the names of all the Layertypes defined in hid.h.
 *
 * Order here has to match the order of typedef enum LayertypeType there.
 * They're used for parsing/writing layer types from/to the layout file.
 */
static char *layertype_name[LT_NUM_LAYERTYPES + 1] = {
  "copper",       /* LT_COPPER */
  "silk",         /* LT_SILK */
  "mask",         /* LT_MASK */
  "paste",        /* LT_PASTE */
  "outline",      /* LT_OUTLINE */
  "route",        /* LT_ROUTE */
  "keepout",      /* LT_KEEPOUT */
  "fab",          /* LT_FAB */
  "assy",         /* LT_ASSY */
  "notes",        /* LT_NOTES */
  "no_type"       /* LT_NUM_LAYERTYPES */
};


LayertypeType
string_to_layertype (const char *flagstring,
                     int (*error) (const char *msg))
{
  LayertypeType type = 0;

  if (*flagstring == '"')
    flagstring++;

  while (flagstring > (char *)1 && strlen (flagstring) > 1)
    {
      for (type = 0; type < LT_NUM_LAYERTYPES; type++)
        {
          if (strcmp (flagstring, layertype_name[type]) == 0)
            break;
        }

      if (type == LT_NUM_LAYERTYPES)
        flagstring = strchr (flagstring, ',') + 1;
      else
        break;
    }

  return type;
}

const char *
layertype_to_string (LayertypeType type)
{
  const char *rv = "";

  if (type < LT_NUM_LAYERTYPES)
    rv = layertype_name[type];

  return rv;
}

/*!
 * \brief Given a layer without type, try to guess its type, mostly from
 * its name.
 *
 * This is used by parse_y.y for compatibility with old file formats and
 * _not_ used when such flags are already present in the file.
 */
LayertypeType
guess_layertype (const char *name, int layer_number, DataType *data)
{
  LayertypeType type;

  /* First try to find known (partial) matches. */
  for (type = 0; type < LT_NUM_LAYERTYPES; type++)
    {
      if (strcasestr (name, layertype_name[type]))
        break;
    }

  /* Nothing found? Then it's likely copper. */
  if (type == LT_NUM_LAYERTYPES)
    type = LT_COPPER;

  return type;
}

/* --------------------------------------------------------------------------- */

static const char listlayertypes_syntax[] =
  N_("ListLayertypes()");

static const char listlayertypes_help[] =
  N_("List all available layertypes.\n");

/* %start-doc actions ListLayertypes

Lists all available layer types. These are the valid types for the second
argument of @pxref{SetLayertype Action} or when editing the layout file with
a text editor.

%end-doc */

static int
ActionListLayertypes (int argc, char **argv, Coord x, Coord y)
{
  LayertypeType type;

  Message (N_("Available layer types:\n"));
  for (type = 0; type < LT_NUM_LAYERTYPES; type++)
    Message ("    %s (%d)\n", layertype_name[type], type);

  return 0;
}

/* --------------------------------------------------------------------------- */

static const char setlayertype_syntax[] =
  N_("SetLayertype(layer, type)");

static const char setlayertype_help[] =
  N_("Sets the type of a layer. Type can be given by name or by number.\n"
     "For a list of available types, run ListLayertypes().");

/* %start-doc actions SetLayertype

Layers can have various types, like @emph{copper}, @emph{silk} or
@emph{outline}. Behaviour of GUI and exporters largely depend on these types.
For example, searching for electrical connections searches only layers of type
@emph{copper}, all other layers are ignored.

For a list of available types see @pxref{ListLayertypes Action}.

%end-doc */

int
ActionSetLayertype (int argc, char **argv, Coord x, Coord y)
{
  int index;
  LayertypeType type;

  if (argc != 2)
    AFAIL (setlayertype);

  /* layer array is zero-based, file format counts layers starting at 1. */
  index = atoi (argv[0]) - 1;
  if (index < 0 || index >= max_copper_layer + SILK_LAYER)
    {
      Message (N_("Layer index %d out of range, must be 0 ... %d\n"),
               index + 1, max_copper_layer + SILK_LAYER);
      return 1;
    }

  if (isdigit (argv[1][0]))
    type = atoi (argv[1]);
  else
    type = string_to_layertype (argv[1], NULL);

  if (type < 0 || type >= LT_NUM_LAYERTYPES)
    {
      Message (N_("Invalid layer type (%d) requested. "
                  "See ListLayertypes() for a list.\n"), type);
      return 1;
    }

  PCB->Data->Layer[index].Type = type;

  return 0;
}

HID_Action layerflags_action_list[] = {
  {"ListLayertypes", 0, ActionListLayertypes,
   listlayertypes_help, listlayertypes_syntax}
  ,
  {"SetLayertype", 0, ActionSetLayertype,
   setlayertype_help, setlayertype_syntax}
};

REGISTER_ACTIONS (layerflags_action_list)
