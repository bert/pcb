/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "crosshair.h"
#include "clip.h"
#include "../hidint.h"
#include "gui.h"
#include "gui-pinout-preview.h"

/* The Linux OpenGL ABI 1.0 spec requires that we define
 * GL_GLEXT_PROTOTYPES before including gl.h or glx.h for extensions
 * in order to get prototypes:
 *   http://www.opengl.org/registry/ABI/
 */
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <gtk/gtkgl.h>
#include "hid/common/hidgl.h"
#include "hid/common/draw_helpers.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");


extern HID ghid_hid;

static hidGC current_gc = NULL;

/* Sets gport->u_gc to the "right" GC to use (wrt mask or window)
*/
#define USE_GC(gc) if (!use_gc(gc)) return

static int cur_mask = -1;

typedef struct render_priv {
  GdkGLConfig *glconfig;
  bool trans_lines;
  bool in_context;
  int subcomposite_stencil_bit;
  char *current_colorname;
  double current_alpha_mult;
} render_priv;


typedef struct hid_gc_struct
{
  HID *me_pointer;

  const char *colorname;
  double alpha_mult;
  gint width;
  gint cap, join;
  gchar xor;
}
hid_gc_struct;

static void
start_subcomposite (void)
{
  render_priv *priv = gport->render_priv;
  int stencil_bit;

  /* Flush out any existing geoemtry to be rendered */
  hidgl_flush_triangles (&buffer);

  glEnable (GL_STENCIL_TEST);                                 /* Enable Stencil test */
  glStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE);                 /* Stencil pass => replace stencil value (with 1) */

  stencil_bit = hidgl_assign_clear_stencil_bit();             /* Get a new (clean) bitplane to stencil with */
  glStencilMask (stencil_bit);                                /* Only write to our subcompositing stencil bitplane */
  glStencilFunc (GL_GREATER, stencil_bit, stencil_bit);       /* Pass stencil test if our assigned bit is clear */

  priv->subcomposite_stencil_bit = stencil_bit;
}

static void
end_subcomposite (void)
{
  render_priv *priv = gport->render_priv;

  /* Flush out any existing geoemtry to be rendered */
  hidgl_flush_triangles (&buffer);

  hidgl_return_stencil_bit (priv->subcomposite_stencil_bit);  /* Relinquish any bitplane we previously used */

  glStencilMask (0);
  glStencilFunc (GL_ALWAYS, 0, 0);                            /* Always pass stencil test */
  glDisable (GL_STENCIL_TEST);                                /* Disable Stencil test */

  priv->subcomposite_stencil_bit = 0;
}


int
ghid_set_layer (const char *name, int group, int empty)
{
  render_priv *priv = gport->render_priv;
  int idx = group;
  if (idx >= 0 && idx < max_group)
    {
      int n = PCB->LayerGroups.Number[group];
      for (idx = 0; idx < n-1; idx ++)
	{
	  int ni = PCB->LayerGroups.Entries[group][idx];
	  if (ni >= 0 && ni < max_copper_layer + 2
	      && PCB->Data->Layer[ni].On)
	    break;
	}
      idx = PCB->LayerGroups.Entries[group][idx];
  }

  end_subcomposite ();
  start_subcomposite ();

  if (idx >= 0 && idx < max_copper_layer + 2)
    {
      priv->trans_lines = true;
      return PCB->Data->Layer[idx].On;
    }
  if (idx < 0)
    {
      switch (SL_TYPE (idx))
	{
	case SL_INVISIBLE:
	  return PCB->InvisibleObjectsOn;
	case SL_MASK:
	  if (SL_MYSIDE (idx))
	    return TEST_FLAG (SHOWMASKFLAG, PCB);
	  return 0;
	case SL_SILK:
	  priv->trans_lines = true;
	  if (SL_MYSIDE (idx))
	    return PCB->ElementOn;
	  return 0;
	case SL_ASSY:
	  return 0;
	case SL_PDRILL:
	case SL_UDRILL:
	  return 1;
	case SL_RATS:
	  if (PCB->RatOn)
	    priv->trans_lines = true;
	  return PCB->RatOn;
	}
    }
  return 0;
}

static void
ghid_end_layer (void)
{
  end_subcomposite ();
}

void
ghid_destroy_gc (hidGC gc)
{
  g_free (gc);
}

hidGC
ghid_make_gc (void)
{
  hidGC rv;

  rv = g_new0 (hid_gc_struct, 1);
  rv->me_pointer = &ghid_hid;
  rv->colorname = Settings.BackgroundColor;
  rv->alpha_mult = 1.0;
  return rv;
}

