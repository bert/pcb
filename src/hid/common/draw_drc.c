#include "global.h"
#include "hid.h"
#include "hid_draw.h"
#include "data.h" /* For global "PCB" variable */
#include "misc.h" /* For GetArcEnds() */
#include "rotate.h" /* For RotateLineLowLevel() */
#include "polygon.h"
#include "draw_helpers.h"

/* Hacky chaining of functions */
static HID_DRAW_CLASS orig_class;


/*-----------------------------------------------------------
 * Draws the outline of a line
 */
static void
thindraw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2, Coord thick)
{
  Coord dx, dy, ox, oy;
  double h;

  dx = x2 - x1;
  dy = y2 - y1;
  if (dx != 0 || dy != 0)
    h = 0.5 * thick / sqrt (SQUARE (dx) + SQUARE (dy));
  else
    h = 0.0;
  ox = dy * h + 0.5 * SGN (dy);
  oy = -(dx * h + 0.5 * SGN (dx));
  hid_draw_line (gc, x1 + ox, y1 + oy, x2 + ox, y2 + oy);
  if (abs (ox) >= pixel_slop || abs (oy) >= pixel_slop)
    {
      Angle angle = atan2 (dx, dy) * 57.295779;
      hid_draw_line (gc, x1 - ox, y1 - oy, x2 - ox, y2 - oy);
      hid_draw_arc (gc, x1, y1, thick / 2, thick / 2, angle - 180, 180);
      hid_draw_arc (gc, x2, y2, thick / 2, thick / 2, angle, 180);
    }
}


/*-----------------------------------------------------------
 * Draws the outline of an arc
 */
static void
thindraw_arc (hidGC gc, Coord X, Coord Y, Coord wx, Coord wy, Angle sa, Angle dir, Coord thick)
{
  ArcType arc;
  BoxType *bx;
  Coord wid = thick / 2;

  arc.X = X;
  arc.Y = Y;
  arc.StartAngle = sa;
  arc.Delta = dir;
  arc.Width = wx;
  arc.Height = wy;
  bx = GetArcEnds (&arc);
  /*  sa = sa - 180; */
  hid_draw_arc (gc, arc.X, arc.Y, wy + wid, wy + wid, sa, dir);
  hid_draw_arc (gc, arc.X, arc.Y, wy - wid, wy - wid, sa, dir);
  hid_draw_arc (gc, bx->X1, bx->Y1, wid, wid, sa,      -180 * SGN (dir));
  hid_draw_arc (gc, bx->X2, bx->Y2, wid, wid, sa + dir, 180 * SGN (dir));
}


static void
common_drc_draw_pcb_line (hidGC gc, LineType *line)
{
  orig_class.draw_pcb_line (gc, line);

  if (!TEST_FLAG (SHOWDRCFLAG, PCB) ||
      line->ExtraDrcClearance == 0)
    return;

  hid_draw_set_color (gc, Settings.CrossColor);
  hid_draw_set_line_width (gc, 0);
  thindraw_line (gc, line->Point1.X, line->Point1.Y,
                     line->Point2.X, line->Point2.Y,
                     line->Thickness + line->ExtraDrcClearance * 2);
}

static void
common_drc_draw_pcb_arc (hidGC gc, ArcType *arc)
{
  orig_class.draw_pcb_arc (gc, arc);

  if (!TEST_FLAG (SHOWDRCFLAG, PCB) ||
      arc->ExtraDrcClearance == 0)
    return;

  hid_draw_set_color (gc, Settings.CrossColor);
  hid_draw_set_line_width (gc, 0);
  thindraw_arc (gc, arc->X, arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta, arc->Thickness + arc->ExtraDrcClearance * 2);
}

#if 0
/* ---------------------------------------------------------------------------
 * drawing routine for text objects
 */
static void
common_drc_draw_pcb_text (hidGC gc, TextType *Text, Coord min_line_width)
{
  orig_class.draw_pcb_text (gc, Text, min_line_width);
}
#endif

void
common_drc_draw_pcb_polygon (hidGC gc, PolygonType *polygon, const BoxType *clip_box)
{
  orig_class.draw_pcb_polygon (gc, polygon, clip_box);
}

void
common_drc_fill_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box)
{
  orig_class.fill_pcb_polygon (gc, poly, clip_box);
}

