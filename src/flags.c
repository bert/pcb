/*!
 * \file src/flags.c
 *
 * \brief .
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2005 DJ Delorie
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

#include <glib.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

int pcb_flag_eq (FlagType *f1, FlagType *f2);

/*!
 * \brief .
 *
 * \warning ignore unknowns for now: the only place where we use this
 * function, undo.c, won't care.
 */
int
pcb_flag_eq (FlagType *f1, FlagType *f2)
{
  if (f1->f != f2->f)
    return 0;

  return (memcmp(f1->t, &f2->t, sizeof(f1->t)) == 0);
}

static int
FlagCurrentStyle (void *data)
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
FlagGrid (void *data)
{
  return PCB->Grid > 1;
}

static int
FlagGridSize (void *data)
{
  return PCB->Grid;
}

static int
FlagUnitsMm (void *data)
{
  static const Unit *u = NULL;
  if (u == NULL)
    u = get_unit_struct ("mm");
  return (Settings.grid_unit == u);
}

static int
FlagUnitsMil (void *data)
{
  static const Unit *u = NULL;
  if (u == NULL)
    u = get_unit_struct ("mil");
  return (Settings.grid_unit == u);
}

static int
FlagBuffer (void *data)
{
  return (int) (Settings.BufferNumber + 1);
}

static int
FlagElementName (void  *data)
{
  if (TEST_FLAG (NAMEONPCBFLAG, PCB))
    return 2;
  if (TEST_FLAG (DESCRIPTIONFLAG, PCB))
    return 1;
  return 3;
}

static int
FlagTESTFLAG (void *data)
{
  int bit = GPOINTER_TO_INT (data);
  return TEST_FLAG (bit, PCB) ? 1 : 0;
}

static int
FlagSETTINGS (void *data)
{
  size_t ofs = (size_t)data;
  return *(bool *) ((char *)(&Settings) + ofs);
}

static int
FlagMode (void *data)
{
  int x = GPOINTER_TO_INT (data);
  if (x == -1)
    return Settings.Mode;
  return Settings.Mode == x;
}

