/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <math.h>
#include <time.h>


#include "action.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "global.h"
#include "mymem.h"
#include "draw.h"
#include "clip.h"

#include "hid.h"
#include "../hidint.h"
#include "gui.h"


#if !GTK_CHECK_VERSION(2,8,0) && defined(HAVE_GDK_GDKX_H)
#include <gdk/gdkx.h>
#endif

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif


RCSID ("$Id$");


extern HID ghid_hid;


static void zoom_to (double factor, int x, int y);
static void zoom_by (double factor, int x, int y);

/* Sets gport->u_gc to the "right" GC to use (wrt mask or window)
*/
#define USE_GC(gc) if (!use_gc(gc)) return

static int cur_mask = -1;
static int mask_seq = 0;

int ghid_flip_x = 0, ghid_flip_y = 0;

/* ------------------------------------------------------------ */

/* Px converts view->pcb, Vx converts pcb->view */
      
static inline int 
Vx (int x)
{     
  int rv;
  if (ghid_flip_x) 
    rv = (PCB->MaxWidth - x - gport->view_x0) / gport->zoom + 0.5;
  else
    rv = (x - gport->view_x0) / gport->zoom + 0.5;
  return rv;
}       
      
static inline int 
Vx2 (int x)
{     
  return (x - gport->view_x0) / gport->zoom + 0.5;
}       
      
static inline int
Vy (int y)
{         
  int rv;
  if (ghid_flip_y)
    rv = (PCB->MaxHeight - y - gport->view_y0) / gport->zoom + 0.5;
  else
    rv = (y - gport->view_y0) / gport->zoom + 0.5;
  return rv;
}     
        
static inline int 
Vy2 (int y)
{     
  return (y - gport->view_y0) / gport->zoom + 0.5;
}       
      
static inline int
Vz (int z)
{           
  return z / gport->zoom + 0.5;
}         
                
static inline int
Px (int x)
{  
  int rv = x * gport->zoom + gport->view_x0;
  if (ghid_flip_x)
    rv = PCB->MaxWidth - (x * gport->zoom + gport->view_x0);
  return  rv;
}  

static inline int
Py (int y)
{  
  int rv = y * gport->zoom + gport->view_y0;
  if (ghid_flip_y)
    rv = PCB->MaxHeight - (y * gport->zoom + gport->view_y0);
  return  rv;
}  

static inline int  
Pz (int z)
{
  return (z * gport->zoom);
}

/* ------------------------------------------------------------ */

static void
ghid_pan_fixup ()
{

  /* 
   * don't pan so far to the right that we see way past the right 
   * edge of the board.
   */
  if (gport->view_x0 > PCB->MaxWidth - gport->view_width)
    gport->view_x0 = PCB->MaxWidth - gport->view_width;

  /* 
   * don't pan so far down that we see way past the bottom edge of
   * the board.
   */
  if (gport->view_y0 > PCB->MaxHeight - gport->view_height)
    gport->view_y0 = PCB->MaxHeight - gport->view_height;

  /* don't view above or to the left of the board... ever */
  if (gport->view_x0 < 0)
    gport->view_x0 = 0;

   if (gport->view_y0 < 0)
    gport->view_y0 = 0;


   /* if we can see the entire board and some, then zoom to fit */
   if (gport->view_width > PCB->MaxWidth &&
       gport->view_height > PCB->MaxHeight)
     {
       zoom_by (1, 0, 0);
       return;
     }

   gtk_range_set_value (GTK_RANGE (ghidgui->h_range), gport->view_x0);
   gtk_range_set_value (GTK_RANGE (ghidgui->v_range), gport->view_y0);

   ghid_invalidate_all ();

}

/* ------------------------------------------------------------ */

static const char zoom_syntax[] =
"Zoom()\n"
"Zoom(factor)";


static const char zoom_help[] =
"Various zoom factor changes.";

/* %start-doc actions Zoom
Changes the zoom (magnification) of the view of the board.  If no
arguments are passed, the view is scaled such that the board just fits
inside the visible window (i.e. ``view all'').  Otherwise,
@var{factor} specifies a change in zoom factor.  It may be prefixed by
@code{+}, @code{-}, or @code{=} to change how the zoom factor is
modified.  The @var{factor} is a floating point number, such as
@code{1.5} or @code{0.75}.

@table @code
  
@item +@var{factor}
Values greater than 1.0 cause the board to be drawn smaller; more of
the board will be visible.  Values between 0.0 and 1.0 cause the board
to be drawn bigger; less of the board will be visible.
  
@item -@var{factor}
Values greater than 1.0 cause the board to be drawn bigger; less of
the board will be visible.  Values between 0.0 and 1.0 cause the board
to be drawn smaller; more of the board will be visible.
 
@item =@var{factor}
 
The @var{factor} is an absolute zoom factor; the unit for this value
is "PCB units per screen pixel".  Since PCB units are 0.01 mil, a
@var{factor} of 1000 means 10 mils (0.01 in) per pixel, or 100 DPI,
about the actual resolution of most screens - resulting in an "actual
size" board.  Similarly, a @var{factor} of 100 gives you a 10x actual
size.
 
@end table
 
Note that zoom factors of zero are silently ignored.
 


%end-doc */

static int
Zoom (int argc, char **argv, int x, int y)
{
  double factor;
  const char *vp;
  double v;

  if (argc > 1)
    AFAIL (zoom);

  if (x == 0 && y == 0)
    {
      x = gport->view_width / 2;
      y = gport->view_height / 2;
    }
  else
    {
      /* Px converts view->pcb, Vx converts pcb->view */
      x = Vx (x);
      y = Vy (y);
    }

  if (argc < 1)
    {
      zoom_to (1000000, 0, 0);
      return 0;
    }

  vp = argv[0];
  if (*vp == '+' || *vp == '-' || *vp == '=')
    vp++;
  v = strtod (vp, 0);
  if (v <= 0)
    return 1;
  switch (argv[0][0])
    {
    case '-':
      factor = 1 / v;
      zoom_by (1 / v, x, y);
      break;
    default:
    case '+':
      factor = v;
      zoom_by (v, x, y);
      break;
    case '=':
      /* this needs to set the scale factor absolutely*/
      factor = 1.0;
      zoom_to (v, x, y);
      break;
    }

  return 0;
}


