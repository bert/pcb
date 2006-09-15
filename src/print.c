/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2003 Thomas Nau
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* Change History:
 * 10/11/96 11:37 AJF Added support for a Text() driver function.
 * This was done out of a pressing need to force text to be printed on the
 * silkscreen layer. Perhaps the design is not the best.
 */


/* printing routines
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"

#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <setjmp.h>


#include "data.h"
#include "draw.h"
#include "drill.h"
#include "file.h"
#include "find.h"
#include "error.h"
#include "misc.h"
#include "print.h"
#include "polygon.h"
#include "rtree.h"
#include "search.h"

#include "hid.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

/* ---------------------------------------------------------------------------
 * prints a FAB drawing.
 */

#define TEXT_SIZE	150
#define TEXT_LINE	15000
#define FAB_LINE_W      800

static void
fab_line (int x1, int y1, int x2, int y2)
{
  gui->draw_line (Output.fgGC, x1, y1, x2, y2);
}

static void
fab_circle (int x, int y, int r)
{
  gui->draw_arc (Output.fgGC, x, y, r, r, 0, 180);
  gui->draw_arc (Output.fgGC, x, y, r, r, 180, 180);
}

/* align is 0=left, 1=center, 2=right, add 8 for underline */
static void
text_at (int x, int y, int align, char *fmt, ...)
{
  char tmp[512];
  int w = 0, i;
  TextType t;
  va_list a;
  FontTypePtr font = &PCB->Font;
  va_start (a, fmt);
  vsprintf (tmp, fmt, a);
  va_end (a);
  t.Direction = 0;
  t.TextString = tmp;
  t.Scale = TEXT_SIZE;
  t.Flags = NoFlags ();
  t.X = x;
  t.Y = y;
  for (i = 0; tmp[i]; i++)
    w +=
      (font->Symbol[(int) tmp[i]].Width + font->Symbol[(int) tmp[i]].Delta);
  w = w * TEXT_SIZE / 100;
  t.X -= w * (align & 3) / 2;
  if (t.X < 0)
    t.X = 0;
  DrawTextLowLevel (&t);
  if (align & 8)
    fab_line (t.X,
	      t.Y +
	      font->MaxHeight * TEXT_SIZE /
	      100 + 1000, t.X + w,
	      t.Y + font->MaxHeight * TEXT_SIZE / 100 + 1000);
}

/* Y, +, X, circle, square */
static void
drill_sym (int idx, int x, int y)
{
  int type = idx % 5;
  int size = idx / 5;
  int s2 = (size + 1) * 1600;
  int i;
  switch (type)
    {
    case 0:			/* Y */ ;
      fab_line (x, y, x, y + s2);
      fab_line (x, y, x + s2 * 13 / 15, y - s2 / 2);
      fab_line (x, y, x - s2 * 13 / 15, y - s2 / 2);
      for (i = 1; i <= size; i++)
	fab_circle (x, y, i * 1600);
      break;
    case 1:			/* + */
      ;
      fab_line (x, y - s2, x, y + s2);
      fab_line (x - s2, y, x + s2, y);
      for (i = 1; i <= size; i++)
	{
	  fab_line (x - i * 1600, y - i * 1600, x + i * 1600, y - i * 1600);
	  fab_line (x - i * 1600, y - i * 1600, x - i * 1600, y + i * 1600);
	  fab_line (x - i * 1600, y + i * 1600, x + i * 1600, y + i * 1600);
	  fab_line (x + i * 1600, y - i * 1600, x + i * 1600, y + i * 1600);
	}
      break;
    case 2:			/* X */ ;
      fab_line (x - s2 * 3 / 4, y - s2 * 3 / 4, x + s2 * 3 / 4,
		y + s2 * 3 / 4);
      fab_line (x - s2 * 3 / 4, y + s2 * 3 / 4, x + s2 * 3 / 4,
		y - s2 * 3 / 4);
      for (i = 1; i <= size; i++)
	{
	  fab_line (x - i * 1600, y - i * 1600, x + i * 1600, y - i * 1600);
	  fab_line (x - i * 1600, y - i * 1600, x - i * 1600, y + i * 1600);
	  fab_line (x - i * 1600, y + i * 1600, x + i * 1600, y + i * 1600);
	  fab_line (x + i * 1600, y - i * 1600, x + i * 1600, y + i * 1600);
	}
      break;
    case 3:			/* circle */ ;
      for (i = 0; i <= size; i++)
	fab_circle (x, y, (i + 1) * 1600 - 800);
      break;
    case 4:			/* square */
      for (i = 1; i <= size + 1; i++)
	{
	  fab_line (x - i * 1600, y - i * 1600, x + i * 1600, y - i * 1600);
	  fab_line (x - i * 1600, y - i * 1600, x - i * 1600, y + i * 1600);
	  fab_line (x - i * 1600, y + i * 1600, x + i * 1600, y + i * 1600);
	  fab_line (x + i * 1600, y - i * 1600, x + i * 1600, y + i * 1600);
	}
      break;
    }
}

