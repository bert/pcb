/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "crosshair.h"
#include "clip.h"
#include "../hidint.h"
#include "gui.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif


RCSID ("$Id$");


extern HID ghid_hid;


/* Sets gport->u_gc to the "right" GC to use (wrt mask or window)
*/
#define USE_GC(gc) if (!use_gc(gc)) return

static int cur_mask = -1;
static int mask_seq = 0;

/* ------------------------------------------------------------ */

static inline int
Vx2 (int x)
{
  return (x - gport->view_x0) / gport->zoom + 0.5;
}

static inline int
Vy2 (int y)
{
  return (y - gport->view_y0) / gport->zoom + 0.5;
}

/* ------------------------------------------------------------ */

typedef struct hid_gc_struct
{
  HID *me_pointer;
  GdkGC *gc;

  gchar *colorname;
  gint width;
  gint cap, join;
  gchar xor;
  gchar erase;
  gint mask_seq;
}
hid_gc_struct;


void
ghid_destroy_gc (hidGC gc)
{
  if (gc->gc)
    g_object_unref (gc->gc);
  g_free (gc);
}

hidGC
ghid_make_gc (void)
{
  hidGC rv;

  rv = g_new0 (hid_gc_struct, 1);
  rv->me_pointer = &ghid_hid;
  rv->colorname = Settings.BackgroundColor;
  return rv;
}

void
ghid_draw_grid (void)
{
  static GdkPoint *points = 0;
  static int npoints = 0;
  int x1, y1, x2, y2, n, i;
  double x, y;

  if (!Settings.DrawGrid)
    return;
  if (Vz (PCB->Grid) < MIN_GRID_DISTANCE)
    return;
  if (!gport->grid_gc)
    {
      if (gdk_color_parse (Settings.GridColor, &gport->grid_color))
	{
	  gport->grid_color.red ^= gport->bg_color.red;
	  gport->grid_color.green ^= gport->bg_color.green;
	  gport->grid_color.blue ^= gport->bg_color.blue;
	  gdk_color_alloc (gport->colormap, &gport->grid_color);
	}
      gport->grid_gc = gdk_gc_new (gport->drawable);
      gdk_gc_set_function (gport->grid_gc, GDK_XOR);
      gdk_gc_set_foreground (gport->grid_gc, &gport->grid_color);
    }
  x1 = GRIDFIT_X (SIDE_X (gport->view_x0), PCB->Grid);
  y1 = GRIDFIT_Y (SIDE_Y (gport->view_y0), PCB->Grid);
  x2 = GRIDFIT_X (SIDE_X (gport->view_x0 + gport->view_width - 1), PCB->Grid);
  y2 =
    GRIDFIT_Y (SIDE_Y (gport->view_y0 + gport->view_height - 1), PCB->Grid);
  if (x1 > x2)
    {
      int tmp = x1;
      x1 = x2;
      x2 = tmp;
    }
  if (y1 > y2)
    {
      int tmp = y1;
      y1 = y2;
      y2 = tmp;
    }
  if (Vx (x1) < 0)
    x1 += PCB->Grid;
  if (Vy (y1) < 0)
    y1 += PCB->Grid;
  if (Vx (x2) >= gport->width)
    x2 -= PCB->Grid;
  if (Vy (y2) >= gport->height)
    y2 -= PCB->Grid;
  n = (int) ((x2 - x1) / PCB->Grid + 0.5) + 1;
  if (n > npoints)
    {
      npoints = n + 10;
      points = realloc (points, npoints * sizeof (GdkPoint));
    }
  n = 0;
  for (x = x1; x <= x2; x += PCB->Grid)
    {
      points[n].x = Vx (x);
      n++;
    }
  if (n == 0)
    return;
  for (y = y1; y <= y2; y += PCB->Grid)
    {
      int vy = Vy (y);
      for (i = 0; i < n; i++)
	points[i].y = vy;
      gdk_draw_points (gport->drawable, gport->grid_gc, points, n);
    }
}

/* ------------------------------------------------------------ */
void
ghid_draw_bg_image (void)
{
  static GdkPixbuf *pixbuf;
  GdkInterpType interp_type;
  gint x, y, w, h, w_src, h_src;
  static gint w_scaled, h_scaled;

  if (!ghidgui->bg_pixbuf)
    return;

  w = PCB->MaxWidth / gport->zoom;
  h = PCB->MaxHeight / gport->zoom;
  x = gport->view_x0 / gport->zoom;
  y = gport->view_y0 / gport->zoom;

  if (w_scaled != w || h_scaled != h)
    {
      if (pixbuf)
	g_object_unref (G_OBJECT (pixbuf));

      w_src = gdk_pixbuf_get_width (ghidgui->bg_pixbuf);
      h_src = gdk_pixbuf_get_height (ghidgui->bg_pixbuf);
      if (w > w_src && h > h_src)
	interp_type = GDK_INTERP_NEAREST;
      else
	interp_type = GDK_INTERP_BILINEAR;

      pixbuf =
	gdk_pixbuf_scale_simple (ghidgui->bg_pixbuf, w, h, interp_type);
      w_scaled = w;
      h_scaled = h;
    }
  if (pixbuf)
    gdk_pixbuf_render_to_drawable (pixbuf, gport->drawable, gport->bg_gc,
				   x, y, 0, 0,
				   w - x, h - y, GDK_RGB_DITHER_NORMAL, 0, 0);
}

