/*!
 * \file src/misc.c
 *
 * \brief Misc functions used by several modules.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
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
 *
 * Contact addresses for paper mail and Email:
 *
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *
 * Thomas.Nau@rz.uni-ulm.de
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdarg.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <memory.h>
#include <ctype.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include "global.h"

#include "box.h"
#include "crosshair.h"
#include "create.h"
#include "data.h"
#include "draw.h"
#include "file.h"
#include "error.h"
#include "mymem.h"
#include "misc.h"
#include "move.h"
#include "pcb-printf.h"
#include "polygon.h"
#include "remove.h"
#include "rtree.h"
#include "rotate.h"
#include "rubberband.h"
#include "search.h"
#include "set.h"
#include "undo.h"
#include "action.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/*	forward declarations	*/
static char *BumpName (char *);
static void GetGridLockCoordinates (int, void *, void *, void *,
                                    Coord *, Coord *);


/* Local variables */

/*! 
 * \brief Used by SaveStackAndVisibility() and 
 * RestoreStackAndVisibility().
 */
static struct
{
  bool ElementOn, InvisibleObjectsOn, PinOn, ViaOn, RatOn;
  int LayerStack[MAX_ALL_LAYER];
  bool LayerOn[MAX_ALL_LAYER];
  int cnt;
} SavedStack;

/*!
 * \brief Distance() should be used so that there is only one place to
 * deal with overflow/precision errors.
 */
double
Distance (double x1, double y1, double x2, double y2)
{
  return hypot(x2 - x1, y2 - y1);
}

/*!
 * \brief Bring an angle into [0, 360] range.
 */
Angle
NormalizeAngle (Angle a)
{
  while (a < 0)
    a += 360.0;
  while (a >= 360.0)
    a -= 360.0;
  return a;
}

/*!
 * \brief GetValue() returns a numeric value passed from the string and
 * sets the bool variable absolute to false if it leads with a +/-
 * character.
 */
double
GetValue (const char *val, const char *units, bool * absolute)
{
  return GetValueEx(val, units, absolute, NULL, "cmil");
}

double
GetValueEx (const char *val, const char *units, bool * absolute, UnitList extra_units, const char *default_unit)
{
  double value;
  int n = -1;
  bool scaled = 0;
  bool dummy;

  /* Allow NULL to be passed for absolute */
  if(absolute == NULL)
    absolute = &dummy;

  /* if the first character is a sign we have to add the
   * value to the current one
   */
  if (*val == '=')
    {
      *absolute = true;
      if (sscanf (val+1, "%lf%n", &value, &n) < 1)
        return 0;
      n++;
    }
  else
    {
      if (isdigit ((int) *val))
        *absolute = true;
      else
        *absolute = false;
      if (sscanf (val, "%lf%n", &value, &n) < 1)
        return 0;
    }
  if (!units && n > 0)
    units = val + n;

  while (units && *units == ' ')
    units ++;
    
  if (units && *units)
    {
      int i;
      const Unit *unit = get_unit_struct (units);
      if (unit != NULL)
        {
          value  = unit_to_coord (unit, value);
          scaled = 1;
        }
      if (extra_units)
        {
          for (i = 0; *extra_units[i].suffix; ++i)
            {
              if (strncmp (units, extra_units[i].suffix, strlen(extra_units[i].suffix)) == 0)
                {
                  value *= extra_units[i].scale;
                  if (extra_units[i].flags & UNIT_PERCENT)
                    value /= 100.0;
                  scaled = 1;
                }
            }
        }
    }
  /* Apply default unit */
  if (!scaled && default_unit && *default_unit)
    {
      int i;
      const Unit *unit = get_unit_struct (default_unit);
      if (extra_units)
        for (i = 0; *extra_units[i].suffix; ++i)
          if (strcmp (extra_units[i].suffix, default_unit) == 0)
            {
              value *= extra_units[i].scale;
              if (extra_units[i].flags & UNIT_PERCENT)
                value /= 100.0;
              scaled = 1;
            }
      if (!scaled && unit != NULL)
        value = unit_to_coord (unit, value);
    }

  return value;
}

/*!
 * \brief Extract a unit-less value from a string.
 *
 * \param val       String containing the value to be read.
 *
 * \param absolute  Returns whether the returned value is an absolute one.
 *
 * \return The value read, with sign.
 *
 * This is the same as GetValue() and GetValueEx(), but totally ignoring units.
 * Typical application is a list selector, like the type of thermal to apply
 * to a pin.
 */
double GetUnitlessValue (const char *val, bool *absolute) {
  double value;

  if (*val == '=')
    {
      *absolute = true;
      val++;
    }
  else
    {
      if (isdigit ((int) *val))
        *absolute = true;
      else
        *absolute = false;
    }

  if (sscanf (val, "%lf", &value) < 1)
    return 0.;

  return value;
}

/*!
 * \brief Sets the bounding box of a point (which is silly).
 */
void
SetPointBoundingBox (PointType *Pnt)
{
  Pnt->X2 = Pnt->X + 1;
  Pnt->Y2 = Pnt->Y + 1;
}

/*!
 * \brief Sets the bounding box of a pin or via.
 */
void
SetPinBoundingBox (PinType *Pin)
{
  Coord width;

  /* the bounding box covers the extent of influence
   * so it must include the clearance values too
   */
  width = MAX (Pin->Clearance + PIN_SIZE (Pin), Pin->Mask) / 2;

  /* Adjust for our discrete polygon approximation */
  width = (double)width * POLY_CIRC_RADIUS_ADJ + 0.5;

  Pin->BoundingBox.X1 = Pin->X - width;
  Pin->BoundingBox.Y1 = Pin->Y - width;
  Pin->BoundingBox.X2 = Pin->X + width;
  Pin->BoundingBox.Y2 = Pin->Y + width;
  close_box(&Pin->BoundingBox);
}

/*!
 * \brief Sets the bounding box of a pad.
 */
void
SetPadBoundingBox (PadType *Pad)
{
  Coord width;
  Coord deltax;
  Coord deltay;

  /* the bounding box covers the extent of influence
   * so it must include the clearance values too
   */
  width = (Pad->Thickness + Pad->Clearance + 1) / 2;
  width = MAX (width, (Pad->Mask + 1) / 2);
  deltax = Pad->Point2.X - Pad->Point1.X;
  deltay = Pad->Point2.Y - Pad->Point1.Y;

  if (TEST_FLAG (SQUAREFLAG, Pad) && deltax != 0 && deltay != 0)
    {
      /* slanted square pad */
      double theta;
      Coord btx, bty;

      /* T is a vector half a thickness long, in the direction of
          one of the corners.  */
      theta = atan2 (deltay, deltax);
      btx = width * cos (theta + M_PI/4) * sqrt(2.0);
      bty = width * sin (theta + M_PI/4) * sqrt(2.0);


      Pad->BoundingBox.X1 = MIN (MIN (Pad->Point1.X - btx, Pad->Point1.X - bty),
                                 MIN (Pad->Point2.X + btx, Pad->Point2.X + bty));
      Pad->BoundingBox.X2 = MAX (MAX (Pad->Point1.X - btx, Pad->Point1.X - bty),
                                 MAX (Pad->Point2.X + btx, Pad->Point2.X + bty));
      Pad->BoundingBox.Y1 = MIN (MIN (Pad->Point1.Y + btx, Pad->Point1.Y - bty),
                                 MIN (Pad->Point2.Y - btx, Pad->Point2.Y + bty));
      Pad->BoundingBox.Y2 = MAX (MAX (Pad->Point1.Y + btx, Pad->Point1.Y - bty),
                                 MAX (Pad->Point2.Y - btx, Pad->Point2.Y + bty));
    }
  else
    {
      /* Adjust for our discrete polygon approximation */
      width = (double)width * POLY_CIRC_RADIUS_ADJ + 0.5;

      Pad->BoundingBox.X1 = MIN (Pad->Point1.X, Pad->Point2.X) - width;
      Pad->BoundingBox.X2 = MAX (Pad->Point1.X, Pad->Point2.X) + width;
      Pad->BoundingBox.Y1 = MIN (Pad->Point1.Y, Pad->Point2.Y) - width;
      Pad->BoundingBox.Y2 = MAX (Pad->Point1.Y, Pad->Point2.Y) + width;
    }
  close_box(&Pad->BoundingBox);
}

/*!
 * \brief Sets the bounding box of a line.
 */
void
SetLineBoundingBox (LineType *Line)
{
  Coord width = (Line->Thickness + Line->Clearance + 1) / 2;

  /* Adjust for our discrete polygon approximation */
  width = (double)width * POLY_CIRC_RADIUS_ADJ + 0.5;

  Line->BoundingBox.X1 = MIN (Line->Point1.X, Line->Point2.X) - width;
  Line->BoundingBox.X2 = MAX (Line->Point1.X, Line->Point2.X) + width;
  Line->BoundingBox.Y1 = MIN (Line->Point1.Y, Line->Point2.Y) - width;
  Line->BoundingBox.Y2 = MAX (Line->Point1.Y, Line->Point2.Y) + width;
  close_box(&Line->BoundingBox);
  SetPointBoundingBox (&Line->Point1);
  SetPointBoundingBox (&Line->Point2);
}

/*!
 * \brief Sets the bounding box of a polygons.
 */
void
SetPolygonBoundingBox (PolygonType *Polygon)
{
  Polygon->BoundingBox.X1 = Polygon->BoundingBox.Y1 = MAX_COORD;
  Polygon->BoundingBox.X2 = Polygon->BoundingBox.Y2 = 0;
  POLYGONPOINT_LOOP (Polygon);
  {
    MAKEMIN (Polygon->BoundingBox.X1, point->X);
    MAKEMIN (Polygon->BoundingBox.Y1, point->Y);
    MAKEMAX (Polygon->BoundingBox.X2, point->X);
    MAKEMAX (Polygon->BoundingBox.Y2, point->Y);
  }
  /* boxes don't include the lower right corner */
  close_box(&Polygon->BoundingBox);
  END_LOOP;
}

/*!
 * \brief Sets the bounding box of an element.
 */