static int
FlagHaveRegex (void *data)
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
FlagLayerShown (void *data)
{
  int n = GPOINTER_TO_INT (data);
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
FlagLayerActive (void *data)
{
  int test_layer = GPOINTER_TO_INT (data);
  int current_layer;
  if (PCB->RatDraw)
    current_layer = FL_RATS;
  else if (PCB->SilkActive)
    current_layer = FL_SILK;
  else
    return 0;

  return current_layer == test_layer;
}

#define OFFSET_POINTER(a, b) (&(((a *)0)->b))

HID_Flag flags_flag_list[] = {
  {"style",                FlagCurrentStyle, NULL},
  {"grid",                 FlagGrid,         NULL},
  {"gridsize",             FlagGridSize,     NULL},
  {"elementname",          FlagElementName,  NULL},
  {"have_regex",           FlagHaveRegex,    NULL},

  {"silk_shown",           FlagLayerShown,   GINT_TO_POINTER (FL_SILK)},
  {"pins_shown",           FlagLayerShown,   GINT_TO_POINTER (FL_PINS)},
  {"rats_shown",           FlagLayerShown,   GINT_TO_POINTER (FL_RATS)},
  {"vias_shown",           FlagLayerShown,   GINT_TO_POINTER (FL_VIAS)},
  {"back_shown",           FlagLayerShown,   GINT_TO_POINTER (FL_BACK)},
  {"mask_shown",           FlagLayerShown,   GINT_TO_POINTER (FL_MASK)},

  {"silk_active",          FlagLayerActive,  GINT_TO_POINTER (FL_SILK)},
  {"rats_active",          FlagLayerActive,  GINT_TO_POINTER (FL_RATS)},

  {"mode",                 FlagMode,         GINT_TO_POINTER (-1)},
  {"nomode",               FlagMode,         GINT_TO_POINTER (NO_MODE)},
  {"arcmode",              FlagMode,         GINT_TO_POINTER (ARC_MODE)},
  {"arrowmode",            FlagMode,         GINT_TO_POINTER (ARROW_MODE)},
  {"copymode",             FlagMode,         GINT_TO_POINTER (COPY_MODE)},
  {"insertpointmode",      FlagMode,         GINT_TO_POINTER (INSERTPOINT_MODE)},
  {"linemode",             FlagMode,         GINT_TO_POINTER (LINE_MODE)},
  {"lockmode",             FlagMode,         GINT_TO_POINTER (LOCK_MODE)},
  {"movemode",             FlagMode,         GINT_TO_POINTER (MOVE_MODE)},
  {"pastebuffermode",      FlagMode,         GINT_TO_POINTER (PASTEBUFFER_MODE)},
  {"polygonmode",          FlagMode,         GINT_TO_POINTER (POLYGON_MODE)},
  {"polygonholemode",      FlagMode,         GINT_TO_POINTER (POLYGONHOLE_MODE)},
  {"rectanglemode",        FlagMode,         GINT_TO_POINTER (RECTANGLE_MODE)},
  {"removemode",           FlagMode,         GINT_TO_POINTER (REMOVE_MODE)},
  {"rotatemode",           FlagMode,         GINT_TO_POINTER (ROTATE_MODE)},
  {"rubberbandmovemode",   FlagMode,         GINT_TO_POINTER (RUBBERBANDMOVE_MODE)},
  {"textmode",             FlagMode,         GINT_TO_POINTER (TEXT_MODE)},
  {"thermalmode",          FlagMode,         GINT_TO_POINTER (THERMAL_MODE)},
  {"viamode",              FlagMode,         GINT_TO_POINTER (VIA_MODE)},

  {"shownumber",           FlagTESTFLAG,     GINT_TO_POINTER (SHOWNUMBERFLAG)},
  {"localref",             FlagTESTFLAG,     GINT_TO_POINTER (LOCALREFFLAG)},
  {"checkplanes",          FlagTESTFLAG,     GINT_TO_POINTER (CHECKPLANESFLAG)},
  {"showdrc",              FlagTESTFLAG,     GINT_TO_POINTER (SHOWDRCFLAG)},
  {"rubberband",           FlagTESTFLAG,     GINT_TO_POINTER (RUBBERBANDFLAG)},
  {"description",          FlagTESTFLAG,     GINT_TO_POINTER (DESCRIPTIONFLAG)},
  {"nameonpcb",            FlagTESTFLAG,     GINT_TO_POINTER (NAMEONPCBFLAG)},
  {"autodrc",              FlagTESTFLAG,     GINT_TO_POINTER (AUTODRCFLAG)},
  {"alldirection",         FlagTESTFLAG,     GINT_TO_POINTER (ALLDIRECTIONFLAG)},
  {"swapstartdir",         FlagTESTFLAG,     GINT_TO_POINTER (SWAPSTARTDIRFLAG)},
  {"uniquename",           FlagTESTFLAG,     GINT_TO_POINTER (UNIQUENAMEFLAG)},
  {"clearnew",             FlagTESTFLAG,     GINT_TO_POINTER (CLEARNEWFLAG)},
  {"snappin",              FlagTESTFLAG,     GINT_TO_POINTER (SNAPPINFLAG)},
  {"showmask",             FlagTESTFLAG,     GINT_TO_POINTER (SHOWMASKFLAG)},
  {"thindraw",             FlagTESTFLAG,     GINT_TO_POINTER (THINDRAWFLAG)},
  {"orthomove",            FlagTESTFLAG,     GINT_TO_POINTER (ORTHOMOVEFLAG)},
  {"liveroute",            FlagTESTFLAG,     GINT_TO_POINTER (LIVEROUTEFLAG)},
  {"thindrawpoly",         FlagTESTFLAG,     GINT_TO_POINTER (THINDRAWPOLYFLAG)},
  {"locknames",            FlagTESTFLAG,     GINT_TO_POINTER (LOCKNAMESFLAG)},
  {"onlynames",            FlagTESTFLAG,     GINT_TO_POINTER (ONLYNAMESFLAG)},
  {"newfullpoly",          FlagTESTFLAG,     GINT_TO_POINTER (NEWFULLPOLYFLAG)},
  {"hidenames",            FlagTESTFLAG,     GINT_TO_POINTER (HIDENAMESFLAG)},
  {"autoburiedvias",       FlagTESTFLAG,     GINT_TO_POINTER (AUTOBURIEDVIASFLAG)},

  {"grid_units_mm",        FlagUnitsMm,      NULL},
  {"grid_units_mil",       FlagUnitsMil,     NULL},

  {"fullpoly",             FlagSETTINGS,     OFFSET_POINTER (SettingType, FullPoly)},
  {"clearline",            FlagSETTINGS,     OFFSET_POINTER (SettingType, ClearLine)},
  {"uniquenames",          FlagSETTINGS,     OFFSET_POINTER (SettingType, UniqueNames)},
  {"showsolderside",       FlagSETTINGS,     OFFSET_POINTER (SettingType, ShowBottomSide)},
  {"savelastcommand",      FlagSETTINGS,     OFFSET_POINTER (SettingType, SaveLastCommand)},
  {"saveintmp",            FlagSETTINGS,     OFFSET_POINTER (SettingType, SaveInTMP)},
  {"drawgrid",             FlagSETTINGS,     OFFSET_POINTER (SettingType, DrawGrid)},
  {"ratwarn",              FlagSETTINGS,     OFFSET_POINTER (SettingType, RatWarn)},
  {"stipplepolygons",      FlagSETTINGS,     OFFSET_POINTER (SettingType, StipplePolygons)},
  {"alldirectionlines",    FlagSETTINGS,     OFFSET_POINTER (SettingType, AllDirectionLines)},
  {"rubberbandmode",       FlagSETTINGS,     OFFSET_POINTER (SettingType, RubberBandMode)},
  {"swapstartdirection",   FlagSETTINGS,     OFFSET_POINTER (SettingType, SwapStartDirection)},
  {"showdrcmode",          FlagSETTINGS,     OFFSET_POINTER (SettingType, ShowDRC)},
  {"resetafterelement",    FlagSETTINGS,     OFFSET_POINTER (SettingType, ResetAfterElement)},
  {"ringbellwhenfinished", FlagSETTINGS,     OFFSET_POINTER (SettingType, RingBellWhenFinished)},

  {"buffer",               FlagBuffer,       NULL},

};

REGISTER_FLAGS (flags_flag_list)

