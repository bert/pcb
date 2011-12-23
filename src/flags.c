/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2005 DJ Delorie
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "global.h"
#include "data.h"
#include "pcb-printf.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static int
FlagCurrentStyle (int dummy)
{
  STYLE_LOOP (PCB);
  {
    if (style->Thick == Settings.LineThickness &&
	style->Diameter == Settings.ViaThickness &&
	style->Hole == Settings.ViaDrillingHole &&
	style->Keepaway == Settings.Keepaway)
      return n + 1;
  }
  END_LOOP;
  return 0;
}

static int
FlagGrid (int dummy)
{
  return PCB->Grid > 1;
}

static int
FlagGridSize (int dummy)
{
  return PCB->Grid;
}

static int
FlagUnitsMm (int dummy)
{
  static const Unit *u = NULL;
  if (u == NULL)
    u = get_unit_struct ("mm");
  return (Settings.grid_unit == u);
}

static int
FlagUnitsMil (int dummy)
{
  static const Unit *u = NULL;
  if (u == NULL)
    u = get_unit_struct ("mil");
  return (Settings.grid_unit == u);
}

static int
FlagBuffer (int dummy)
{
  return (int) (Settings.BufferNumber + 1);
}

static int
FlagElementName (int dummy)
{
  if (TEST_FLAG (NAMEONPCBFLAG, PCB))
    return 2;
  if (TEST_FLAG (DESCRIPTIONFLAG, PCB))
    return 1;
  return 3;
}

static int
FlagTESTFLAG (int bit)
{
  return TEST_FLAG (bit, PCB) ? 1 : 0;
}

static int
FlagSETTINGS (int ofs)
{
  return *(bool *) ((char *) (&Settings) + ofs);
}

static int
FlagMode (int x)
{
  if (x == -1)
    return Settings.Mode;
  return Settings.Mode == x;
}

static int
FlagHaveRegex (int x)
{
#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
  return 1;
#else
  return 0;
#endif
}

enum {
  FL_SILK = -6,
  FL_PINS,
  FL_RATS,
  FL_VIAS,
  FL_BACK,
  FL_MASK
};

static int
FlagLayerShown (int n)
{
  switch (n)
    {
    case FL_SILK:
      return PCB->ElementOn;
    case FL_PINS:
      return PCB->PinOn;
    case FL_RATS:
      return PCB->RatOn;
    case FL_VIAS:
      return PCB->ViaOn;
    case FL_BACK:
      return PCB->InvisibleObjectsOn;
    case FL_MASK:
      return TEST_FLAG (SHOWMASKFLAG, PCB);
    default:
      if (n >= 0 && n < max_copper_layer)
	return PCB->Data->Layer[n].On;
    }
  return 0;
}

static int
FlagLayerActive (int n)
{
  int current_layer;
  if (PCB->RatDraw)
    current_layer = FL_RATS;
  else if (PCB->SilkActive)
    current_layer = FL_SILK;
  else
    return 0;

  return current_layer == n;
}

/* The cast to (int) is ONLY valid because we know we are
 * taking offsets on structures where the offset will fit
 * in an integer variable. It silences compile warnings on
 * 64bit machines.
 */
#define OffsetOf(a,b) (int)(size_t)(&(((a *)0)->b))