void
SetElementBoundingBox (DataType *Data, ElementType *Element,
                       FontType *Font)
{
  BoxType *box, *vbox;

  if (Data && Data->element_tree)
    r_delete_entry (Data->element_tree, (BoxType *) Element);
  /* first update the text objects */
  ELEMENTTEXT_LOOP (Element);
  {
    if (Data && Data->name_tree[n])
      r_delete_entry (Data->name_tree[n], (BoxType *) text);
    SetTextBoundingBox (Font, text);
    if (Data && !Data->name_tree[n])
      Data->name_tree[n] = r_create_tree (NULL, 0, 0);
    if (Data)
      r_insert_entry (Data->name_tree[n], (BoxType *) text, 0);
  }
  END_LOOP;

  /* do not include the elementnames bounding box which
   * is handled separately
   */
  box = &Element->BoundingBox;
  vbox = &Element->VBox;
  box->X1 = box->Y1 = MAX_COORD;
  box->X2 = box->Y2 = 0;
  ELEMENTLINE_LOOP (Element);
  {
    SetLineBoundingBox (line);
    MAKEMIN (box->X1, line->Point1.X - (line->Thickness + 1) / 2);
    MAKEMIN (box->Y1, line->Point1.Y - (line->Thickness + 1) / 2);
    MAKEMIN (box->X1, line->Point2.X - (line->Thickness + 1) / 2);
    MAKEMIN (box->Y1, line->Point2.Y - (line->Thickness + 1) / 2);
    MAKEMAX (box->X2, line->Point1.X + (line->Thickness + 1) / 2);
    MAKEMAX (box->Y2, line->Point1.Y + (line->Thickness + 1) / 2);
    MAKEMAX (box->X2, line->Point2.X + (line->Thickness + 1) / 2);
    MAKEMAX (box->Y2, line->Point2.Y + (line->Thickness + 1) / 2);
  }
  END_LOOP;
  ARC_LOOP (Element);
  {
    SetArcBoundingBox (arc);
    MAKEMIN (box->X1, arc->BoundingBox.X1);
    MAKEMIN (box->Y1, arc->BoundingBox.Y1);
    MAKEMAX (box->X2, arc->BoundingBox.X2);
    MAKEMAX (box->Y2, arc->BoundingBox.Y2);
  }
  END_LOOP;
  *vbox = *box;
  PIN_LOOP (Element);
  {
    if (Data && Data->pin_tree)
      r_delete_entry (Data->pin_tree, (BoxType *) pin);
    SetPinBoundingBox (pin);
    if (Data)
      {
        if (!Data->pin_tree)
          Data->pin_tree = r_create_tree (NULL, 0, 0);
        r_insert_entry (Data->pin_tree, (BoxType *) pin, 0);
      }
    MAKEMIN (box->X1, pin->BoundingBox.X1);
    MAKEMIN (box->Y1, pin->BoundingBox.Y1);
    MAKEMAX (box->X2, pin->BoundingBox.X2);
    MAKEMAX (box->Y2, pin->BoundingBox.Y2);
    MAKEMIN (vbox->X1, pin->X - pin->Thickness / 2);
    MAKEMIN (vbox->Y1, pin->Y - pin->Thickness / 2);
    MAKEMAX (vbox->X2, pin->X + pin->Thickness / 2);
    MAKEMAX (vbox->Y2, pin->Y + pin->Thickness / 2);
  }
  END_LOOP;
  PAD_LOOP (Element);
  {
    if (Data && Data->pad_tree)
      r_delete_entry (Data->pad_tree, (BoxType *) pad);
    SetPadBoundingBox (pad);
    if (Data)
      {
        if (!Data->pad_tree)
          Data->pad_tree = r_create_tree (NULL, 0, 0);
        r_insert_entry (Data->pad_tree, (BoxType *) pad, 0);
      }
    MAKEMIN (box->X1, pad->BoundingBox.X1);
    MAKEMIN (box->Y1, pad->BoundingBox.Y1);
    MAKEMAX (box->X2, pad->BoundingBox.X2);
    MAKEMAX (box->Y2, pad->BoundingBox.Y2);
    MAKEMIN (vbox->X1,
             MIN (pad->Point1.X, pad->Point2.X) - pad->Thickness / 2);
    MAKEMIN (vbox->Y1,
             MIN (pad->Point1.Y, pad->Point2.Y) - pad->Thickness / 2);
    MAKEMAX (vbox->X2,
             MAX (pad->Point1.X, pad->Point2.X) + pad->Thickness / 2);
    MAKEMAX (vbox->Y2,
             MAX (pad->Point1.Y, pad->Point2.Y) + pad->Thickness / 2);
  }
  END_LOOP;
  /* now we set the EDGE2FLAG of the pad if Point2
   * is closer to the outside edge than Point1
   */
  PAD_LOOP (Element);
  {
    if (pad->Point1.Y == pad->Point2.Y)
      {
        /* horizontal pad */
        if (box->X2 - pad->Point2.X < pad->Point1.X - box->X1)
          SET_FLAG (EDGE2FLAG, pad);
        else
          CLEAR_FLAG (EDGE2FLAG, pad);
      }
    else
      {
        /* vertical pad */
        if (box->Y2 - pad->Point2.Y < pad->Point1.Y - box->Y1)
          SET_FLAG (EDGE2FLAG, pad);
        else
          CLEAR_FLAG (EDGE2FLAG, pad);
      }
  }
  END_LOOP;

  /* mark pins with component orientation */
  if ((box->X2 - box->X1) > (box->Y2 - box->Y1))
    {
      PIN_LOOP (Element);
      {
        SET_FLAG (EDGE2FLAG, pin);
      }
      END_LOOP;
    }
  else
    {
      PIN_LOOP (Element);
      {
        CLEAR_FLAG (EDGE2FLAG, pin);
      }
      END_LOOP;
    }
  close_box(box);
  close_box(vbox);
  if (Data && !Data->element_tree)
    Data->element_tree = r_create_tree (NULL, 0, 0);
  if (Data)
    r_insert_entry (Data->element_tree, box, 0);
}

/*!
 * \brief Creates the bounding box of a text object.
 */
void
SetTextBoundingBox (FontType *FontPtr, TextType *Text)
{
  SymbolType *symbol = FontPtr->Symbol;
  unsigned char *s = (unsigned char *) Text->TextString;
  int i;
  int space;

  Coord minx, miny, maxx, maxy, tx;
  Coord min_final_radius;
  Coord min_unscaled_radius;
  bool first_time = true;

  minx = miny = maxx = maxy = tx = 0;

  /* Calculate the bounding box based on the larger of the thicknesses
   * the text might clamped at on silk or copper layers.
   */
  min_final_radius = MAX (PCB->minWid, PCB->minSlk) / 2;

  /* Pre-adjust the line radius for the fact we are initially computing the
   * bounds of the un-scaled text, and the thickness clamping applies to
   * scaled text.
   */
  min_unscaled_radius = UNSCALE_TEXT (min_final_radius, Text->Scale);

  /* calculate size of the bounding box */
  for (; s && *s; s++)
    {
      if (*s <= MAX_FONTPOSITION && symbol[*s].Valid)
        {
          LineType *line = symbol[*s].Line;
          for (i = 0; i < symbol[*s].LineN; line++, i++)
            {
              /* Clamp the width of text lines at the minimum thickness.
               * NB: Divide 4 in thickness calculation is comprised of a factor
               *     of 1/2 to get a radius from the center-line, and a factor
               *     of 1/2 because some stupid reason we render our glyphs
               *     at half their defined stroke-width.
               */
               Coord unscaled_radius = MAX (min_unscaled_radius, line->Thickness / 4);

              if (first_time)
                {
                  minx = maxx = line->Point1.X;
                  miny = maxy = line->Point1.Y;
                  first_time = false;
                }

              minx = MIN (minx, line->Point1.X - unscaled_radius + tx);
              miny = MIN (miny, line->Point1.Y - unscaled_radius);
              minx = MIN (minx, line->Point2.X - unscaled_radius + tx);
              miny = MIN (miny, line->Point2.Y - unscaled_radius);
              maxx = MAX (maxx, line->Point1.X + unscaled_radius + tx);
              maxy = MAX (maxy, line->Point1.Y + unscaled_radius);
              maxx = MAX (maxx, line->Point2.X + unscaled_radius + tx);
              maxy = MAX (maxy, line->Point2.Y + unscaled_radius);
            }
          space = symbol[*s].Delta;
        }
      else
        {
          BoxType *ds = &FontPtr->DefaultSymbol;
          Coord w = ds->X2 - ds->X1;

          minx = MIN (minx, ds->X1 + tx);
          miny = MIN (miny, ds->Y1);
          minx = MIN (minx, ds->X2 + tx);
          miny = MIN (miny, ds->Y2);
          maxx = MAX (maxx, ds->X1 + tx);
          maxy = MAX (maxy, ds->Y1);
          maxx = MAX (maxx, ds->X2 + tx);
          maxy = MAX (maxy, ds->Y2);

          space = w / 5;
        }
      tx += symbol[*s].Width + space;
    }

  /* scale values */
  minx = SCALE_TEXT (minx, Text->Scale);
  miny = SCALE_TEXT (miny, Text->Scale);
  maxx = SCALE_TEXT (maxx, Text->Scale);
  maxy = SCALE_TEXT (maxy, Text->Scale);

  /* set upper-left and lower-right corner;
   * swap coordinates if necessary (origin is already in 'swapped')
   * and rotate box
   */

  if (TEST_FLAG (ONSOLDERFLAG, Text))
    {
      Text->BoundingBox.X1 = Text->X + minx;
      Text->BoundingBox.Y1 = Text->Y - miny;
      Text->BoundingBox.X2 = Text->X + maxx;
      Text->BoundingBox.Y2 = Text->Y - maxy;
      RotateBoxLowLevel (&Text->BoundingBox, Text->X, Text->Y,
                         (4 - Text->Direction) & 0x03);
    }
  else
    {
      Text->BoundingBox.X1 = Text->X + minx;
      Text->BoundingBox.Y1 = Text->Y + miny;
      Text->BoundingBox.X2 = Text->X + maxx;
      Text->BoundingBox.Y2 = Text->Y + maxy;
      RotateBoxLowLevel (&Text->BoundingBox,
                         Text->X, Text->Y, Text->Direction);
    }

  /* the bounding box covers the extent of influence
   * so it must include the clearance values too
   */
  Text->BoundingBox.X1 -= PCB->Bloat;
  Text->BoundingBox.Y1 -= PCB->Bloat;
  Text->BoundingBox.X2 += PCB->Bloat;
  Text->BoundingBox.Y2 += PCB->Bloat;
  close_box(&Text->BoundingBox);
}

/*!
 * \brief Returns true if data area is empty.
 */
bool
IsDataEmpty (DataType *Data)
{
  bool hasNoObjects;
  Cardinal i;

  hasNoObjects = (Data->ViaN == 0);
  hasNoObjects &= (Data->ElementN == 0);
  for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
    hasNoObjects = hasNoObjects &&
      Data->Layer[i].LineN == 0 &&
      Data->Layer[i].ArcN == 0 &&
      Data->Layer[i].TextN == 0 && Data->Layer[i].PolygonN == 0;
  return (hasNoObjects);
}

int
FlagIsDataEmpty (int parm)
{
  int i = IsDataEmpty (PCB->Data);
  return parm ? !i : i;
}

/* FLAG(DataEmpty,FlagIsDataEmpty,0) */
/* FLAG(DataNonEmpty,FlagIsDataEmpty,1) */