static void
ghid_draw_grid (BoxTypePtr drawn_area)
{
  if (Vz (PCB->Grid) < MIN_GRID_DISTANCE)
    return;

  if (gdk_color_parse (Settings.GridColor, &gport->grid_color))
    {
      gport->grid_color.red ^= gport->bg_color.red;
      gport->grid_color.green ^= gport->bg_color.green;
      gport->grid_color.blue ^= gport->bg_color.blue;
    }

  glEnable (GL_COLOR_LOGIC_OP);
  glLogicOp (GL_XOR);

  glColor3f (gport->grid_color.red / 65535.,
             gport->grid_color.green / 65535.,
             gport->grid_color.blue / 65535.);

  hidgl_draw_grid (drawn_area);

  glDisable (GL_COLOR_LOGIC_OP);
}

static void
ghid_draw_bg_image (void)
{
  static GLuint texture_handle = 0;

  if (!ghidgui->bg_pixbuf)
    return;

  if (texture_handle == 0)
    {
      int width =             gdk_pixbuf_get_width (ghidgui->bg_pixbuf);
      int height =            gdk_pixbuf_get_height (ghidgui->bg_pixbuf);
      int rowstride =         gdk_pixbuf_get_rowstride (ghidgui->bg_pixbuf);
      int bits_per_sample =   gdk_pixbuf_get_bits_per_sample (ghidgui->bg_pixbuf);
      int n_channels =        gdk_pixbuf_get_n_channels (ghidgui->bg_pixbuf);
      unsigned char *pixels = gdk_pixbuf_get_pixels (ghidgui->bg_pixbuf);

      g_warn_if_fail (bits_per_sample == 8);
      g_warn_if_fail (rowstride == width * n_channels);

      glGenTextures (1, &texture_handle);
      glBindTexture (GL_TEXTURE_2D, texture_handle);

      /* XXX: We should proabbly determine what the maxmimum texture supported is,
       *      and if our image is larger, shrink it down using GDK pixbuf routines
       *      rather than having it fail below.
       */

      glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                    (n_channels == 4) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, pixels);
    }

  glBindTexture (GL_TEXTURE_2D, texture_handle);

  glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glEnable (GL_TEXTURE_2D);

  /* Render a quad with the background as a texture */

  glBegin (GL_QUADS);
  glTexCoord2d (0., 0.);
  glVertex3i (0,             0,              0);
  glTexCoord2d (1., 0.);
  glVertex3i (PCB->MaxWidth, 0,              0);
  glTexCoord2d (1., 1.);
  glVertex3i (PCB->MaxWidth, PCB->MaxHeight, 0);
  glTexCoord2d (0., 1.);
  glVertex3i (0,             PCB->MaxHeight, 0);
  glEnd ();

  glDisable (GL_TEXTURE_2D);
}

void
ghid_use_mask (int use_it)
{
  static int stencil_bit = 0;

  if (use_it == cur_mask)
    return;

  /* Flush out any existing geoemtry to be rendered */
  hidgl_flush_triangles (&buffer);

  switch (use_it)
    {
    case HID_MASK_BEFORE:
      /* The HID asks not to receive this mask type, so warn if we get it */
      g_return_if_reached ();

    case HID_MASK_CLEAR:
      /* Write '1' to the stencil buffer where the solder-mask should not be drawn. */
      glColorMask (0, 0, 0, 0);                             /* Disable writting in color buffer */
      glEnable (GL_STENCIL_TEST);                           /* Enable Stencil test */
      stencil_bit = hidgl_assign_clear_stencil_bit();       /* Get a new (clean) bitplane to stencil with */
      glStencilFunc (GL_ALWAYS, stencil_bit, stencil_bit);  /* Always pass stencil test, write stencil_bit */
      glStencilMask (stencil_bit);                          /* Only write to our subcompositing stencil bitplane */
      glStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE);           /* Stencil pass => replace stencil value (with 1) */
      break;

    case HID_MASK_AFTER:
      /* Drawing operations as masked to areas where the stencil buffer is '0' */
      glColorMask (1, 1, 1, 1);                   /* Enable drawing of r, g, b & a */
      glStencilFunc (GL_GEQUAL, 0, stencil_bit);  /* Draw only where our bit of the stencil buffer is clear */
      glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);    /* Stencil buffer read only */
      break;

    case HID_MASK_OFF:
      /* Disable stenciling */
      hidgl_return_stencil_bit (stencil_bit);     /* Relinquish any bitplane we previously used */
      glDisable (GL_STENCIL_TEST);                /* Disable Stencil test */
      break;
    }
  cur_mask = use_it;
}


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
}

void
ghid_set_special_colors (HID_Attribute * ha)
{
  if (!ha->name || !ha->value)
    return;
  if (!strcmp (ha->name, "background-color"))
    {
      ghid_map_color_string (*(char **) ha->value, &gport->bg_color);
      set_special_grid_color ();
    }
  else if (!strcmp (ha->name, "off-limit-color"))
  {
      ghid_map_color_string (*(char **) ha->value, &gport->offlimits_color);
    }
  else if (!strcmp (ha->name, "grid-color"))
    {
      ghid_map_color_string (*(char **) ha->value, &gport->grid_color);
      set_special_grid_color ();
    }
}

