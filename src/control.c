/* $Id$ */

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

/* control panel
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "global.h"

#include "control.h"
#include "data.h"
#include "draw.h"
#include "output.h"
#include "misc.h"
#include "menu.h"
#include "set.h"

#include <X11/Shell.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Toggle.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID("$Id$");

/* ---------------------------------------------------------------------------
 * include bitmap data
 */
#include "mode_icon.data"

/* ---------------------------------------------------------------------------
 * some local defines
 */
#define	BUTTONS_PER_ROW		2	/* number of mode buttons per row */

/* ---------------------------------------------------------------------------
 * some local types
 */
typedef struct			/* description for a single mode button */
{
  char *Name,			/* the widgets name and label */
   *Label;
  int Mode;			/* mode ID */
  char *Bitmap;			/* background bitmap data */
  unsigned int Width,		/* bitmap size */
    Height;
  Widget W;
}
ModeButtonType, *ModeButtonTypePtr;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static Widget AddLabel (String, Widget, Widget, Widget);
static void AddDrawingLayerSelection (Widget);
static void AddOnOffSelection (Widget);
static void UpdateDrawingLayerSelection (void);
static void UpdateOnOffSelection (void);
static void PushOnTopOfLayerStack (int);
static void CBPOPUP_SetDrawingLayer (Widget, XtPointer, XtPointer);
static void CB_SetOnOff (Widget, XtPointer, XtPointer);
static void CB_SetDrawingLayer (Widget, XtPointer, XtPointer);
static void CB_SetMode (Widget, XtPointer, XtPointer);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Widget OnOffWidgets[MAX_LAYER + 6];	/* widgets for on/off selection */

static PopupEntryType DrawingLayerMenuEntries[MAX_LAYER + 3];
static PopupMenuType DrawingLayerMenu =
  { "drawingLayer", NULL, DrawingLayerMenuEntries,
  CBPOPUP_SetDrawingLayer, NULL, NULL
};
static MenuButtonType DrawingLayerMenuButton =
  { "drawingLayer", "DrawingLayer", &DrawingLayerMenu, NULL };

static ModeButtonType ModeButtons[] = {
  {"viaIcon", "via", VIA_MODE, ViaModeIcon_bits,
   ViaModeIcon_width, ViaModeIcon_height, NULL},
  {"lineIcon", "line", LINE_MODE, LineModeIcon_bits,
   LineModeIcon_width, LineModeIcon_height, NULL},
  {"arcIcon", "arc", ARC_MODE, ArcModeIcon_bits,
   ArcModeIcon_width, ArcModeIcon_height, NULL},
  {"textIcon", "text", TEXT_MODE, TextModeIcon_bits,
   TextModeIcon_width, TextModeIcon_height, NULL},
  {"rectangleIcon", "rectangle", RECTANGLE_MODE, RectModeIcon_bits,
   RectModeIcon_width, RectModeIcon_height, NULL},
  {"polygonIcon", "polygon", POLYGON_MODE, PolygonModeIcon_bits,
   PolygonModeIcon_width, PolygonModeIcon_height, NULL},
  {"bufferIcon", "buffer", PASTEBUFFER_MODE, BufferModeIcon_bits,
   BufferModeIcon_width, BufferModeIcon_height, NULL},
  {"removeIcon", "remove", REMOVE_MODE, RemoveModeIcon_bits,
   RemoveModeIcon_width, RemoveModeIcon_height, NULL},
  {"rotateIcon", "rotate", ROTATE_MODE, RotateModeIcon_bits,
   RotateModeIcon_width, RotateModeIcon_height, NULL},
  {"insertPointIcon", "insertPoint", INSERTPOINT_MODE,
   InsertPointModeIcon_bits, InsertPointModeIcon_width,
   InsertPointModeIcon_height, NULL},
  {"thermalIcon", "thermal", THERMAL_MODE, ThermalModeIcon_bits,
   ThermalModeIcon_width, ThermalModeIcon_height, NULL},
  {"selectIcon", "select", ARROW_MODE, SelModeIcon_bits,
   SelModeIcon_width, SelModeIcon_height, NULL},
  {"panIcon", "pan", PAN_MODE, PanModeIcon_bits,
   PanModeIcon_width, PanModeIcon_height, NULL},
  {"lockIcon", "lock", LOCK_MODE, LockModeIcon_bits,
   LockModeIcon_width, LockModeIcon_height, NULL}
};

/* ---------------------------------------------------------------------------
 * adds a label without border, surrounded by additional space
 * at the specified position
 */
