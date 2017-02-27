
#include "global.h"
#include "data.h"
#include "misc.h"
#include "rtree.h"
#include "draw_funcs.h"
#include "draw.h"
#include "hid_draw.h"

static void
_draw_pv (PinType *pv, bool draw_hole)
{
  if (TEST_FLAG (THINDRAWFLAG, PCB))
    hid_draw_thin_pcb_pv (Output.fgGC, Output.fgGC, pv, draw_hole, false);
  else
    hid_draw_fill_pcb_pv (Output.fgGC, Output.bgGC, pv, draw_hole, false);
}

static void
draw_pin (PinType *pin, const BoxType *drawn_area, void *userdata)
{
  _draw_pv (pin, false);
}

static void
draw_pin_mask (PinType *pin, const BoxType *drawn_area, void *userdata)
{
  if (TEST_FLAG (THINDRAWFLAG, PCB) || TEST_FLAG (THINDRAWPOLYFLAG, PCB))
    hid_draw_thin_pcb_pv (Output.pmGC, Output.pmGC, pin, false, true);
  else
    hid_draw_fill_pcb_pv (Output.pmGC, Output.pmGC, pin, false, true);
}

static void
draw_via (PinType *via, const BoxType *drawn_area, void *userdata)
{
  _draw_pv (via, false);
}

static void
draw_via_mask (PinType *via, const BoxType *drawn_area, void *userdata)
{
  if (TEST_FLAG (THINDRAWFLAG, PCB) || TEST_FLAG (THINDRAWPOLYFLAG, PCB))
    hid_draw_thin_pcb_pv (Output.pmGC, Output.pmGC, via, false, true);
  else
    hid_draw_fill_pcb_pv (Output.pmGC, Output.pmGC, via, false, true);
}

static void
draw_hole (PinType *pv, const BoxType *drawn_area, void *userdata)
{
  if (!TEST_FLAG (THINDRAWFLAG, PCB))
    hid_draw_fill_circle (Output.bgGC, pv->X, pv->Y, pv->DrillingHole / 2);

  if (TEST_FLAG (THINDRAWFLAG, PCB) || TEST_FLAG (HOLEFLAG, pv))
    {
      hid_draw_set_line_cap (Output.fgGC, Round_Cap);
      hid_draw_set_line_width (Output.fgGC, 0);

      hid_draw_arc (Output.fgGC, pv->X, pv->Y, pv->DrillingHole / 2, pv->DrillingHole / 2, 0, 360);
    }
}

static void
_draw_pad (hidGC gc, PadType *pad, bool clear, bool mask)
{
  if (clear && !mask && pad->Clearance <= 0)
    return;

  if (TEST_FLAG (THINDRAWFLAG, PCB) ||
      (clear && TEST_FLAG (THINDRAWPOLYFLAG, PCB)))
    hid_draw_thin_pcb_pad (gc, pad, clear, mask);
  else
    hid_draw_fill_pcb_pad (gc, pad, clear, mask);
}

static void
draw_pad (PadType *pad, const BoxType *drawn_area, void *userdata)
{
  _draw_pad (Output.fgGC, pad, false, false);
}

static void
draw_pad_mask (PadType *pad, const BoxType *drawn_area, void *userdata)
{
  if (pad->Mask <= 0)
    return;

  _draw_pad (Output.pmGC, pad, true, true);
}

static void
draw_pad_paste (PadType *pad, const BoxType *drawn_area, void *userdata)
{
  if (TEST_FLAG (NOPASTEFLAG, pad) || pad->Mask <= 0)
    return;

  if (pad->Mask < pad->Thickness)
    _draw_pad (Output.fgGC, pad, true, true);
  else
    _draw_pad (Output.fgGC, pad, false, false);
}

static void
draw_line (LineType *line, const BoxType *drawn_area, void *userdata)
{
  hid_draw_pcb_line (Output.fgGC, line);
}

static void
draw_rat (RatType *rat, const BoxType *drawn_area, void *userdata)
{
  if (Settings.RatThickness < 100)
    rat->Thickness = pixel_slop * Settings.RatThickness;
  /* rats.c set VIAFLAG if this rat goes to a containing poly: draw a donut */
  if (TEST_FLAG(VIAFLAG, rat))
    {
      int w = rat->Thickness;

      if (TEST_FLAG (THINDRAWFLAG, PCB))
        hid_draw_set_line_width (Output.fgGC, 0);
      else
        hid_draw_set_line_width (Output.fgGC, w);
      hid_draw_arc (Output.fgGC, rat->Point1.X, rat->Point1.Y, w * 2, w * 2, 0, 360);
    }
  else
    hid_draw_pcb_line (Output.fgGC, (LineType *) rat);
}