typedef struct
{
  int color_set;
  GdkColor color;
  int xor_set;
  GdkColor xor_color;
  double red;
  double green;
  double blue;
} ColorCache;

static void
set_gl_color_for_gc (hidGC gc)
{
  render_priv *priv = gport->render_priv;
  static void *cache = NULL;
  hidval cval;
  ColorCache *cc;
  double r, g, b, a;

  if (priv->current_colorname != NULL &&
      strcmp (priv->current_colorname, gc->colorname) == 0 &&
      priv->current_alpha_mult == gc->alpha_mult)
    return;

  free (priv->current_colorname);
  priv->current_colorname = strdup (gc->colorname);
  priv->current_alpha_mult = gc->alpha_mult;

  if (gport->colormap == NULL)
    gport->colormap = gtk_widget_get_colormap (gport->top_window);
  if (strcmp (gc->colorname, "erase") == 0)
    {
      r = gport->bg_color.red   / 65535.;
      g = gport->bg_color.green / 65535.;
      b = gport->bg_color.blue  / 65535.;
      a = 1.0;
    }
  else if (strcmp (gc->colorname, "drill") == 0)
    {
      r = gport->offlimits_color.red   / 65535.;
      g = gport->offlimits_color.green / 65535.;
      b = gport->offlimits_color.blue  / 65535.;
      a = 0.85;
    }
  else
    {
      if (hid_cache_color (0, gc->colorname, &cval, &cache))
        cc = (ColorCache *) cval.ptr;
      else
        {
          cc = (ColorCache *) malloc (sizeof (ColorCache));
          memset (cc, 0, sizeof (*cc));
          cval.ptr = cc;
          hid_cache_color (1, gc->colorname, &cval, &cache);
        }

      if (!cc->color_set)
        {
          if (gdk_color_parse (gc->colorname, &cc->color))
            gdk_color_alloc (gport->colormap, &cc->color);
          else
            gdk_color_white (gport->colormap, &cc->color);
          cc->red   = cc->color.red   / 65535.;
          cc->green = cc->color.green / 65535.;
          cc->blue  = cc->color.blue  / 65535.;
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
              cc->red   = cc->color.red   / 65535.;
              cc->green = cc->color.green / 65535.;
              cc->blue  = cc->color.blue  / 65535.;
              cc->xor_set = 1;
            }
        }
      r = cc->red;
      g = cc->green;
      b = cc->blue;
      a = 0.7;
    }
  if (1) {
    double maxi, mult;
    a *= gc->alpha_mult;
    if (!priv->trans_lines)
      a = 1.0;
    maxi = r;
    if (g > maxi) maxi = g;
    if (b > maxi) maxi = b;
    mult = MIN (1 / a, 1 / maxi);
#if 1
    r = r * mult;
    g = g * mult;
    b = b * mult;
#endif
  }

  if(!priv->in_context)
    return;

  hidgl_flush_triangles (&buffer);
  glColor4d (r, g, b, a);
}

void
ghid_set_color (hidGC gc, const char *name)
{
  gc->colorname = name;
  set_gl_color_for_gc (gc);
}

void
ghid_set_alpha_mult (hidGC gc, double alpha_mult)
{
  gc->alpha_mult = alpha_mult;
  set_gl_color_for_gc (gc);
}

void
ghid_set_line_cap (hidGC gc, EndCapStyle style)
{
  gc->cap = style;
}

void
ghid_set_line_width (hidGC gc, int width)
{
  gc->width = width;
}


void
ghid_set_draw_xor (hidGC gc, int xor)
{
  /* NOT IMPLEMENTED */

  /* Only presently called when setting up a crosshair GC.
   * We manage our own drawing model for that anyway. */
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

static void
ghid_invalidate_current_gc (void)
{
  current_gc = NULL;
}

static int
use_gc (hidGC gc)
{
  if (gc->me_pointer != &ghid_hid)
    {
      fprintf (stderr, "Fatal: GC from another HID passed to GTK HID\n");
      abort ();
    }

  if (current_gc == gc)
    return 1;

  current_gc = gc;

  set_gl_color_for_gc (gc);
  return 1;
}

void
ghid_draw_line (hidGC gc, int x1, int y1, int x2, int y2)
{
  USE_GC (gc);

  hidgl_draw_line (gc->cap, gc->width, x1, y1, x2, y2, gport->zoom);
}

void
ghid_draw_arc (hidGC gc, int cx, int cy, int xradius, int yradius,
                         int start_angle, int delta_angle)
{
  USE_GC (gc);

  hidgl_draw_arc (gc->width, cx, cy, xradius, yradius,
                  start_angle, delta_angle, gport->zoom);
}

void
ghid_draw_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  USE_GC (gc);

  hidgl_draw_rect (x1, y1, x2, y2);
}


void
ghid_fill_circle (hidGC gc, int cx, int cy, int radius)
{
  USE_GC (gc);

  hidgl_fill_circle (cx, cy, radius, gport->zoom);
}


