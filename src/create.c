/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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

static char *rcsid = "$Id$";

/* functions used to create vias, pins ...
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <memory.h>
#include <setjmp.h>
#include <stdlib.h>

#include "global.h"

#include "create.h"
#include "data.h"
#include "dialog.h"
#include "draw.h"
#include "error.h"
#include "mymem.h"
#include "misc.h"
#include "parse_l.h"
#include "rtree.h"
#include "search.h"
#include "set.h"
#include "undo.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static int ID = 1;		/* current object ID; incremented after */
				/* each creation of an object */

/* ----------------------------------------------------------------------
 * some local prototypes
 */
static void AddTextToElement (TextTypePtr, FontTypePtr,
			      Location, Location, BYTE, char *, int, int);

/* ---------------------------------------------------------------------------
 * creates a new paste buffer
 */
DataTypePtr
CreateNewBuffer (void)
{
  DataTypePtr data;
  data = (DataTypePtr) MyCalloc (1, sizeof (DataType), "CreateNewBuffer()");
  return data;
}

/* ---------------------------------------------------------------------------
 * creates a new PCB
 */
PCBTypePtr
CreateNewPCB (Boolean SetDefaultNames)
{
  PCBTypePtr ptr;
  int i;

  /* allocate memory, switch all layers on and copy resources */
  ptr = MyCalloc (1, sizeof (PCBType), "CreateNewPCB()");
  ptr->Data = CreateNewBuffer ();

  /* copy default settings */
  ptr->ConnectedColor = Settings.ConnectedColor;
  ptr->ElementColor = Settings.ElementColor;
  ptr->RatColor = Settings.RatColor;
  ptr->InvisibleObjectsColor = Settings.InvisibleObjectsColor;
  ptr->InvisibleMarkColor = Settings.InvisibleMarkColor;
  ptr->ElementSelectedColor = Settings.ElementSelectedColor;
  ptr->RatSelectedColor = Settings.RatSelectedColor;
  ptr->PinColor = Settings.PinColor;
  ptr->PinSelectedColor = Settings.PinSelectedColor;
  ptr->ViaColor = Settings.ViaColor;
  ptr->ViaSelectedColor = Settings.ViaSelectedColor;
  ptr->WarnColor = Settings.WarnColor;
  ptr->MaskColor = Settings.MaskColor;
  for (i = 0; i < MAX_LAYER; i++)
    {
      ptr->Data->Layer[i].Color = Settings.LayerColor[i];
      ptr->Data->Layer[i].SelectedColor = Settings.LayerSelectedColor[i];
    }
  ptr->Data->Layer[MAX_LAYER + COMPONENT_LAYER].Color =
    Settings.ShowSolderSide ? Settings.
    InvisibleObjectsColor : Settings.ElementColor;
  ptr->Data->Layer[MAX_LAYER + COMPONENT_LAYER].SelectedColor =
    Settings.ElementSelectedColor;
  ptr->Data->Layer[MAX_LAYER + SOLDER_LAYER].Color =
    Settings.ShowSolderSide ? Settings.
    ElementColor : Settings.InvisibleObjectsColor;
  ptr->Data->Layer[MAX_LAYER + SOLDER_LAYER].SelectedColor =
    Settings.ElementSelectedColor;

  if (SetDefaultNames)
    for (i = 0; i < MAX_LAYER; i++)
      ptr->Data->Layer[i].Name = MyStrdup (Settings.DefaultLayerName[i],
					   "CreateNewPCB()");
  ptr->Data->Layer[MAX_LAYER + COMPONENT_LAYER].Name =
    MyStrdup ("silk", "CreateNewPCB()");
  ptr->Data->Layer[MAX_LAYER + SOLDER_LAYER].Name =
    MyStrdup ("silk", "CreateNewPCB()");

  ptr->SilkActive = False;
  ptr->RatDraw = False;
  SET_FLAG (NAMEONPCBFLAG, ptr);
  SET_FLAG (SHOWNUMBERFLAG, ptr);
  if (Settings.AllDirectionLines)
    SET_FLAG (ALLDIRECTIONFLAG, ptr);
  ptr->Clipping = 1;		/* this is the most useful starting point for now */
  if (Settings.RubberBandMode)
    SET_FLAG (RUBBERBANDFLAG, ptr);
  if (Settings.SwapStartDirection)
    SET_FLAG (SWAPSTARTDIRFLAG, ptr);
  if (Settings.UniqueNames)
    SET_FLAG (UNIQUENAMEFLAG, ptr);
  if (Settings.SnapPin)
    SET_FLAG (SNAPPINFLAG, ptr);
  if (Settings.ClearLine)
    SET_FLAG (CLEARNEWFLAG, ptr);
  ptr->Grid = Settings.Grid;
  ptr->LayerGroups = Settings.LayerGroups;
  STYLE_LOOP (ptr, 
    {
      *style = Settings.RouteStyle[n];
    }
  );
  ptr->Zoom = Settings.Zoom;
  ptr->MaxWidth = Settings.MaxWidth;
  ptr->MaxHeight = Settings.MaxHeight;
  ptr->ID = ID++;
  ptr->ThermScale = 0.5;
  return (ptr);
}

