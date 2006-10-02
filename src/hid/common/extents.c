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
extents_set_layer (const char *name, int group)
{
  int idx = group;
  if (idx >= 0 && idx < max_layer)
    {
      idx = PCB->LayerGroups.Entries[idx][0];
    }
  if (idx >= 0 && idx < max_layer + 2)
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
  hidGC rv = malloc (sizeof (hid_gc_struct));
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
extents_set_line_width (hidGC gc, int width)
{
  gc->width = width;
}

static void
extents_set_draw_xor (hidGC gc, int xor)
{
}

static void
extents_set_draw_faded (hidGC gc, int faded)
{
}

static void
extents_set_line_cap_angle (hidGC gc, int x1, int y1, int x2, int y2)
{
}

#define PEX(x,w) if (box.X1 > (x)-(w)) box.X1 = (x)-(w); \
	if (box.X2 < (x)+(w)) box.X2 = (x)+(w)
#define PEY(y,w) if (box.Y1 > (y)-(w)) box.Y1 = (y)-(w); \
	if (box.Y2 < (y)+(w)) box.Y2 = (y)+(w)

static void
extents_draw_line (hidGC gc, int x1, int y1, int x2, int y2)
{
  PEX (x1, gc->width);
  PEY (y1, gc->width);
  PEX (x2, gc->width);
  PEY (y2, gc->width);
}

static void
extents_draw_arc (hidGC gc, int cx, int cy, int width, int height,
		  int start_angle, int end_angle)
{
  /* Naive but good enough.  */
  PEX (cx, width + gc->width);
  PEY (cy, height + gc->width);
}

static void
extents_draw_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  PEX (x1, gc->width);
  PEY (y1, gc->width);
  PEX (x2, gc->width);
  PEY (y2, gc->width);
}

static void
extents_fill_circle (hidGC gc, int cx, int cy, int radius)
{
  PEX (cx, radius);
  PEY (cy, radius);
}

static void
extents_fill_polygon (hidGC gc, int n_coords, int *x, int *y)
{
  int i;
  for (i = 0; i < n_coords; i++)
    {
      PEX (x[i], 0);
      PEY (y[i], 0);
    }
}

static void
extents_fill_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  PEX (x1, 0);
  PEY (y1, 0);
  PEX (x2, 0);
  PEY (y2, 0);
}

static HID extents_hid = {
  sizeof (HID),
  "extents-extents",
  "used to calculate extents",
  0,				/* gui */
  0,				/* printer */
  0,				/* exporter */
  1,				/* poly before */
  0,				/* poly after */

  0 /* extents_get_export_options */ ,
  0 /* extents_do_export */ ,
  0 /* extents_parse_arguments */ ,

  0 /* extents_invalidate_wh */ ,
  0 /* extents_invalidate_lr */ ,
  0 /* extents_invalidate_all */ ,
  extents_set_layer,
  extents_make_gc,
  extents_destroy_gc,
  extents_use_mask,
  extents_set_color,
  extents_set_line_cap,
  extents_set_line_width,
  extents_set_draw_xor,
  extents_set_draw_faded,
  extents_set_line_cap_angle,
  extents_draw_line,
  extents_draw_arc,
  extents_draw_rect,
  extents_fill_circle,
  extents_fill_polygon,
  extents_fill_rect,

  0 /* extents_calibrate */ ,
  0 /* extents_shift_is_pressed */ ,
  0 /* extents_control_is_pressed */ ,
  0 /* extents_get_coords */ ,
  0 /* extents_set_crosshair */ ,
  0 /* extents_add_timer */ ,
  0 /* extents_stop_timer */ ,

  0 /* extents_log */ ,
  0 /* extents_logv */ ,
  0 /* extents_confirm_dialog */ ,
  0 /* extents_report_dialog */ ,
  0 /* extents_prompt_for */ ,
  0 /* extents_attribute_dialog */ ,
  0 /* extents_show_item */ ,
  0				/* extents_beep */
};

BoxType *
hid_get_extents (void *item)
{
  BoxType region;

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