void
ghid_fill_polygon (hidGC gc, int n_coords, int *x, int *y)
{
  USE_GC (gc);

  hidgl_fill_polygon (n_coords, x, y);
}

void
ghid_fill_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box)
{
  USE_GC (gc);

  hidgl_fill_pcb_polygon (poly, clip_box, gport->zoom);
}

void
ghid_thindraw_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box)
{
  common_thindraw_pcb_polygon (gc, poly, clip_box);
  ghid_set_alpha_mult (gc, 0.25);
  ghid_fill_pcb_polygon (gc, poly, clip_box);
  ghid_set_alpha_mult (gc, 1.0);
}

void
ghid_fill_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  USE_GC (gc);

  hidgl_fill_rect (x1, y1, x2, y2);
}

void
ghid_invalidate_lr (int left, int right, int top, int bottom)
{
  ghid_invalidate_all ();
}

void
ghid_invalidate_all ()
{
  ghid_draw_area_update (gport, NULL);
}

void
ghid_notify_crosshair_change (bool changes_complete)
{
  /* We sometimes get called before the GUI is up */
  if (gport->drawing_area == NULL)
    return;

  /* FIXME: We could just invalidate the bounds of the crosshair attached objects? */
  ghid_invalidate_all ();
}

void
ghid_notify_mark_change (bool changes_complete)
{
  /* We sometimes get called before the GUI is up */
  if (gport->drawing_area == NULL)
    return;

  /* FIXME: We could just invalidate the bounds of the mark? */
  ghid_invalidate_all ();
}

static void
draw_right_cross (gint x, gint y, gint z)
{
  glVertex3i (x, 0, z);
  glVertex3i (x, PCB->MaxHeight, z);
  glVertex3i (0, y, z);
  glVertex3i (PCB->MaxWidth, y, z);
}

static void
draw_slanted_cross (gint x, gint y, gint z)
{
  gint x0, y0, x1, y1;

  x0 = x + (PCB->MaxHeight - y);
  x0 = MAX(0, MIN (x0, PCB->MaxWidth));
  x1 = x - y;
  x1 = MAX(0, MIN (x1, PCB->MaxWidth));
  y0 = y + (PCB->MaxWidth - x);
  y0 = MAX(0, MIN (y0, PCB->MaxHeight));
  y1 = y - x;
  y1 = MAX(0, MIN (y1, PCB->MaxHeight));
  glVertex3i (x0, y0, z);
  glVertex3i (x1, y1, z);

  x0 = x - (PCB->MaxHeight - y);
  x0 = MAX(0, MIN (x0, PCB->MaxWidth));
  x1 = x + y;
  x1 = MAX(0, MIN (x1, PCB->MaxWidth));
  y0 = y + x;
  y0 = MAX(0, MIN (y0, PCB->MaxHeight));
  y1 = y - (PCB->MaxWidth - x);
  y1 = MAX(0, MIN (y1, PCB->MaxHeight));
  glVertex3i (x0, y0, z);
  glVertex3i (x1, y1, z);
}

static void
draw_dozen_cross (gint x, gint y, gint z)
{
  gint x0, y0, x1, y1;
  gdouble tan60 = sqrt (3);

  x0 = x + (PCB->MaxHeight - y) / tan60;
  x0 = MAX(0, MIN (x0, PCB->MaxWidth));
  x1 = x - y / tan60;
  x1 = MAX(0, MIN (x1, PCB->MaxWidth));
  y0 = y + (PCB->MaxWidth - x) * tan60;
  y0 = MAX(0, MIN (y0, PCB->MaxHeight));
  y1 = y - x * tan60;
  y1 = MAX(0, MIN (y1, PCB->MaxHeight));
  glVertex3i (x0, y0, z);
  glVertex3i (x1, y1, z);

  x0 = x + (PCB->MaxHeight - y) * tan60;
  x0 = MAX(0, MIN (x0, PCB->MaxWidth));
  x1 = x - y * tan60;
  x1 = MAX(0, MIN (x1, PCB->MaxWidth));
  y0 = y + (PCB->MaxWidth - x) / tan60;
  y0 = MAX(0, MIN (y0, PCB->MaxHeight));
  y1 = y - x / tan60;
  y1 = MAX(0, MIN (y1, PCB->MaxHeight));
  glVertex3i (x0, y0, z);
  glVertex3i (x1, y1, z);

  x0 = x - (PCB->MaxHeight - y) / tan60;
  x0 = MAX(0, MIN (x0, PCB->MaxWidth));
  x1 = x + y / tan60;
  x1 = MAX(0, MIN (x1, PCB->MaxWidth));
  y0 = y + x * tan60;
  y0 = MAX(0, MIN (y0, PCB->MaxHeight));
  y1 = y - (PCB->MaxWidth - x) * tan60;
  y1 = MAX(0, MIN (y1, PCB->MaxHeight));
  glVertex3i (x0, y0, z);
  glVertex3i (x1, y1, z);

  x0 = x - (PCB->MaxHeight - y) * tan60;
  x0 = MAX(0, MIN (x0, PCB->MaxWidth));
  x1 = x + y * tan60;
  x1 = MAX(0, MIN (x1, PCB->MaxWidth));
  y0 = y + x / tan60;
  y0 = MAX(0, MIN (y0, PCB->MaxHeight));
  y1 = y - (PCB->MaxWidth - x) / tan60;
  y1 = MAX(0, MIN (y1, PCB->MaxHeight));
  glVertex3i (x0, y0, z);
  glVertex3i (x1, y1, z);
}

