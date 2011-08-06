/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "global.h"
#include "data.h"

#include "hid.h"
#include "../hidint.h"
#include "hid/common/draw_helpers.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

#ifndef MAXINT
#define MAXINT (((unsigned int)(~0))>>1)
#endif

static BoxType box;

typedef struct hid_gc_struct
{
  int width;
} hid_gc_struct;

static int
extents_set_layer (const char *name, int group, int empty)
{
  int idx = group;
  if (idx >= 0 && idx < max_group)
    {
      idx = PCB->LayerGroups.Entries[idx][0];
    }
  if (idx >= 0 && idx < max_copper_layer + 2)
    return 1;
  if (idx < 0)
    {
      switch (SL_TYPE (idx))
	{
	case SL_INVISIBLE:
	case SL_MASK:
	case SL_ASSY:
	  return 0;
	case SL_SILK:
	case SL_PDRILL:
	case SL_UDRILL:
	  return 1;
	}
    }
  return 0;
}

static hidGC
extents_make_gc (void)
{
  hidGC rv = (hidGC)malloc (sizeof (hid_gc_struct));
  memset (rv, 0, sizeof (hid_gc_struct));
  return rv;
}

static void
extents_destroy_gc (hidGC gc)
{
  free (gc);
}

static void
extents_use_mask (int use_it)
{
}

static void
extents_set_color (hidGC gc, const char *name)
{
}

static void
extents_set_line_cap (hidGC gc, EndCapStyle style)
{
}

static void
extents_set_line_width (hidGC gc, Coord width)
{
  gc->width = width;
}

static void
extents_set_draw_xor (hidGC gc, int xor_)
{
}

#define PEX(x,w) if (box.X1 > (x)-(w)) box.X1 = (x)-(w); \
	if (box.X2 < (x)+(w)) box.X2 = (x)+(w)
#define PEY(y,w) if (box.Y1 > (y)-(w)) box.Y1 = (y)-(w); \
	if (box.Y2 < (y)+(w)) box.Y2 = (y)+(w)

static void
extents_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  PEX (x1, gc->width);
  PEY (y1, gc->width);
  PEX (x2, gc->width);
  PEY (y2, gc->width);
}

static void
extents_draw_arc (hidGC gc, Coord cx, Coord cy, Coord width, Coord height,
		  Angle start_angle, Angle end_angle)
{
  /* Naive but good enough.  */
  PEX (cx, width + gc->width);
  PEY (cy, height + gc->width);
}

static void
extents_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  PEX (x1, gc->width);
  PEY (y1, gc->width);
  PEX (x2, gc->width);
  PEY (y2, gc->width);
}

static void
extents_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius)
{
  PEX (cx, radius);
  PEY (cy, radius);
}

static void
extents_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y)
{
  int i;
  for (i = 0; i < n_coords; i++)
    {
      PEX (x[i], 0);
      PEY (y[i], 0);
    }
}

static void
extents_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  PEX (x1, 0);
  PEY (y1, 0);
  PEX (x2, 0);
  PEY (y2, 0);
}

static HID extents_hid;

void
hid_extents_init (void)
{
  static bool initialised = false;

  if (initialised)
    return;

  memset (&extents_hid, 0, sizeof (HID));

  common_draw_helpers_init (&extents_hid);

  extents_hid.struct_size         = sizeof (HID);
  extents_hid.name                = "extents-extents";
  extents_hid.description         = "used to calculate extents";
  extents_hid.poly_before         = 1;

  extents_hid.set_layer           = extents_set_layer;
  extents_hid.make_gc             = extents_make_gc;
  extents_hid.destroy_gc          = extents_destroy_gc;
  extents_hid.use_mask            = extents_use_mask;
  extents_hid.set_color           = extents_set_color;
  extents_hid.set_line_cap        = extents_set_line_cap;
  extents_hid.set_line_width      = extents_set_line_width;
  extents_hid.set_draw_xor        = extents_set_draw_xor;
  extents_hid.draw_line           = extents_draw_line;
  extents_hid.draw_arc            = extents_draw_arc;
  extents_hid.draw_rect           = extents_draw_rect;
  extents_hid.fill_circle         = extents_fill_circle;
  extents_hid.fill_polygon        = extents_fill_polygon;
  extents_hid.fill_rect           = extents_fill_rect;

  initialised = true;
}

BoxType *
hid_get_extents (void *item)
{
  BoxType region;

  /* As this isn't a real "HID", we need to ensure we are initialised. */
  hid_extents_init ();

  box.X1 = MAXINT;
  box.Y1 = MAXINT;
  box.X2 = -MAXINT;
  box.Y2 = -MAXINT;

  region.X1 = -MAXINT;
  region.Y1 = -MAXINT;
  region.X2 = MAXINT;
  region.Y2 = MAXINT;
  hid_expose_callback (&extents_hid, &region, item);

  return &box;
}