bool
IsLayerEmpty (LayerType *layer)
{
  return (layer->LineN == 0
	  && layer->TextN == 0
	  && layer->PolygonN == 0
	  && layer->ArcN == 0);
}

bool
IsLayerNumEmpty (int num)
{
  return IsLayerEmpty (PCB->Data->Layer+num);
}

bool
IsLayerGroupEmpty (int num)
{
  int i;
  for (i=0; i<PCB->LayerGroups.Number[num]; i++)
    if (!IsLayerNumEmpty (PCB->LayerGroups.Entries[num][i]))
      return false;
  return true;
}

bool
IsPasteEmpty (int side)
{
  bool paste_empty = true;
  ALLPAD_LOOP (PCB->Data);
  {
    if (ON_SIDE (pad, side) && !TEST_FLAG (NOPASTEFLAG, pad) && pad->Mask > 0)
      {
        paste_empty = false;
        break;
      }
  }
  ENDALL_LOOP;
  return paste_empty;
}


typedef struct
{
  int nplated;
  int nunplated;
  Cardinal group_from;
  Cardinal group_to;
} HoleCountStruct;

static int
hole_counting_callback (const BoxType * b, void *cl)
{
  PinType *pin = (PinType *) b;
  HoleCountStruct *hcs = (HoleCountStruct *) cl;

  if (hcs->group_from != 0
          || hcs->group_to != 0)
    {
      if (VIA_IS_BURIED (pin))
        {
          if (hcs->group_from == GetLayerGroupNumberByNumber (pin->BuriedFrom)
              && hcs->group_to == GetLayerGroupNumberByNumber (pin->BuriedTo))
	    goto count;
	}
      return 1;
    }

count:
  if (TEST_FLAG (HOLEFLAG, pin))
    hcs->nunplated++;
  else
    hcs->nplated++;
  return 1;
}

/*!
 * \brief Counts the number of plated and unplated holes in the design
 * within a given area of the board.
 *
 * To count for the whole board, pass NULL to the within_area.
 */
void
CountHoles (int *plated, int *unplated, const BoxType *within_area)
{
  HoleCountStruct hcs = {0, 0, 0, 0};

  r_search (PCB->Data->pin_tree, within_area, NULL, hole_counting_callback, &hcs);
  r_search (PCB->Data->via_tree, within_area, NULL, hole_counting_callback, &hcs);

  if (plated != NULL)   *plated   = hcs.nplated;
  if (unplated != NULL) *unplated = hcs.nunplated;
}

/*!
 * \brief Counts the number of plated and unplated holes in the design
 * within a given area of the board.
 *
 * To count for the whole board, pass NULL to the within_area.
 */
void
CountHolesEx (int *plated, int *unplated, const BoxType *within_area, Cardinal group_from, Cardinal group_to)
{
  HoleCountStruct hcs = {0, 0, group_from, group_to};

  r_search (PCB->Data->pin_tree, within_area, NULL, hole_counting_callback, &hcs);
  r_search (PCB->Data->via_tree, within_area, NULL, hole_counting_callback, &hcs);

  if (plated != NULL)   *plated   = hcs.nplated;
  if (unplated != NULL) *unplated = hcs.nunplated;
}

/*!
 * \brief Gets minimum and maximum coordinates.
 *
 * \return NULL if layout is empty.
 */
BoxType *
GetDataBoundingBox (DataType *Data)
{
  static BoxType box;
  /* FIX ME: use r_search to do this much faster */

  /* preset identifiers with highest and lowest possible values */
  box.X1 = box.Y1 = MAX_COORD;
  box.X2 = box.Y2 = -MAX_COORD;

  /* now scan for the lowest/highest X and Y coordinate */
  VIA_LOOP (Data);
  {
    box.X1 = MIN (box.X1, via->X - via->Thickness / 2);
    box.Y1 = MIN (box.Y1, via->Y - via->Thickness / 2);
    box.X2 = MAX (box.X2, via->X + via->Thickness / 2);
    box.Y2 = MAX (box.Y2, via->Y + via->Thickness / 2);
  }
  END_LOOP;
  ELEMENT_LOOP (Data);
  {
    box.X1 = MIN (box.X1, element->BoundingBox.X1);
    box.Y1 = MIN (box.Y1, element->BoundingBox.Y1);
    box.X2 = MAX (box.X2, element->BoundingBox.X2);
    box.Y2 = MAX (box.Y2, element->BoundingBox.Y2);
    {
      TextType *text = &NAMEONPCB_TEXT (element);
      box.X1 = MIN (box.X1, text->BoundingBox.X1);
      box.Y1 = MIN (box.Y1, text->BoundingBox.Y1);
      box.X2 = MAX (box.X2, text->BoundingBox.X2);
      box.Y2 = MAX (box.Y2, text->BoundingBox.Y2);
    };
  }
  END_LOOP;
  ALLLINE_LOOP (Data);
  {
    box.X1 = MIN (box.X1, line->Point1.X - line->Thickness / 2);
    box.Y1 = MIN (box.Y1, line->Point1.Y - line->Thickness / 2);
    box.X1 = MIN (box.X1, line->Point2.X - line->Thickness / 2);
    box.Y1 = MIN (box.Y1, line->Point2.Y - line->Thickness / 2);
    box.X2 = MAX (box.X2, line->Point1.X + line->Thickness / 2);
    box.Y2 = MAX (box.Y2, line->Point1.Y + line->Thickness / 2);
    box.X2 = MAX (box.X2, line->Point2.X + line->Thickness / 2);
    box.Y2 = MAX (box.Y2, line->Point2.Y + line->Thickness / 2);
  }
  ENDALL_LOOP;
  ALLARC_LOOP (Data);
  {
    box.X1 = MIN (box.X1, arc->BoundingBox.X1);
    box.Y1 = MIN (box.Y1, arc->BoundingBox.Y1);
    box.X2 = MAX (box.X2, arc->BoundingBox.X2);
    box.Y2 = MAX (box.Y2, arc->BoundingBox.Y2);
  }
  ENDALL_LOOP;
  ALLTEXT_LOOP (Data);
  {
    box.X1 = MIN (box.X1, text->BoundingBox.X1);
    box.Y1 = MIN (box.Y1, text->BoundingBox.Y1);
    box.X2 = MAX (box.X2, text->BoundingBox.X2);
    box.Y2 = MAX (box.Y2, text->BoundingBox.Y2);
  }
  ENDALL_LOOP;
  ALLPOLYGON_LOOP (Data);
  {
    box.X1 = MIN (box.X1, polygon->BoundingBox.X1);
    box.Y1 = MIN (box.Y1, polygon->BoundingBox.Y1);
    box.X2 = MAX (box.X2, polygon->BoundingBox.X2);
    box.Y2 = MAX (box.Y2, polygon->BoundingBox.Y2);
  }
  ENDALL_LOOP;
  return (IsDataEmpty (Data) ? NULL : &box);
}

/*!
 * \brief Centers the displayed PCB around the specified point (X,Y),
 * and move the crosshair there.
 *
 * If warp_pointer is true warp the pointer to the crosshair.
 */
void
CenterDisplay (Coord X, Coord Y, bool warp_pointer)
{
  Coord save_grid = PCB->Grid;

  PCB->Grid = 1;

  if (MoveCrosshairAbsolute (X, Y))
    notify_crosshair_change (true);

  if (warp_pointer)
    gui->set_crosshair (Crosshair.X, Crosshair.Y,
                        HID_SC_CENTER_IN_VIEWPORT_AND_WARP_POINTER );
  else
    gui->set_crosshair (Crosshair.X, Crosshair.Y, HID_SC_CENTER_IN_VIEWPORT);

  PCB->Grid = save_grid;
}

/*!
 * \brief Transforms symbol coordinates so that the left edge of each
 * symbol is at the zero position.
 *
 * The y coordinates are moved so that min(y) = 0.
 * 
 */
void
SetFontInfo (FontType *Ptr)
{
  Cardinal i, j;
  SymbolType *symbol;
  LineType *line;
  Coord totalminy = MAX_COORD;

  /* calculate cell with and height (is at least DEFAULT_CELLSIZE)
   * maximum cell width and height
   * minimum x and y position of all lines
   */
  Ptr->MaxWidth = DEFAULT_CELLSIZE;
  Ptr->MaxHeight = DEFAULT_CELLSIZE;
  for (i = 0, symbol = Ptr->Symbol; i <= MAX_FONTPOSITION; i++, symbol++)
    {
      Coord minx, miny, maxx, maxy;

      /* next one if the index isn't used or symbol is empty (SPACE) */
      if (!symbol->Valid || !symbol->LineN)
        continue;

      minx = miny = MAX_COORD;
      maxx = maxy = 0;
      for (line = symbol->Line, j = symbol->LineN; j; j--, line++)
        {
          minx = MIN (minx, line->Point1.X);
          miny = MIN (miny, line->Point1.Y);
          minx = MIN (minx, line->Point2.X);
          miny = MIN (miny, line->Point2.Y);
          maxx = MAX (maxx, line->Point1.X);
          maxy = MAX (maxy, line->Point1.Y);
          maxx = MAX (maxx, line->Point2.X);
          maxy = MAX (maxy, line->Point2.Y);
        }

      /* move symbol to left edge */
      for (line = symbol->Line, j = symbol->LineN; j; j--, line++)
        MOVE_LINE_LOWLEVEL (line, -minx, 0);

      /* set symbol bounding box with a minimum cell size of (1,1) */
      symbol->Width = maxx - minx + 1;
      symbol->Height = maxy + 1;

      /* check total min/max  */
      Ptr->MaxWidth = MAX (Ptr->MaxWidth, symbol->Width);
      Ptr->MaxHeight = MAX (Ptr->MaxHeight, symbol->Height);
      totalminy = MIN (totalminy, miny);
    }

  /* move coordinate system to the upper edge (lowest y on screen) */
  for (i = 0, symbol = Ptr->Symbol; i <= MAX_FONTPOSITION; i++, symbol++)
    if (symbol->Valid)
      {
        symbol->Height -= totalminy;
        for (line = symbol->Line, j = symbol->LineN; j; j--, line++)
          MOVE_LINE_LOWLEVEL (line, 0, -totalminy);
      }

  /* setup the box for the default symbol */
  Ptr->DefaultSymbol.X1 = Ptr->DefaultSymbol.Y1 = 0;
  Ptr->DefaultSymbol.X2 = Ptr->DefaultSymbol.X1 + Ptr->MaxWidth;
  Ptr->DefaultSymbol.Y2 = Ptr->DefaultSymbol.Y1 + Ptr->MaxHeight;
}

static Coord
GetNum (char **s, const char *default_unit)
{
  /* Read value */
  Coord ret_val = GetValueEx (*s, NULL, NULL, NULL, default_unit);
  /* Advance pointer */
  while(isalnum(**s) || **s == '.')
     (*s)++;
  return ret_val;
}