static void
draw_crosshair (gint x, gint y, gint z)
{
  static enum crosshair_shape prev = Basic_Crosshair_Shape;

  draw_right_cross (x, y, z);
  if (prev == Union_Jack_Crosshair_Shape)
    draw_slanted_cross (x, y, z);
  if (prev == Dozen_Crosshair_Shape)
    draw_dozen_cross (x, y, z);
  prev = Crosshair.shape;
}

#define VCW 16 /* Crosshair pan-marker width in pixels */
#define VCD 8  /* Crosshair pan-marker depth in pixels */

void
ghid_show_crosshair (gboolean paint_new_location)
{
  gint x, y, z;
  gboolean draw_markers;
  int vcw = VCW * gport->zoom;
  int vcd = VCD * gport->zoom;
  static int done_once = 0;
  static GdkColor cross_color;

  if (!paint_new_location)
    return;

  if (!done_once)
    {
      done_once = 1;
      /* FIXME: when CrossColor changed from config */
      ghid_map_color_string (Settings.CrossColor, &cross_color);
    }
  x = gport->x_crosshair;
  y = gport->y_crosshair;
  z = 0;

  glEnable (GL_COLOR_LOGIC_OP);
  glLogicOp (GL_XOR);

  hidgl_flush_triangles (&buffer);

  glColor3f (cross_color.red / 65535.,
             cross_color.green / 65535.,
             cross_color.blue / 65535.);

  if (x >= 0 && paint_new_location)
    {
      glBegin (GL_LINES);
      draw_crosshair (x, y, z);
      glEnd ();
    }

  draw_markers = ghidgui->auto_pan_on && have_crosshair_attachments ();
  if (x >= 0 && paint_new_location && draw_markers)
    {
      glBegin (GL_QUADS);
      glVertex3i (SIDE_X (gport->view_x0),                            y - vcd,       z);
      glVertex3i (SIDE_X (gport->view_x0),                            y - vcd + vcw, z);
      glVertex3i (SIDE_X (gport->view_x0 + vcd),                      y - vcd + vcw, z);
      glVertex3i (SIDE_X (gport->view_x0 + vcd),                      y - vcd,       z);

      glVertex3i (SIDE_X (gport->view_x0 + gport->view_width),        y - vcd,       z);
      glVertex3i (SIDE_X (gport->view_x0 + gport->view_width),        y - vcd + vcw, z);
      glVertex3i (SIDE_X (gport->view_x0 + gport->view_width - vcd),  y - vcd + vcw, z);
      glVertex3i (SIDE_X (gport->view_x0 + gport->view_width - vcd),  y - vcd,       z);

      glVertex3i (x - vcd,       SIDE_Y (gport->view_y0),                            z);
      glVertex3i (x - vcd,       SIDE_Y (gport->view_y0 + vcd),                      z);
      glVertex3i (x - vcd + vcw, SIDE_Y (gport->view_y0 + vcd),                      z);
      glVertex3i (x - vcd + vcw, SIDE_Y (gport->view_y0),                            z);

      glVertex3i (x - vcd,       SIDE_Y (gport->view_y0 + gport->view_height - vcd), z);
      glVertex3i (x - vcd,       SIDE_Y (gport->view_y0 + gport->view_height),       z);
      glVertex3i (x - vcd + vcw, SIDE_Y (gport->view_y0 + gport->view_height),       z);
      glVertex3i (x - vcd + vcw, SIDE_Y (gport->view_y0 + gport->view_height - vcd), z);

      glEnd ();
    }

  glDisable (GL_COLOR_LOGIC_OP);
}

void
ghid_init_renderer (int *argc, char ***argv, GHidPort *port)
{
  render_priv *priv;

  port->render_priv = priv = g_new0 (render_priv, 1);

  gtk_gl_init(argc, argv);

  /* setup GL-context */
  priv->glconfig = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGBA    |
                                              GDK_GL_MODE_STENCIL |
                                              GDK_GL_MODE_DOUBLE);
  if (!priv->glconfig)
    {
      printf ("Could not setup GL-context!\n");
      return; /* Should we abort? */
    }

  /* Setup HID function pointers specific to the GL renderer*/
  ghid_hid.end_layer = ghid_end_layer;
  ghid_hid.fill_pcb_polygon = ghid_fill_pcb_polygon;
  ghid_hid.thindraw_pcb_polygon = ghid_thindraw_pcb_polygon;
}