static int
count_drill_lines (DrillInfoTypePtr AllDrills)
{
  int n, ds = 0;
  for (n = AllDrills->DrillN - 1; n >= 0; n--)
    {
      DrillTypePtr drill = &(AllDrills->Drill[n]);
      if (drill->PinCount + drill->ViaCount > drill->UnplatedCount)
	ds++;
      if (drill->UnplatedCount)
	ds++;
    }
  return ds;
}


int
PrintFab_overhang (void)
{
  DrillInfoTypePtr AllDrills = GetDrillInfo (PCB->Data);
  int ds = count_drill_lines (AllDrills);
  if (ds < 4)
    ds = 4;
  return (ds + 2) * TEXT_LINE;
}

void
PrintFab (void)
{
  PinType tmp_pin;
  DrillInfoTypePtr AllDrills;
  int i, n, yoff, total_drills = 0, ds = 0;
  time_t currenttime;
  char utcTime[64];
  tmp_pin.Flags = NoFlags ();
  AllDrills = GetDrillInfo (PCB->Data);
  RoundDrillInfo (AllDrills, 100);
  yoff = -TEXT_LINE;

  /* count how many drill description lines will be needed */
  ds = count_drill_lines (AllDrills);

  /*
   * When we only have a few drill sizes we need to make sure the
   * drill table header doesn't fall on top of the board info
   * section.
   */
  if (ds < 4)
    {
      yoff -= (4 - ds) * TEXT_LINE;
    }

  gui->set_line_width (Output.fgGC, FAB_LINE_W);

  for (n = AllDrills->DrillN - 1; n >= 0; n--)
    {
      int plated_sym = -1, unplated_sym = -1;
      DrillTypePtr drill = &(AllDrills->Drill[n]);
      if (drill->PinCount + drill->ViaCount > drill->UnplatedCount)
	plated_sym = --ds;
      if (drill->UnplatedCount)
	unplated_sym = --ds;
      gui->set_color (Output.fgGC, PCB->PinColor);
      for (i = 0; i < drill->PinN; i++)
	drill_sym (TEST_FLAG (HOLEFLAG, drill->Pin[i]) ?
		   unplated_sym : plated_sym, drill->Pin[i]->X,
		   drill->Pin[i]->Y);
      if (plated_sym != -1)
	{
	  drill_sym (plated_sym, 100 * TEXT_SIZE, yoff + 100 * TEXT_SIZE / 4);
	  text_at (135000, yoff, 200, "YES");
	  text_at (98000, yoff, 200, "%d",
		   drill->PinCount + drill->ViaCount - drill->UnplatedCount);

	  if (unplated_sym != -1)
	    yoff -= TEXT_LINE;
	}
      if (unplated_sym != -1)
	{
	  drill_sym (unplated_sym, 100 * TEXT_SIZE,
		     yoff + 100 * TEXT_SIZE / 4);
	  text_at (140000, yoff, 200, "NO");
	  text_at (98000, yoff, 200, "%d", drill->UnplatedCount);
	}
      gui->set_color (Output.fgGC, PCB->ElementColor);
      text_at (45000, yoff, 200, "%0.3f",
	       drill->DrillSize / 100000. + 0.0004);
      if (plated_sym != -1 && unplated_sym != -1)
	text_at (45000, yoff + TEXT_LINE, 200, "%0.3f",
		 drill->DrillSize / 100000. + 0.0004);
      yoff -= TEXT_LINE;
      total_drills += drill->PinCount;
      total_drills += drill->ViaCount;
    }

  gui->set_color (Output.fgGC, PCB->ElementColor);
  text_at (0, yoff, 900, "Symbol");
  text_at (41000, yoff, 900, "Diam. (Inch)");
  text_at (95000, yoff, 900, "Count");
  text_at (130000, yoff, 900, "Plated?");
  yoff -= TEXT_LINE;
  text_at (0, yoff, 0,
	   "There are %d different drill sizes used in this layout, %d holes total",
	   AllDrills->DrillN, total_drills);
  /* Create a portable timestamp. */
  currenttime = time (NULL);
  strftime (utcTime, sizeof utcTime, "%c UTC", gmtime (&currenttime));
  yoff = -TEXT_LINE;
  for (i = 0; i < max_layer; i++)
    {
      LayerType *l = LAYER_PTR (i);
      if (l->Name && l->LineN)
	{
	  if (strcasecmp ("route", l->Name) == 0)
	    break;
	  if (strcasecmp ("outline", l->Name) == 0)
	    break;
	}
    }
  if (i == max_layer)
    {
      gui->set_line_width (Output.fgGC, 1000);
      gui->draw_line (Output.fgGC, 0, 0, PCB->MaxWidth, 0);
      gui->draw_line (Output.fgGC, 0, 0, 0, PCB->MaxHeight);
      gui->draw_line (Output.fgGC, PCB->MaxWidth, 0, PCB->MaxWidth,
		      PCB->MaxHeight);
      gui->draw_line (Output.fgGC, 0, PCB->MaxHeight, PCB->MaxWidth,
		      PCB->MaxHeight);
      /*FPrintOutline (); */
      gui->set_line_width (Output.fgGC, FAB_LINE_W);
      text_at (200000, yoff, 0,
	       "Maximum Dimensions: %d mils wide, %d mils high",
	       PCB->MaxWidth / 100, PCB->MaxHeight / 100);
      text_at (PCB->MaxWidth / 2, PCB->MaxHeight + 2000, 1,
	       "Board outline is the centerline of this 10 mil"
	       " rectangle - 0,0 to %d,%d mils",
	       PCB->MaxWidth / 100, PCB->MaxHeight / 100);
    }
  else
    {
      LayerTypePtr layer = LAYER_PTR (i);
      gui->set_line_width (Output.fgGC, 1000);
      LINE_LOOP (layer);
      {
	gui->draw_line (Output.fgGC, line->Point1.X, line->Point1.Y,
			line->Point2.X, line->Point2.Y);
      }
      END_LOOP;
      ARC_LOOP (layer);
      {
	gui->draw_arc (Output.fgGC, arc->X, arc->Y, arc->Width,
		       arc->Height, arc->StartAngle, arc->Delta);
      }
      END_LOOP;
      TEXT_LOOP (layer);
      {
	DrawTextLowLevel (text);
      }
      END_LOOP;
      gui->set_line_width (Output.fgGC, FAB_LINE_W);
      text_at (PCB->MaxWidth / 2, PCB->MaxHeight + 2000, 1,
	       "Board outline is the centerline of this path");
    }
  yoff -= TEXT_LINE;
  text_at (200000, yoff, 0, "Date: %s", utcTime);
  yoff -= TEXT_LINE;
  text_at (200000, yoff, 0, "Author: %s", pcb_author ());
  yoff -= TEXT_LINE;
  text_at (200000, yoff, 0,
	   "Title: %s - Fabrication Drawing", UNKNOWN (PCB->Name));
}