/*!
 * \brief Serializes the route style list .
 *
 *  Right now n_styles should always be set to NUM_STYLES,
 *  since that is the number of route styles ParseRouteString()
 *  expects to parse.
 */
char *
make_route_string (RouteStyleType rs[], int n_styles)
{
  GString *str = g_string_new ("");
  gint i;

  for (i = 0; i < n_styles; ++i)
    {
      char *r_string;
      // don't much like how this is done, but it'll have to do for now
      if (rs[i].ViaMask != 0)
        r_string = pcb_g_strdup_printf ("%s,%$mr,%$mr,%$mr,%$mr,%$mr",
                                            rs[i].Name,
                                            rs[i].Thick, rs[i].Diameter,
                                            rs[i].Hole, rs[i].Keepaway,
                                            rs[i].ViaMask);
      else
        r_string = pcb_g_strdup_printf ("%s,%$mr,%$mr,%$mr,%$mr", rs[i].Name,
                                        rs[i].Thick, rs[i].Diameter,
                                        rs[i].Hole, rs[i].Keepaway);
      if (i > 0)
        g_string_append_c (str, ':');
      g_string_append (str, r_string);
      g_free (r_string);
    }
  return g_string_free (str, FALSE);
}

/*!
 * \brief Parses the routes definition string
 * 
 * Which is a colon separated list of comma separated Name, Dimension,
 * Dimension, Dimension[, Dimension, Dimension].
 *
 * \example Signal,20,40,20,10,0:Power,40,60,28,10,0:...
 */
int
ParseRouteString (char *s, RouteStyleType *routeStyle, const char *default_unit)
{
  int i, n, style;
  char Name[256];
  char *orig_s = s;
  
  Coord *style_items[5];

  memset (routeStyle, 0, NUM_STYLES * sizeof (RouteStyleType));
  style = 0;
  while (style < NUM_STYLES)
    {
      // Extract the name
      while (*s && isspace ((int) *s))  s++;
      // this will break for style names greater than 256 characters
      for (i = 0; *s && *s != ','; i++)  Name[i] = *s++;
      Name[i] = '\0';
      routeStyle->Name = strdup (Name);
      
      // setup some default values
      routeStyle->Keepaway = MIL_TO_COORD(10);
      routeStyle->ViaMask = 0;

      // Now extract the numerical parameters
      style_items[0] = &(routeStyle->Thick);
      style_items[1] = &(routeStyle->Diameter);
      style_items[2] = &(routeStyle->Hole);
      style_items[3] = &(routeStyle->Keepaway);
      style_items[4] = &(routeStyle->ViaMask);
      for (n = 0; n < 5; n++)
        {
        if (!isdigit ((int) *++s))  goto error;
        *style_items[n] = GetNum(&s, default_unit);
        // find the next field, skipping any white space
        while (*s && isspace ((int) *s))  s++;
        if (*s != ',')  break;
        while (*s && isspace ((int) *s))  s++;
        }

      // how did we get out of the loop?
      // if we're at the end of the string, we're done
      if (*s == '\0')  break;
      // there's another route style to parse, advance past the delimiter
      else if (*s == ':')  s++;
      // otherwise, we got an unexpected character, which is an error
      else {
        fprintf(stderr, "unexpected character: %c\n", *s);
        goto error;
      }
      
      // otherwise, prepare for the next loop
      style++; routeStyle++;
    }
  return (0);

error:
  fprintf(stderr, "error parsing route string: %s\n", orig_s);
  fprintf(stderr, "parsed %ld characters.\n", s - orig_s);
  fprintf(stderr, "on style number %d\n", style+1);
  fprintf(stderr, "character that caused the error: %c\n", *s);
  fprintf(stderr, "values of current struct: \n");
  for (n=0; n < 5; n++) fprintf(stderr, "%d: %ld ", n, *style_items[n]);
  memset (routeStyle, 0, NUM_STYLES * sizeof (RouteStyleType));
  return (1);
}

/*!
 * \brief Parses the group definition string.
 *
 * Which is a colon separated list of comma separated layer numbers.
 *
 * \example (1,2,b:4,6,8,t)
 */
int
ParseGroupString (char *group_string, LayerGroupType *LayerGroup, int *LayerN)
{
  char *s;
  int group, member, layer;
  bool c_set = false,        /* flags for the two special layers to */
    s_set = false;              /* provide a default setting for old formats */
  int groupnum[MAX_ALL_LAYER];

  *LayerN = 0;

  /* Deterimine the maximum layer number */
  for (s = group_string; s && *s; s++)
    {
      while (*s && isspace ((int) *s))
        s++;

      switch (*s)
        {
        case 'c':
        case 'C':
        case 't':
        case 'T':
        case 's':
        case 'S':
        case 'b':
        case 'B':
          break;

        default:
          if (!isdigit ((int) *s))
            goto error;
          *LayerN = MAX (*LayerN, atoi (s));
          break;
        }

      while (*++s && isdigit ((int) *s));

      /* ignore white spaces and check for separator */
      while (*s && isspace ((int) *s))
        s++;

      if (*s == '\0')
        break;

      if (*s != ':' && *s != ',')
        goto error;
    }

  /* clear struct */
  memset (LayerGroup, 0, sizeof (LayerGroupType));

  /* Clear assignments */
  for (layer = 0; layer < MAX_ALL_LAYER; layer++)
    groupnum[layer] = -1;

  /* loop over all groups */
  for (s = group_string, group = 0;
       s && *s && group < *LayerN;
       group++)
    {
      while (*s && isspace ((int) *s))
        s++;

      /* loop over all group members */
      for (member = 0; *s; s++)
        {
          /* ignore white spaces and get layernumber */
          while (*s && isspace ((int) *s))
            s++;
          switch (*s)
            {
            case 'c':
            case 'C':
            case 't':
            case 'T':
              layer = *LayerN + TOP_SILK_LAYER;
              c_set = true;
              break;

            case 's':
            case 'S':
            case 'b':
            case 'B':
              layer = *LayerN + BOTTOM_SILK_LAYER;
              s_set = true;
              break;

            default:
              layer = atoi (s) - 1;
              break;
            }
          if (layer > *LayerN + MAX (BOTTOM_SILK_LAYER, TOP_SILK_LAYER) ||
              member >= *LayerN + 1)
            goto error;
          groupnum[layer] = group;
          LayerGroup->Entries[group][member++] = layer;
          while (*++s && isdigit ((int) *s));

          /* ignore white spaces and check for separator */
          while (*s && isspace ((int) *s))
            s++;
          if (!*s || *s == ':')
            break;
        }
      LayerGroup->Number[group] = member;
      if (*s == ':')
        s++;
    }

  /* If no explicit solder or component layer group was found in the layer
   * group string, make group 0 the bottom side, and group 1 the top side.
   * This is done by assigning the relevant silkscreen layers to those groups.
   */
  if (!s_set)
    LayerGroup->Entries[0][LayerGroup->Number[0]++] = *LayerN + BOTTOM_SILK_LAYER;
  if (!c_set)
    LayerGroup->Entries[1][LayerGroup->Number[1]++] = *LayerN + TOP_SILK_LAYER;

  /* Assign a unique layer group to each layer that was not explicitly
   * assigned a particular group by its presence in the layer group string.
   */
  for (layer = 0; layer < *LayerN && group < *LayerN; layer++)
    if (groupnum[layer] == -1)
      {
        LayerGroup->Entries[group][0] = layer;
        LayerGroup->Number[group] = 1;
        group++;
      }
  return (0);

  /* reset structure on error */
error:
  memset (LayerGroup, 0, sizeof (LayerGroupType));
  return (1);
}

/*!
 * \brief Sets up any remaining layer type guesses.
 */
void
AssignDefaultLayerTypes()
{
  int num_found;
  Cardinal outline_layer = -1;

  /*
   * There can be only one outline layer. During parsing guess_layertype()
   * applied well known cases already, but as this function operates on a
   * single layer only, it might end up with more than one hit for the whole
   * file. Especially after loading an older layout without saved flags.
   */
  num_found = 0;
  LAYER_TYPE_LOOP (PCB->Data, max_copper_layer, LT_OUTLINE)
    outline_layer = n;
    num_found++;
  END_LOOP;

  if (num_found != 1)
    /* No or duplicate outline! Try to find a layer which is named exactly
       "outline". */
    LAYER_TYPE_LOOP (PCB->Data, max_copper_layer, LT_OUTLINE)
      if ( ! strcasecmp (layer->Name, "outline"))
        {
          outline_layer = n;
          num_found = 1;
          break;
        }
    END_LOOP;

  if (num_found != 1)
    /* Next, try to find a layer which is named exactly "route". */
    LAYER_TYPE_LOOP (PCB->Data, max_copper_layer, LT_OUTLINE)
      if ( ! strcasecmp (layer->Name, "route"))
        {
          outline_layer = n;
          num_found = 1;
          break;
        }
    END_LOOP;

  if (num_found != 1)
    /* As last resort, take the first layer claiming to be outline. */
    LAYER_TYPE_LOOP (PCB->Data, max_copper_layer, LT_OUTLINE)
      outline_layer = n;
      num_found = 1;
      break;
    END_LOOP;

  /* Make sure our found outline layer is the only one. */
  LAYER_TYPE_LOOP (PCB->Data, max_copper_layer, LT_OUTLINE)
    if (n == outline_layer)
      layer->Type = LT_OUTLINE;
    else
      layer->Type = LT_ROUTE;  /* best guess */
  END_LOOP;
}

extern void pcb_main_uninit(void);

/*!
 * \brief Quits application.
 */
void
QuitApplication (void)
{
  /*
   * save data if necessary.  It not needed, then don't trigger EmergencySave
   * via our atexit() registering of EmergencySave().  We presumeably wanted to
   * exit here and thus it is not an emergency.
   */
  if (PCB->Changed && Settings.SaveInTMP)
    EmergencySave ();
  else
    DisableEmergencySave ();

  if (gui->do_exit == NULL)
    {
      pcb_main_uninit ();
      exit (0);
    }
  else
    gui->do_exit (gui);
}

/*!
 * \brief Creates a filename from a template.
 *
 * "%f" is replaced by the filename.
 *
 * "%p" is replaced by the searchpath.
 */