void
ghid_init_drawing_widget (GtkWidget *widget, GHidPort *port)
{
  render_priv *priv = port->render_priv;

  gtk_widget_set_gl_capability (widget,
                                priv->glconfig,
                                NULL,
                                TRUE,
                                GDK_GL_RGBA_TYPE);
}

void
ghid_drawing_area_configure_hook (GHidPort *port)
{
}

gboolean
ghid_start_drawing (GHidPort *port)
{
  GtkWidget *widget = port->drawing_area;
  GdkGLContext *pGlContext = gtk_widget_get_gl_context (widget);
  GdkGLDrawable *pGlDrawable = gtk_widget_get_gl_drawable (widget);

  /* make GL-context "current" */
  if (!gdk_gl_drawable_gl_begin (pGlDrawable, pGlContext))
    return FALSE;

  port->render_priv->in_context = true;

  return TRUE;
}

void
ghid_end_drawing (GHidPort *port)
{
  GtkWidget *widget = port->drawing_area;
  GdkGLDrawable *pGlDrawable = gtk_widget_get_gl_drawable (widget);

  if (gdk_gl_drawable_is_double_buffered (pGlDrawable))
    gdk_gl_drawable_swap_buffers (pGlDrawable);
  else
    glFlush ();

  port->render_priv->in_context = false;

  /* end drawing to current GL-context */
  gdk_gl_drawable_gl_end (pGlDrawable);
}

void
ghid_screen_update (void)
{
}

#define Z_NEAR 3.0
gboolean
ghid_drawing_area_expose_cb (GtkWidget *widget,
                             GdkEventExpose *ev,
                             GHidPort *port)
{
  BoxType region;

  ghid_start_drawing (port);

  hidgl_init ();

  /* If we don't have any stencil bits available,
     we can't use the hidgl polygon drawing routine */
  /* TODO: We could use the GLU tessellator though */
  if (hidgl_stencil_bits() == 0)
    ghid_hid.fill_pcb_polygon = common_fill_pcb_polygon;

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glViewport (0, 0, widget->allocation.width, widget->allocation.height);

  glEnable (GL_SCISSOR_TEST);
  glScissor (ev->area.x,
             widget->allocation.height - ev->area.height - ev->area.y,
             ev->area.width, ev->area.height);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glOrtho (0, widget->allocation.width, widget->allocation.height, 0, 0, 100);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  glTranslatef (0.0f, 0.0f, -Z_NEAR);

  glScalef ((ghid_flip_x ? -1. : 1.) / port->zoom,
            (ghid_flip_y ? -1. : 1.) / port->zoom,
            ((ghid_flip_x == ghid_flip_y) ? 1. : -1.) / port->zoom);
  glTranslatef (ghid_flip_x ? port->view_x0 - PCB->MaxWidth  :
                             -port->view_x0,
                ghid_flip_y ? port->view_y0 - PCB->MaxHeight :
                             -port->view_y0, 0);

  glEnable (GL_STENCIL_TEST);
  glClearColor (port->offlimits_color.red / 65535.,
                port->offlimits_color.green / 65535.,
                port->offlimits_color.blue / 65535.,
                1.);
  glStencilMask (~0);
  glClearStencil (0);
  glClear (GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  hidgl_reset_stencil_usage ();

  /* Disable the stencil test until we need it - otherwise it gets dirty */
  glDisable (GL_STENCIL_TEST);
  glStencilMask (0);
  glStencilFunc (GL_ALWAYS, 0, 0);

  region.X1 = MIN (Px (ev->area.x), Px (ev->area.x + ev->area.width + 1));
  region.X2 = MAX (Px (ev->area.x), Px (ev->area.x + ev->area.width + 1));
  region.Y1 = MIN (Py (ev->area.y), Py (ev->area.y + ev->area.height + 1));
  region.Y2 = MAX (Py (ev->area.y), Py (ev->area.y + ev->area.height + 1));

  glColor3f (port->bg_color.red / 65535.,
             port->bg_color.green / 65535.,
             port->bg_color.blue / 65535.);

  glBegin (GL_QUADS);
  glVertex3i (0,             0,              0);
  glVertex3i (PCB->MaxWidth, 0,              0);
  glVertex3i (PCB->MaxWidth, PCB->MaxHeight, 0);
  glVertex3i (0,             PCB->MaxHeight, 0);
  glEnd ();

  ghid_draw_bg_image ();

  hidgl_init_triangle_array (&buffer);
  ghid_invalidate_current_gc ();
  hid_expose_callback (&ghid_hid, &region, 0);
  hidgl_flush_triangles (&buffer);

  ghid_draw_grid (&region);

  ghid_invalidate_current_gc ();

  DrawAttached ();
  DrawMark ();
  hidgl_flush_triangles (&buffer);

  ghid_show_crosshair (TRUE);

  hidgl_flush_triangles (&buffer);

  ghid_end_drawing (port);

  return FALSE;
}

/* This realize callback is used to work around a crash bug in some mesa
 * versions (observed on a machine running the intel i965 driver. It isn't
 * obvious why it helps, but somehow fiddling with the GL context here solves
 * the issue. The problem appears to have been fixed in recent mesa versions.
 */
void
ghid_port_drawing_realize_cb (GtkWidget *widget, gpointer data)
{
  GdkGLContext *glcontext = gtk_widget_get_gl_context (widget);
  GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (widget);

  if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext))
    return;

  gdk_gl_drawable_gl_end (gldrawable);
  return;
}