/* ---------------------------------------------------------------------------
 * creates a new via
 */
PinTypePtr
CreateNewVia (DataTypePtr Data,
	      Location X, Location Y,
	      BDimension Thickness, BDimension Clearance, BDimension Mask,
	      BDimension DrillingHole, char *Name, int Flags)
{
  PinTypePtr Via;

  VIA_LOOP (Data, 
    {
      if ((float) (via->X - X) * (float) (via->X - X) +
	  (float) (via->Y - Y) * (float) (via->Y - Y) <=
	  ((float) (via->Thickness / 2 + Thickness / 2) *
	   (float) (via->Thickness / 2 + Thickness / 2)))
	return (NULL);		/* don't allow via stacking */
    }
  );

  Via = GetViaMemory (Data);

  if (!Via)
    return (Via);
  /* copy values */
  Via->X = X;
  Via->Y = Y;
  Via->Thickness = Thickness;
  Via->Clearance = Clearance;
  Via->Mask = Mask;
  Via->DrillingHole = DrillingHole;
  Via->Name = MyStrdup (Name, "CreateNewVia()");
  Via->Flags = Flags & ~WARNFLAG;
  Via->ID = ID++;
  SetPinBoundingBox(Via);
  if (!Data->via_tree)
    Data->via_tree = r_create_tree(NULL, 0, 0);
  r_insert_entry(Data->via_tree, (BoxTypePtr)Via, 0);
  return (Via);
}

struct line_info
{
  Location X1, X2, Y1, Y2;
  BDimension Thickness;
  LineType test, *ans;
  jmp_buf env;
};
  
static int
line_callback (const BoxType *b, void *cl)
{
  LineTypePtr line = (LineTypePtr)b;
  struct line_info *i = (struct line_info *)cl;

  if (line->Point1.X == i->X1 &&
      line->Point2.X == i->X2 &&
      line->Point1.Y == i->Y1 &&
      line->Point2.Y == i->Y2)
    {
      i->ans = (LineTypePtr)(-1);
      longjmp (i->env, 1);
    }
    /* check the other point order */
  if (line->Point1.X == i->X1 &&
      line->Point2.X == i->X2 &&
      line->Point1.Y == i->Y1 &&
      line->Point2.Y == i->Y2)
    {
      i->ans = (LineTypePtr)(-1);
      longjmp (i->env, 1);
    }
  if (line->Point2.X == i->X1 &&
      line->Point1.X == i->X2 &&
      line->Point2.Y == i->Y1 &&
      line->Point1.Y == i->Y2)
    {
      i->ans = (LineTypePtr)-1;
      longjmp (i->env, 1);
    }
     /* remove unncessary line points */
  if (line->Thickness == i->Thickness)
    {
      if (line->Point1.X == i->X1 && line->Point1.Y == i->Y1)
        {
           i->test.Point1.X = line->Point2.X;
           i->test.Point1.Y = line->Point2.Y;
           i->test.Point2.X = i->X2;
           i->test.Point2.Y = i->Y2;
	   if (IsPointOnLine ((float) i->X1, (float) i->Y1, 0.0, &i->test))
	     {
	       i->ans = line;
               longjmp (i->env, 1);
             } 
	}
      else if (line->Point2.X == i->X1 && line->Point2.Y == i->Y1)
        {
	   i->test.Point1.X = line->Point1.X;
	   i->test.Point1.Y = line->Point1.Y;
	   i->test.Point2.X = i->X2;
	   i->test.Point2.Y = i->Y2;
	   if (IsPointOnLine ((float) i->X1, (float) i->Y1, 0.0, &i->test))
	     {
	       i->ans = line;
               longjmp (i->env, 1);
	     }
        }
      else if (line->Point1.X == i->X2 && line->Point1.Y == i->Y2)
        {
           i->test.Point1.X = line->Point2.X;
           i->test.Point1.Y = line->Point2.Y;
           i->test.Point2.X = i->X1;
           i->test.Point2.Y = i->Y1;
           if (IsPointOnLine ((float) i->X2, (float) i->Y2, 0.0, &i->test))
             {
	       i->ans = line;
               longjmp (i->env, 1);
             }
        }
      else if (line->Point2.X == i->X2 && line->Point2.Y == i->Y2)
        {
          i->test.Point1.X = line->Point1.X;
	  i->test.Point1.Y = line->Point1.Y;
	  i->test.Point2.X = i->X1;
	  i->test.Point2.Y = i->Y1;
	  if (IsPointOnLine ((float) i->X2, (float) i->Y2, 0.0, &i->test))
	    {
	      i->ans = line;
              longjmp (i->env, 1);
            }
        }
    }
  return 0;
}

    
/* ---------------------------------------------------------------------------
 * creates a new line on a layer and checks for overlap and extension
 */