void
common_drc_thindraw_pcb_polygon (hidGC gc, PolygonType *poly,
                             const BoxType *clip_box)
{
  orig_class.thindraw_pcb_polygon (gc, poly, clip_box);
}

void
common_drc_thindraw_pcb_pad (hidGC gc, PadType *pad, bool clear, bool mask)
{
  PadType drc_pad;

  orig_class.thindraw_pcb_pad (gc, pad, clear, mask);

  if (!TEST_FLAG (SHOWDRCFLAG, PCB) ||
      pad->ExtraDrcClearance == 0)
    return;

  drc_pad = *pad;
  drc_pad.Clearance = pad->ExtraDrcClearance * 2;

  hid_draw_set_color (gc, Settings.CrossColor);
  orig_class.thindraw_pcb_pad (gc, &drc_pad, true /*clear*/, false /*mask*/);
}

void
common_drc_fill_pcb_pad (hidGC gc, PadType *pad, bool clear, bool mask)
{
  PadType drc_pad;

  orig_class.fill_pcb_pad (gc, pad, clear, mask);

  if (!TEST_FLAG (SHOWDRCFLAG, PCB) ||
      pad->ExtraDrcClearance == 0)
    return;

  drc_pad = *pad;
  drc_pad.Clearance = pad->ExtraDrcClearance * 2;

  hid_draw_set_color (gc, Settings.CrossColor);
  orig_class.thindraw_pcb_pad (gc, &drc_pad, true /*clear*/, false /*mask*/);
}

void
common_drc_fill_pcb_pv (hidGC fg_gc, hidGC bg_gc, PinType *pv, bool drawHole, bool mask)
{
  PinType drc_pv;

  orig_class.fill_pcb_pv (fg_gc, bg_gc, pv, drawHole, mask);

  if (!TEST_FLAG (SHOWDRCFLAG, PCB) ||
      pv->ExtraDrcClearance == 0)
    return;

  drc_pv = *pv;
  drc_pv.Thickness += pv->ExtraDrcClearance * 2;

  hid_draw_set_color (fg_gc, Settings.CrossColor);
  orig_class.thindraw_pcb_pv (fg_gc, bg_gc, &drc_pv, false /*drawHole*/, false /*mask*/);
}

void
common_drc_thindraw_pcb_pv (hidGC fg_gc, hidGC bg_gc, PinType *pv, bool drawHole, bool mask)
{
  PinType drc_pv;

  orig_class.thindraw_pcb_pv (fg_gc, bg_gc, pv, drawHole, mask);

  if (!TEST_FLAG (SHOWDRCFLAG, PCB) ||
      pv->ExtraDrcClearance == 0)
    return;

  drc_pv = *pv;
  pcb_printf ("via clearance was %mn, extra %mn\n", drc_pv.Thickness, pv->ExtraDrcClearance);
  drc_pv.Thickness += pv->ExtraDrcClearance * 2;
  pcb_printf ("via clearance is now %mn\n", drc_pv.Thickness);

  hid_draw_set_color (fg_gc, Settings.CrossColor);
  orig_class.thindraw_pcb_pv (fg_gc, bg_gc, &drc_pv, false /*drawHole*/, false /*mask*/);
}

void
common_draw_drc_class_init (HID_DRAW_CLASS *klass)
{
  orig_class = *klass; /* Copy function pointers from parent... allows chaining */

  klass->draw_pcb_line        = common_drc_draw_pcb_line;
  klass->draw_pcb_arc         = common_drc_draw_pcb_arc;
  klass->draw_pcb_polygon     = common_drc_fill_pcb_polygon;
//  klass->draw_pcb_text        = common_drc_fill_pcb_text;

  klass->fill_pcb_polygon     = common_drc_fill_pcb_polygon;
  klass->thindraw_pcb_polygon = common_drc_thindraw_pcb_polygon;
  klass->fill_pcb_pad         = common_drc_fill_pcb_pad;
  klass->thindraw_pcb_pad     = common_drc_thindraw_pcb_pad;
  klass->fill_pcb_pv          = common_drc_fill_pcb_pv;
  klass->thindraw_pcb_pv      = common_drc_thindraw_pcb_pv;
}

void
common_draw_drc_init (HID_DRAW *graphics)
{
}