static Widget
AddLabel (String Label, Widget Parent, Widget Top, Widget Left)
{
  Widget label;
  Dimension height;

  label = XtVaCreateManagedWidget ("label", labelWidgetClass,
				   Parent,
				   XtNlabel, Label,
				   XtNfromHoriz, Left,
				   XtNfromVert, Top,
				   XtNjustify, XtJustifyCenter,
				   LAYOUT_TOP, NULL);
  XtVaGetValues (label, XtNheight, &height, NULL);
  XtVaSetValues (label, XtNheight, 3 * height / 2, NULL);
  return (label);
}

/* ---------------------------------------------------------------------------
 * create drawing-layer selection menu
 */
static void
AddDrawingLayerSelection (Widget Parent)
{
  static char name[MAX_LAYER + 2][20];
  long int i;
  Widget label;
  Pixel background;

  /* fill struct and initialize menu button */
  for (i = 0; i < MAX_LAYER + 2; i++)
    {
      /* the widgets name is used for the 'CheckEntry()' funtion
       * and must be declared static therefore
       */
      sprintf (name[i], "layer%-li", i + 1);
      DrawingLayerMenuEntries[i].Name = name[i];
      DrawingLayerMenuEntries[i].Label = name[i];
      DrawingLayerMenuEntries[i].Callback = CB_SetDrawingLayer;
      DrawingLayerMenuEntries[i].ClientData = (XtPointer) i;
    }

  /* init routine exits on NULL pointer */
  DrawingLayerMenuEntries[i].Name = NULL;
  DrawingLayerMenuEntries[i].Label = NULL;
  label = AddLabel ("active", Parent, NULL, NULL);
  InitMenuButton (Parent, &DrawingLayerMenuButton, label, NULL);
  XtVaGetValues (DrawingLayerMenuButton.W, XtNbackground, &background, NULL);
  XtVaSetValues (DrawingLayerMenuButton.W, XtNforeground, background, NULL);
}

/* ---------------------------------------------------------------------------
 * adds the layer On/Off selection buttons to our dialog (downwards)
 * the label for the layer buttons are set by UpdateOnOffSelection()
 */
static void
AddOnOffSelection (Widget Parent)
{
  static char name[MAX_LAYER + 5][20];
  Widget last;
  long int i;
  char *text;
  Pixel color;

  /* insert a label at the top */
  last = AddLabel ("on/off", Parent, NULL, NULL);

  for (i = 0; i < MAX_LAYER + 5; i++)
    {
      switch (i)
	{
	case MAX_LAYER:	/* settings for elements */
	  color = PCB->ElementColor;
	  text = "silk";
	  strcpy (name[i], "elements");
	  break;

	case MAX_LAYER + 1:	/* settings for pins */
	  color = PCB->PinColor;
	  text = "pins/pads";
	  strcpy (name[i], "pins");
	  break;

	case MAX_LAYER + 2:	/* settings for vias */
	  color = PCB->ViaColor;
	  text = "vias";
	  strcpy (name[i], "vias");
	  break;

	case MAX_LAYER + 3:	/* settings for objects on opposite side */
	  color = PCB->InvisibleObjectsColor;
	  text = "far side";
	  strcpy (name[i], "invisible");
	  break;

	case MAX_LAYER + 4:	/* settings for rats */
	  color = PCB->RatColor;
	  text = "rat lines";
	  strcpy (name[i], "rats");
	  break;


	default:		/* layers */
	  color = PCB->Data->Layer[i].Color;
	  text = "";
	  sprintf (name[i], "layer%-li", i + 1);
	  break;
	}

      /* create widget and install callback function */
      OnOffWidgets[i] = XtVaCreateManagedWidget (name[i], toggleWidgetClass,
						 Parent,
						 XtNlabel, text,
						 XtNforeground, color,
						 XtNfromHoriz, NULL,
						 XtNfromVert, last,
						 LAYOUT_TOP, NULL);
      XtAddEventHandler (OnOffWidgets[i], EnterWindowMask, False,
			 CB_StopScroll, NULL);
      last = OnOffWidgets[i];
      XtAddCallback (last, XtNcallback, CB_SetOnOff, (XtPointer) i);
    }
}

/* ---------------------------------------------------------------------------
 * updates the drawing-layer selection menu
 */