LineTypePtr
CreateDrawnLineOnLayer (LayerTypePtr Layer,
			Location X1, Location Y1,
			Location X2, Location Y2,
			BDimension Thickness, BDimension Clearance, int Flags)
{
  struct line_info info;
  BoxType search;

  search.X1 = MIN(X1,X2);
  search.X2 = MAX(X1,X2);
  search.Y1 = MIN(Y1,Y2);
  search.Y2 = MAX(Y1,Y2);
  info.X1 = X1;
  info.X2 = X2;
  info.Y1 = Y1;
  info.Y2 = Y2;
  info.Thickness = Thickness;
  info.test.Thickness = 0;
  info.test.Flags = NOFLAG;
  info.ans = NULL;
  /* prevent stacking of duplicate lines
   * and remove needless intermediate points
   * verify that the layer is on the board first!
   */
  if (setjmp (info.env) == 0)
    {
      r_search(Layer->line_tree, &search, NULL, line_callback, &info);
      return CreateNewLineOnLayer (Layer, X1, Y1, X2, Y2,
		       Thickness, Clearance, Flags);
    }
   
  if ((void *)info.ans == (void *)(-1))
    return NULL; /* stacked line */
  /* remove unneccessary points */
  if (info.ans)
    {
      /* must do this BEFORE getting new line memory */
      MoveObjectToRemoveUndoList (LINE_TYPE, Layer, info.ans, info.ans);
      X1 = info.test.Point1.X;
      X2 = info.test.Point2.X;
      Y1 = info.test.Point1.Y;
      Y2 = info.test.Point2.Y;
    }
  return CreateNewLineOnLayer (Layer, X1, Y1, X2, Y2,
	       Thickness, Clearance, Flags);
}

LineTypePtr
CreateNewLineOnLayer (LayerTypePtr Layer,
		      Location X1, Location Y1,
		      Location X2, Location Y2,
		      BDimension Thickness, BDimension Clearance, int Flags)
{
  LineTypePtr Line;

  Line = GetLineMemory (Layer);
  if (!Line)
    return (Line);
  Line->ID = ID++;
  Line->Flags = Flags;
  Line->Thickness = Thickness;
  Line->Clearance = Clearance;
  Line->Point1.X = X1;
  Line->Point1.Y = Y1;
  Line->Point1.ID = ID++;
  Line->Point2.X = X2;
  Line->Point2.Y = Y2;
  Line->Point2.ID = ID++;
  SetLineBoundingBox(Line);
  SetPointBoundingBox(&Line->Point1);
  SetPointBoundingBox(&Line->Point2);
  if (!Layer->line_tree)
    Layer->line_tree = r_create_tree(NULL, 0, 0);
  r_insert_entry(Layer->line_tree, (BoxTypePtr)Line, 0);
  return (Line);
}

/* ---------------------------------------------------------------------------
 * creates a new rat-line
 */
RatTypePtr
CreateNewRat (DataTypePtr Data, Location X1, Location Y1,
	      Location X2, Location Y2, Cardinal group1,
	      Cardinal group2, BDimension Thickness, int Flags)
{
  RatTypePtr Line = GetRatMemory (Data);

  if (!Line)
    return (Line);

  Line->ID = ID++;
  Line->Flags = Flags | RATFLAG;
  Line->Thickness = Thickness;
  Line->Point1.X = X1;
  Line->Point1.Y = Y1;
  Line->Point1.ID = ID++;
  Line->Point2.X = X2;
  Line->Point2.Y = Y2;
  Line->Point2.ID = ID++;
  Line->group1 = group1;
  Line->group2 = group2;
  return (Line);
}