#define WHICH_GC(gc) (cur_mask == HID_MASK_CLEAR ? gport->mask_gc : (gc)->gc)

void
ghid_use_mask (int use_it)
{
  static int mask_seq_id = 0;
  static GdkDrawable *old;
  GdkColor color;

  if (use_it == HID_FLUSH_DRAW_Q)
    {
      gdk_flush ();
      return;
    }
  else if (use_it == HID_LIVE_DRAWING)
    {
      old = gport->drawable;
      gport->drawable = gport->drawing_area->window;
      return;
    }
  else if (use_it == HID_LIVE_DRAWING_OFF)
    {
      gport->drawable = old;
      return;
    }

  if (!gport->pixmap)
    return;
  if (use_it == cur_mask)
    return;
  switch (use_it)
    {
    case HID_MASK_OFF:
      gport->drawable = gport->pixmap;
      mask_seq = 0;
      break;

    case HID_MASK_BEFORE:
      printf ("gtk doesn't support mask_before!\n");
      abort ();

    case HID_MASK_CLEAR:
      if (!gport->mask)
	gport->mask = gdk_pixmap_new (0, gport->width, gport->height, 1);
      gport->drawable = gport->mask;
      mask_seq = 0;
      if (!gport->mask_gc)
	{
	  gport->mask_gc = gdk_gc_new (gport->drawable);
	}
      color.pixel = 1;
      gdk_gc_set_foreground (gport->mask_gc, &color);
      gdk_draw_rectangle (gport->drawable, gport->mask_gc, TRUE, 0, 0,
			  gport->width, gport->height);
      color.pixel = 0;
      gdk_gc_set_foreground (gport->mask_gc, &color);
      break;

    case HID_MASK_AFTER:
      mask_seq_id++;
      if (!mask_seq_id)
	mask_seq_id = 1;
      mask_seq = mask_seq_id;

      gport->drawable = gport->pixmap;
      break;

    }
  cur_mask = use_it;
}


typedef struct
{
  int color_set;
  GdkColor color;
  int xor_set;
  GdkColor xor_color;
} ColorCache;


  /* Config helper functions for when the user changes color preferences.
     |  set_special colors used in the gtkhid.
   */
static void
set_special_grid_color (void)
{
  if (!gport->colormap)
    return;
  gport->grid_color.red ^= gport->bg_color.red;
  gport->grid_color.green ^= gport->bg_color.green;
  gport->grid_color.blue ^= gport->bg_color.blue;
  gdk_color_alloc (gport->colormap, &gport->grid_color);
  if (gport->grid_gc)
    gdk_gc_set_foreground (gport->grid_gc, &gport->grid_color);
}

void
ghid_set_special_colors (HID_Attribute * ha)
{
  if (!ha->name || !ha->value)
    return;
  if (!strcmp (ha->name, "background-color") && gport->bg_gc)
    {
      ghid_map_color_string (*(char **) ha->value, &gport->bg_color);
      gdk_gc_set_foreground (gport->bg_gc, &gport->bg_color);
      set_special_grid_color ();
    }
  else if (!strcmp (ha->name, "off-limit-color") && gport->offlimits_gc)
    {
      ghid_map_color_string (*(char **) ha->value, &gport->offlimits_color);
      gdk_gc_set_foreground (gport->offlimits_gc, &gport->offlimits_color);
    }
  else if (!strcmp (ha->name, "grid-color") && gport->grid_gc)
    {
      ghid_map_color_string (*(char **) ha->value, &gport->grid_color);
      set_special_grid_color ();
    }
}