gboolean
ghid_pinout_preview_expose (GtkWidget *widget,
                            GdkEventExpose *ev)
{
  GdkGLContext* pGlContext = gtk_widget_get_gl_context (widget);
  GdkGLDrawable* pGlDrawable = gtk_widget_get_gl_drawable (widget);
  GhidPinoutPreview *pinout = GHID_PINOUT_PREVIEW (widget);
  double save_zoom;
  int da_w, da_h;
  int save_left, save_top;
  int save_width, save_height;
  int save_view_width, save_view_height;
  double xz, yz;

  save_zoom = gport->zoom;
  save_width = gport->width;
  save_height = gport->height;
  save_left = gport->view_x0;
  save_top = gport->view_y0;
  save_view_width = gport->view_width;
  save_view_height = gport->view_height;

  /* Setup zoom factor for drawing routines */

  gdk_window_get_geometry (widget->window, 0, 0, &da_w, &da_h, 0);
  xz = (double) pinout->x_max / da_w;
  yz = (double) pinout->y_max / da_h;
  if (xz > yz)
    gport->zoom = xz;
  else
    gport->zoom = yz;

  gport->width = da_w;
  gport->height = da_h;
  gport->view_width = da_w * gport->zoom;
  gport->view_height = da_h * gport->zoom;
  gport->view_x0 = (pinout->x_max - gport->view_width) / 2;
  gport->view_y0 = (pinout->y_max - gport->view_height) / 2;

  /* make GL-context "current" */
  if (!gdk_gl_drawable_gl_begin (pGlDrawable, pGlContext)) {
    return FALSE;
  }
  gport->render_priv->in_context = true;

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glViewport (0, 0, widget->allocation.width, widget->allocation.height);

  glEnable (GL_SCISSOR_TEST);
  glScissor (ev->area.x,
             widget->allocation.height - ev->area.height - ev->area.y,
             ev->area.width, ev->area.height);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glOrtho (0, widget->allocation.width, widget->allocation.height, 0, 0, 100);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  glTranslatef (0.0f, 0.0f, -Z_NEAR);

  glClearColor (gport->bg_color.red / 65535.,
                gport->bg_color.green / 65535.,
                gport->bg_color.blue / 65535.,
                1.);
  glStencilMask (~0);
  glClearStencil (0);
  glClear (GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  hidgl_reset_stencil_usage ();

  /* call the drawing routine */
  hidgl_init_triangle_array (&buffer);
  ghid_invalidate_current_gc ();
  glPushMatrix ();
  glScalef ((ghid_flip_x ? -1. : 1.) / gport->zoom,
            (ghid_flip_y ? -1. : 1.) / gport->zoom, 1);
  glTranslatef (ghid_flip_x ? gport->view_x0 - PCB->MaxWidth  :
                             -gport->view_x0,
                ghid_flip_y ? gport->view_y0 - PCB->MaxHeight :
                             -gport->view_y0, 0);
  hid_expose_callback (&ghid_hid, NULL, &pinout->element);
  hidgl_flush_triangles (&buffer);
  glPopMatrix ();

  if (gdk_gl_drawable_is_double_buffered (pGlDrawable))
    gdk_gl_drawable_swap_buffers (pGlDrawable);
  else
    glFlush ();

  /* end drawing to current GL-context */
  gport->render_priv->in_context = false;
  gdk_gl_drawable_gl_end (pGlDrawable);

  gport->zoom = save_zoom;
  gport->width = save_width;
  gport->height = save_height;
  gport->view_x0 = save_left;
  gport->view_y0 = save_top;
  gport->view_width = save_view_width;
  gport->view_height = save_view_height;

  return FALSE;
}


GdkPixmap *
ghid_render_pixmap (int cx, int cy, double zoom, int width, int height, int depth)
{
  GdkGLConfig *glconfig;
  GdkPixmap *pixmap;
  GdkGLPixmap *glpixmap;
  GdkGLContext* glcontext;
  GdkGLDrawable* gldrawable;
  double save_zoom;
  int save_left, save_top;
  int save_width, save_height;
  int save_view_width, save_view_height;
  BoxType region;

  save_zoom = gport->zoom;
  save_width = gport->width;
  save_height = gport->height;
  save_left = gport->view_x0;
  save_top = gport->view_y0;
  save_view_width = gport->view_width;
  save_view_height = gport->view_height;

  /* Setup rendering context for drawing routines
   */

  glconfig = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGB     |
                                        GDK_GL_MODE_STENCIL |
                                        GDK_GL_MODE_SINGLE);

  pixmap = gdk_pixmap_new (NULL, width, height, depth);
  glpixmap = gdk_pixmap_set_gl_capability (pixmap, glconfig, NULL);
  gldrawable = GDK_GL_DRAWABLE (glpixmap);
  glcontext = gdk_gl_context_new (gldrawable, NULL, FALSE, GDK_GL_RGBA_TYPE);

  /* Setup zoom factor for drawing routines */

  gport->zoom = zoom;
  gport->width = width;
  gport->height = height;
  gport->view_width = width * gport->zoom;
  gport->view_height = height * gport->zoom;
  gport->view_x0 = ghid_flip_x ? PCB->MaxWidth - cx : cx;
  gport->view_x0 -= gport->view_height / 2;
  gport->view_y0 = ghid_flip_y ? PCB->MaxHeight - cy : cy;
  gport->view_y0 -= gport->view_width  / 2;

  /* make GL-context "current" */
  if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext)) {
    return NULL;
  }
  gport->render_priv->in_context = true;

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glViewport (0, 0, width, height);

  glEnable (GL_SCISSOR_TEST);
  glScissor (0, 0, width, height);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glOrtho (0, width, height, 0, 0, 100);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  glTranslatef (0.0f, 0.0f, -Z_NEAR);

  glClearColor (gport->bg_color.red / 65535.,
                gport->bg_color.green / 65535.,
                gport->bg_color.blue / 65535.,
                1.);
  glStencilMask (~0);
  glClearStencil (0);
  glClear (GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  hidgl_reset_stencil_usage ();

  /* call the drawing routine */
  hidgl_init_triangle_array (&buffer);
  ghid_invalidate_current_gc ();
  glPushMatrix ();
  glScalef ((ghid_flip_x ? -1. : 1.) / gport->zoom,
            (ghid_flip_y ? -1. : 1.) / gport->zoom, 1);
  glTranslatef (ghid_flip_x ? gport->view_x0 - PCB->MaxWidth  :
                             -gport->view_x0,
                ghid_flip_y ? gport->view_y0 - PCB->MaxHeight :
                             -gport->view_y0, 0);
  region.X1 = MIN(Px(0), Px(gport->width + 1));
  region.Y1 = MIN(Py(0), Py(gport->height + 1));
  region.X2 = MAX(Px(0), Px(gport->width + 1));
  region.Y2 = MAX(Py(0), Py(gport->height + 1));
  hid_expose_callback (&ghid_hid, &region, NULL);
  hidgl_flush_triangles (&buffer);
  glPopMatrix ();

  glFlush ();

  /* end drawing to current GL-context */
  gport->render_priv->in_context = false;
  gdk_gl_drawable_gl_end (gldrawable);

  gdk_pixmap_unset_gl_capability (pixmap);

  g_object_unref (glconfig);
  g_object_unref (glcontext);

  gport->zoom = save_zoom;
  gport->width = save_width;
  gport->height = save_height;
  gport->view_x0 = save_left;
  gport->view_y0 = save_top;
  gport->view_width = save_view_width;
  gport->view_height = save_view_height;

  return pixmap;
}