static void
draw_arc (ArcType *arc, const BoxType *drawn_area, void *userdata)
{
  hid_draw_pcb_arc (Output.fgGC, arc);
}

static void
draw_poly (PolygonType *polygon, const BoxType *drawn_area, void *userdata)
{
  hid_draw_pcb_polygon (Output.fgGC, polygon, drawn_area);
}

static void
set_object_color (AnyObjectType *obj, char *warn_color, char *selected_color,
                  char *connected_color, char *found_color, char *normal_color)
{
  char *color;

  if      (warn_color      != NULL && TEST_FLAG (WARNFLAG,      obj)) color = warn_color;
  else if (selected_color  != NULL && TEST_FLAG (SELECTEDFLAG,  obj)) color = selected_color;
  else if (connected_color != NULL && TEST_FLAG (CONNECTEDFLAG, obj)) color = connected_color;
  else if (found_color     != NULL && TEST_FLAG (FOUNDFLAG,     obj)) color = found_color;
  else                                                                color = normal_color;

  hid_draw_set_color (Output.fgGC, color);
}

static void
set_layer_object_color (LayerType *layer, AnyObjectType *obj)
{
  set_object_color (obj, NULL, layer->SelectedColor, PCB->ConnectedColor, PCB->FoundColor, layer->Color);
}

static int
line_callback (const BoxType * b, void *cl)
{
  LayerType *layer = cl;
  LineType *line = (LineType *)b;

  set_layer_object_color (layer, (AnyObjectType *) line);
  dapi->draw_line (line, NULL, NULL);
  return 1;
}

static int
arc_callback (const BoxType * b, void *cl)
{
  LayerType *layer = cl;
  ArcType *arc = (ArcType *)b;

  set_layer_object_color (layer, (AnyObjectType *) arc);
  dapi->draw_arc (arc, NULL, NULL);
  return 1;
}

struct poly_info {
  const const BoxType *drawn_area;
  LayerType *layer;
};

static int
poly_callback (const BoxType * b, void *cl)
{
  struct poly_info *i = cl;
  PolygonType *polygon = (PolygonType *)b;

  set_layer_object_color (i->layer, (AnyObjectType *) polygon);
  dapi->draw_poly (polygon, i->drawn_area, NULL);
  return 1;
}

static int
text_callback (const BoxType * b, void *cl)
{
  LayerType *layer = cl;
  TextType *text = (TextType *)b;
  int min_silk_line;

  if (TEST_FLAG (SELECTEDFLAG, text))
    hid_draw_set_color (Output.fgGC, layer->SelectedColor);
  else
    hid_draw_set_color (Output.fgGC, layer->Color);
  if (layer == &PCB->Data->SILKLAYER ||
      layer == &PCB->Data->BACKSILKLAYER)
    min_silk_line = PCB->minSlk;
  else
    min_silk_line = PCB->minWid;
  hid_draw_pcb_text (Output.fgGC, text, min_silk_line);
  return 1;
}

static void
set_pv_inlayer_color (PinType *pv, LayerType *layer, int type)
{
  if (TEST_FLAG (WARNFLAG, pv))          hid_draw_set_color (Output.fgGC, PCB->WarnColor);
  else if (TEST_FLAG (SELECTEDFLAG, pv)) hid_draw_set_color (Output.fgGC, (type == VIA_TYPE) ? PCB->ViaSelectedColor
                                                                                             : PCB->PinSelectedColor);
  else if (TEST_FLAG (FOUNDFLAG, pv))    hid_draw_set_color (Output.fgGC, PCB->ConnectedColor);
  else                                   hid_draw_set_color (Output.fgGC, layer->Color);
}

static int
pin_inlayer_callback (const BoxType * b, void *cl)
{
  set_pv_inlayer_color ((PinType *)b, cl, PIN_TYPE);
  dapi->draw_pin ((PinType *)b, NULL, NULL);
  return 1;
}