void
ghid_set_color (hidGC gc, const char *name)
{
  static void *cache = 0;
  hidval cval;

  if (name == NULL)
    {
      fprintf (stderr, "%s():  name = NULL, setting to magenta\n",
	       __FUNCTION__);
      name = "magenta";
    }

  gc->colorname = (char *) name;
  if (!gc->gc)
    return;
  if (gport->colormap == 0)
    gport->colormap = gtk_widget_get_colormap (gport->top_window);

  if (strcmp (name, "erase") == 0)
    {
      gdk_gc_set_foreground (gc->gc, &gport->bg_color);
      gc->erase = 1;
    }
  else if (strcmp (name, "drill") == 0)
    {
      gdk_gc_set_foreground (gc->gc, &gport->offlimits_color);
      gc->erase = 0;
    }
  else
    {
      ColorCache *cc;
      if (hid_cache_color (0, name, &cval, &cache))
	cc = (ColorCache *) cval.ptr;
      else
	{
	  cc = (ColorCache *) malloc (sizeof (ColorCache));
	  memset (cc, 0, sizeof (*cc));
	  cval.ptr = cc;
	  hid_cache_color (1, name, &cval, &cache);
	}

      if (!cc->color_set)
	{
	  if (gdk_color_parse (name, &cc->color))
	    gdk_color_alloc (gport->colormap, &cc->color);
	  else
	    gdk_color_white (gport->colormap, &cc->color);
	  cc->color_set = 1;
	}
      if (gc->xor)
	{
	  if (!cc->xor_set)
	    {
	      cc->xor_color.red = cc->color.red ^ gport->bg_color.red;
	      cc->xor_color.green = cc->color.green ^ gport->bg_color.green;
	      cc->xor_color.blue = cc->color.blue ^ gport->bg_color.blue;
	      gdk_color_alloc (gport->colormap, &cc->xor_color);
	      cc->xor_set = 1;
	    }
	  gdk_gc_set_foreground (gc->gc, &cc->xor_color);
	}
      else
	{
	  gdk_gc_set_foreground (gc->gc, &cc->color);
	}

      gc->erase = 0;
    }
}

void
ghid_set_line_cap (hidGC gc, EndCapStyle style)
{

  switch (style)
    {
    case Trace_Cap:
    case Round_Cap:
      gc->cap = GDK_CAP_ROUND;
      gc->join = GDK_JOIN_ROUND;
      break;
    case Square_Cap:
    case Beveled_Cap:
      gc->cap = GDK_CAP_PROJECTING;
      gc->join = GDK_JOIN_MITER;
      break;
    }
  if (gc->gc)
    gdk_gc_set_line_attributes (WHICH_GC (gc),
				Vz (gc->width), GDK_LINE_SOLID,
				gc->cap, gc->join);
}

void
ghid_set_line_width (hidGC gc, int width)
{

  gc->width = width;
  if (gc->gc)
    gdk_gc_set_line_attributes (WHICH_GC (gc),
				Vz (gc->width), GDK_LINE_SOLID,
				gc->cap, gc->join);
}

void
ghid_set_draw_xor (hidGC gc, int xor)
{
  gc->xor = xor;
  if (!gc->gc)
    return;
  gdk_gc_set_function (gc->gc, xor ? GDK_XOR : GDK_COPY);
  ghid_set_color (gc, gc->colorname);
}

void
ghid_set_draw_faded (hidGC gc, int faded)
{
  printf ("ghid_set_draw_faded(%p,%d) -- not implemented\n", gc, faded);
}

void
ghid_set_line_cap_angle (hidGC gc, int x1, int y1, int x2, int y2)
{
  printf ("ghid_set_line_cap_angle() -- not implemented\n");
}

static int
use_gc (hidGC gc)
{

  if (gc->me_pointer != &ghid_hid)
    {
      fprintf (stderr, "Fatal: GC from another HID passed to GTK HID\n");
      abort ();
    }

  if (!gport->pixmap)
    return 0;
  if (!gc->gc)
    {
      gc->gc = gdk_gc_new (gport->top_window->window);
      ghid_set_color (gc, gc->colorname);
      ghid_set_line_width (gc, gc->width);
      ghid_set_line_cap (gc, gc->cap);
      ghid_set_draw_xor (gc, gc->xor);
    }
  if (gc->mask_seq != mask_seq)
    {
      if (mask_seq)
	gdk_gc_set_clip_mask (gc->gc, gport->mask);
      else
	gdk_gc_set_clip_mask (gc->gc, NULL);
      gc->mask_seq = mask_seq;
    }
  gport->u_gc = WHICH_GC (gc);
  return 1;
}

void
ghid_draw_line (hidGC gc, int x1, int y1, int x2, int y2)
{
  double dx1, dy1, dx2, dy2;

  dx1 = Vx ((double) x1);
  dy1 = Vy ((double) y1);
  dx2 = Vx ((double) x2);
  dy2 = Vy ((double) y2);

  if (!ClipLine (0, 0, gport->width, gport->height,
		 &dx1, &dy1, &dx2, &dy2, gc->width / gport->zoom))
    return;

  USE_GC (gc);
  gdk_draw_line (gport->drawable, gport->u_gc, dx1, dy1, dx2, dy2);
}