HID_Flag flags_flag_list[] = {
  {"style", FlagCurrentStyle, 0},
  {"grid", FlagGrid, 0},
  {"gridsize", FlagGridSize, 0},
  {"elementname", FlagElementName, 0},
  {"have_regex", FlagHaveRegex, 0},

  {"silk_shown", FlagLayerShown, FL_SILK},
  {"pins_shown", FlagLayerShown, FL_PINS},
  {"rats_shown", FlagLayerShown, FL_RATS},
  {"vias_shown", FlagLayerShown, FL_VIAS},
  {"back_shown", FlagLayerShown, FL_BACK},
  {"mask_shown", FlagLayerShown, FL_MASK},
  {"silk_active", FlagLayerActive, FL_SILK},
  {"rats_active", FlagLayerActive, FL_RATS},

  {"mode", FlagMode, -1},
  {"nomode", FlagMode, NO_MODE},
  {"arcmode", FlagMode, ARC_MODE},
  {"arrowmode", FlagMode, ARROW_MODE},
  {"copymode", FlagMode, COPY_MODE},
  {"insertpointmode", FlagMode, INSERTPOINT_MODE},
  {"linemode", FlagMode, LINE_MODE},
  {"lockmode", FlagMode, LOCK_MODE},
  {"movemode", FlagMode, MOVE_MODE},
  {"pastebuffermode", FlagMode, PASTEBUFFER_MODE},
  {"polygonmode", FlagMode, POLYGON_MODE},
  {"polygonholemode", FlagMode, POLYGONHOLE_MODE},
  {"rectanglemode", FlagMode, RECTANGLE_MODE},
  {"removemode", FlagMode, REMOVE_MODE},
  {"rotatemode", FlagMode, ROTATE_MODE},
  {"rubberbandmovemode", FlagMode, RUBBERBANDMOVE_MODE},
  {"textmode", FlagMode, TEXT_MODE},
  {"thermalmode", FlagMode, THERMAL_MODE},
  {"viamode", FlagMode, VIA_MODE},

  {"shownumber", FlagTESTFLAG, SHOWNUMBERFLAG},
  {"localref", FlagTESTFLAG, LOCALREFFLAG},
  {"checkplanes", FlagTESTFLAG, CHECKPLANESFLAG},
  {"showdrc", FlagTESTFLAG, SHOWDRCFLAG},
  {"rubberband", FlagTESTFLAG, RUBBERBANDFLAG},
  {"description", FlagTESTFLAG, DESCRIPTIONFLAG},
  {"nameonpcb", FlagTESTFLAG, NAMEONPCBFLAG},
  {"autodrc", FlagTESTFLAG, AUTODRCFLAG},
  {"alldirection", FlagTESTFLAG, ALLDIRECTIONFLAG},
  {"swapstartdir", FlagTESTFLAG, SWAPSTARTDIRFLAG},
  {"uniquename", FlagTESTFLAG, UNIQUENAMEFLAG},
  {"clearnew", FlagTESTFLAG, CLEARNEWFLAG},
  {"snappin", FlagTESTFLAG, SNAPPINFLAG},
  {"showmask", FlagTESTFLAG, SHOWMASKFLAG},
  {"thindraw", FlagTESTFLAG, THINDRAWFLAG},
  {"orthomove", FlagTESTFLAG, ORTHOMOVEFLAG},
  {"liveroute", FlagTESTFLAG, LIVEROUTEFLAG},
  {"thindrawpoly", FlagTESTFLAG, THINDRAWPOLYFLAG},
  {"locknames", FlagTESTFLAG, LOCKNAMESFLAG},
  {"onlynames", FlagTESTFLAG, ONLYNAMESFLAG},
  {"newfullpoly", FlagTESTFLAG, NEWFULLPOLYFLAG},
  {"hidenames", FlagTESTFLAG, HIDENAMESFLAG},

  {"fullpoly", FlagSETTINGS, OffsetOf (SettingType, FullPoly)},
  {"grid_units_mm", FlagUnitsMm, -1},
  {"grid_units_mil", FlagUnitsMil, -1},
  {"clearline", FlagSETTINGS, OffsetOf (SettingType, ClearLine)},
  {"uniquenames", FlagSETTINGS, OffsetOf (SettingType, UniqueNames)},
  {"showsolderside", FlagSETTINGS, OffsetOf (SettingType, ShowSolderSide)},
  {"savelastcommand", FlagSETTINGS, OffsetOf (SettingType, SaveLastCommand)},
  {"saveintmp", FlagSETTINGS, OffsetOf (SettingType, SaveInTMP)},
  {"drawgrid", FlagSETTINGS, OffsetOf (SettingType, DrawGrid)},
  {"ratwarn", FlagSETTINGS, OffsetOf (SettingType, RatWarn)},
  {"stipplepolygons", FlagSETTINGS, OffsetOf (SettingType, StipplePolygons)},
  {"alldirectionlines", FlagSETTINGS,
	OffsetOf (SettingType, AllDirectionLines)},
  {"rubberbandmode", FlagSETTINGS, OffsetOf (SettingType, RubberBandMode)},
  {"swapstartdirection", FlagSETTINGS,
	OffsetOf (SettingType, SwapStartDirection)},
  {"showdrcmode", FlagSETTINGS, OffsetOf (SettingType, ShowDRC)},
  {"resetafterelement", FlagSETTINGS,
	OffsetOf (SettingType, ResetAfterElement)},
  {"ringbellwhenfinished", FlagSETTINGS,
	OffsetOf (SettingType, RingBellWhenFinished)},

  {"buffer", FlagBuffer, 0},

};

REGISTER_FLAGS (flags_flag_list)