static int
via_inlayer_callback (const BoxType * b, void *cl)
{
  set_pv_inlayer_color ((PinType *)b, cl, VIA_TYPE);
  dapi->draw_via ((PinType *)b, NULL, NULL);
  return 1;
}

static int
pad_inlayer_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *)b;
  LayerType *layer = cl;
  int bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);
  int group = GetLayerGroupNumberByPointer (layer);

  int side = (group == bottom_group) ? BOTTOM_SIDE : TOP_SIDE;

  if (ON_SIDE (pad, side))
    {
      if (TEST_FLAG (WARNFLAG, pad))          hid_draw_set_color (Output.fgGC, PCB->WarnColor);
      else if (TEST_FLAG (SELECTEDFLAG, pad)) hid_draw_set_color (Output.fgGC, PCB->PinSelectedColor);
      else if (TEST_FLAG (FOUNDFLAG, pad))    hid_draw_set_color (Output.fgGC, PCB->ConnectedColor);
      else                                    hid_draw_set_color (Output.fgGC, layer->Color);

      dapi->draw_pad (pad, NULL, NULL);
    }
  return 1;
}

static int
pin_hole_callback (const BoxType * b, void *cl)
{
  PinType *pin = (PinType *)b;
  int plated = cl ? *(int *) cl : -1;

  if ((plated == 0 && !TEST_FLAG (HOLEFLAG, pin)) ||
      (plated == 1 &&  TEST_FLAG (HOLEFLAG, pin)))
    return 1;

  set_object_color ((AnyObjectType *) pin, PCB->WarnColor,
                    PCB->PinSelectedColor, NULL, NULL, Settings.BlackColor);

  dapi->draw_pin_hole (pin, NULL, NULL);
  return 1;
}

static int
via_hole_callback (const BoxType * b, void *cl)
{
  PinType *via = (PinType *)b;
  int plated = cl ? *(int *) cl : -1;

  if ((plated == 0 && !TEST_FLAG (HOLEFLAG, via)) ||
      (plated == 1 &&  TEST_FLAG (HOLEFLAG, via)))
    return 1;

  set_object_color ((AnyObjectType *) via, PCB->WarnColor,
                    PCB->ViaSelectedColor, NULL, NULL, Settings.BlackColor);

  dapi->draw_via_hole (via, NULL, NULL);
  return 1;
}

static int
pin_callback (const BoxType * b, void *cl)
{
  set_object_color ((AnyObjectType *)b,
                    PCB->WarnColor, PCB->PinSelectedColor,
                    PCB->ConnectedColor, PCB->FoundColor, PCB->PinColor);

  dapi->draw_pin ((PinType *)b, NULL, NULL);
  return 1;
}

static int
via_callback (const BoxType * b, void *cl)
{
  set_object_color ((AnyObjectType *)b,
                    PCB->WarnColor, PCB->ViaSelectedColor,
                    PCB->ConnectedColor, PCB->FoundColor, PCB->ViaColor);

  dapi->draw_via ((PinType *)b, NULL, NULL);
  return 1;
}

static int
pad_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  int *side = cl;

  if (ON_SIDE (pad, *side))
    {
      set_object_color ((AnyObjectType *)pad, PCB->WarnColor,
                        PCB->PinSelectedColor, PCB->ConnectedColor, PCB->FoundColor,
                        FRONT (pad) ? PCB->PinColor : PCB->InvisibleObjectsColor);

      dapi->draw_pad (pad, NULL, NULL);
    }
  return 1;
}

static void
draw_ppv (int group, const BoxType *drawn_area, void *userdata)
{
  int top_group = GetLayerGroupNumberBySide (TOP_SIDE);
  int bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);
  int side;
  bool is_gui = hid_draw_is_gui (Output.fgGC->hid_draw); /*!< Kludge */

  if (PCB->PinOn || !is_gui)
    {
      /* draw element pins */
      r_search (PCB->Data->pin_tree, drawn_area, NULL, pin_callback, NULL);

      /* draw element pads */
      if (group == top_group)
        {
          side = TOP_SIDE;
          r_search (PCB->Data->pad_tree, drawn_area, NULL, pad_callback, &side);
        }

      if (group == bottom_group)
        {
          side = BOTTOM_SIDE;
          r_search (PCB->Data->pad_tree, drawn_area, NULL, pad_callback, &side);
        }
    }

  /* draw vias */
  if (PCB->ViaOn)
    r_search (PCB->Data->via_tree, drawn_area, NULL, via_callback, NULL);

  dapi->draw_holes (-1, drawn_area, NULL);
}