void
ghid_draw_arc (hidGC gc, int cx, int cy,
	       int xradius, int yradius, int start_angle, int delta_angle)
{
  gint vrx, vry;
  gint w, h, radius;

  w = gport->width * gport->zoom;
  h = gport->height * gport->zoom;
  radius = (xradius > yradius) ? xradius : yradius;
  if (SIDE_X (cx) < gport->view_x0 - radius
      || SIDE_X (cx) > gport->view_x0 + w + radius
      || SIDE_Y (cy) < gport->view_y0 - radius
      || SIDE_Y (cy) > gport->view_y0 + h + radius)
    return;

  USE_GC (gc);
  vrx = Vz (xradius);
  vry = Vz (yradius);

  if (ghid_flip_x)
    {
      start_angle = 180 - start_angle;
      delta_angle = -delta_angle;
    }
  if (ghid_flip_y)
    {
      start_angle = -start_angle;
      delta_angle = -delta_angle;
    }
  /* make sure we fall in the -180 to +180 range */
  start_angle = (start_angle + 360 + 180) % 360 - 180;

  gdk_draw_arc (gport->drawable, gport->u_gc, 0,
		Vx (cx) - vrx, Vy (cy) - vry,
		vrx * 2, vry * 2, (start_angle + 180) * 64, delta_angle * 64);
}

void
ghid_draw_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  gint w, h, lw;

  lw = gc->width;
  w = gport->width * gport->zoom;
  h = gport->height * gport->zoom;

  if ((SIDE_X (x1) < gport->view_x0 - lw
       && SIDE_X (x2) < gport->view_x0 - lw)
      || (SIDE_X (x1) > gport->view_x0 + w + lw
	  && SIDE_X (x2) > gport->view_x0 + w + lw)
      || (SIDE_Y (y1) < gport->view_y0 - lw
	  && SIDE_Y (y2) < gport->view_y0 - lw)
      || (SIDE_Y (y1) > gport->view_y0 + h + lw
	  && SIDE_Y (y2) > gport->view_y0 + h + lw))
    return;

  x1 = Vx (x1);
  y1 = Vy (y1);
  x2 = Vx (x2);
  y2 = Vy (y2);

  if (x1 > x2)
    {
      gint xt = x1;
      x1 = x2;
      x2 = xt;
    }
  if (y1 > y2)
    {
      gint yt = y1;
      y1 = y2;
      y2 = yt;
    }

  USE_GC (gc);
  gdk_draw_rectangle (gport->drawable, gport->u_gc, FALSE,
		      x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}


void
ghid_fill_circle (hidGC gc, int cx, int cy, int radius)
{
  gint w, h, vr;

  w = gport->width * gport->zoom;
  h = gport->height * gport->zoom;
  if (SIDE_X (cx) < gport->view_x0 - radius
      || SIDE_X (cx) > gport->view_x0 + w + radius
      || SIDE_Y (cy) < gport->view_y0 - radius
      || SIDE_Y (cy) > gport->view_y0 + h + radius)
    return;

  USE_GC (gc);
  vr = Vz (radius);
  gdk_draw_arc (gport->drawable, gport->u_gc, TRUE,
		Vx (cx) - vr, Vy (cy) - vr, vr * 2, vr * 2, 0, 360 * 64);
}

void
ghid_fill_polygon (hidGC gc, int n_coords, int *x, int *y)
{
  static GdkPoint *points = 0;
  static int npoints = 0;
  int i;
  USE_GC (gc);

  if (npoints < n_coords)
    {
      npoints = n_coords + 1;
      points = realloc (points, npoints * sizeof (GdkPoint));
    }
  for (i = 0; i < n_coords; i++)
    {
      points[i].x = Vx (x[i]);
      points[i].y = Vy (y[i]);
    }
  gdk_draw_polygon (gport->drawable, gport->u_gc, 1, points, n_coords);
}

void
ghid_fill_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  gint w, h, lw, xx, yy;

  lw = gc->width;
  w = gport->width * gport->zoom;
  h = gport->height * gport->zoom;

  if ((SIDE_X (x1) < gport->view_x0 - lw
       && SIDE_X (x2) < gport->view_x0 - lw)
      || (SIDE_X (x1) > gport->view_x0 + w + lw
	  && SIDE_X (x2) > gport->view_x0 + w + lw)
      || (SIDE_Y (y1) < gport->view_y0 - lw
	  && SIDE_Y (y2) < gport->view_y0 - lw)
      || (SIDE_Y (y1) > gport->view_y0 + h + lw
	  && SIDE_Y (y2) > gport->view_y0 + h + lw))
    return;

  x1 = Vx (x1);
  y1 = Vy (y1);
  x2 = Vx (x2);
  y2 = Vy (y2);
  if (x2 < x1)
    {
      xx = x1;
      x1 = x2;
      x2 = xx;
    }
  if (y2 < y1)
    {
      yy = y1;
      y1 = y2;
      y2 = yy;
    }
  USE_GC (gc);
  gdk_draw_rectangle (gport->drawable, gport->u_gc, TRUE,
		      x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}