/* ---------------------------------------------------------------------------
 * creates a new arc on a layer
 */
ArcTypePtr
CreateNewArcOnLayer (LayerTypePtr Layer,
		     Location X1, Location Y1,
		     BDimension width, int sa,
		     int dir, BDimension Thickness,
		     BDimension Clearance, int Flags)
{
  ArcTypePtr Arc;

  ARC_LOOP (Layer, 
    {
      if (arc->X == X1 && arc->Y == Y1 && arc->Width == width &&
	  (arc->StartAngle + 360) % 360 == (sa + 360) % 360 &&
	  arc->Delta == dir)
	return (NULL);		/* prevent stacked arcs */
    }
  );
  Arc = GetArcMemory (Layer);
  if (!Arc)
    return (Arc);

  Arc->ID = ID++;
  Arc->Flags = Flags;
  Arc->Thickness = Thickness;
  Arc->Clearance = Clearance;
  Arc->X = X1;
  Arc->Y = Y1;
  Arc->Width = Arc->Height = width;
  Arc->StartAngle = sa;
  Arc->Delta = dir;
  SetArcBoundingBox (Arc);
  if (!Layer->arc_tree)
    Layer->arc_tree = r_create_tree(NULL, 0, 0);
  r_insert_entry(Layer->arc_tree, (BoxTypePtr)Arc, 0);
  return (Arc);
}


/* ---------------------------------------------------------------------------
 * creates a new polygon from the old formats rectangle data
 */
PolygonTypePtr
CreateNewPolygonFromRectangle (LayerTypePtr Layer,
			       Location X1, Location Y1,
			       Location X2, Location Y2, int Flags)
{
  PolygonTypePtr polygon = CreateNewPolygon (Layer, Flags);
  if (!polygon)
    return (polygon);

  CreateNewPointInPolygon (polygon, X1, Y1);
  CreateNewPointInPolygon (polygon, X2, Y1);
  CreateNewPointInPolygon (polygon, X2, Y2);
  CreateNewPointInPolygon (polygon, X1, Y2);
  SetPolygonBoundingBox (polygon);
  return (polygon);
}

/* ---------------------------------------------------------------------------
 * creates a new text on a layer
 */
TextTypePtr
CreateNewText (LayerTypePtr Layer, FontTypePtr PCBFont,
	       Location X, Location Y,
	       BYTE Direction, int Scale, char *TextString, int Flags)
{
  TextTypePtr text = GetTextMemory (Layer);
  if (!text)
    return (text);

  /* copy values, width and height are set by drawing routine
   * because at ths point we don't know which symbols are available
   */
  text->X = X;
  text->Y = Y;
  text->Direction = Direction;
  text->Flags = Flags;
  text->Scale = Scale;
  text->TextString = MyStrdup (TextString, "CreateNewText()");

  /* calculate size of the bounding box */
  SetTextBoundingBox (PCBFont, text);
  text->ID = ID++;
  return (text);
}

/* ---------------------------------------------------------------------------
 * creates a new polygon on a layer
 */
PolygonTypePtr
CreateNewPolygon (LayerTypePtr Layer, int Flags)
{
  PolygonTypePtr polygon = GetPolygonMemory (Layer);

  /* copy values */
  polygon->Flags = Flags;
  polygon->ID = ID++;
  return (polygon);
}

/* ---------------------------------------------------------------------------
 * creates a new point in a polygon
 */
PointTypePtr
CreateNewPointInPolygon (PolygonTypePtr Polygon, Location X, Location Y)
{
  PointTypePtr point = GetPointMemoryInPolygon (Polygon);

  /* copy values */
  point->X = X;
  point->Y = Y;
  point->ID = ID++;
  return (point);
}

/* ---------------------------------------------------------------------------
 * creates an new element
 * memory is allocated if needed
 */