static void
draw_holes (int plated, const BoxType *drawn_area, void *userdata)
{
  if (PCB->PinOn)
    r_search (PCB->Data->pin_tree, drawn_area, NULL, pin_hole_callback, &plated);
  if (PCB->ViaOn)
    r_search (PCB->Data->via_tree, drawn_area, NULL, via_hole_callback, &plated);
}

static void
draw_layer (LayerType *layer, const BoxType *drawn_area, void *userdata)
{
  int top_group = GetLayerGroupNumberBySide (TOP_SIDE);
  int bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);
  int layer_num = GetLayerNumber (PCB->Data, layer);
  int group = GetLayerGroupNumberByPointer (layer);
  struct poly_info info = {drawn_area, layer};

  /* print the non-clearing polys */
  r_search (layer->polygon_tree, drawn_area, NULL, poly_callback, &info);

  if (TEST_FLAG (CHECKPLANESFLAG, PCB))
    return;

  r_search (layer->line_tree, drawn_area, NULL, line_callback, layer);
  r_search (layer->arc_tree,  drawn_area, NULL, arc_callback,  layer);
  r_search (layer->text_tree, drawn_area, NULL, text_callback, layer);

  /* We should check hid_draw_is_gui() here, but it's kinda cool seeing the
     auto-outline magically disappear when you first add something to
     the "outline" layer.  */

  if (strcmp (layer->Name, "outline") == 0 ||
      strcmp (layer->Name, "route") == 0)
    {
      if (IsLayerEmpty (layer))
        {
          hid_draw_set_color (Output.fgGC, layer->Color);
          hid_draw_set_line_width (Output.fgGC, PCB->minWid);
          hid_draw_rect (Output.fgGC, 0, 0, PCB->MaxWidth, PCB->MaxHeight);
        }
      return;
    }

  /*! /note
   *  The following (drawing pins, pads and vias on each layer
   *  was NOT done by the old draw.c DrawLayer function.
   *
   *  We still have draw_ppv, called at the end of draw.c DrawLayerGroup
   *  for non-gui HIDs, so this is possibly not fully fixed up yet.
   *  (Possibly we duplicate some drawing).
   *
   *  Not clear if we should draw per layer, per layer-group. Since colouring
   *  is per layer - it is prettier per layer (but then, the layers in each
   *  group will overlay their colours if more than one layer per group.
   *
   *  Non-3D rendering HIDs probably don't actually care about in-layer pins
   *  and vias, since they can get away with drawing such items once, on the
   *  top-most layer. Pads come either at the top or bottom of the z-order,
   *  depending on which side of the board is being edited.
   */

  /* Don't draw vias on silk layers */
  if (layer_num >= max_copper_layer)
    return;

  r_search (PCB->Data->pin_tree, drawn_area, NULL, pin_inlayer_callback, layer);

  /* draw element pads */
  if (group == top_group)
    r_search (PCB->Data->pad_tree, drawn_area, NULL, pad_inlayer_callback, layer);

  if (group == bottom_group)
    r_search (PCB->Data->pad_tree, drawn_area, NULL, pad_inlayer_callback, layer);

  /* draw vias */
  r_search (PCB->Data->via_tree, drawn_area, NULL, via_inlayer_callback, layer);
  r_search (PCB->Data->pin_tree, drawn_area, NULL, pin_hole_callback, NULL);
  r_search (PCB->Data->via_tree, drawn_area, NULL, via_hole_callback, NULL);
}

struct draw_funcs d_f = {
  .draw_pin       = draw_pin,
  .draw_pin_mask  = draw_pin_mask,
  .draw_pin_hole  = draw_hole,
  .draw_via       = draw_via,
  .draw_via_mask  = draw_via_mask,
  .draw_via_hole  = draw_hole,
  .draw_pad       = draw_pad,
  .draw_pad_mask  = draw_pad_mask,
  .draw_pad_paste = draw_pad_paste,
  .draw_line      = draw_line,
  .draw_rat       = draw_rat,
  .draw_arc       = draw_arc,
  .draw_poly      = draw_poly,

  .draw_ppv       = draw_ppv,
  .draw_holes     = draw_holes,
  .draw_layer     = draw_layer,
};

struct draw_funcs *dapi = &d_f;