static void
UpdateDrawingLayerSelection (void)
{
  int i;

  /* PCB may have changed; we have to make sure that the
   * labels are set correct
   */
  for (i = 0; i < MAX_LAYER; i++)
    XtVaSetValues (DrawingLayerMenuEntries[i].W,
		   XtNlabel, UNKNOWN (PCB->Data->Layer[i].Name),
		   XtNforeground, PCB->Data->Layer[i].Color, NULL);
  XtVaSetValues (DrawingLayerMenuEntries[i].W,
		 XtNlabel, UNKNOWN (PCB->Data->Layer[i].Name),
		 XtNforeground, PCB->ElementColor, NULL);
  i++;
  XtVaSetValues (DrawingLayerMenuEntries[i].W,
		 XtNlabel, "Netlist", XtNforeground, PCB->RatColor, NULL);
  /* set the label of the menu-button itself */
  if (PCB->RatDraw)
    XtVaSetValues (DrawingLayerMenuButton.W,
		   XtNlabel, "Netlist", XtNbackground, PCB->RatColor, NULL);
  else
    XtVaSetValues (DrawingLayerMenuButton.W,
		   XtNlabel, UNKNOWN (CURRENT->Name),
		   XtNbackground, CURRENT->Color, NULL);
}

/* ---------------------------------------------------------------------------
 * updates the layer On/Off selection buttons
 */
static void
UpdateOnOffSelection (void)
{
  int i;

  /* the buttons for elements, vias and pins never
   * change their label, so we only check the layer buttons
   */
  for (i = 0; i < MAX_LAYER; i++)
    XtVaSetValues (OnOffWidgets[i],
		   XtNlabel, UNKNOWN (PCB->Data->Layer[i].Name),
		   XtNstate, PCB->Data->Layer[i].On, NULL);

  /* now set the state of elements, pins and vias */
  XtVaSetValues (OnOffWidgets[i++], XtNstate, PCB->ElementOn, NULL);
  XtVaSetValues (OnOffWidgets[i++], XtNstate, PCB->PinOn, NULL);
  XtVaSetValues (OnOffWidgets[i++], XtNstate, PCB->ViaOn, NULL);
  XtVaSetValues (OnOffWidgets[i++], XtNstate, PCB->InvisibleObjectsOn, NULL);
  XtVaSetValues (OnOffWidgets[i++], XtNstate, PCB->RatOn, NULL);
}

/* ----------------------------------------------------------------------
 * lookup the group to which a layer belongs to
 * returns MAX_LAYER if no group is found
 */
Cardinal
GetGroupOfLayer (Cardinal Layer)
{
  int group, i;

  if (Layer == MAX_LAYER)
    return (Layer);
  for (group = 0; group < MAX_LAYER; group++)
    for (i = 0; i < PCB->LayerGroups.Number[group]; i++)
      if (PCB->LayerGroups.Entries[group][i] == Layer)
	return (group);
  return (MAX_LAYER);
}

/* ----------------------------------------------------------------------
 * changes the visibility of all layers in a group
 * returns the number of changed layers
 */