char *
EvaluateFilename (char *Template, char *Path, char *Filename, char *Parameter)
{
  static DynamicStringType command;
  char *p;

  if (Settings.verbose)
    {
      printf ("EvaluateFilename:\n");
      printf ("\tTemplate: \033[33m%s\033[0m\n", Template);
      printf ("\tPath: \033[33m%s\033[0m\n", Path);
      printf ("\tFilename: \033[33m%s\033[0m\n", Filename);
      printf ("\tParameter: \033[33m%s\033[0m\n", Parameter);
    }

  DSClearString (&command);

  for (p = Template; p && *p; p++)
    {
      /* copy character or add string to command */
      if (*p == '%'
          && (*(p + 1) == 'f' || *(p + 1) == 'p' || *(p + 1) == 'a'))
        switch (*(++p))
          {
          case 'a':
            DSAddString (&command, Parameter);
            break;
          case 'f':
            DSAddString (&command, Filename);
            break;
          case 'p':
            DSAddString (&command, Path);
            break;
          }
      else
        DSAddCharacter (&command, *p);
    }
  DSAddCharacter (&command, '\0');
  if (Settings.verbose)
    printf ("EvaluateFilename: \033[32m%s\033[0m\n", command.Data);

  return strdup (command.Data);
}

/*!
 * \brief Concatenates directory and filename.
 *
 * If directory != NULL expands them with a shell and returns the found
 * name(s) or NULL.
 */
char *
ExpandFilename (char *Dirname, char *Filename)
{
  static DynamicStringType answer;
  char *command;
  FILE *pipe;
  int c;

  /* allocate memory for commandline and build it */
  DSClearString (&answer);
  if (Dirname)
    {
      command = (char *)calloc (strlen (Filename) + strlen (Dirname) + 7,
                        sizeof (char));
      sprintf (command, "echo %s/%s", Dirname, Filename);
    }
  else
    {
      command = (char *)calloc (strlen (Filename) + 6, sizeof (char));
      sprintf (command, "echo %s", Filename);
    }

  /* execute it with shell */
  if ((pipe = popen (command, "r")) != NULL)
    {
      /* discard all but the first returned line */
      for (;;)
        {
          if ((c = fgetc (pipe)) == EOF || c == '\n' || c == '\r')
            break;
          else
            DSAddCharacter (&answer, c);
        }

      free (command);
      return (pclose (pipe) ? NULL : answer.Data);
    }

  /* couldn't be expanded by the shell */
  PopenErrorMessage (command);
  free (command);
  return (NULL);
}


/*!
 * \brief Returns the layer number for the passed pointer.
 */
int
GetLayerNumber (DataType *Data, LayerType *Layer)
{
  int i;

  for (i = 0; i < MAX_ALL_LAYER; i++)
    if (Layer == &Data->Layer[i])
      break;
  return (i);
}

/*!
 * \brief Move layer (number is passed in) to top of layerstack.
 */
static void
PushOnTopOfLayerStack (int NewTop)
{
  int i;

  /* ignore silk and other extra layers */
  if (NewTop < max_copper_layer)
    {
      /* first find position of passed one */
      for (i = 0; i < max_copper_layer; i++)
        if (LayerStack[i] == NewTop)
          break;

      /* bring this element to the top of the stack */
      for (; i; i--)
        LayerStack[i] = LayerStack[i - 1];
      LayerStack[0] = NewTop;
    }
}


/*!
 * \brief Changes the visibility of all layers in a group.
 *
 * \return The number of changed layers.
 */
int
ChangeGroupVisibility (int Layer, bool On, bool ChangeStackOrder)
{
  int group, i, changed = 1;    /* at least the current layer changes */

  /* Warning: these special case values must agree with what gui-top-window.c
     |  thinks the are.
   */

  if (Settings.verbose)
    printf ("ChangeGroupVisibility(Layer=%d, On=%d, ChangeStackOrder=%d)\n",
            Layer, On, ChangeStackOrder);

  /* decrement 'i' to keep stack in order of layergroup */
  group = GetLayerGroupNumberByNumber (Layer);
  for (i = PCB->LayerGroups.Number[group]; i;)
    {
      int layer = PCB->LayerGroups.Entries[group][--i];

      /* don't count the passed member of the group */
      if (layer != Layer && layer < max_copper_layer)
        {
          PCB->Data->Layer[layer].On = On;

          /* push layer on top of stack if switched on */
          if (On && ChangeStackOrder)
            PushOnTopOfLayerStack (layer);
          changed++;
        }
    }

  /* change at least the passed layer */
  PCB->Data->Layer[Layer].On = On;
  if (On && ChangeStackOrder)
    PushOnTopOfLayerStack (Layer);

  /* update control panel and exit */
  hid_action ("LayersChanged");
  return (changed);
}

/*!
 * \brief Given a string description of a layer stack, adjust the layer
 * stack to correspond.
 */
void
LayerStringToLayerStack (char *s)
{
  static int listed_layers = 0;
  int l = strlen (s);
  char **args;
  int i, argn, lno;
  int prev_sep = 1;

  s = strdup (s);
  args = (char **) malloc (l * sizeof (char *));
  argn = 0;

  for (i=0; i<l; i++)
    {
      switch (s[i])
	{
	case ' ':
	case '\t':
	case ',':
	case ';':
	case ':':
	  prev_sep = 1;
	  s[i] = '\0';
	  break;
	default:
	  if (prev_sep)
	    args[argn++] = s+i;
	  prev_sep = 0;
	  break;
	}
    }

  for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
    {
      if (i < max_copper_layer)
        LayerStack[i] = i;
      PCB->Data->Layer[i].On = false;
    }
  PCB->ElementOn = false;
  PCB->InvisibleObjectsOn = false;
  PCB->PinOn = false;
  PCB->ViaOn = false;
  PCB->RatOn = false;
  CLEAR_FLAG (SHOWMASKFLAG, PCB);
  Settings.ShowBottomSide = 0;

  for (i=argn-1; i>=0; i--)
    {
      if (strcasecmp (args[i], "rats") == 0)
	PCB->RatOn = true;
      else if (strcasecmp (args[i], "invisible") == 0)
	PCB->InvisibleObjectsOn = true;
      else if (strcasecmp (args[i], "pins") == 0)
	PCB->PinOn = true;
      else if (strcasecmp (args[i], "vias") == 0)
	PCB->ViaOn = true;
      else if (strcasecmp (args[i], "elements") == 0
	       || strcasecmp (args[i], "silk") == 0)
	PCB->ElementOn = true;
      else if (strcasecmp (args[i], "mask") == 0)
	SET_FLAG (SHOWMASKFLAG, PCB);
      else if (strcasecmp (args[i], "solderside") == 0)
	Settings.ShowBottomSide = 1;
      else if (isdigit ((int) args[i][0]))
	{
	  lno = atoi (args[i]);
	  ChangeGroupVisibility (lno, true, true);
	}
      else
	{
	  int found = 0;
	  for (lno = 0; lno < max_copper_layer; lno++)
	    if (strcasecmp (args[i], PCB->Data->Layer[lno].Name) == 0)
	      {
		ChangeGroupVisibility (lno, true, true);
		found = 1;
		break;
	      }
	  if (!found)
	    {
	      fprintf(stderr, _("Warning: layer \"%s\" not known\n"), args[i]);
	      if (!listed_layers)
		{
		  fprintf (stderr, _("Named layers in this board are:\n"));
		  listed_layers = 1;
		  for (lno=0; lno < max_copper_layer; lno ++)
		    fprintf(stderr, "\t%s\n", PCB->Data->Layer[lno].Name);
		  fprintf(stderr, _("Also: component, solder, rats, invisible, "
			"pins, vias, elements or silk, mask, solderside.\n"));
		}
	    }
	}
    }
}

/*!
 * \brief Returns the layergroup number for the passed pointer.
 */
int
GetLayerGroupNumberByPointer (LayerType *Layer)
{
  return (GetLayerGroupNumberByNumber (GetLayerNumber (PCB->Data, Layer)));
}

/*!
 * \brief Returns the layergroup number for the passed layer number.
 */
int
GetLayerGroupNumberByNumber (Cardinal Layer)
{
  int group, entry;

  for (group = 0; group < max_group; group++)
    for (entry = 0; entry < PCB->LayerGroups.Number[group]; entry++)
      if (PCB->LayerGroups.Entries[group][entry] == Layer)
        return (group);

  /* since every layer belongs to a group it is safe to return
   * the value without boundary checking
   */
  return (group);
}

/*!
 * \brief Returns the layergroup number for the passed side (TOP_SIDE or
 * BOTTOM_SIDE).
 */
int
GetLayerGroupNumberBySide (int side)
{
  /* Find the relavant board side layer group by determining the
   * layer group associated with the relevant side's silk-screen
   */
  return GetLayerGroupNumberByNumber (
      side == TOP_SIDE ? top_silk_layer : bottom_silk_layer);
}

/*!
 * \brief Returns a pointer to an objects bounding box.
 *
 * Data is valid until the routine is called again.
 */
BoxType *
GetObjectBoundingBox (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  switch (Type)
    {
    case LINE_TYPE:
    case ARC_TYPE:
    case TEXT_TYPE:
    case POLYGON_TYPE:
    case PAD_TYPE:
    case PIN_TYPE:
    case ELEMENTNAME_TYPE:
      return (BoxType *)Ptr2;
    case VIA_TYPE:
    case ELEMENT_TYPE:
      return (BoxType *)Ptr1;
    case POLYGONPOINT_TYPE:
    case LINEPOINT_TYPE:
      return (BoxType *)Ptr3;
    default:
      Message (_("Request for bounding box of unsupported type %d\n"), Type);
      return (BoxType *)Ptr2;
    }
}

/*!
 * \brief Computes the bounding box of an arc.
 */