static void
zoom_to (double new_zoom, int x, int y)
{
  double max_zoom, xfrac, yfrac;
  int cx, cy;

  /* gport->zoom:
   * zoom value is PCB units per screen pixel.  Larger numbers mean zooming
   * out - the largest value means you are looking at the whole board.
   *
   * PCB units per screen pixel
   *
   * gport->view_width and gport->view_height are in PCB coordinates
   */

#ifdef DEBUG
  printf ("\nzoom_to( %g, %d, %d)\n", new_zoom, x, y);
#endif

  xfrac = (double) x / (double) gport->view_width;
  yfrac = (double) y / (double) gport->view_height;

  if (ghid_flip_x)
    xfrac = 1-xfrac;
  if (ghid_flip_y)
    yfrac = 1-yfrac;

  /* Find the zoom that would just make the entire board fit */
  max_zoom = PCB->MaxWidth / gport->width;
  if (max_zoom < PCB->MaxHeight / gport->height)
    max_zoom = PCB->MaxHeight / gport->height;

#ifdef DEBUG
  printf ("zoom_to():  max_zoom = %g\n", max_zoom);
#endif

  /* 
   * clip the zooming so we can never have more than 1 pixel per PCB
   * unit and never zoom out more than viewing the entire board
   */
     
  if (new_zoom < 1)
    new_zoom = 1;
  if (new_zoom > max_zoom)
    new_zoom = max_zoom;

#ifdef DEBUG
  printf ("max_zoom = %g, xfrac = %g, yfrac = %g, new_zoom = %g\n", 
	  max_zoom, xfrac, yfrac, new_zoom);
#endif

  /* find center x and y */
  cx = gport->view_x0 + gport->view_width * xfrac * gport->zoom;
  cy = gport->view_y0 + gport->view_height * yfrac * gport->zoom;

#ifdef DEBUG
  printf ("zoom_to():  x0 = %d, cx = %d\n", gport->view_x0, cx);
  printf ("zoom_to():  y0 = %d, cy = %d\n", gport->view_y0, cy);
#endif

  if (gport->zoom != new_zoom)
    {
      gdouble xtmp, ytmp;
      gint x0, y0;

      xtmp = (gport->view_x - gport->view_x0) / (gdouble) gport->view_width;
      ytmp = (gport->view_y - gport->view_y0) / (gdouble) gport->view_height;
      
      gport->zoom = new_zoom;
      pixel_slop = new_zoom;
      ghid_port_ranges_scale(FALSE);

      x0 = gport->view_x - xtmp * gport->view_width;
      if (x0 < 0)
	x0 = 0;
      gport->view_x0 = x0;

      y0 = gport->view_y - ytmp * gport->view_height;
      if (y0 < 0)
	y0 = 0;
      gport->view_y0 = y0;
      
      ghidgui->adjustment_changed_holdoff = TRUE;
      gtk_range_set_value (GTK_RANGE (ghidgui->h_range), gport->view_x0);
      gtk_range_set_value (GTK_RANGE (ghidgui->v_range), gport->view_y0);
      ghidgui->adjustment_changed_holdoff = FALSE;
      
      ghid_port_ranges_changed();
    }

#ifdef DEBUG
  printf ("zoom_to():  new x0 = %d\n", gport->view_x0);
  printf ("zoom_to():  new y0 = %d\n", gport->view_y0);
#endif
  ghid_set_status_line_label ();
}

void
zoom_by (double factor, int x, int y)
{
#ifdef DEBUG
  printf ("\nzoom_by( %g, %d, %d).  old gport->zoom = %g\n", 
	  factor, x, y, gport->zoom);
#endif
  zoom_to (gport->zoom * factor, x, y);
}

/* ------------------------------------------------------------ */

static void
draw_grid ()
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
  y2 = GRIDFIT_Y (SIDE_Y (gport->view_y0 + gport->view_height - 1), PCB->Grid);
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
      points =
	MyRealloc (points, npoints * sizeof (GdkPoint), "gtk_draw_grid");
    }
  n = 0;
  for (x = x1; x <= x2; x += PCB->Grid)
    {
      points[n].x = Vx (x);
      n++;
    }
  for (y = y1; y <= y2; y += PCB->Grid)
    {
      int vy = Vy (y);
      for (i = 0; i < n; i++)
	points[i].y = vy;
      gdk_draw_points (gport->drawable, gport->grid_gc, points, n);
    }
}

/* ------------------------------------------------------------ */

HID_Attribute *
ghid_get_export_options (int *n_ret)
{
  return NULL;
}


void
ghid_invalidate_wh (int x, int y, int width, int height, int last)
{
  ghid_invalidate_all ();
}

void
ghid_invalidate_lr (int left, int right, int top, int bottom, int last)
{
  ghid_invalidate_all ();
}

static void
ghid_draw_bg_image(void)
{
	static GdkPixbuf	*pixbuf;
	GdkInterpType	interp_type;
	gint	x, y, w, h, w_src, h_src;
	static gint	w_scaled, h_scaled;

	if (!ghidgui->bg_pixbuf)
		return;

	w = PCB->MaxWidth / gport->zoom;
	h = PCB->MaxHeight / gport->zoom;
	x = gport->view_x0 / gport->zoom;
	y = gport->view_y0 / gport->zoom;

	if (w_scaled != w || h_scaled != h)
		{
		if (pixbuf)
			g_object_unref(G_OBJECT(pixbuf));

		w_src = gdk_pixbuf_get_width(ghidgui->bg_pixbuf);
		h_src = gdk_pixbuf_get_height(ghidgui->bg_pixbuf);
		if (w > w_src && h > h_src)
			interp_type = GDK_INTERP_NEAREST;
		else
			interp_type = GDK_INTERP_BILINEAR;

		pixbuf = gdk_pixbuf_scale_simple(ghidgui->bg_pixbuf, w, h, interp_type);
		w_scaled = w;
		h_scaled = h;
		}
	if (pixbuf)
		gdk_pixbuf_render_to_drawable(pixbuf, gport->drawable, gport->bg_gc,
            x, y, 0, 0,
            w - x, h - y, GDK_RGB_DITHER_NORMAL, 0, 0);
	}

void
ghid_invalidate_all ()
{
  int eleft, eright, etop, ebottom;
  BoxType region;

  if (!gport->pixmap)
    return;

  region.X1 = MIN(Px(0), Px(gport->width + 1));
  region.Y1 = MIN(Py(0), Py(gport->height + 1));
  region.X2 = MAX(Px(0), Px(gport->width + 1));
  region.Y2 = MAX(Py(0), Py(gport->height + 1));

  eleft = Vx (0);
  eright = Vx (PCB->MaxWidth);
  etop = Vy (0);
  ebottom = Vy (PCB->MaxHeight);
  if (eleft > eright)
    {
      int tmp = eleft;
      eleft = eright;
      eright = tmp;
    }
  if (etop > ebottom)
    {
      int tmp = etop;
      etop = ebottom;
      ebottom = tmp;
    }

  if (eleft > 0)
    gdk_draw_rectangle (gport->drawable, gport->offlimits_gc,
			1, 0, 0, eleft, gport->height);
  else
    eleft = 0;
  if (eright < gport->width)
    gdk_draw_rectangle (gport->drawable, gport->offlimits_gc,
			1, eright, 0, gport->width - eright, gport->height);
  else
    eright = gport->width;
  if (etop > 0)
    gdk_draw_rectangle (gport->drawable, gport->offlimits_gc,
			1, eleft, 0, eright - eleft + 1, etop);
  else
    etop = 0;
  if (ebottom < gport->height)
    gdk_draw_rectangle (gport->drawable, gport->offlimits_gc,
			1, eleft, ebottom, eright - eleft + 1,
			gport->height - ebottom);
  else
    ebottom = gport->height;

  gdk_draw_rectangle (gport->drawable, gport->bg_gc, 1,
		      eleft, etop, eright - eleft + 1, ebottom - etop + 1);

  ghid_draw_bg_image();

  hid_expose_callback (&ghid_hid, &region, 0);
  draw_grid ();
  if (ghidgui->need_restore_crosshair)
    RestoreCrosshair (FALSE);
  ghidgui->need_restore_crosshair = FALSE;
  ghid_screen_update ();
}


int
ghid_set_layer (const char *name, int group)
{
  int idx = (group >= 0
	     && group <
	     max_layer) ? PCB->LayerGroups.Entries[group][0] : group;

  if (idx >= 0 && idx < max_layer + 2)
    return /*pinout ? 1 : */ PCB->Data->Layer[idx].On;
  if (idx < 0)
    {
      switch (SL_TYPE (idx))
	{
	case SL_INVISIBLE:
	  return /* pinout ? 0 : */ PCB->InvisibleObjectsOn;
	case SL_MASK:
	  if (SL_MYSIDE (idx) /*&& !pinout */ )
	    return TEST_FLAG (SHOWMASKFLAG, PCB);
	  return 0;
	case SL_SILK:
	  if (SL_MYSIDE (idx) /*|| pinout */ )
	    return PCB->ElementOn;
	  return 0;
	case SL_ASSY:
	  return 0;
	case SL_PDRILL:
	case SL_UDRILL:
	  return 1;
	}
    }
  return 0;
}

