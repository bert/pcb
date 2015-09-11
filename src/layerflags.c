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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <ctype.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "globalconst.h"
#include "global.h"
#include "compat.h"
#include "hid.h"
#include "strflags.h"


/**
 * These are the names of all the Layertypes defined in hid.h. Order here
 * has to match the order of typedef enum LayertypeType there.
 *
 * They're used for parsing/writing layer types from/to the layout file.
 */
static char *layertype_name[LT_NUM_LAYERTYPES + 1] = {
  "copper",       /* LT_COPPER */
  "silk",         /* LT_SILK */
  "mask",         /* LT_MASK */
  "pdrill",       /* LT_PDRILL */
  "udrill",       /* LT_UDRILL */
  "paste",        /* LT_PASTE */
  "invisible",    /* LT_INVISIBLE */
  "fab",          /* LT_FAB */
  "assy",         /* LT_ASSY */
  "outline",      /* LT_OUTLINE */
  "route",        /* LT_ROUTE */
  "notes",        /* LT_NOTES */
  "keepout",      /* LT_KEEPOUT */
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
 * Given a layer without type, try to guess its type, mostly from its name.
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

