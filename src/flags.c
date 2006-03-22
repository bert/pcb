/* $Id$ */

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

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID("$Id$");

static int
FlagCurrentStyle()
{
  STYLE_LOOP (PCB);
  {
    if (style->Thick == Settings.LineThickness &&
	style->Diameter == Settings.ViaThickness &&
	style->Hole == Settings.ViaDrillingHole &&
	style->Keepaway == Settings.Keepaway)
      return n+1;
  }
  END_LOOP;
  return 0;
}

static int
FlagGrid()
{
  return PCB->Grid > 1.0;
}

static int
FlagGridSize()
{
  return (int)(PCB->Grid + 0.5);
}

static int
FlagElementName()
{
  if (TEST_FLAG (NAMEONPCBFLAG, PCB))
    return 2;
  if (TEST_FLAG (DESCRIPTIONFLAG, PCB))
    return 1;
  return 3;
}

static int
FlagTESTFLAG(int bit)
{
  return TEST_FLAG(bit, PCB) ? 1 : 0;
}

static int
FlagSETTINGS(int ofs)
{
  return *(Boolean *)((char *)(&Settings) + ofs);
}

static int
FlagMode(int x)
{
  if (x == -1)
    return Settings.Mode;
  return Settings.Mode == x;
}

#define OffsetOf(a,b) (int)(&(((a *)0)->b))

HID_Flag flags_flag_list[] = {
  { "style", FlagCurrentStyle, 0 },
  { "grid", FlagGrid, 0 },
  { "gridsize", FlagGridSize, 0 },
  { "elementname", FlagElementName, 0 },

  { "mode", FlagMode, -1 },
  { "nomode", FlagMode, NO_MODE },
  { "arcmode", FlagMode, ARC_MODE },
  { "arrowmode", FlagMode, ARROW_MODE },
  { "copymode", FlagMode, COPY_MODE },
  { "insertpointmode", FlagMode, INSERTPOINT_MODE },
  { "linemode", FlagMode, LINE_MODE },
  { "lockmode", FlagMode, LOCK_MODE },
  { "movemode", FlagMode, MOVE_MODE },
  { "pastebuffermode", FlagMode, PASTEBUFFER_MODE },
  { "polygonmode", FlagMode, POLYGON_MODE },
  { "rectanglemode", FlagMode, RECTANGLE_MODE },
  { "removemode", FlagMode, REMOVE_MODE },
  { "rotatemode", FlagMode, ROTATE_MODE },
  { "rubberbandmovemode", FlagMode, RUBBERBANDMOVE_MODE },
  { "textmode", FlagMode, TEXT_MODE },
  { "thermalmode", FlagMode, THERMAL_MODE },
  { "viamode", FlagMode, VIA_MODE },

  { "shownumber", FlagTESTFLAG, SHOWNUMBERFLAG },
  { "localref", FlagTESTFLAG, LOCALREFFLAG },
  { "checkplanes", FlagTESTFLAG, CHECKPLANESFLAG },
  { "showdrc", FlagTESTFLAG, SHOWDRCFLAG },
  { "rubberband", FlagTESTFLAG, RUBBERBANDFLAG },
  { "description", FlagTESTFLAG, DESCRIPTIONFLAG },
  { "nameonpcb", FlagTESTFLAG, NAMEONPCBFLAG },
  { "autodrc", FlagTESTFLAG, AUTODRCFLAG },
  { "alldirection", FlagTESTFLAG, ALLDIRECTIONFLAG },
  { "swapstartdir", FlagTESTFLAG, SWAPSTARTDIRFLAG },
  { "uniquename", FlagTESTFLAG, UNIQUENAMEFLAG },
  { "clearnew", FlagTESTFLAG, CLEARNEWFLAG },
  { "snappin", FlagTESTFLAG, SNAPPINFLAG },
  { "showmask", FlagTESTFLAG, SHOWMASKFLAG },
  { "orthomove", FlagTESTFLAG, ORTHOMOVEFLAG },
  { "liveroute", FlagTESTFLAG, LIVEROUTEFLAG },

  { "grid_units_mm", FlagSETTINGS, OffsetOf(SettingType,grid_units_mm)},
  { "clearline", FlagSETTINGS, OffsetOf(SettingType,ClearLine)},
  { "uniquenames", FlagSETTINGS, OffsetOf(SettingType,UniqueNames)},
  { "uselogwindow", FlagSETTINGS, OffsetOf(SettingType,UseLogWindow)},
  { "raiselogwindow", FlagSETTINGS, OffsetOf(SettingType,RaiseLogWindow)},
  { "showsolderside", FlagSETTINGS, OffsetOf(SettingType,ShowSolderSide)},
  { "savelastcommand", FlagSETTINGS, OffsetOf(SettingType,SaveLastCommand)},
  { "saveintmp", FlagSETTINGS, OffsetOf(SettingType,SaveInTMP)},
  { "drawgrid", FlagSETTINGS, OffsetOf(SettingType,DrawGrid)},
  { "ratwarn", FlagSETTINGS, OffsetOf(SettingType,RatWarn)},
  { "stipplepolygons", FlagSETTINGS, OffsetOf(SettingType,StipplePolygons)},
  { "alldirectionlines", FlagSETTINGS, OffsetOf(SettingType,AllDirectionLines)},
  { "rubberbandmode", FlagSETTINGS, OffsetOf(SettingType,RubberBandMode)},
  { "swapstartdirection", FlagSETTINGS, OffsetOf(SettingType,SwapStartDirection)},
  { "showdrc", FlagSETTINGS, OffsetOf(SettingType,ShowDRC)},
  { "liveroute", FlagSETTINGS, OffsetOf(SettingType,liveRouting)},
  { "resetafterelement", FlagSETTINGS, OffsetOf(SettingType,ResetAfterElement)},
  { "ringbellwhenfinished", FlagSETTINGS, OffsetOf(SettingType,RingBellWhenFinished)},
};
REGISTER_FLAGS(flags_flag_list)