#define WHICH_GC(gc) (cur_mask == HID_MASK_CLEAR ? gport->mask_gc : (gc)->gc)

void
ghid_use_mask (int use_it)
{
  static int mask_seq_id = 0;
  GdkColor color;

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

void
ghid_extents_use_mask (int use_it)
{
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

  dx1 = Vx ((double)x1);
  dy1 = Vy ((double)y1);
  dx2 = Vx ((double)x2);
  dy2 = Vy ((double)y2);

  if (! ClipLine (0, 0, gport->width, gport->height,
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


  /* make sure we fall in the -180 to +180 range */
  start_angle = (start_angle + 360 + 180) % 360 - 180;
  if (ghid_flip_x)
    {
      start_angle = 180 - start_angle;
      delta_angle = - delta_angle;
    }
  if (ghid_flip_y)
    {
      start_angle = - start_angle;
      delta_angle = - delta_angle;					
    }

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

  if (x1 > x2) { gint xt = x1; x1 = x2; x2 = xt; }
  if (y1 > y2) { gint yt = y1; y1 = y2; y2 = yt; }

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
		Vx (cx) - vr, Vy (cy) - vr,
		vr * 2, vr * 2, 0, 360 * 64);
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
      points = MyRealloc (points,
			  npoints * sizeof (GdkPoint), (char *) __FUNCTION__);
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

void
ghid_extents_draw_line (hidGC gc, int x1, int y1, int x2, int y2)
{
  printf ("ghid_extents_draw_line() -- not implemented\n");
}

void
ghid_extents_draw_arc (hidGC gc, int cx, int cy,
		       int xradius, int yradius,
		       int start_angle, int delta_angle)
{
  printf ("ghid_extents_draw_arc() -- not implemented\n");
}

void
ghid_extents_draw_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  printf ("ghid_extents_draw_rect() -- not implemented\n");
}

void
ghid_extents_fill_circle (hidGC gc, int cx, int cy, int radius)
{
  printf ("ghid_extents_fill_circle() -- not implemented\n");
}

void
ghid_extents_fill_polygon (hidGC gc, int n_coords, int *x, int *y)
{
  printf ("ghid_extents_fill_polygon() -- not implemented\n");
}

void
ghid_extents_fill_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  printf ("ghid_extents_fill_rect() -- not implemented\n");
}

void
ghid_calibrate (double xval, double yval)
{
  printf ("ghid_calibrate() -- not implemented\n");
}

static int ghid_gui_is_up = 0;

void
ghid_notify_gui_is_up ()
{
  ghid_gui_is_up = 1;
}

int
ghid_shift_is_pressed ()
{
  GdkModifierType mask;
  GHidPort *out = &ghid_port;

  if( ! ghid_gui_is_up ) 
    return 0;

  gdk_window_get_pointer (out->drawing_area->window, NULL, NULL, &mask);
  return (mask & GDK_SHIFT_MASK) ? TRUE : FALSE;
}

int
ghid_control_is_pressed ()
{
  GdkModifierType mask;
  GHidPort *out = &ghid_port;

  if( ! ghid_gui_is_up )
    return 0;

  gdk_window_get_pointer (out->drawing_area->window, NULL, NULL, &mask);
  return (mask & GDK_CONTROL_MASK) ? TRUE : FALSE;
}

void
ghid_set_crosshair (int x, int y, int action)
{
  if (gport->x_crosshair != x || gport->y_crosshair != y)
    {
      ghid_set_cursor_position_labels ();
      gport->x_crosshair = x;
      gport->y_crosshair = y;

      /*
       * FIXME - does this trigger the idle_proc stuff?  It is in the
       * lesstif HID.  Maybe something is needed here?
       *
       * need_idle_proc ();
       */

    }

  /*
   * Pan the viewport so that the crosshair (which is in a fixed
   * location relative to the board) lands where the pointer
   * is.  What happens is the crosshair is moved on the board
   * (see above) and then we move the board here to line it up
   * again.  We do this by figuring out where the pointer is
   * in board coordinates and we know where the crosshair is
   * in board coordinates.  Then we know how far to pan.
   */
  if (action == HID_SC_PAN_VIEWPORT)
    {
      GdkDisplay *display;
      GdkScreen *screen;
      gint pos_x, pos_y, xofs, yofs;
      
      display = gdk_display_get_default ();
      screen = gdk_display_get_default_screen (display); 
      
      /* figure out where the pointer is relative to the display */ 
      gdk_display_get_pointer (display, NULL, &pos_x, &pos_y, NULL); 
      
      /*
       * Figure out where the drawing area is on the screen so we can
       * figure out where the pointer is relative to the viewport.
       */ 
      gdk_window_get_origin (gport->drawing_area->window, &xofs, &yofs);
      
      pos_x -= xofs;
      pos_y -= yofs;

      /*
       * pointer is at
       *  px = gport->view_x0 + pos_x * gport->zoom
       *  py = gport->view_y0 + pos_y * gport->zoom
       *
       * cross hair is at
       *  x
       *  y
       *
       * we need to shift x0 by (x - px) and y0 by (y - py)
       * x0 = x0 + x - (x0 + pos_x * zoom)
       *    = x - pos_x*zoom
       */

      if (ghid_flip_x)
        gport->view_x0 = x - (gport->view_width - pos_x * gport->zoom);
      else
        gport->view_x0 = x - pos_x * gport->zoom;

      if (ghid_flip_y)
        gport->view_y0 = y - (gport->view_height - pos_y * gport->zoom);
      else
        gport->view_y0 = y - pos_y * gport->zoom;

      ghid_pan_fixup();

      action = HID_SC_WARP_POINTER;
    }

  if (action == HID_SC_WARP_POINTER)
    {
#if GTK_CHECK_VERSION(2,8,0)
    gint xofs, yofs;
    GdkDisplay *display;
    GdkScreen *screen;

    display = gdk_display_get_default ();
    screen = gdk_display_get_default_screen (display); 

    /*
     * Figure out where the drawing area is on the screen because
     * gdk_display_warp_pointer will warp relative to the whole display
     * but the value we've been given is relative to your drawing area
     */ 
    gdk_window_get_origin (gport->drawing_area->window, &xofs, &yofs);

    /* 
     * Note that under X11, gdk_display_warp_pointer is just a wrapper around XWarpPointer, but
     * hopefully by avoiding the direct call to an X function we might still work under windows
     * and other non-X11 based gdk's
     */
    gdk_display_warp_pointer (display, screen, xofs + Vx2 (x), yofs + Vy2 (y));


#else
#  ifdef HAVE_GDK_GDKX_H
    gint xofs, yofs;
    gdk_window_get_origin (gport->drawing_area->window, &xofs, &yofs);
    XWarpPointer (GDK_DRAWABLE_XDISPLAY (gport->drawing_area->window),
		  None, GDK_WINDOW_XID (gport->drawing_area->window),
		  0, 0, 0, 0,
		  xofs + Vx2 (x), yofs + Vy2 (y));
#  else
#    error  "sorry.  You need gtk+>=2.8.0 unless you are on X windows"
#  endif
#endif
    }
}

typedef struct
{
  void (*func) ();
  gint id;
  hidval user_data;
}
GuiTimer;

  /* We need a wrapper around the hid timer because a gtk timer needs
     |  to return FALSE else the timer will be restarted.
   */
static gboolean
ghid_timer (GuiTimer * timer)
{
  (*timer->func) (timer->user_data);
  ghid_mode_cursor (Settings.Mode);
  return FALSE;			/* Turns timer off */
}

hidval
ghid_add_timer (void (*func) (hidval user_data),
		unsigned long milliseconds, hidval user_data)
{
  GuiTimer *timer = g_new0 (GuiTimer, 1);
  hidval ret;

  timer->func = func;
  timer->user_data = user_data;
  timer->id = gtk_timeout_add (milliseconds, (GtkFunction) ghid_timer, timer);
  ret.ptr = (void *) timer;
  return ret;
}

void
ghid_stop_timer (hidval timer)
{
  void *ptr = timer.ptr;

  gtk_timeout_remove (((GuiTimer *) ptr)->id);
  g_free( ptr );
}

typedef struct
{
  void (*func) ( hidval, int, unsigned int, hidval );
  hidval user_data;
  int fd;
  GIOChannel *channel;
  gint id;
}
GuiWatch;

  /* We need a wrapper around the hid file watch to pass the correct flags
   */
static gboolean
ghid_watch (GIOChannel *source, GIOCondition condition, gpointer data)
{
  unsigned int pcb_condition = 0;
  hidval x;
  GuiWatch *watch = (GuiWatch*)data;

  if (condition & G_IO_IN)
    pcb_condition |= PCB_WATCH_READABLE;
  if (condition & G_IO_OUT)
    pcb_condition |= PCB_WATCH_WRITABLE;
  if (condition & G_IO_ERR)
    pcb_condition |= PCB_WATCH_ERROR;
  if (condition & G_IO_HUP)
    pcb_condition |= PCB_WATCH_HANGUP;

  x.ptr = (void *) watch;
  watch->func (x, watch->fd, pcb_condition, watch->user_data);
  ghid_mode_cursor (Settings.Mode);

  return TRUE;  /* Leave watch on */
}

hidval
ghid_watch_file (int fd, unsigned int condition, void (*func) (hidval watch, int fd, unsigned int condition, hidval user_data),
  hidval user_data)
{
  GuiWatch *watch = g_new0 (GuiWatch, 1);
  hidval ret;
  unsigned int glib_condition = 0;

  if (condition & PCB_WATCH_READABLE)
    glib_condition |= G_IO_IN;
  if (condition & PCB_WATCH_WRITABLE)
    glib_condition |= G_IO_OUT;
  if (condition & PCB_WATCH_ERROR)
    glib_condition |= G_IO_ERR;
  if (condition & PCB_WATCH_HANGUP)
    glib_condition |= G_IO_HUP;

  watch->func = func;
  watch->user_data = user_data;
  watch->fd = fd;
  watch->channel = g_io_channel_unix_new( fd );
  watch->id = g_io_add_watch( watch->channel, glib_condition, ghid_watch, watch );

  ret.ptr = (void *) watch;
  return ret;
}

void
ghid_unwatch_file (hidval data)
{
  GuiWatch *watch = (GuiWatch*)data.ptr;

  g_io_channel_shutdown( watch->channel, TRUE, NULL ); 
  g_io_channel_unref( watch->channel );
  g_free( watch );
}

typedef struct
{
  GSource source;
  void (*func) (hidval user_data);
  hidval user_data; 
} BlockHookSource;

static gboolean ghid_block_hook_prepare  (GSource     *source,
                                             gint     *timeout);
static gboolean ghid_block_hook_check    (GSource     *source);
static gboolean ghid_block_hook_dispatch (GSource     *source,
                                          GSourceFunc  callback,
                                          gpointer     user_data);

static GSourceFuncs ghid_block_hook_funcs = {
  ghid_block_hook_prepare,
  ghid_block_hook_check,
  ghid_block_hook_dispatch,
  NULL // No destroy notification
};

static gboolean
ghid_block_hook_prepare (GSource *source,
                         gint    *timeout)
{
  hidval data = ((BlockHookSource *)source)->user_data;
  ((BlockHookSource *)source)->func( data );
  return FALSE;
}

static gboolean
ghid_block_hook_check (GSource *source)
{
  return FALSE;
}

static gboolean
ghid_block_hook_dispatch (GSource     *source,
                          GSourceFunc  callback,
                          gpointer     user_data)
{
  return FALSE;
}

static hidval
ghid_add_block_hook (void (*func) (hidval data),
                     hidval user_data)
{
  hidval ret;
  BlockHookSource *source;

  source = (BlockHookSource *)g_source_new (&ghid_block_hook_funcs, sizeof( BlockHookSource ));

  source->func = func;
  source->user_data = user_data;

  g_source_attach ((GSource *)source, NULL);

  ret.ptr = (void *) source;
  return ret;
}

static void
ghid_stop_block_hook (hidval mlpoll)
{
  GSource *source = (GSource *)mlpoll.ptr;
  g_source_destroy( source );
}

int
ghid_confirm_dialog (char *msg, ...)
{
  int rv;
  va_list ap;
  char *cancelmsg = NULL, *okmsg = NULL;

  va_start (ap, msg);
  cancelmsg = va_arg (ap, char *);
  okmsg = va_arg (ap, char *);
  va_end (ap);

  if (!cancelmsg)
    {
      cancelmsg = _("_Cancel");
      okmsg = _("_OK");
    }
  if (!okmsg)
    {
      okmsg = _("_OK");
    }

  rv = ghid_dialog_confirm (msg, cancelmsg, okmsg);

  return rv;
}

int
ghid_close_confirm_dialog ()
{
  switch (ghid_dialog_close_confirm ())
    {
    case GUI_DIALOG_CLOSE_CONFIRM_SAVE:
      {
        if (hid_actionl ("Save", NULL))
          { /* Save failed */
            return 0; /* Cancel */
          } else {
            return 1; /* Close */
          }
      }
    case GUI_DIALOG_CLOSE_CONFIRM_NOSAVE:
      {
        return 1; /* Close */
      }
    case GUI_DIALOG_CLOSE_CONFIRM_CANCEL:
    default:
      {
        return 0; /* Cancel */
      }
    }
}

void
ghid_report_dialog (char *title, char *msg)
{
  ghid_dialog_report (title, msg);
}

char *
ghid_prompt_for (char *msg, char *default_string)
{
  char *rv;

  rv = ghid_dialog_input (msg, default_string);
  return rv;
}

/* FIXME -- implement a proper file select dialog */
#ifdef FIXME
char *
ghid_fileselect (const char *title, const char *descr,
		 char *default_file, char *default_ext,
		 const char *history_tag, int flags)
{
  char *rv;

  rv = ghid_dialog_input (title, default_file);
  return rv;
}
#endif

void
ghid_show_item (void *item)
{
  ghid_pinout_window_show (&ghid_port, (ElementTypePtr) item);
}

void
ghid_beep ()
{
  gdk_beep ();
}

static int
ghid_progress (int so_far, int total, const char *message)
{
  return 0;
}

/* ---------------------------------------------------------------------- */

HID ghid_hid = {
  sizeof (HID),
  "gtk",
  "Gtk - The Gimp Toolkit",
  1,				/* gui */
  0,				/* printer */
  0,				/* exporter */
  0,				/* poly before */
  1,				/* poly after */
  0,				/* poly dicer */

  ghid_get_export_options,
  ghid_do_export,
  ghid_parse_arguments,

  ghid_invalidate_wh,
  ghid_invalidate_lr,
  ghid_invalidate_all,
  ghid_set_layer,
  ghid_make_gc,
  ghid_destroy_gc,
  ghid_use_mask,
  ghid_set_color,
  ghid_set_line_cap,
  ghid_set_line_width,
  ghid_set_draw_xor,
  ghid_set_draw_faded,
  ghid_set_line_cap_angle,
  ghid_draw_line,
  ghid_draw_arc,
  ghid_draw_rect,
  ghid_fill_circle,
  ghid_fill_polygon,
  ghid_fill_rect,

  ghid_calibrate,
  ghid_shift_is_pressed,
  ghid_control_is_pressed,
  ghid_get_coords,
  ghid_set_crosshair,
  ghid_add_timer,
  ghid_stop_timer,
  ghid_watch_file,
  ghid_unwatch_file,
  ghid_add_block_hook,
  ghid_stop_block_hook,

  ghid_log,
  ghid_logv,
  ghid_confirm_dialog,
  ghid_close_confirm_dialog,
  ghid_report_dialog,
  ghid_prompt_for,
  ghid_fileselect,
  ghid_attribute_dialog,
  ghid_show_item,
  ghid_beep,
  ghid_progress
};

HID ghid_extents = {
  sizeof (HID),
  "ghid_extents",
  "used to calculate extents",
  1,				/* gui */
  0,				/* printer */
  0,				/* exporter */
  0,				/* poly before */
  1,				/* poly after */
  0,				/* poly dicer */

  0 /* ghid_get_export_options */ ,
  0 /* ghid_do_export */ ,
  0 /* ghid_parse_arguments */ ,

  0 /* ghid_invalidate_wh */ ,
  0 /* ghid_invalidate_lr */ ,
  0 /* ghid_invalidate_all */ ,
  0 /* ghid_set_layer */ ,
  0 /* ghid_make_gc */ ,
  0 /* ghid_destroy_gc */ ,
  ghid_extents_use_mask,
  0 /* ghid_set_color */ ,
  0 /* ghid_set_line_cap */ ,
  0 /* ghid_set_line_width */ ,
  0 /* ghid_set_draw_xor */ ,
  0 /* ghid_set_draw_faded */ ,
  0 /* ghid_set_line_cap_angle */ ,
  ghid_extents_draw_line,
  ghid_extents_draw_arc,
  ghid_extents_draw_rect,
  ghid_extents_fill_circle,
  ghid_extents_fill_polygon,
  ghid_extents_fill_rect,

  0 /* ghid_calibrate */ ,
  0 /* ghid_shift_is_pressed */ ,
  0 /* ghid_control_is_pressed */ ,
  0 /* ghid_get_coords */ ,
  0 /* ghid_set_crosshair */ ,
  0 /* ghid_add_timer */ ,
  0 /* ghid_stop_timer */ ,
  0 /* ghid_watch_file */ ,
  0 /* ghid_unwatch_file */ ,
  0 /* ghid_add_block_hook */ ,
  0 /* ghid_stop_block_hook */ ,

  0 /* ghid_log */ ,
  0 /* ghid_logv */ ,
  0 /* ghid_confirm_dialog */ ,
  0 /* ghid_close_confirm_dialog */ ,
  0 /* ghid_report_dialog */ ,
  0 /* ghid_prompt_for */ ,
  0 /* ghid_attribute_dialog */ ,
  0 /* ghid_show_item */ ,
  0 /* ghid_beep */ ,
  0 /* ghid_progress */
};

/* ------------------------------------------------------------ 
 *
 * Actions specific to the GTK HID follow from here
 *
 */


/* ------------------------------------------------------------ */
static const char about_syntax[] =
"About()";

static const char about_help[] =
"Tell the user about this version of PCB.";

/* %start-doc actions About

This just pops up a dialog telling the user which version of
@code{pcb} they're running.

%end-doc */


static int
About (int argc, char **argv, int x, int y)
{
  ghid_dialog_about ();
  return 0;
}

/* ------------------------------------------------------------ */
static const char getxy_syntax[] =
"GetXY()";

static const char getxy_help[] =
"Get a coordinate.";

/* %start-doc actions GetXY

Prompts the user for a coordinate, if one is not already selected.

%end-doc */

static int
GetXY (int argc, char **argv, int x, int y)
{
  return 0;
}

/* ---------------------------------------------------------------------- */

static int PointCursor (int argc, char **argv, int x, int y)
{
  if (!ghidgui)
    return 0;

  if (argc > 0)
    ghid_point_cursor ();
  else
    ghid_mode_cursor (Settings.Mode);
  return 0;
}

/* ---------------------------------------------------------------------- */

static int
RouteStylesChanged (int argc, char **argv, int x, int y)
{
  gint n;

  if (PCB && PCB->RouteStyle[0].Name)
    for (n = 0; n < NUM_STYLES; ++n)
      ghid_route_style_set_button_label ((&PCB->RouteStyle[n])->Name, n);
  return 0;
}

/* ---------------------------------------------------------------------- */

int
PCBChanged (int argc, char **argv, int x, int y)
{
  if (!ghidgui)
    return 0;

  ghid_window_set_name_label (PCB->Name);

  if (!gport->pixmap)
    return 0;
  RouteStylesChanged (0, NULL, 0, 0);
  ghid_port_ranges_scale (TRUE);
  ghid_port_ranges_pan (0, 0, FALSE);
  ghid_port_ranges_zoom (0);
  ghid_port_ranges_changed ();
  ghid_sync_with_new_layout ();
  return 0;
}

/* ---------------------------------------------------------------------- */

static int
LayerGroupsChanged (int argc, char **argv, int x, int y)
{
  printf ("LayerGroupsChanged -- not implemented\n");
  return 0;
}

/* ---------------------------------------------------------------------- */

static int
LibraryChanged (int argc, char **argv, int x, int y)
{
  ghid_library_window_show (&ghid_port, FALSE);
  return 0;
}

/* ---------------------------------------------------------------------- */

static int
Command (int argc, char **argv, int x, int y)
{
  ghid_handle_user_command (TRUE);
  return 0;
}

/* ---------------------------------------------------------------------- */

static int
Load (int argc, char **argv, int x, int y)
{
  char *function;
  char *name = NULL;

  static gchar *current_element_dir = NULL;
  static gchar *current_layout_dir = NULL;
  static gchar *current_netlist_dir = NULL;

  /* we've been given the file name */
  if (argc > 1)
    return hid_actionv ("LoadFrom", argc, argv);

  function = argc ? argv[0] : "Layout";

  if (strcasecmp (function, "Netlist") == 0)
    {
      name = ghid_dialog_file_select_open (_("Load netlist file"),
					   &current_netlist_dir,
					   Settings.FilePath);
    }
  else if (strcasecmp (function, "ElementToBuffer") == 0)
    {
      name = ghid_dialog_file_select_open (_("Load element to buffer"),
					   &current_element_dir,
					   Settings.LibraryTree);
    }
  else if (strcasecmp (function, "LayoutToBuffer") == 0)
    {
      name = ghid_dialog_file_select_open (_("Load layout file to buffer"),
					   &current_layout_dir,
					   Settings.FilePath);
    }
  else if (strcasecmp (function, "Layout") == 0)
    {
      name = ghid_dialog_file_select_open (_("Load layout file"),
					   &current_layout_dir,
					   Settings.FilePath);
    }

  if (name)
    {
      if (Settings.verbose)
      	fprintf (stderr, "%s:  Calling LoadFrom(%s, %s)\n", __FUNCTION__,
		 function, name);
      hid_actionl ("LoadFrom", function, name, NULL);
      g_free (name);
    }

  return 0;
}


/* ---------------------------------------------------------------------- */
static const char save_syntax[] =
"Save()\n"
"Save(Layout|LayoutAs)\n"
"Save(AllConnections|AllUnusedPins|ElementConnections)\n"
"Save(PasteBuffer)";

static const char save_help[] =
"Save layout and/or element data to a user-selected file.";

/* %start-doc actions Save

This action is a GUI front-end to the core's @code{SaveTo} action
(@pxref{SaveTo Action}).  If you happen to pass a filename, like
@code{SaveTo}, then @code{SaveTo} is called directly.  Else, the
user is prompted for a filename to save, and then @code{SaveTo} is
called with that filename.

%end-doc */

static int
Save (int argc, char **argv, int x, int y)
{
  char *function;
  char *name;
  char *prompt;

  static gchar *current_dir = NULL;

  if (argc > 1)
    return hid_actionv ("SaveTo", argc, argv);

  function = argc ? argv[0] : "Layout";

  if (strcasecmp (function, "Layout") == 0)
    if (PCB->Filename)
      return hid_actionl ("SaveTo", "Layout", PCB->Filename, NULL);

  if (strcasecmp (function, "PasteBuffer") == 0)
    prompt = _("Save element as");
  else
    prompt = _("Save layout as");
  
  name = ghid_dialog_file_select_save (prompt,
				       &current_dir,
				       PCB->Filename, Settings.FilePath);
  
  if (name)
    {
      FILE *exist;
      exist = fopen (name, "r");
      if (exist)
	{
	  fclose (exist);
	  if (ghid_dialog_confirm (_("File exists!  Ok to overwrite?"), NULL, NULL))
	    {
	      if (Settings.verbose)
		fprintf (stderr, "Overwriting %s\n", name);
	    }
	  else
	    {
	      g_free (name);
	      return 1;
	    }
	}
      
      if (Settings.verbose)
	fprintf (stderr, "%s:  Calling SaveTo(%s, %s)\n", 
		 __FUNCTION__, function, name);
      
      if (strcasecmp (function, "PasteBuffer") == 0)
	hid_actionl ("PasteBuffer", "Save", name, NULL);
      else
	{
	  /* 
	   * if we got this far and the function is Layout, then
	   * we really needed it to be a LayoutAs.  Otherwise 
	   * ActionSaveTo() will ignore the new file name we
	   * just obtained.
	   */
	  if (strcasecmp (function, "Layout") == 0)
	    hid_actionl ("SaveTo", "LayoutAs", name, NULL);
	  else
	    hid_actionl ("SaveTo", function, name, NULL);
	}
      g_free (name);
    }
  else
    {
      return 1;
    }

  return 0;
}

/* ---------------------------------------------------------------------- */
static const char swapsides_syntax[] =
"SwapSides(|v|h|r)";

static const char swapsides_help[] =
"Swaps the side of the board you're looking at.";

/* %start-doc actions SwapSides

This action changes the way you view the board.

@table @code

@item v
Flips the board over vertically (up/down).

@item h
Flips the board over horizontally (left/right), like flipping pages in
a book.

@item r
Rotates the board 180 degrees without changing sides.

@end table

If no argument is given, the board isn't moved but the opposite side
is shown.

Normally, this action changes which pads and silk layer are drawn as
true silk, and which are drawn as the "invisible" layer.  It also
determines which solder mask you see.

As a special case, if the layer group for the side you're looking at
is visible and currently active, and the layer group for the opposite
is not visible (i.e. disabled), then this action will also swap which
layer group is visible and active, effectively swapping the ``working
side'' of the board.

%end-doc */


static int
SwapSides (int argc, char **argv, int x, int y)
{
  gint flipd;
  int do_flip_x = 0;
  int do_flip_y = 0;
  int comp_group = GetLayerGroupNumberByNumber (max_layer + COMPONENT_LAYER);
  int solder_group = GetLayerGroupNumberByNumber (max_layer + SOLDER_LAYER);
  int active_group = GetLayerGroupNumberByNumber (LayerStack[0]);
  int comp_showing =
    PCB->Data->Layer[PCB->LayerGroups.Entries[comp_group][0]].On;
  int solder_showing =
    PCB->Data->Layer[PCB->LayerGroups.Entries[solder_group][0]].On;


  if (argc > 0)
    {
      switch (argv[0][0]) {
      case 'h':
      case 'H':
	ghid_flip_x = ! ghid_flip_x;
	do_flip_x = 1;
	break;
      case 'v':
      case 'V':
	ghid_flip_y = ! ghid_flip_y;
	do_flip_y = 1;
	break;
      case 'r':
      case 'R':
	ghid_flip_x = ! ghid_flip_x;
	ghid_flip_y = ! ghid_flip_y;
	do_flip_x = 1;
	do_flip_y = 1;
	break;
      default:
	return 1;
      }
      /* SwapSides will swap this */
      Settings.ShowSolderSide = (ghid_flip_x == ghid_flip_y);
    }

  Settings.ShowSolderSide = !Settings.ShowSolderSide;
  if (Settings.ShowSolderSide)
    {
      if (active_group == comp_group && comp_showing && !solder_showing)
	{
	  ChangeGroupVisibility (PCB->LayerGroups.Entries[comp_group][0], 0,
				 0);
	  ChangeGroupVisibility (PCB->LayerGroups.Entries[solder_group][0], 1,
				 1);
	}
    }
  else
    {
      if (active_group == solder_group && solder_showing && !comp_showing)
	{
	  ChangeGroupVisibility (PCB->LayerGroups.Entries[solder_group][0], 0,
				 0);
	  ChangeGroupVisibility (PCB->LayerGroups.Entries[comp_group][0], 1,
				 1);
	}
    }

  /* Update coordinates so that the current location stays where it was on the
     other side; we need to do this since the actual flip center is the
     center of the board while the user expect the center would be the current
     location */
  if (do_flip_x)
    {
	flipd = PCB->MaxWidth / 2 - gport->view_x;
	ghid_port_ranges_pan (2 * flipd, 0, TRUE);
    }
  if (do_flip_y)
    {
	flipd = PCB->MaxHeight / 2 - gport->view_y;
	ghid_port_ranges_pan (0, 2 * flipd, TRUE);
    }

  ghid_invalidate_all ();
  return 0;
}

/* ------------------------------------------------------------ */

static const char print_syntax[] =
"Print()";

static const char print_help[] =
"Print the layout.";

/* %start-doc actions Print

This will find the default printing HID, prompt the user for its
options, and print the layout.

%end-doc */

static int
Print (int argc, char **argv, int x, int y)
{
  HID **hids;
  int i;
  HID *printer = NULL;

  hids = hid_enumerate ();
  for (i = 0; hids[i]; i++)
    {
      if (hids[i]->printer)
	printer = hids[i];
    }

  if (printer == NULL)
    {
      gui->log (_("Can't find a suitable printer HID"));
      return -1;
    }

  /* check if layout is empty */
  if (!IsDataEmpty (PCB->Data))
    {
      ghid_dialog_print (printer);
    }
  else
    gui->log (_("Can't print empty layout"));

  return 0;
}


/* ------------------------------------------------------------ */

static HID_Attribute
printer_calibrate_attrs[] = {
  {"Enter Values here:", "",
   HID_Label, 0, 0, {0, 0, 0}, 0, 0},
  {"x-calibration", "X scale for calibrating your printer",
   HID_Real, 0.5, 25, {0, 0, 1.00}, 0, 0},
  {"y-calibration", "Y scale for calibrating your printer",
   HID_Real, 0.5, 25, {0, 0, 1.00}, 0, 0}
};
static HID_Attr_Val printer_calibrate_values[3];

static const char printcalibrate_syntax[] =
"PrintCalibrate()";

static const char printcalibrate_help[] =
"Calibrate the printer.";

/* %start-doc actions PrintCalibrate

This will print a calibration page, which you would measure and type
the measurements in, so that future printouts will be more precise.

%end-doc */

static int
PrintCalibrate (int argc, char **argv, int x, int y)
{
  HID *printer = hid_find_printer ();
  printer->calibrate (0.0, 0.0);

  if (gui->attribute_dialog (printer_calibrate_attrs, 3,
			     printer_calibrate_values,
			     "Printer Calibration Values",
			     "Enter calibration values for your printer"))
    return 1;
  printer->calibrate (printer_calibrate_values[1].real_value,
		      printer_calibrate_values[2].real_value);
  return 0;
}

/* ------------------------------------------------------------ */

static int
Export (int argc, char **argv, int x, int y)
{

  /* check if layout is empty */
  if (!IsDataEmpty (PCB->Data))
    {
      ghid_dialog_export ();
    }
  else
    gui->log (_("Can't export empty layout"));

  return 0;
}

/* ------------------------------------------------------------ */

static int
Benchmark (int argc, char **argv, int x, int y)
{
  int i = 0;
  time_t start, end;
  BoxType region;
  GdkDisplay *display;

  display = gdk_drawable_get_display (gport->drawable);

  region.X1 = 0;
  region.Y1 = 0;
  region.X2 = PCB->MaxWidth;
  region.Y2 = PCB->MaxHeight;

  gdk_display_sync (display);
  time (&start);
  do
    {
      hid_expose_callback (&ghid_hid, &region, 0);
      gdk_display_sync (display);
      time (&end);
      i++;
    }
  while (end - start < 10);

  printf ("%g redraws per second\n", i / 10.0);

  return 0;
}

/* ------------------------------------------------------------ */

static const char center_syntax[] =
"Center()\n";

static const char center_help[] =
"Moves the pointer to the center of the window.";

/* %start-doc actions Center

Move the pointer to the center of the window, but only if it's
currently within the window already.

%end-doc */

static int
Center(int argc, char **argv, int x, int y)
{
  int x0, y0, w2, h2, dx, dy;
 
  if (argc != 0)
    AFAIL (center);

  x = GRIDFIT_X (SIDE_X (x), PCB->Grid);
  y = GRIDFIT_Y (SIDE_Y (y), PCB->Grid);

  w2 = gport->view_width / 2;
  h2 = gport->view_height / 2;
  x0 = x - w2;
  y0 = y - h2;

  if (x0 < 0) 
    {
      x0 = 0;
      x = x0 + w2;
    }

  if (y0 < 0)
    {
      y0 = 0;
      y = y0 + w2;
    }

  dx = (x0 - gport->view_x0) / gport->zoom ;
  dy = (y0 - gport->view_y0) / gport->zoom;
  gport->view_x0 = x0;
  gport->view_y0 = y0;


  ghid_pan_fixup ();

  /* Move the pointer to the center of the window, but only if it's
     currently within the window already.  Watch out for edges,
     though.  */

#if GTK_CHECK_VERSION(2,8,0)
  {
    GdkDisplay *display;
    GdkScreen *screen;
    gint cx, cy;

    display = gdk_display_get_default ();
    screen = gdk_display_get_default_screen (display); 

    /* figure out where the pointer is and then move it from there by the specified delta */
    gdk_display_get_pointer (display, NULL, &cx, &cy, NULL); 
    gdk_display_warp_pointer (display, screen, cx - dx, cy - dy);

    /* 
     * Note that under X11, gdk_display_warp_pointer is just a wrapper around XWarpPointer, but
     * hopefully by avoiding the direct call to an X function we might still work under windows
     * and other non-X11 based gdk's
     */
  }
#else  
#  ifdef HAVE_GDK_GDKX_H
  {

    Window w_src, w_dst; 
    w_src = GDK_WINDOW_XID (gport->drawing_area->window);
    w_dst = w_src;

    /* don't warp with the auto drc - that creates auto-scroll chaos */
    if (TEST_FLAG (AUTODRCFLAG, PCB) && Settings.Mode == LINE_MODE
	&& Crosshair.AttachedLine.State != STATE_FIRST)
      return 0;
    
    XWarpPointer (GDK_DRAWABLE_XDISPLAY (gport->drawing_area->window),
		 w_src, w_dst,
		 0, 0, 0, 0,
		 Vx2 (x), Vy2 (y));
    
    /* XWarpPointer creates Motion events normally bound to
     *  EventMoveCrosshair.
     *  We don't do any updates when EventMoveCrosshair
     *  is called the next time to prevent from rounding errors
     */
    /* FIXME?
     * IgnoreMotionEvents = ignore;
     */
  }
#  endif
#endif

  return 0;
}

/* ------------------------------------------------------------ */
static const char cursor_syntax[] =
"Cursor(Type,DeltaUp,DeltaRight,Units)";

static const char cursor_help[] =
"Move the cursor.";

/* %start-doc actions Cursor

This action moves the mouse cursor.  Unlike other actions which take
coordinates, this action's coordinates are always relative to the
user's view of the board.  Thus, a positive @var{DeltaUp} may move the
cursor towards the board origin if the board is inverted.

Type is one of @samp{Pan} or @samp{Warp}.  @samp{Pan} causes the
viewport to move such that the crosshair is under the mouse cursor.
@samp{Warp} causes the mouse cursor to move to be above the crosshair.

@var{Units} can be one of the following:

@table @samp

@item mil
@itemx mm
The cursor is moved by that amount, in board units.

@item grid
The cursor is moved by that many grid points.

@item view
The values are percentages of the viewport's view.  Thus, a pan of
@samp{100} would scroll the viewport by exactly the width of the
current view.

@item board
The values are percentages of the board size.  Thus, a move of
@samp{50,50} moves you halfway across the board.

@end table

%end-doc */

static int
CursorAction(int argc, char **argv, int x, int y)
{
  int pan_warp = HID_SC_DO_NOTHING;
  double dx, dy;
  double xu = 0.0, yu = 0.0;

  if (argc != 4)
    AFAIL (cursor);

  if (strcasecmp (argv[0], "pan") == 0)
    pan_warp = HID_SC_PAN_VIEWPORT;
  else if (strcasecmp (argv[0], "warp") == 0)
    pan_warp = HID_SC_WARP_POINTER;
  else
    AFAIL (cursor);

  dx = strtod (argv[1], 0);
  if (ghid_flip_x)
    dx = -dx;
  dy = strtod (argv[2], 0);
  if (!ghid_flip_y)
    dy = -dy;

  /* 
   * xu and yu are the scale factors that we multiply dx and dy by to
   * come up with PCB internal units.
   */
  if (strncasecmp (argv[3], "mm", 2) == 0)
    xu = yu = MM_TO_COOR;
  else if (strncasecmp (argv[3], "mil", 3) == 0)
    xu = yu = 100;
  else if (strncasecmp (argv[3], "grid", 4) == 0)
    xu = yu = PCB->Grid;
  else if (strncasecmp (argv[3], "view", 4) == 0)
    {
      xu = gport->view_width / 100.0;
      yu = gport->view_height / 100.0;
    }
  else if (strncasecmp (argv[3], "board", 4) == 0)
    {
      xu = PCB->MaxWidth / 100.0;
      yu = PCB->MaxHeight / 100.0;
    }

  EventMoveCrosshair (Crosshair.X+(int)(dx*xu), Crosshair.Y+(int)(dy*yu));
  gui->set_crosshair (Crosshair.X, Crosshair.Y, pan_warp);

  return 0;
}
/* ------------------------------------------------------------ */

static const char dowindows_syntax[] =
"DoWindows(1|2|3|4)\n"
"DoWindows(Layout|Library|Log|Netlist|Preferences)";

static const char dowindows_help[] =
"Open various GUI windows.";

/* %start-doc actions DoWindows

@table @code

@item 1
@itemx Layout
Open the layout window.  Since the layout window is always shown
anyway, this has no effect.

@item 2
@itemx Library
Open the library window.

@item 3
@itemx Log
Open the log window.

@item 4
@itemx Netlist
Open the netlist window.

@item 5
@itemx Preferences
Open the preferences window.

@end table

%end-doc */

static int
DoWindows (int argc, char **argv, int x, int y)
{
  char *a = argc == 1 ? argv[0] : "";

  if (strcmp (a, "1") == 0 || strcasecmp (a, "Layout") == 0)
    {
    }
  else if (strcmp (a, "2") == 0 || strcasecmp (a, "Library") == 0)
    {
      ghid_library_window_show (gport, TRUE);
    }
  else if (strcmp (a, "3") == 0 || strcasecmp (a, "Log") == 0)
    {
      ghid_log_window_show (TRUE);
    }
  else if (strcmp (a, "4") == 0 || strcasecmp (a, "Netlist") == 0)
    {
      ghid_netlist_window_show (gport, TRUE);
    }
  else if (strcmp (a, "5") == 0 || strcasecmp (a, "Preferences") == 0)
    {
      ghid_config_window_show ();
    }
  else
    {
      AFAIL (dowindows);
    }

  return 0;
}

/* ------------------------------------------------------------ */
static const char setunits_syntax[] =
"SetUnits(mm|mil)";

static const char setunits_help[] =
"Set the default measurement units.";

/* %start-doc actions SetUnits

@table @code

@item mil
Sets the display units to mils (1/1000 inch).

@item mm
Sets the display units to millimeters.

@end table

%end-doc */

static int
SetUnits (int argc, char **argv, int x, int y)
{
  if (argc == 0)
    return 0;
  if (strcmp (argv[0], "mil") == 0)
    Settings.grid_units_mm = 0;
  if (strcmp (argv[0], "mm") == 0)
    Settings.grid_units_mm = 1;

  ghid_config_handle_units_changed ();

  ghid_set_status_line_label ();

  /* FIXME ?
   * lesstif_sizes_reset ();
   * lesstif_styles_update_values ();
   */
  return 0;
}

/* ------------------------------------------------------------ */
static const char popup_syntax[] =
"Popup(MenuName, [Button])";

static const char popup_help[] =
"Bring up the popup menu specified by @code{MenuName}.\n"
"If called by a mouse event then the mouse button number\n"
"must be specified as the optional second argument.";

/* %start-doc actions Popup

This just pops up the specified menu.  The menu must have been defined
as a named subresource of the Popups resource in the menu resource
file.  If called as a response to a mouse button click, the mouse 
button number must be specified as the second argument.  

%end-doc */


static int
Popup (int argc, char **argv, int x, int y)
{
  GtkWidget *menu;
  char *element;
  guint button;

  if (argc != 1 && argc != 2)
    AFAIL (popup);

  if (argc == 1)
    button = 0;
  else
    button = atoi (argv[1]);

  if ( (element = (char *) malloc ( (strlen (argv[0]) + 2) * sizeof (char))) == NULL )
    {
      fprintf (stderr, "Popup():  malloc failed\n");
      exit (1);
    }

  sprintf (element, "/%s", argv[0]);
  printf ("Loading popup \"%s\". Button = %u\n", element, button);

  menu = gtk_ui_manager_get_widget (ghidgui->ui_manager, element);
  free (element);

  if (! GTK_IS_MENU (menu))
    {
      Message ("The specified popup menu \"%s\" has not been defined.\n", argv[0]);
      return 1;
    }
  else
    {
      ghidgui->in_popup = TRUE;
      gtk_widget_grab_focus (ghid_port.drawing_area);
      gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, 
		      gtk_get_current_event_time());
    }
  return 0;
}