void
SetArcBoundingBox (ArcType *Arc)
{
  double ca1, ca2, sa1, sa2;
  double minx, maxx, miny, maxy;
  Angle ang1, ang2;
  Coord width;

  /* first put angles into standard form:
   *  ang1 < ang2, both angles between 0 and 720 */
  Arc->Delta = CLAMP (Arc->Delta, -360, 360);

  if (Arc->Delta > 0)
    {
      ang1 = NormalizeAngle (Arc->StartAngle);
      ang2 = NormalizeAngle (Arc->StartAngle + Arc->Delta);
    }
  else
    {
      ang1 = NormalizeAngle (Arc->StartAngle + Arc->Delta);
      ang2 = NormalizeAngle (Arc->StartAngle);
    }
  if (ang1 > ang2)
    ang2 += 360;
  /* Make sure full circles aren't treated as zero-length arcs */
  if (Arc->Delta == 360 || Arc->Delta == -360)
    ang2 = ang1 + 360;

  /* calculate sines, cosines */
  sa1 = sin (M180 * ang1);
  ca1 = cos (M180 * ang1);
  sa2 = sin (M180 * ang2);
  ca2 = cos (M180 * ang2);

  minx = MIN (ca1, ca2);
  maxx = MAX (ca1, ca2);
  miny = MIN (sa1, sa2);
  maxy = MAX (sa1, sa2);

  /* Check for extreme angles */
  if ((ang1 <= 0   && ang2 >= 0)   || (ang1 <= 360 && ang2 >= 360)) maxx = 1;
  if ((ang1 <= 90  && ang2 >= 90)  || (ang1 <= 450 && ang2 >= 450)) maxy = 1;
  if ((ang1 <= 180 && ang2 >= 180) || (ang1 <= 540 && ang2 >= 540)) minx = -1;
  if ((ang1 <= 270 && ang2 >= 270) || (ang1 <= 630 && ang2 >= 630)) miny = -1;

  /* Finally, calcate bounds, converting sane geometry into pcb geometry */
  Arc->BoundingBox.X1 = Arc->X - Arc->Width * maxx;
  Arc->BoundingBox.X2 = Arc->X - Arc->Width * minx;
  Arc->BoundingBox.Y1 = Arc->Y + Arc->Height * miny;
  Arc->BoundingBox.Y2 = Arc->Y + Arc->Height * maxy;

  width = (Arc->Thickness + Arc->Clearance) / 2;

  /* Adjust for our discrete polygon approximation */
  width = (double)width * MAX (POLY_CIRC_RADIUS_ADJ, (1.0 + POLY_ARC_MAX_DEVIATION)) + 0.5;

  Arc->BoundingBox.X1 -= width;
  Arc->BoundingBox.X2 += width;
  Arc->BoundingBox.Y1 -= width;
  Arc->BoundingBox.Y2 += width;
  close_box(&Arc->BoundingBox);

  /* Update the arc end-points */
  Arc->Point1.X = Arc->X - (double)Arc->Width  * ca1;
  Arc->Point1.Y = Arc->Y + (double)Arc->Height * sa1;
  Arc->Point2.X = Arc->X - (double)Arc->Width  * ca2;
  Arc->Point2.Y = Arc->Y + (double)Arc->Height * sa2;
}

/*!
 * \brief Resets the layerstack setting.
 */
void
ResetStackAndVisibility (void)
{
  int top_group;
  Cardinal i;

  for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
    {
      if (i < max_copper_layer)
        LayerStack[i] = i;
      PCB->Data->Layer[i].On = true;
    }
  PCB->ElementOn = true;
  PCB->InvisibleObjectsOn = true;
  PCB->PinOn = true;
  PCB->ViaOn = true;
  PCB->RatOn = true;

  /* Bring the component group to the front and make it active.  */
  top_group = GetLayerGroupNumberBySide (TOP_SIDE);
  ChangeGroupVisibility (PCB->LayerGroups.Entries[top_group][0], 1, 1);
}

/*!
 * \brief Saves the layerstack setting.
 */
void
SaveStackAndVisibility (void)
{
  Cardinal i;
  static bool run = false;

  if (run == false)
    {
      SavedStack.cnt = 0;
      run = true;
    }

  if (SavedStack.cnt != 0)
    {
      fprintf (stderr,
               "SaveStackAndVisibility()  layerstack was already saved and not"
               "yet restored.  cnt = %d\n", SavedStack.cnt);
    }

  for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
    {
      if (i < max_copper_layer)
        SavedStack.LayerStack[i] = LayerStack[i];
      SavedStack.LayerOn[i] = PCB->Data->Layer[i].On;
    }
  SavedStack.ElementOn = PCB->ElementOn;
  SavedStack.InvisibleObjectsOn = PCB->InvisibleObjectsOn;
  SavedStack.PinOn = PCB->PinOn;
  SavedStack.ViaOn = PCB->ViaOn;
  SavedStack.RatOn = PCB->RatOn;
  SavedStack.cnt++;
}

/*!
 * \brief Restores the layerstack setting.
 */
void
RestoreStackAndVisibility (void)
{
  Cardinal i;

  if (SavedStack.cnt == 0)
    {
      fprintf (stderr, "RestoreStackAndVisibility()  layerstack has not"
               " been saved.  cnt = %d\n", SavedStack.cnt);
      return;
    }
  else if (SavedStack.cnt != 1)
    {
      fprintf (stderr, "RestoreStackAndVisibility()  layerstack save count is"
               " wrong.  cnt = %d\n", SavedStack.cnt);
    }

  for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
    {
      if (i < max_copper_layer)
        LayerStack[i] = SavedStack.LayerStack[i];
      PCB->Data->Layer[i].On = SavedStack.LayerOn[i];
    }
  PCB->ElementOn = SavedStack.ElementOn;
  PCB->InvisibleObjectsOn = SavedStack.InvisibleObjectsOn;
  PCB->PinOn = SavedStack.PinOn;
  PCB->ViaOn = SavedStack.ViaOn;
  PCB->RatOn = SavedStack.RatOn;

  SavedStack.cnt--;
}

/*!
 * \brief Returns pointer to current working directory.
 *
 * If 'path' is not NULL, then the current working directory is copied
 * to the array pointed to by 'path'.
 */
char *
GetWorkingDirectory (char *path)
{
#ifdef HAVE_GETCWD
  return getcwd (path, MAXPATHLEN);
#else
  /* seems that some BSD releases lack of a prototype for getwd() */
  return getwd (path);
#endif

}

/*!
 * \brief Write a string to the passed file pointer.
 *
 * Some special characters are quoted.
 */
void
CreateQuotedString (DynamicStringType *DS, char *S)
{
  DSClearString (DS);
  DSAddCharacter (DS, '"');
  while (*S)
    {
      if (*S == '"' || *S == '\\')
        DSAddCharacter (DS, '\\');
      DSAddCharacter (DS, *S++);
    }
  DSAddCharacter (DS, '"');
}

BoxType *
GetArcEnds (ArcType *Arc)
{
  static BoxType box;
  box.X1 = Arc->X - Arc->Width * cos (Arc->StartAngle * M180);
  box.Y1 = Arc->Y + Arc->Height * sin (Arc->StartAngle * M180);
  box.X2 = Arc->X - Arc->Width * cos ((Arc->StartAngle + Arc->Delta) * M180);
  box.Y2 = Arc->Y + Arc->Height * sin ((Arc->StartAngle + Arc->Delta) * M180);
  return &box;
}


/*!
 * \todo Doesn't this belong in change.c ??
 */
void
ChangeArcAngles (LayerType *Layer, ArcType *a,
                 Angle new_sa, Angle new_da)
{
  if (new_da >= 360)
    {
      new_da = 360;
      new_sa = 0;
    }
  RestoreToPolygon (PCB->Data, ARC_TYPE, Layer, a);
  r_delete_entry (Layer->arc_tree, (BoxType *) a);
  AddObjectToChangeAnglesUndoList (ARC_TYPE, a, a, a);
  a->StartAngle = new_sa;
  a->Delta = new_da;
  SetArcBoundingBox (a);
  r_insert_entry (Layer->arc_tree, (BoxType *) a, 0);
  ClearFromPolygon (PCB->Data, ARC_TYPE, Layer, a);
}

static char *
BumpName (char *Name)
{
  int num;
  char c, *start;
  static char temp[256];

  start = Name;
  /* seek end of string */
  while (*Name != 0)
    Name++;
  /* back up to potential number */
  for (Name--; isdigit ((int) *Name); Name--);
  Name++;
  if (*Name)
    num = atoi (Name) + 1;
  else
    num = 1;
  c = *Name;
  *Name = 0;
  sprintf (temp, "%s%d", start, num);
  /* if this is not our string, put back the blown character */
  if (start != temp)
    *Name = c;
  return (temp);
}

/*!
 * \brief Make a unique name for the name on board.
 *
 * This can alter the contents of the input string.
 */
char *
UniqueElementName (DataType *Data, char *Name)
{
  bool unique = true;
  /* null strings are ok */
  if (!Name || !*Name)
    return (Name);

  for (;;)
    {
      ELEMENT_LOOP (Data);
      {
        if (NAMEONPCB_NAME (element) &&
            NSTRCMP (NAMEONPCB_NAME (element), Name) == 0)
          {
            Name = BumpName (Name);
            unique = false;
            break;
          }
      }
      END_LOOP;
      if (unique)
        return (Name);
      unique = true;
    }
}

static void
GetGridLockCoordinates (int type, void *ptr1,
                        void *ptr2, void *ptr3, Coord * x,
                        Coord * y)
{
  switch (type)
    {
    case VIA_TYPE:
      *x = ((PinType *) ptr2)->X;
      *y = ((PinType *) ptr2)->Y;
      break;
    case LINE_TYPE:
      *x = ((LineType *) ptr2)->Point1.X;
      *y = ((LineType *) ptr2)->Point1.Y;
      break;
    case TEXT_TYPE:
    case ELEMENTNAME_TYPE:
      *x = ((TextType *) ptr2)->X;
      *y = ((TextType *) ptr2)->Y;
      break;
    case ELEMENT_TYPE:
      *x = ((ElementType *) ptr2)->MarkX;
      *y = ((ElementType *) ptr2)->MarkY;
      break;
    case POLYGON_TYPE:
      *x = ((PolygonType *) ptr2)->Points[0].X;
      *y = ((PolygonType *) ptr2)->Points[0].Y;
      break;

    case LINEPOINT_TYPE:
    case POLYGONPOINT_TYPE:
      *x = ((PointType *) ptr3)->X;
      *y = ((PointType *) ptr3)->Y;
      break;
    case ARC_TYPE:
      {
        BoxType *box;

        box = GetArcEnds ((ArcType *) ptr2);
        *x = box->X1;
        *y = box->Y1;
        break;
      }
    }
}

void
AttachForCopy (Coord PlaceX, Coord PlaceY)
{
  Coord mx = 0, my = 0;

  Crosshair.AttachedObject.RubberbandN = 0;
  if (! TEST_FLAG (SNAPPINFLAG, PCB))
    {
      /* dither the grab point so that the mark, center, etc
       * will end up on a grid coordinate
       */
      GetGridLockCoordinates (Crosshair.AttachedObject.Type,
                              Crosshair.AttachedObject.Ptr1,
                              Crosshair.AttachedObject.Ptr2,
                              Crosshair.AttachedObject.Ptr3, &mx, &my);
      mx = GridFit (mx, PCB->Grid, PCB->GridOffsetX) - mx;
      my = GridFit (my, PCB->Grid, PCB->GridOffsetY) - my;
    }
  Crosshair.AttachedObject.X = PlaceX - mx;
  Crosshair.AttachedObject.Y = PlaceY - my;
  if (!Marked.status || TEST_FLAG (LOCALREFFLAG, PCB))
    SetLocalRef (PlaceX - mx, PlaceY - my, true);
  Crosshair.AttachedObject.State = STATE_SECOND;

  /* get boundingbox of object and set cursor range */
  crosshair_update_range();

  /* get all attached objects if necessary */
  if ((Settings.Mode != COPY_MODE) && TEST_FLAG (RUBBERBANDFLAG, PCB))
    LookupRubberbandLines (Crosshair.AttachedObject.Type,
                           Crosshair.AttachedObject.Ptr1,
                           Crosshair.AttachedObject.Ptr2,
                           Crosshair.AttachedObject.Ptr3);
  if (Settings.Mode != COPY_MODE &&
      (Crosshair.AttachedObject.Type == ELEMENT_TYPE ||
       Crosshair.AttachedObject.Type == VIA_TYPE ||
       Crosshair.AttachedObject.Type == LINE_TYPE ||
       Crosshair.AttachedObject.Type == LINEPOINT_TYPE))
    LookupRatLines (Crosshair.AttachedObject.Type,
                    Crosshair.AttachedObject.Ptr1,
                    Crosshair.AttachedObject.Ptr2,
                    Crosshair.AttachedObject.Ptr3);
}