HID *
ghid_request_debug_draw (void)
{
  GHidPort *port = gport;
  GtkWidget *widget = port->drawing_area;

  ghid_start_drawing (port);

  glViewport (0, 0, widget->allocation.width, widget->allocation.height);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glOrtho (0, widget->allocation.width, widget->allocation.height, 0, 0, 100);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  glTranslatef (0.0f, 0.0f, -Z_NEAR);

  hidgl_init_triangle_array (&buffer);
  ghid_invalidate_current_gc ();

  /* Setup stenciling */
  glDisable (GL_STENCIL_TEST);

  glPushMatrix ();
  glScalef ((ghid_flip_x ? -1. : 1.) / port->zoom,
            (ghid_flip_y ? -1. : 1.) / port->zoom,
            (ghid_flip_x == ghid_flip_y) ? 1. : -1.);
  glTranslatef (ghid_flip_x ? port->view_x0 - PCB->MaxWidth  :
                             -port->view_x0,
                ghid_flip_y ? port->view_y0 - PCB->MaxHeight :
                             -port->view_y0, 0);

  return &ghid_hid;
}

void
ghid_flush_debug_draw (void)
{
  GtkWidget *widget = gport->drawing_area;
  GdkGLDrawable *pGlDrawable = gtk_widget_get_gl_drawable (widget);

  hidgl_flush_triangles (&buffer);

  if (gdk_gl_drawable_is_double_buffered (pGlDrawable))
    gdk_gl_drawable_swap_buffers (pGlDrawable);
  else
    glFlush ();
}

void
ghid_finish_debug_draw (void)
{
  hidgl_flush_triangles (&buffer);
  glPopMatrix ();

  ghid_end_drawing (gport);
}