static int
Busy (int argc, char **argv, int x, int y)
{
  ghid_watch_cursor ();
  return 0;
}

HID_Action ghid_main_action_list[] = {
  {"About", 0, About, about_help, about_syntax},
  {"Benchmark", 0, Benchmark},
  {"Busy", 0, Busy},
  {"Center", "Click on a location to center", Center, center_help, center_syntax},
  {"Command", 0, Command},
  {"Cursor", 0, CursorAction, cursor_help, cursor_syntax},
  {"DoWindows", 0, DoWindows, dowindows_help, dowindows_syntax},
  {"Export", 0, Export},
  {"GetXY", "", GetXY, getxy_help, getxy_syntax},
  {"LayerGroupsChanged", 0, LayerGroupsChanged},
  {"LibraryChanged", 0, LibraryChanged},
  {"Load", 0, Load},
  {"PCBChanged", 0, PCBChanged},
  {"PointCursor", 0, PointCursor},
  {"Popup", 0, Popup, popup_help, popup_syntax},
  {"Print", 0, Print,
   print_help, print_syntax},
  {"PrintCalibrate", 0, PrintCalibrate,
   printcalibrate_help, printcalibrate_syntax},
  {"RouteStylesChanged", 0, RouteStylesChanged},
  {"Save", 0, Save, save_help, save_syntax},
  {"SetUnits", 0, SetUnits, setunits_help, setunits_syntax},
  {"SwapSides", 0, SwapSides, swapsides_help, swapsides_syntax},
  {"Zoom", "Click on zoom focus", Zoom, zoom_help, zoom_syntax}
};