/*!
 * \brief Return nonzero if the given file exists and is readable.
 */
int
FileExists (const char *name)
{
  FILE *f;
  f = fopen (name, "r");
  if (f)
    {
      fclose (f);
      return 1;
    }
  return 0;
}

char *
Concat (const char *first, ...)
{
  char *rv;
  int len;
  va_list a;

  len = strlen (first);
  rv = (char *) malloc (len + 1);
  strcpy (rv, first);

  va_start (a, first);
  while (1)
    {
      const char *s = va_arg (a, const char *);
      if (!s)
        break;
      len += strlen (s);
      rv = (char *) realloc (rv, len + 1);
      strcat (rv, s);
    }
  va_end (a);
  return rv;
}

int
mem_any_set (unsigned char *ptr, int bytes)
{
  while (bytes--)
    if (*ptr++)
      return 1;
  return 0;
}

/*!
 * \brief This just fills in a FlagType with current flags.
 */
FlagType
MakeFlags (unsigned int flags)
{
  FlagType rv;
  memset (&rv, 0, sizeof (rv));
  rv.f = flags;
  return rv;
}

/*!
 * \brief This converts old flag bits (from saved PCB files) to new
 * format.
 */
FlagType
OldFlags (unsigned int flags)
{
  FlagType rv;
  int i, f;
  memset (&rv, 0, sizeof (rv));
  /* If we move flag bits around, this is where we map old bits to them.  */
  rv.f = flags & 0xffff;
  f = 0x10000;
  for (i = 0; i < 8; i++)
    {
      /* use the closest thing to the old thermal style */
      if (flags & f)
	rv.t[i / 2] |= (1 << (4 * (i % 2)));
      f <<= 1;
    }
  return rv;
}

FlagType
AddFlags (FlagType flag, unsigned int flags)
{
  flag.f |= flags;
  return flag;
}

FlagType
MaskFlags (FlagType flag, unsigned int flags)
{
  flag.f &= ~flags;
  return flag;
}

/* Layer Group Functions. */

/*!
 * \brief Returns group actually moved to (i.e. either group or
 * previous).
 */
int
MoveLayerToGroup (int layer, int group)
{
  int prev, i, j;

  if (layer < 0 || layer > max_copper_layer + 1)
    return -1;
  prev = GetLayerGroupNumberByNumber (layer);
  if ((layer == bottom_silk_layer
        && group == GetLayerGroupNumberByNumber (top_silk_layer))
      || (layer == top_silk_layer
          && group == GetLayerGroupNumberByNumber (bottom_silk_layer))
      || (group < 0 || group >= max_group) || (prev == group))
    return prev;

  /* Remove layer from prev group */
  for (j = i = 0; i < PCB->LayerGroups.Number[prev]; i++)
    if (PCB->LayerGroups.Entries[prev][i] != layer)
      PCB->LayerGroups.Entries[prev][j++] = PCB->LayerGroups.Entries[prev][i];
  PCB->LayerGroups.Number[prev]--;

  /* Add layer to new group.  */
  i = PCB->LayerGroups.Number[group]++;
  PCB->LayerGroups.Entries[group][i] = layer;

  return group;
}

/*!
 * \brief Returns pointer to private buffer.
 */
char *
LayerGroupsToString (LayerGroupType *lg)
{
#if MAX_ALL_LAYER < 9999
  /* Allows for layer numbers 0..9999 */
  static char buf[(MAX_ALL_LAYER) * 5 + 1];
#endif
  char *cp = buf;
  char sep = 0;
  int group, entry;
  for (group = 0; group < max_group; group++)
    if (PCB->LayerGroups.Number[group])
      {
        if (sep)
          *cp++ = ':';
        sep = 1;
        for (entry = 0; entry < PCB->LayerGroups.Number[group]; entry++)
          {
            int layer = PCB->LayerGroups.Entries[group][entry];
            if (layer == top_silk_layer)
              {
                *cp++ = 'c';
              }
            else if (layer == bottom_silk_layer)
              {
                *cp++ = 's';
              }
            else
              {
                sprintf (cp, "%d", layer + 1);
                while (*++cp)
                  ;
              }
            if (entry != PCB->LayerGroups.Number[group] - 1)
              *cp++ = ',';
          }
      }
  *cp++ = 0;
  return buf;
}

int
GetMaxBottomLayer ()
{
  int l = 0;
  int group, i;

  group = GetLayerGroupNumberBySide (BOTTOM_SIDE);
  for (i = 0; i < PCB->LayerGroups.Number[group]; i++)
    {
      if (PCB->LayerGroups.Entries[group][i] < max_copper_layer)
        l = max (l, PCB->LayerGroups.Entries[group][i]);
    }
  return l;
}


/*!
 * \brief Sanitize buried via
 *
 * - remove degraded vias
 * - ensure correct order of layers
 * - change full size vias to TH vias
 */
void
SanitizeBuriedVia (PinType *via)
{
  /* we expect that via is burried/blind */

  if (via->BuriedFrom > via->BuriedTo)
    {
      int tmp = via->BuriedFrom;
      via->BuriedFrom = via->BuriedTo;
      via->BuriedTo = tmp;
    }

  /* check, if via was extended to full stack (after first layer removal)*/
  /* convert it in TH via in such case */
  if (via->BuriedTo == GetMaxBottomLayer ()
      && via->BuriedFrom == 0)
    via->BuriedTo = 0;
}

#if 0
bool
IsLayerMoveSafe (int old_index, int new_index)
{
  VIA_LOOP (PCB->Data);
    {
      if (VIA_IS_BURIED (via))
        {
          /* moving from below to inside */
          if ((old_index < via->BuriedFrom)
              && (new_index < via->BuriedTo))
            return false;

          /* moving from above to inside */
          if ((old_index > via->BuriedTo)
              && (new_index > via->BuriedFrom))
            return false;
        }
    }
  END_LOOP;

  return true;
}

#endif

/*!
 * \brief Update buried vias after layer move
 *
 */

void
ChangeBuriedViasAfterLayerMove (int old_index, int new_index)
{
  VIA_LOOP (PCB->Data);
    {
      if (VIA_IS_BURIED (via))
        {
          /* do nothing, if both layers are below the via */
          if ((old_index < via->BuriedFrom)
              && (new_index < via->BuriedFrom))
            continue;

          /* do nothing, if both layers are above the via */
          if ((old_index > via->BuriedTo)
              && (new_index > via->BuriedTo))
            continue;

          /* moving via top layer - via top follows layer */
          if (old_index == via->BuriedFrom)
            {
	      AddObjectToSetViaLayersUndoList (via, via, via);
              via->BuriedFrom = new_index;
            }

          /* moving via bottom layer - via bottom follows layer */
          else if (old_index == via->BuriedTo)
            {
	      AddObjectToSetViaLayersUndoList (via, via, via);
              via->BuriedTo = new_index;
            }

          /* moving layer covered by via inside the via (except top and bottom) - do nothing */
          else if (VIA_ON_LAYER (via, old_index)
              && VIA_ON_LAYER (via, new_index))
            continue;

          /* moving from inside to below - shrink via DANGEROUS op*/
          else if (VIA_ON_LAYER (via, old_index)
              && (new_index < via->BuriedFrom))
            {
	      AddObjectToSetViaLayersUndoList (via, via, via);
	      via->BuriedFrom++;
            }

          /* moving from inside to above - shrink via DANGEROUS op*/
          else if (VIA_ON_LAYER (via, old_index)
              && (new_index > via->BuriedTo))
            {
	      AddObjectToSetViaLayersUndoList (via, via, via);
	      via->BuriedTo--;
            }

          /* moving from above to below - shift via up */
          else if ((old_index > via->BuriedTo)
              && (new_index < via->BuriedFrom))
            {
	      AddObjectToSetViaLayersUndoList (via, via, via);
              via->BuriedFrom++;
              via->BuriedTo++;
            }

          /* moving from below to above - shift via down */
          else if ((old_index < via->BuriedFrom)
              && (new_index >= via->BuriedTo))
            {
	      AddObjectToSetViaLayersUndoList (via, via, via);
              via->BuriedFrom--;
              via->BuriedTo--;
            }

          /* moving from below to inside - extend via down DANGEROUS op*/
          else if ((old_index < via->BuriedFrom)
              && (new_index < via->BuriedTo))
            {
	      AddObjectToSetViaLayersUndoList (via, via, via);
              via->BuriedFrom--;
            }

          /* moving from above to inside - extend via up DANGEROUS op*/
          else if ((old_index > via->BuriedTo)
              && (new_index > via->BuriedFrom))
            {
	      AddObjectToSetViaLayersUndoList ( via, via, via);
              via->BuriedTo++;
            }
          else
            {
              Message (_("Layer move: failed via update: %d -> %d, via: %d:%d\n"), old_index, new_index, via->BuriedFrom, via->BuriedTo);
              continue;
            }

          SanitizeBuriedVia (via);
        }
    }
  END_LOOP;
  RemoveDegradedVias ();
}

/*!
 * \brief Update buried vias after new layer create
 *
 */
void
ChangeBuriedViasAfterLayerCreate (int index)
{
  VIA_LOOP (PCB->Data);
    {
      if (VIA_IS_BURIED (via))
        {
          if (index <= via->BuriedFrom
	      || index <= via->BuriedTo)
            AddObjectToSetViaLayersUndoList (via, via, via);

          if (index <= via->BuriedFrom)
            via->BuriedFrom++;
          if (index <= via->BuriedTo)
            via->BuriedTo++;
        }
    }
  END_LOOP;
}

/*!
 * \brief Update buried vias after layer delete
 *
 */
void
ChangeBuriedViasAfterLayerDelete (int index)
{
  VIA_LOOP (PCB->Data);
    {
      if (VIA_IS_BURIED (via))
        {
          if (index < via->BuriedFrom
	      || index <= via->BuriedTo)
            AddObjectToSetViaLayersUndoList (via, via, via);

          if (index < via->BuriedFrom)
            via->BuriedFrom--;
          if (index <= via->BuriedTo)
            via->BuriedTo--;

          SanitizeBuriedVia (via);
        }
    }
  END_LOOP;
  RemoveDegradedVias ();
}