ElementTypePtr
CreateNewElement (DataTypePtr Data, ElementTypePtr Element,
		  FontTypePtr PCBFont,
		  int Flags,
		  char *Description, char *NameOnPCB, char *Value,
		  Location TextX, Location TextY, BYTE Direction,
		  int TextScale, int TextFlags, Boolean uniqueName)
{
  if (!Element)
    Element = GetElementMemory (Data);

  /* copy values and set additional information */
  AddTextToElement (&DESCRIPTION_TEXT (Element), PCBFont, TextX, TextY,
		    Direction, Description, TextScale, TextFlags);
  if (uniqueName)
    NameOnPCB = UniqueElementName (Data, NameOnPCB);
  AddTextToElement (&NAMEONPCB_TEXT (Element), PCBFont, TextX, TextY,
		    Direction, NameOnPCB, TextScale, TextFlags);
  AddTextToElement (&VALUE_TEXT (Element), PCBFont, TextX, TextY,
		    Direction, Value, TextScale, TextFlags);
  Element->Flags = Flags;
  Element->ID = ID++;
  return (Element);
}

/* ---------------------------------------------------------------------------
 * creates a new arc in an element
 */
ArcTypePtr
CreateNewArcInElement (ElementTypePtr Element,
		       Location X, Location Y,
		       BDimension Width, BDimension Height,
		       int Angle, int Delta, BDimension Thickness)
{
  ArcTypePtr arc = Element->Arc;

  /* realloc new memory if necessary and clear it */
  if (Element->ArcN >= Element->ArcMax)
    {
      Element->ArcMax += STEP_ELEMENTARC;
      arc = MyRealloc (arc, Element->ArcMax * sizeof (ArcType),
		       "CreateNewArcInElement()");
      Element->Arc = arc;
      memset (arc + Element->ArcN, 0, STEP_ELEMENTARC * sizeof (ArcType));
    }

  /* set Delta (0,360], StartAngle in [0,360) */
  if ((Delta = Delta % 360) == 0)
    Delta = 360;
  if (Delta < 0)
    {
      Angle += Delta;
      Delta = -Delta;
    }
  if ((Angle = Angle % 360) < 0)
    Angle += 360;

  /* copy values */
  arc = arc + Element->ArcN++;
  arc->X = X;
  arc->Y = Y;
  arc->Width = Width;
  arc->Height = Height;
  arc->StartAngle = Angle;
  arc->Delta = Delta;
  arc->Thickness = Thickness;
  arc->ID = ID++;
  return (arc);
}

/* ---------------------------------------------------------------------------
 * creates a new line for an element
 */
LineTypePtr
CreateNewLineInElement (ElementTypePtr Element,
			Location X1, Location Y1,
			Location X2, Location Y2, BDimension Thickness)
{
  LineTypePtr line = Element->Line;

  if (Thickness == 0)
    return (NULL);
  /* realloc new memory if necessary and clear it */
  if (Element->LineN >= Element->LineMax)
    {
      Element->LineMax += STEP_ELEMENTLINE;
      line = MyRealloc (line, Element->LineMax * sizeof (LineType),
			"CreateNewLineInElement()");
      Element->Line = line;
      memset (line + Element->LineN, 0, STEP_ELEMENTLINE * sizeof (LineType));
    }

  /* copy values */
  line = line + Element->LineN++;
  line->Point1.X = X1;
  line->Point1.Y = Y1;
  line->Point2.X = X2;
  line->Point2.Y = Y2;
  line->Thickness = Thickness;
  line->ID = ID++;
  return (line);
}

/* ---------------------------------------------------------------------------
 * creates a new pin in an element
 */
PinTypePtr
CreateNewPin (ElementTypePtr Element,
	      Location X, Location Y,
	      BDimension Thickness, BDimension Clearance, BDimension Mask,
	      BDimension DrillingHole, char *Name, char *Number, int Flags)
{
  PinTypePtr pin = GetPinMemory (Element);

  /* copy values */
  pin->X = X;
  pin->Y = Y;
  pin->Thickness = Thickness;
  pin->Clearance = Clearance;
  pin->Mask = Mask;
  pin->DrillingHole = DrillingHole;
  pin->Name = MyStrdup (Name, "CreateNewPin()");
  pin->Number = MyStrdup (Number, "CreateNewPin()");
  pin->Flags = Flags & ~WARNFLAG;
  pin->ID = ID++;
  return (pin);
}

/* ---------------------------------------------------------------------------
 * creates a new pad in an element
 */