REGISTER_ACTIONS (ghid_main_action_list)


static int
flag_flipx (int x)
{ 
  return ghid_flip_x;
} 
static int  
flag_flipy (int x)
{ 
  return ghid_flip_y;
} 

HID_Flag ghid_main_flag_list[] = {
  {"flip_x", flag_flipx, 0},
  {"flip_y", flag_flipy, 0}
};  

REGISTER_FLAGS (ghid_main_flag_list)

#include "dolists.h"

/*
 * We will need these for finding the windows installation
 * directory.  Without that we can't find our fonts and
 * footprint libraries.
 */
#ifdef WIN32
#include <windows.h>
#include <winreg.h>
#endif

void
hid_gtk_init ()
{
  #ifdef WIN32

  char * tmps;
  char * share_dir;

  tmps = g_win32_get_package_installation_directory (PACKAGE "-" VERSION, NULL);
#define REST_OF_PATH G_DIR_SEPARATOR_S "share" G_DIR_SEPARATOR_S PACKAGE
  share_dir = (char *) malloc(strlen(tmps) + 
			  strlen(REST_OF_PATH) +
			  1);
  sprintf (share_dir, "%s%s", tmps, REST_OF_PATH);
  free (tmps);
#undef REST_OF_PATH
  printf ("\"Share\" installation path is \"%s\"\n", share_dir);
#endif  

  hid_register_hid (&ghid_hid);
  apply_default_hid (&ghid_extents, &ghid_hid);
#include "gtk_lists.h"
}