int
ChangeGroupVisibility (Cardinal Layer, Boolean On, Boolean ChangeStackOrder)
{
  int group, i, changed = 1;	/* at least the current layer changes */

  /* special case of rat (netlist layer) */
  if (Layer == MAX_LAYER + 1)
    {
      PCB->RatOn = On;
      PCB->RatDraw = On && ChangeStackOrder;
      if (PCB->RatDraw)
	SetMode (NO_MODE);
      UpdateOnOffSelection ();
      return (0);
    }
  else
    PCB->RatDraw = (On && ChangeStackOrder) ? False : PCB->RatDraw;

  /* special case of silk layer */
  if (Layer == MAX_LAYER)
    {
      PCB->ElementOn = On;
      PCB->SilkActive = On && ChangeStackOrder;
      PCB->Data->SILKLAYER.On = On;
      PCB->Data->BACKSILKLAYER.On = (On && PCB->InvisibleObjectsOn);
      UpdateOnOffSelection ();
      return (0);
    }
  PCB->SilkActive = (On && ChangeStackOrder) ? False : PCB->SilkActive;

  /* decrement 'i' to keep stack in order of layergroup */
  if ((group = GetGroupOfLayer (Layer)) < MAX_LAYER)
    for (i = PCB->LayerGroups.Number[group]; i;)
      {
	int layer = PCB->LayerGroups.Entries[group][--i];

	/* dont count the passed member of the group */
	if (layer != Layer && layer < MAX_LAYER)
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
  UpdateOnOffSelection ();
  return (changed);
}

/* ---------------------------------------------------------------------------
 * move layer (number is passed in) to top of layerstack
 */
static void
PushOnTopOfLayerStack (int NewTop)
{
  int i;

  /* ignore COMPONENT_LAYER and SOLDER_LAYER */
  if (NewTop < MAX_LAYER)
    {
      /* first find position of passed one */
      for (i = 0; i < MAX_LAYER; i++)
	if (LayerStack[i] == NewTop)
	  break;

      /* bring this element to the top of the stack */
      for (; i; i--)
	LayerStack[i] = LayerStack[i - 1];
      LayerStack[0] = NewTop;
    }
}

/* ---------------------------------------------------------------------------
 * callback routine, called when drawing-layer menu pops up
 */
static void
CBPOPUP_SetDrawingLayer (Widget W, XtPointer ClientData, XtPointer CallData)
{
  char name[10];

  RemoveCheckFromMenu (&DrawingLayerMenu);
  if (PCB->RatDraw)
    sprintf (name, "layer%-i", MAX_LAYER + 2);
  else
    sprintf (name, "layer%-i", MIN (INDEXOFCURRENT, MAX_LAYER) + 1);
  CheckEntry (&DrawingLayerMenu, name);

}

/* ---------------------------------------------------------------------------
 * callback routine, called when current layer is changed
 *
 * this routine is either called from one of the toggle widgets or
 * from the output window by using accelerators
 */
static void
CB_SetDrawingLayer (Widget W, XtPointer ClientData, XtPointer CallData)
{
  LayerTypePtr lay;

  ChangeGroupVisibility ((long int) ClientData, True, True);
  lay = CURRENT;
  if (PCB->RatDraw)
    XtVaSetValues (DrawingLayerMenuButton.W,
		   XtNlabel, "Netlist", XtNbackground, PCB->RatColor, NULL);
  else
    XtVaSetValues (DrawingLayerMenuButton.W,
		   XtNlabel, UNKNOWN (lay->Name),
		   XtNbackground, lay->Color, NULL);
  UpdateAll ();
}

/* ---------------------------------------------------------------------------
 * callback routine, called when On/Off flag of layer is changed
 */
static void
CB_SetOnOff (Widget W, XtPointer ClientData, XtPointer CallData)
{
  Boolean state, Redraw = False;
  long int layer = (long int) ClientData;

  /* get new state of widget */
  XtVaGetValues (W, XtNstate, &state, NULL);

  /* set redraw flag if object exists */
  switch (layer)
    {
    case MAX_LAYER + 1:	/* settings for pins */
      PCB->PinOn = state;
      Redraw |= (PCB->Data->ElementN != 0);
      break;

    case MAX_LAYER + 2:	/* settings for vias */
      PCB->ViaOn = state;
      Redraw |= (PCB->Data->ViaN != 0);
      break;

    case MAX_LAYER + 3:	/* settings for invisible objects */
      PCB->InvisibleObjectsOn = state;
      PCB->Data->BACKSILKLAYER.On = (state && PCB->ElementOn);
      Redraw = True;
      break;

    case MAX_LAYER + 4:	/* settings for rat lines */
      PCB->RatOn = state;
      Redraw |= (PCB->Data->RatN != 0);
      break;

    default:
      {
	int i, group;

	/* check if active layer is in the group;
	 * if YES, make a different one active if possible
	 */
	if ((group = GetGroupOfLayer (layer)) ==
	    GetGroupOfLayer (MIN (MAX_LAYER, INDEXOFCURRENT)))
	  {
	    /* find next visible layer and make it
	     * active so this one can be turned off
	     */
	    for (i = (layer + 1) % (MAX_LAYER + 1);
		 i != layer; i = (i + 1) % (MAX_LAYER + 1))
	      if (PCB->Data->Layer[i].On == True &&
		  GetGroupOfLayer (i) != group)
		break;
	    if (i != layer)
	      {
		ChangeGroupVisibility ((int) i, True, True);
		XtVaSetValues (DrawingLayerMenuButton.W,
			       XtNlabel, UNKNOWN (CURRENT->Name),
			       XtNbackground, CURRENT->Color, NULL);
	      }
	    else
	      {
		/* everything else off, we can't turn this off too */
		XtVaSetValues (W, XtNstate, True, NULL);
		return;
	      }
	  }

	/* switch layer group on/off */
	ChangeGroupVisibility (layer, state, False);
	Redraw = True;
	break;
      }
    }
  if (Redraw)
    UpdateAll ();
}

/* ---------------------------------------------------------------------------
 * initializes dialog box to get settings like
 *   visible layers
 *   drawing layer
 * returns popup shell widget
 */
void
InitControlPanel (Widget Parent, Widget Top, Widget Left)
{
  Widget masterform, visible, drawing;

  masterform = XtVaCreateManagedWidget ("controlMasterForm", formWidgetClass,
					Parent,
					XtNfromHoriz, Left,
					XtNfromVert, Top, LAYOUT_TOP, NULL);
  InitOutputPanner (masterform, NULL, NULL);
  visible = XtVaCreateManagedWidget ("onOffSelection", formWidgetClass,
				     masterform,
				     XtNfromVert, Output.panner,
				     XtNfromHoriz, NULL, NULL);
  drawing = XtVaCreateManagedWidget ("currentSelection", formWidgetClass,
				     masterform,
				     XtNfromVert, visible,
				     XtNfromHoriz, NULL, NULL);

  /* we now add the other widgets to the two forms */
  AddOnOffSelection (visible);
  AddDrawingLayerSelection (drawing);
  Output.Control = masterform;
}

/* ---------------------------------------------------------------------------
 * updates label and sizes in control panel
 */
void
UpdateControlPanel (void)
{
  UpdateOnOffSelection ();
  UpdateDrawingLayerSelection ();
}

/* ---------------------------------------------------------------------------
 * callback routine, called when current mode is changed
 */
static void
CB_SetMode (Widget W, XtPointer ClientData, XtPointer CallData)
{
  ModeButtonTypePtr button;

  /* the radio group is determined by the widget of button 0 */
  button = (ModeButtonTypePtr) XawToggleGetCurrent (ModeButtons[0].W);
  if (button)
    SetMode (button->Mode);
}

/* ---------------------------------------------------------------------------
 * updates the mode selection buttons to reflect the currently set mode
 */
void
UpdateModeSelection (void)
{
  /* reset all buttons if no mode is selected;
   * the first buttons widget identifies the radio group whereas
   * radioData points to the array member itself
   */
  int i;

  for (i = 0; i < ENTRIES (ModeButtons); i++)
    if (Settings.Mode == ModeButtons[i].Mode)
      {
	XawToggleSetCurrent (ModeButtons[0].W, (XtPointer) & ModeButtons[i]);
	break;
      }
}

/* ---------------------------------------------------------------------------
 * initializes mode buttons
 */
Widget
InitModeButtons (Widget Parent, Widget Top, Widget Left)
{
  Widget masterform;
  int i;

#if 0
  masterform = XtVaCreateManagedWidget ("toolMasterForm", formWidgetClass,
					Parent,
					XtNfromHoriz, Left,
					XtNfromVert, Top, LAYOUT_TOP, NULL);
#else
  masterform = Parent;
#endif
  for (i = 0; i < ENTRIES (ModeButtons); i++)
    {
      Pixmap bitmap;

      /* create background pixmap */
      bitmap = XCreateBitmapFromData (Dpy, XtWindow (Parent),
				      ModeButtons[i].Bitmap,
				      ModeButtons[i].Width,
				      ModeButtons[i].Height);

      /* create radio button; use label only if pixmap creation failed
       * layout has to be set here else X11R4 fails
       */
      if (bitmap != BadAlloc)
	ModeButtons[i].W = XtVaCreateManagedWidget (ModeButtons[i].Name,
						    toggleWidgetClass,
						    masterform,
						    XtNbitmap, bitmap,
						    XtNfromHoriz, Left,
						    XtNfromVert, Top,
						    LAYOUT_TOP, NULL);
      else
	ModeButtons[i].W = XtVaCreateManagedWidget (ModeButtons[i].Name,
						    toggleWidgetClass,
						    masterform,
						    XtNlabel,
						    ModeButtons[i].Label,
						    XtNfromHoriz, Left,
						    XtNfromVert, Top,
						    LAYOUT_TOP, NULL);

      XtAddEventHandler (ModeButtons[i].W, EnterWindowMask, False,
			 CB_StopScroll, NULL);
      /* the first entry identifies the radiogroup,
       * we have to make sure that no NULL is passed to XtNradioData
       */
      XtVaSetValues (ModeButtons[i].W,
		     XtNradioGroup, ModeButtons[0].W,
		     XtNradioData, (XtPointer) & ModeButtons[i], NULL);
      XtAddCallback (ModeButtons[i].W, XtNcallback,
		     CB_SetMode, (XtPointer) & ModeButtons[i]);

      /* update position for next widget */
      if (i % BUTTONS_PER_ROW == BUTTONS_PER_ROW - 1)
	{
	  Left = NULL;
	  Top = ModeButtons[i].W;
	}
      else
	Left = ModeButtons[i].W;
    }
  return (ModeButtons[i - 1].W);
}