PadTypePtr
CreateNewPad (ElementTypePtr Element,
	      Location X1, Location Y1, Location X2, Location Y2,
	      BDimension Thickness, BDimension Clearance, BDimension Mask,
	      char *Name, char *Number, int Flags)
{
  PadTypePtr pad = GetPadMemory (Element);

  /* copy values */
  pad->Point1.X = MIN (X1, X2);	/* works since either X1 == X2 or Y1 == Y2 */
  pad->Point1.Y = MIN (Y1, Y2);
  pad->Point2.X = MAX (X1, X2);
  pad->Point2.Y = MAX (Y1, Y2);
  pad->Thickness = Thickness;
  pad->Clearance = Clearance;
  pad->Mask = Mask;
  pad->Name = MyStrdup (Name, "CreateNewPad()");
  pad->Number = MyStrdup (Number, "CreateNewPad()");
  pad->Flags = Flags & ~WARNFLAG;
  pad->ID = ID++;
  return (pad);
}

/* ---------------------------------------------------------------------------
 * creates a new textobject as part of an element
 * copies the values to the appropriate text object
 */
static void
AddTextToElement (TextTypePtr Text, FontTypePtr PCBFont,
		  Location X, Location Y,
		  BYTE Direction, char *TextString, int Scale, int Flags)
{
  MyFree (&Text->TextString);
  Text->X = X;
  Text->Y = Y;
  Text->Direction = Direction;
  Text->Flags = Flags;
  Text->Scale = Scale;
  Text->TextString = (TextString && *TextString) ?
    MyStrdup (TextString, "AddTextToElement()") : NULL;

  /* calculate size of the bounding box */
  SetTextBoundingBox (PCBFont, Text);
  Text->ID = ID++;
}

/* ---------------------------------------------------------------------------
 * creates a new line in a symbol
 */
LineTypePtr
CreateNewLineInSymbol (SymbolTypePtr Symbol,
		       Location X1, Location Y1,
		       Location X2, Location Y2, BDimension Thickness)
{
  LineTypePtr line = Symbol->Line;

  /* realloc new memory if necessary and clear it */
  if (Symbol->LineN >= Symbol->LineMax)
    {
      Symbol->LineMax += STEP_SYMBOLLINE;
      line = MyRealloc (line, Symbol->LineMax * sizeof (LineType),
			"CreateNewLineInSymbol()");
      Symbol->Line = line;
      memset (line + Symbol->LineN, 0, STEP_SYMBOLLINE * sizeof (LineType));
    }

  /* copy values */
  line = line + Symbol->LineN++;
  line->Point1.X = X1;
  line->Point1.Y = Y1;
  line->Point2.X = X2;
  line->Point2.Y = Y2;
  line->Thickness = Thickness;
  return (line);
}

/* ---------------------------------------------------------------------------
 * parses a file with font information and installs it
 * checks directories given as colon seperated list by resource fontPath
 * if the fonts filename doesn't contain a directory component
 */
void
CreateDefaultFont (void)
{
  if (ParseFont (&PCB->Font, Settings.FontFile))
    Message ("can't find font-symbol-file '%s'\n", Settings.FontFile);
}

/* ---------------------------------------------------------------------------
 * adds a new line to the rubberband list of 'Crosshair.AttachedObject'
 * if Layer == 0  it is a rat line
 */
RubberbandTypePtr
CreateNewRubberbandEntry (LayerTypePtr Layer,
			  LineTypePtr Line, PointTypePtr MovedPoint)
{
  RubberbandTypePtr ptr = GetRubberbandMemory ();

  /* we toggle the RUBBERENDFLAG of the line to determine if */
  /* both points are being moved. */
  Line->Flags ^= RUBBERENDFLAG;
  ptr->Layer = Layer;
  ptr->Line = Line;
  ptr->MovedPoint = MovedPoint;
  return (ptr);
}

/* ---------------------------------------------------------------------------
 * Add a new net to the netlist menu
 */
LibraryMenuTypePtr
CreateNewNet (LibraryTypePtr lib, char *name, char *style)
{
  LibraryMenuTypePtr menu;
  char temp[64];

  sprintf (temp, "  %s", name);
  menu = GetLibraryMenuMemory (lib);
  menu->Name = MyStrdup (temp, "CreateNewNet()");
  if (strcmp ("(unknown)", style) == 0)
    menu->Style = NULL;
  else
    menu->Style = MyStrdup (style, "CreateNewNet()");
  return (menu);
}

/* ---------------------------------------------------------------------------
 * Add a connection to the net
 */
LibraryEntryTypePtr
CreateNewConnection (LibraryMenuTypePtr net, char *conn)
{
  LibraryEntryTypePtr entry = GetLibraryEntryMemory (net);

  entry->ListEntry = MyStrdup (conn, "CreateNewConnection()");
  return (entry);
}