/*!
 * \brief Check if via penetrates layer group
 *
 */
bool
ViaIsOnLayerGroup (PinType *via, int group)
{
  Cardinal layer;

  if (!VIA_IS_BURIED (via))
    return true;

  for (layer = via->BuriedFrom; layer <= via->BuriedTo; layer++)
    {
       if (GetLayerGroupNumberByNumber (layer) == group)
         return true;
    }

  return false;
}

/*!
 * \brief Check if via penetrates any visible layer
 *
 */
bool
ViaIsOnAnyVisibleLayer (PinType *via)
{
  Cardinal layer;

  if (!VIA_IS_BURIED (via))
    return true;

  for (layer = via->BuriedFrom; layer <= via->BuriedTo; layer ++)
    {
      if (PCB->Data->Layer[layer].On)
        return true;
    }

  return false;
}


char *
pcb_author (void)
{
#ifdef HAVE_GETPWUID
  static struct passwd *pwentry;
  static char *fab_author = 0;

  if (!fab_author)
    {
      if (Settings.FabAuthor && Settings.FabAuthor[0])
        fab_author = Settings.FabAuthor;
      else
        {
          int len;
          char *comma, *gecos;

          /* ID the user. */
          pwentry = getpwuid (getuid ());
          gecos = pwentry->pw_gecos;
          comma = strchr (gecos, ',');
          if (comma)
            len = comma - gecos;
          else
            len = strlen (gecos);
          fab_author = (char *)malloc (len + 1);
          if (!fab_author)
            {
              perror ("pcb: out of memory.\n");
              exit (-1);
            }
          memcpy (fab_author, gecos, len);
          fab_author[len] = 0;
        }
    }
  return fab_author;
#else
  return "Unknown";
#endif
}


/*!
 * \brief Returns NULL if the name isn't found, else the value for that
 * named attribute.
 */
char *
AttributeGetFromList (AttributeListType *list, char *name)
{
  int i;
  for (i=0; i<list->Number; i++)
    if (strcmp (name, list->List[i].name) == 0)
      return list->List[i].value;
  return NULL;
}

/*!
 * \brief Adds an attribute to the list.
 *
 * If the attribute already exists, whether it's replaced or a second
 * copy added depends on REPLACE.
 *
 * \return Non-zero if an existing attribute was replaced.
 */
int
AttributePutToList (AttributeListType *list, const char *name, const char *value, int replace)
{
  int i;

  /* If we're allowed to replace an existing attribute, see if we
     can.  */
  if (replace)
    {
      for (i=0; i<list->Number; i++)
	if (strcmp (name, list->List[i].name) == 0)
	  {
	    free (list->List[i].value);
	    list->List[i].value = STRDUP (value);
	    return 1;
	  }
    }

  /* At this point, we're going to need to add a new attribute to the
     list.  See if there's room.  */
  if (list->Number >= list->Max)
    {
      list->Max += 10;
      list->List = (AttributeType *) realloc (list->List,
					      list->Max * sizeof (AttributeType));
    }

  /* Now add the new attribute.  */
  i = list->Number;
  list->List[i].name = STRDUP (name);
  list->List[i].value = STRDUP (value);
  list->Number ++;
  return 0;
}

/*!
 * \brief Remove an attribute by name.
 */
void
AttributeRemoveFromList(AttributeListType *list, char *name)
{
  int i, j;
  for (i=0; i<list->Number; i++)
    if (strcmp (name, list->List[i].name) == 0)
      {
	free (list->List[i].name);
	free (list->List[i].value);
	for (j=i; j<list->Number-1; j++)
	  list->List[j] = list->List[j+1];
	list->Number --;
      }
}

/*!
 * \todo In future all use of this should be supplanted by pcb-printf
 * and %mr/%m# spec.
 *
 * These act like you'd expect, except always in the C locale.
 */
const char *
c_dtostr (double d)
{
  static char buf[100];
  int i, f;
  char *bufp = buf;

  if (d < 0)
    {
      *bufp++ = '-';
      d = -d;
    }
  d += 0.0000005;               /* rounding */
  i = floor (d);
  d -= i;
  sprintf (bufp, "%d", i);
  bufp += strlen (bufp);
  *bufp++ = '.';

  f = floor (d * 1000000.0);
  sprintf (bufp, "%06d", f);
  return buf;
}

void
r_delete_element (DataType * data, ElementType * element)
{
  r_delete_entry (data->element_tree, (BoxType *) element);
  PIN_LOOP (element);
  {
    r_delete_entry (data->pin_tree, (BoxType *) pin);
  }
  END_LOOP;
  PAD_LOOP (element);
  {
    r_delete_entry (data->pad_tree, (BoxType *) pad);
  }
  END_LOOP;
  ELEMENTTEXT_LOOP (element);
  {
    r_delete_entry (data->name_tree[n], (BoxType *) text);
  }
  END_LOOP;
}


/*!
 * \brief Returns a string that has a bunch of information about the
 * program.
 *
 * Can be used for things like "about" dialog boxes.
 */
char *
GetInfoString (void)
{
  HID **hids;
  int i;
  static DynamicStringType info;
  static int first_time = 1;

#define TAB "    "

  if (first_time)
    {
      first_time = 0;
      DSAddString (&info,
	  _("This is PCB, an interactive\n"
	    "printed circuit board editor\n"
	    "version "));
      DSAddString (&info,
	    VERSION "\n\n"
	    "Compiled on " __DATE__ " at " __TIME__ "\n\n"
	    "by harry eaton\n\n"
	    "Copyright (C) Thomas Nau 1994, 1995, 1996, 1997\n"
	    "Copyright (C) harry eaton 1998-2007\n"
	    "Copyright (C) C. Scott Ananian 2001\n"
	    "Copyright (C) DJ Delorie 2003, 2004, 2005, 2006, 2007, 2008\n"
	    "Copyright (C) Dan McMahill 2003, 2004, 2005, 2006, 2007, 2008\n\n");
      DSAddString (&info,
	  _("It is licensed under the terms of the GNU\n"
	    "General Public License version 2\n"
	    "See the LICENSE file for more information\n\n"
	    "For more information see:\n"));
      DSAddString (&info, _("PCB homepage: "));
      DSAddString (&info, "http://pcb.geda-project.org\n");
      DSAddString (&info, _("gEDA homepage: "));
      DSAddString (&info, "http://www.geda-project.org\n");
      DSAddString (&info, _("gEDA Wiki: "));
      DSAddString (&info, "http://wiki.geda-project.org\n");

      DSAddString (&info, _("\n----- Compile Time Options -----\n"));
      hids = hid_enumerate ();
      DSAddString (&info, _("GUI:\n"));
      for (i = 0; hids[i]; i++)
        {
          if (hids[i]->gui)
            {
              DSAddString (&info, TAB);
              DSAddString (&info, hids[i]->name);
              DSAddString (&info, " : ");
              DSAddString (&info, hids[i]->description);
              DSAddString (&info, "\n");
            }
        }

      DSAddString (&info, _("Exporters:\n"));
      for (i = 0; hids[i]; i++)
        {
          if (hids[i]->exporter)
            {
              DSAddString (&info, TAB);
              DSAddString (&info, hids[i]->name);
              DSAddString (&info, " : ");
              DSAddString (&info, hids[i]->description);
              DSAddString (&info, "\n");
            }
        }

      DSAddString (&info, _("Printers:\n"));
      for (i = 0; hids[i]; i++)
        {
          if (hids[i]->printer)
            {
              DSAddString (&info, TAB);
              DSAddString (&info, hids[i]->name);
              DSAddString (&info, " : ");
              DSAddString (&info, hids[i]->description);
              DSAddString (&info, "\n");
            }
        }
    }
#undef TAB

  return info.Data;
}

#ifdef MKDIR_IS_PCBMKDIR
#error "Don't know how to create a directory on this system."
#endif

/*!
 * \brief mkdir() implentation, mostly for plugins, which don't have our
 * config.h.
 */
int
pcb_mkdir (const char *path, int mode)
{
  return MKDIR (path, mode);
}

/*!
 * \brief Returns a best guess about the orientation of an element.
 *
 * The value corresponds to the rotation; a difference is the right
 * value to pass to RotateElementLowLevel.
 * However, the actual value is no indication of absolute rotation; only
 * relative rotation is meaningful.
 *
 * \return a relative rotation for an element, useful only for comparing
 * two similar footprints.
 */
int 
ElementOrientation (ElementType *e)
{
  Coord pin1x, pin1y, pin2x, pin2y, dx, dy;
  bool found_pin1 = 0;
  bool found_pin2 = 0;

  /* in case we don't find pin 1 or 2, make sure we have initialized these variables */
  pin1x = 0;
  pin1y = 0;
  pin2x = 0;
  pin2y = 0;

  PIN_LOOP (e);
  {
    if (NSTRCMP (pin->Number, "1") == 0)
      {
	pin1x = pin->X;
	pin1y = pin->Y;
	found_pin1 = 1;
      }
    else if (NSTRCMP (pin->Number, "2") == 0)
      {
	pin2x = pin->X;
	pin2y = pin->Y;
	found_pin2 = 1;
      }
  }
  END_LOOP;

  PAD_LOOP (e);
  {
    if (NSTRCMP (pad->Number, "1") == 0)
      {
	pin1x = (pad->Point1.X + pad->Point2.X) / 2;
	pin1y = (pad->Point1.Y + pad->Point2.Y) / 2;
	found_pin1 = 1;
      }
    else if (NSTRCMP (pad->Number, "2") == 0)
      {
	pin2x = (pad->Point1.X + pad->Point2.X) / 2;
	pin2y = (pad->Point1.Y + pad->Point2.Y) / 2;
	found_pin2 = 1;
      }
  }
  END_LOOP;

  if (found_pin1 && found_pin2)
    {
      dx = pin2x - pin1x;
      dy = pin2y - pin1y;
    }
  else if (found_pin1 && (pin1x || pin1y))
    {
      dx = pin1x;
      dy = pin1y;
    }
  else if (found_pin2 && (pin2x || pin2y))
    {
      dx = pin2x;
      dy = pin2y;
    }
  else
    return 0;

  if (abs(dx) > abs(dy))
    return dx > 0 ? 0 : 2;
  return dy > 0 ? 3 : 1;
}

int
ActionListRotations(int argc, char **argv, Coord x, Coord y)
{
  ELEMENT_LOOP (PCB->Data);
  {
    printf("%d %s\n", ElementOrientation(element), NAMEONPCB_NAME(element));
  }
  END_LOOP;

  return 0;
}

HID_Action misc_action_list[] = {
  {"ListRotations", 0, ActionListRotations,
   0,0},
};

REGISTER_ACTIONS (misc_action_list)
