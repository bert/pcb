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

/* initializes menus and handles callbacks
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <ctype.h>
#include <sys/types.h>
#include <X11/Intrinsic.h>

#include "global.h"

#include "action.h"
#include "buffer.h"
#include "data.h"
#include "dialog.h"
#include "draw.h"
#include "gui.h"
#include "mymem.h"
#include "menu.h"
#include "misc.h"
#include "output.h"
#include "sizedialog.h"

#include <X11/cursorfont.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * include icon data
 */
#include "check_icon.data"

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void CBPOPUP_Display (Widget, XtPointer, XtPointer);
static void CBPOPUP_Settings (Widget, XtPointer, XtPointer);
static void CBPOPUP_File (Widget, XtPointer, XtPointer);
static void CBPOPUP_Buffer (Widget, XtPointer, XtPointer);
static void CBPOPUP_Sizes (Widget, XtPointer, XtPointer);
static void CBPOPUP_Window (Widget, XtPointer, XtPointer);
static void CB_Action (Widget, XtPointer, XtPointer);
static void CB_Position (Widget, XtPointer, XtPointer);
static void CB_ElementPosition (Widget, XtPointer, XtPointer);
static void CB_TextPosition (Widget, XtPointer, XtPointer);
static void CB_ObjectPosition (Widget, XtPointer, XtPointer);
static void CB_About (Widget, XtPointer, XtPointer);
static void InitPopupTree (Widget, PopupEntryTypePtr);
static void InitPopupMenu (Widget, PopupMenuTypePtr);
static void DoWindows (Widget, XtPointer, XtPointer);
static Widget InitializeSizesMenu (Widget, Widget, Widget);
static Boolean InitCheckPixmap (void);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Pixmap Check = BadAlloc;

/*-----------------------------------------------------------------------------
 * pop up menu for third mouse button
 */
static PopupEntryType pMenuEntries[] = {
  {"header", "Operations on Selections", NULL, NULL, NULL},
  {"unselect", "Unselect All", CB_Action, "Unselect,All", NULL},
  {"remove", "Remove Selected", CB_Action,
   "RemoveSelected", NULL},
  {"copy", "Copy Selection to Buffer", CB_Action,
   "PasteBuffer,Clear\n" "PasteBuffer,AddSelected\n" "Mode,PasteBuffer",
   NULL},
  {"cut", "Cut Selection to Buffer", CB_Action,
   "PasteBuffer,Clear\n"
   "PasteBuffer,AddSelected\n" "RemoveSelected\n" "Mode,PasteBuffer",
   NULL},
  {"convert", "Convert Selection to Element", CB_Action,
   "Select,Convert", NULL},
  {"bash", "break element to pieces", CB_Action,
   "RipUp,Element", NULL},
  {"autoplace", "Auto-place Selected", CB_Action,
   "AutoPlaceSelected", NULL},
  {"autoroute", "Auto-route Selected Rats", CB_Action,
   "AutoRoute,SelectedRats", NULL},
  {"ripup", "Rip-up selected auto-routed tracks", CB_Action,
   "RipUp,Selected", NULL},
  {"header", "Operations on This Location", NULL, NULL, NULL},
  {"hide", "Toggle Name Visibility", CB_Action,
   "ToggleHideName,Selected", NULL},
  {"edit", "Edit Name", CB_Action, "ChangeName,Object", NULL},
  {"report", "Object Report", CB_Action, "Report,Object", NULL},
  {"rotate", "Rotate Object CCW", CB_Action,
   "Mode,Save\n" "Mode,Rotate\n" "Mode,Notify\n" "Mode,Restore", NULL},
  {"rotate", "Rotate Object CW", CB_Action,
   "Mode,Save\n" "Atomic,Save\n" "Mode,Rotate\n" "Mode,Notify\n"
   "Atomic,Restore" "Mode,Notify\n" " Atomic,Restore"
   "Mode,Notify\n" "Atomic,Block" "Mode,Restore", NULL},
  {"flip", "Send To Other Side", CB_Action, "Flip,Object", NULL},
  {"therm", "Toggle Thermal", CB_Action,
   "Mode,Save\n" "Mode,Thermal\n" "Mode,Notify\n" "Mode,Restore", NULL},
  {"lookup", "Lookup Connections", CB_Action,
   "Connection,Find", NULL},
#if 0
  {"mark", "Position reference here", CB_Action,
   "MarkCrosshair,Center", NULL},
#endif
  {"line", NULL, NULL, NULL, NULL},
  {"undo", "Undo Last Operation", CB_Action, "Undo", NULL},
  {"redo", "Redo Last Undone Operation", CB_Action, "Redo", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"lmode", "Line Tool", CB_Action, "Mode,Line", NULL},
  {"vmode", "Via Tool", CB_Action, "Mode,Via", NULL},
  {"rmode", "Rectangle Tool", CB_Action, "Mode,Rectangle", NULL},
  {"amode", "Selection Tool", CB_Action, "Mode,Arrow", NULL},
  {"tmode", "Text Tool", CB_Action, "Mode,Text", NULL},
  {"nmode", "Panner Tool", CB_Action, "Mode,None", NULL},
  {NULL, NULL, NULL, NULL, NULL}
};
static PopupMenuType pMenu =
  { "pmenu", NULL, pMenuEntries, NULL, NULL, NULL };
static PopupEntryType p2MenuEntries[] = {
  {"nmode", "No Tool", CB_Action, "Mode,None", NULL},
  {NULL, NULL, NULL, NULL, NULL}
};
static PopupMenuType p2Menu =
  { "p2menu", NULL, p2MenuEntries, NULL, NULL, NULL };

/* ---------------------------------------------------------------------------
 * file menu button
 */
static PopupEntryType FileMenuEntries[] = {
  {"about", "About...", CB_About, NULL, NULL},
  {"save", "save layout", CB_Action, "Save,Layout", NULL},
  {"saveas", "save layout as...", CB_Action, "Save,LayoutAs", NULL},
  {"load", "load layout", CB_Action, "Load,Layout", NULL},
  {"loadelement", "load element data to paste-buffer", CB_Action,
   "PasteBuffer,Clear\n" "Load,ElementToBuffer", NULL},
  {"loadlayout", "load layout data to paste-buffer", CB_Action,
   "PasteBuffer,Clear\n" "Load,LayoutToBuffer", NULL},
  {"loadnetlist", "load netlist file", CB_Action, "Load,Netlist", NULL},
  {"print", "print layout...", CB_Action, "Print", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"header", "save connection data of", NULL, NULL, NULL},
  {"savesingle", "a single element", CB_ElementPosition,
   "Save,ElementConnections", NULL},
  {"saveall", "all elements", CB_Action, "Save,AllConnections", NULL},
  {"saveunused", "unused pins", CB_Action, "Save,AllUnusedPins", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"new", "start new layout", CB_Action, "New", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"quit", "quit program", CB_Action, "Quit", NULL},
  {NULL, NULL, NULL, NULL, NULL}
};
static PopupMenuType FileMenu =
  { "file", NULL, FileMenuEntries, CBPOPUP_File, NULL, NULL };
static MenuButtonType FileMenuButton = { "file", "File", &FileMenu, NULL };

/* ---------------------------------------------------------------------------
 * edit menu button
 */
static PopupEntryType EditMenuEntries[] = {
  {"undo", "undo last operation", CB_Action, "Undo", NULL},
  {"redo", "redo last undone operation", CB_Action, "Redo", NULL},
  {"clear", "clear undo-buffer", CB_Action, "Undo,ClearList", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"cut", "cut selection to buffer", CB_Position,
   "PasteBuffer,Clear\n"
   "PasteBuffer,AddSelected\n" "RemoveSelected\n" "Mode,PasteBuffer",
   NULL},
  {"copy", "copy selection to buffer", CB_Position,
   "PasteBuffer,Clear\n" "PasteBuffer,AddSelected\n" "Mode,PasteBuffer",
   NULL},
  {"paste", "paste buffer to layout", CB_Action, "Mode,PasteBuffer", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"unselect", "unselect all", CB_Action, "Unselect,All", NULL},
  {"select", "select all", CB_Action, "Select,All", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"header", "Edit Names", NULL, NULL, NULL},
  {"edit", "edit text on layout", CB_TextPosition, "ChangeName,Object", NULL},
  {"layoutname", "edit name of layout", CB_Action, "ChangeName,Layout", NULL},

  {"layername", "edit name of active layer", CB_Action, "ChangeName,Layer",
   NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"boardSize", "change board size...", CB_Action, "AdjustStyle,0", NULL},
  {NULL, NULL, NULL, NULL, NULL}
};
static PopupMenuType EditMenu =
  { "edit", NULL, EditMenuEntries, NULL, NULL, NULL };
static MenuButtonType EditMenuButton = { "edits", "Edit", &EditMenu, NULL };

/* ---------------------------------------------------------------------------
 * display menu button
 */
static PopupEntryType DisplayMenuEntries[] = {
  {"redraw", "redraw layout", CB_Action, "Display,Redraw", NULL},
  {"center", "center layout", CB_Position, "Display,Center", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"displayGrid", "display grid", CB_Action, "Display,Grid", NULL},
  {"toggleGrid", "realign grid", CB_Position, "Display,ToggleGrid", NULL},
  {"solderSide", "view solder side", CB_Position, "SwapSides", NULL},
  {"showMask", "show soldermask", CB_Action, "Display,ToggleMask", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"header", "grid setting", NULL, NULL, NULL},
  {"grid01", " 1 mil", CB_Action, "SetValue,Grid,100", NULL},
  {"grid.1mm", "0.1 mm", CB_Action, "SetValue,Grid,393.7007874", NULL},
  {"grid10", "10 mil", CB_Action, "SetValue,Grid,1000", NULL},
  {"grid1mm", "1.0 mm", CB_Action, "SetValue,Grid,3937.007874", NULL},
  {"grid25", "25 mil", CB_Action, "SetValue,Grid,2500", NULL},
  {"grid50", "50 mil", CB_Action, "SetValue,Grid,5000", NULL},
  {"grid100", "100 mil", CB_Action, "SetValue,Grid,10000", NULL},
  {"gridInc", "increment by 5 mil", CB_Action, "SetValue,Grid,+500", NULL},
  {"gridDec", "decrement by 5 mil", CB_Action, "SetValue,Grid,-500", NULL},

  {"gridIncmm", "increment by 0.1 mm", CB_Action,
   "SetValue,Grid,+393.7007874", NULL},
  {"gridDecmm", "decrement by 0.1 mm", CB_Action,
   "SetValue,Grid,-393.7007874", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"header", "zoom setting", NULL, NULL, NULL},
  {"zoom25", "4 : 1 ", CB_Position, "SetValue,Zoom,=-4", NULL},
  {"zoom5", "2 : 1 ", CB_Position, "SetValue,Zoom,=-2", NULL},
  {"zoom1", "1 : 1 ", CB_Position, "SetValue,Zoom,0", NULL},
  {"zoom2", "1 : 2 ", CB_Position, "SetValue,Zoom,2", NULL},
  {"zoom4", "1 : 4 ", CB_Position, "SetValue,Zoom,4", NULL},
  {"zoom8", "1 : 8 ", CB_Position, "SetValue,Zoom,6", NULL},
  {"zoom16", "1 :16 ", CB_Position, "SetValue,Zoom,8", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"header", "displayed element-name", NULL, NULL, NULL},
  {"description", "description", CB_Action, "Display,Description", NULL},
  {"onPCB", "name on PCB", CB_Action, "Display,NameOnPCB", NULL},
  {"value", "value", CB_Action, "Display,Value", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"pinnum", "pinout shows number", CB_Action, "Display,ToggleName", NULL},
  {"pinout", "open pinout window", CB_ElementPosition, "Display,Pinout",
   NULL},
  {NULL, NULL, NULL, NULL, NULL}
};
static PopupMenuType DisplayMenu =
  { "display", NULL, DisplayMenuEntries, CBPOPUP_Display, NULL, NULL };
static MenuButtonType DisplayMenuButton =
  { "display", "Screen", &DisplayMenu, NULL };

/* ----------------------------------------------------------------------
 * Sizes menu button - contains variable contents
 */
static PopupEntryType SizesMenuEntries[NUM_STYLES * 2 + 2];
static PopupMenuType SizesMenu =
  { "sizes", NULL, SizesMenuEntries, CBPOPUP_Sizes, NULL, NULL };
static MenuButtonType SizesMenuButton =
  { "sizes", "Sizes", &SizesMenu, NULL };

/* ---------------------------------------------------------------------------
 * settings menu button
 */
static PopupEntryType SettingsMenuEntries[] = {
  {"header", "layer-groups", NULL, NULL, NULL},
  {"lgedit", "edit layer-groupings", CB_Action, "EditLayerGroups", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"toggleAllDirections", "'all-direction' lines", CB_Action,
   "Display,Toggle45Degree", NULL},
  {"toggleRubberBandMode", "rubber band mode", CB_Action,
   "Display,ToggleRubberBandMode", NULL},
  {"toggleSwapStartDirection", "auto swap line start angle", CB_Action,
   "Display,ToggleStartDirection", NULL},
  {"toggleUniqueName", "require unique element names", CB_Action,
   "Display,ToggleUniqueNames", NULL},
  {"toggleSnapPin", "crosshair snaps to pins and pads", CB_Action,
   "Display,ToggleSnapPin", NULL},
  {"toggleLocalRef", "auto-zero delta measurements", CB_Action,
   "Display,ToggleLocalRef", NULL},
  {"toggleClearLine", "new lines, arcs clear polygons", CB_Action,
   "Display,ToggleClearLine", NULL},
  {"toggleThindraw", "thin draw", CB_Action,
   "Display,ToggleThindraw", NULL},
  {"toggleOrthoMove", "orthogonal moves", CB_Action,
   "Display,ToggleOrthoMove", NULL},
  {"toggleCheckPlanes", "check polygons", CB_Action,
   "Display,ToggleCheckPlanes", NULL},
  {NULL, NULL, NULL, NULL, NULL}
};
static PopupMenuType SettingsMenu =
  { "settings", NULL, SettingsMenuEntries, CBPOPUP_Settings, NULL, NULL };
static MenuButtonType SettingsMenuButton =
  { "settings", "Settings", &SettingsMenu, NULL };

/* ---------------------------------------------------------------------------
 * selection menu button
 */
static PopupEntryType SelectionMenuEntries[] = {
  {"select", "select all objects", CB_Action, "Select,All", NULL},
  {"selectconnection", "select all connected objects", CB_Action,
   "Select,Connection", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"unselect", "unselect all objects", CB_Action, "Unselect,All", NULL},
  {"unselectconnection", "unselect all connected objects", CB_Action,
   "Unselect,Connection", NULL},
#ifdef HAS_REGEX
  {"line", NULL, NULL, NULL, NULL},
  {"header", "select by name", NULL, NULL, NULL},
  {"allByName", "all objects", CB_Action, "Select,ObjectByName", NULL},
  {"elementByName", "elements", CB_Action, "Select,ElementByName", NULL},
  {"padByName", "pads", CB_Action, "Select,PadByName", NULL},
  {"pinByName", "pins", CB_Action, "Select,PinByName", NULL},
  {"textByName", "text objects", CB_Action, "Select,TextByName", NULL},
  {"viaByName", "vias", CB_Action, "Select,ViaByName", NULL},
#endif
  {"line", NULL, NULL, NULL, NULL},
  {"autoplace", "auto-place selected elements", CB_Action,
   "AutoPlaceSelected", NULL},
  {"flip", "move selected elements to other side", CB_Action,
   "Flip,SelectedElements", NULL},
  {"remove", "delete selected objects", CB_Action,
   "RemoveSelected", NULL},
  {"conv", "convert selection to element", CB_Position,
   "Select,Convert", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"optrats", "optimize selected rats", CB_Action,
   "DeleteRats,SelectedRats\n" "AddRats,SelectedRats", NULL},
  {"autoroute", "auto-route selected rats", CB_Action,
   "AutoRoute,SelectedRats", NULL},
  {"ripup", "Rip-up selected auto-routed tracks", CB_Action,
   "RipUp,Selected", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"header", "change size of selected objects", NULL, NULL, NULL},
  {"decrementline", "lines -10 mil", CB_Action,
   "ChangeSize,SelectedLines,-10,mil", NULL},
  {"incrementline", "lines +10 mil", CB_Action,
   "ChangeSize,SelectedLines,+10,mil", NULL},
  {"decrementpad", "pads -10 mil", CB_Action,
   "ChangeSize,SelectedPads,-10,mil", NULL},
  {"incrementpad", "pads +10 mil", CB_Action,
   "ChangeSize,SelectedPads,+10,mil", NULL},
  {"decrementpin", "pins -10 mil", CB_Action,
   "ChangeSize,SelectedPins,-10,mil", NULL},
  {"incrementpin", "pins +10 mil", CB_Action,
   "ChangeSize,SelectedPins,+10,mil", NULL},
  {"decrementtext", "texts -10 mil", CB_Action,
   "ChangeSize,SelectedTexts,-10,mil", NULL},
  {"incrementtext", "texts +10 mil", CB_Action,
   "ChangeSize,SelectedTexts,+10,mil", NULL},
  {"decrementvia", "vias -10 mil", CB_Action,
   "ChangeSize,SelectedVias,-10,mil", NULL},
  {"incrementvia", "vias +10 mil", CB_Action,
   "ChangeSize,SelectedVias,+10,mil", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"header", "change drilling hole of selected objects", NULL, NULL, NULL},
  {"decrementviahole", "vias -10 mil", CB_Action,
   "ChangeDrillSize,SelectedVias,-10,mil", NULL},
  {"incrementviahole", "vias +10 mil", CB_Action,
   "ChangeDrillSize,SelectedVias,+10,mil", NULL},
  {"decrementpinhole", "pins -10 mil", CB_Action,
   "ChangeDrillSize,SelectedPins,-10,mil", NULL},
  {"incrementpinhole", "pins +10 mil", CB_Action,
   "ChangeDrillSize,SelectedPins,+10,mil", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"header", "change square-flag of selected objects", NULL, NULL, NULL},
  {"elementsquare", "elements", CB_Action,
   "ChangeSquare,SelectedElements", NULL},
  {"pinsquare", "pins", CB_Action,
   "ChangeSquare,SelectedPins", NULL},
  {NULL, NULL, NULL, NULL, NULL}
};
static PopupMenuType SelectionMenu =
  { "selection", NULL, SelectionMenuEntries, NULL, NULL, NULL };
static MenuButtonType SelectionMenuButton =
  { "selection", "Select", &SelectionMenu, NULL };

/* ---------------------------------------------------------------------------
 * paste buffer menu button
 */
static PopupEntryType BufferMenuEntries[] = {
  {"copy", "copy selection to buffer", CB_Position,
   "PasteBuffer,Clear\n" "PasteBuffer,AddSelected\n" "Mode,PasteBuffer",
   NULL},
  {"cut", "cut selection to buffer", CB_Position,
   "PasteBuffer,Clear\n"
   "PasteBuffer,AddSelected\n" "RemoveSelected\n" "Mode,PasteBuffer",
   NULL},
  {"paste", "paste buffer to layout", CB_Action, "Mode,PasteBuffer", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"rotate", "rotate buffer 90 deg CCW", CB_Action,
   "Mode,PasteBuffer\n" "PasteBuffer,Rotate,1",
   NULL},
  {"rotate", "rotate buffer 90 deg CW", CB_Action,
   "Mode,PasteBuffer\n" "PasteBuffer,Rotate,3",
   NULL},
  {"mirror", "Mirror Buffer (up/down)", CB_Action,
   "Mode,PasteBuffer\n" "PasteBuffer,Mirror",
   NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"clear", "clear buffer", CB_Action, "PasteBuffer,Clear", NULL},

  {"convert", "convert buffer to element", CB_Action, "PasteBuffer,Convert",
   NULL},
  {"smash", "break buffer element to pieces", CB_Action,
   "PasteBuffer,Restore", NULL},
  {"saveb", "save buffer elements to file", CB_Action, "PasteBuffer,Save",
   NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"header", "select current buffer", NULL, NULL, NULL},
  {"buffer1", "#1", CB_Action, "PasteBuffer,1", NULL},
  {"buffer2", "#2", CB_Action, "PasteBuffer,2", NULL},
  {"buffer3", "#3", CB_Action, "PasteBuffer,3", NULL},
  {"buffer4", "#4", CB_Action, "PasteBuffer,4", NULL},
  {"buffer5", "#5", CB_Action, "PasteBuffer,5", NULL},
  {NULL, NULL, NULL, NULL, NULL}
};
static PopupMenuType BufferMenu =
  { "buffer", NULL, BufferMenuEntries, CBPOPUP_Buffer, NULL, NULL };
static MenuButtonType BufferMenuButton =
  { "buffer", "Buffer", &BufferMenu, NULL };

/* ---------------------------------------------------------------------------
 * connection menu button
 */
static PopupEntryType ConnectionMenuEntries[] = {
  {"lookup", "lookup connection to object", CB_ObjectPosition,
   "Connection,Find", NULL},
  {"resetPVP", "reset scanned pads/pins/vias", CB_Action,
   "Connection,ResetPinsViasAndPads\n" "Display,Redraw", NULL},
  {"resetLR", "reset scanned lines/polygons", CB_Action,
   "Connection,ResetLinesAndPolygons\n" "Display,Redraw", NULL},
  {"reset", "reset all connections", CB_Action,
   "Connection,Reset\n" "Display,Redraw", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"ratsnest", "optimize rats-nest", CB_Action,
   "DeleteRats,AllRats\n" "AddRats,AllRats", NULL},
  {"eraseRats", "erase rats-nest", CB_Action,
   "DeleteRats,AllRats", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"autoroute", "auto-route selected rats", CB_Action,
   "AutoRoute,Selected", NULL},
  {"autoroute", "auto-route all rats", CB_Action,
   "AutoRoute,AllRats", NULL},
  {"ripup", "rip up all auto-routed tracks", CB_Action,
   "RipUp,All", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"djopt-auto", "Auto-Optimize", CB_Action,
   "djopt,auto\n" "Display,ClearAndRedraw", NULL},
  {"djopt-bump", "Debumpify", CB_Action,
   "djopt,debumpify\n" "Display,ClearAndRedraw", NULL},
  {"djopt-unjaggy", "Unjaggy", CB_Action,
   "djopt,unjaggy\n" "Display,ClearAndRedraw", NULL},
  {"djopt-vianudge", "Vianudge", CB_Action,
   "djopt,vianudge\n" "Display,ClearAndRedraw", NULL},
  {"djopt-viatrim", "Viatrim", CB_Action,
   "djopt,viatrim\n" "Display,ClearAndRedraw", NULL},
  {"djopt-orthopull", "Orthopull", CB_Action,
   "djopt,orthopull\n" "Display,ClearAndRedraw", NULL},
  {"djopt-simple", "SimpleOpts", CB_Action,
   "djopt,simple\n" "Display,ClearAndRedraw", NULL},
  {"djopt-miter", "Miter", CB_Action,
   "djopt,miter\n" "Display,ClearAndRedraw", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"drc", "design rule checker", CB_Action, "DRC", NULL},
  {NULL, NULL, NULL, NULL, NULL}
};

static PopupMenuType ConnectionMenu =
  { "connections", NULL, ConnectionMenuEntries, NULL, NULL, NULL };

static MenuButtonType ConnectionMenuButton =
  { "connections", "Connects", &ConnectionMenu, NULL };

/* ---------------------------------------------------------------------------
 * report menu button
 */
static PopupEntryType ReportMenuEntries[] = {
  {"report", "generate object report", CB_Position, "Report,Object", NULL},

  {"drillreport", "generate drill summary", CB_Action, "Report,DrillReport",
   NULL},
  {"foundreport", "report found pins/pads", CB_Action, "Report,FoundPins",
   NULL},
  {NULL, NULL, NULL, NULL, NULL}
};
static PopupMenuType ReportMenu =
  { "report", NULL, ReportMenuEntries, NULL, NULL, NULL };
static MenuButtonType ReportMenuButton =
  { "report", "Info", &ReportMenu, NULL };

/* ---------------------------------------------------------------------------
 * windows menu button
 */
static PopupEntryType WindowMenuEntries[] = {
  {"layout", "board layout", DoWindows, "1", NULL},
  {"library", "library", DoWindows, "2", NULL},
  {"log", "message log", DoWindows, "3", NULL},
  {"netlist", "netlist", DoWindows, "4", NULL},
  {NULL, NULL, NULL, NULL, NULL}
};
static PopupMenuType WindowMenu =
  { "window", NULL, WindowMenuEntries, CBPOPUP_Window, NULL, NULL };
static MenuButtonType WindowMenuButton =
  { "window", "Window", &WindowMenu, NULL };

static void
FillSizesMenu (void)
{
  static char name[NUM_STYLES * 2 + 1][8];
  static char label[NUM_STYLES * 2 + 1][64];
  static char action[NUM_STYLES * 2 + 1][16];
  int i;

  STYLE_LOOP (PCB, 
    {
      sprintf (name[n], "size%d", n + 1);
      sprintf (label[n], "use '%s' routing style", style->Name);
      sprintf (action[n], "RouteStyle,%d", n + 1);
      SizesMenuEntries[n].Name = name[n];
      SizesMenuEntries[n].Label = label[n];
      SizesMenuEntries[n].Callback = CB_Action;
      SizesMenuEntries[n].ClientData = (XtPointer) action[n];
    }
  );
  for (i = NUM_STYLES; i < 2 * NUM_STYLES; i++)
    {
      sprintf (name[i], "size%d", i + 1);
      sprintf (label[i], "adjust '%s' sizes ...",
	       PCB->RouteStyle[i - NUM_STYLES].Name);
      sprintf (action[i], "AdjustStyle,%d", i - NUM_STYLES + 1);
      SizesMenuEntries[i].Name = name[i];
      SizesMenuEntries[i].Label = label[i];
      SizesMenuEntries[i].Callback = CB_Action;
      SizesMenuEntries[i].ClientData = (XtPointer) action[i];
    }
  sprintf (name[i], "size%d", i + 1);
  sprintf (label[i], "adjust active sizes ...");
  sprintf (action[i], "AdjustStyle,0");
  SizesMenuEntries[i].Name = name[i];
  SizesMenuEntries[i].Label = label[i];
  SizesMenuEntries[i].Callback = CB_Action;
  SizesMenuEntries[i].ClientData = (XtPointer) action[i];
  i++;
  SizesMenuEntries[i].Name = NULL;
  SizesMenuEntries[i].Label = NULL;
}

void
UpdateSizesMenu (void)
{
  int i;

  FillSizesMenu ();
  for (i = 0; i < 2 * NUM_STYLES + 1; i++)
    XtVaSetValues (SizesMenuEntries[i].W, XtNlabel, SizesMenuEntries[i].Label,
		   NULL);
}

/* ----------------------------------------------------------------------
 * Initializes the sizes menu with the routing style names etc.
 */
static Widget
InitializeSizesMenu (Widget Parent, Widget Top, Widget Last)
{
  FillSizesMenu ();
  return (InitMenuButton (Parent, &SizesMenuButton, Top, Last));
}

/* ----------------------------------------------------------------------
 * menu callback for window menu
 */
static void
DoWindows (Widget W, XtPointer ClientData, XtPointer CallData)
{
  int n = atoi ((char *) ClientData);

  switch (n)
    {
    case 1:
      XRaiseWindow (Dpy, XtWindow (Output.Toplevel));
      break;
    case 2:
      XMapRaised (Dpy, Library.Wind);
      break;
    case 3:
      if (Settings.UseLogWindow)
	XMapRaised (Dpy, LogWindID);
      break;
    case 4:
      if (PCB->NetlistLib.Wind)
	XMapRaised (Dpy, PCB->NetlistLib.Wind);
      break;
    }
}


/* ----------------------------------------------------------------------
 * menu callback interface for actions routines that don't need
 * position information
 *
 * ClientData passes a pointer to a comma seperated list of arguments.
 * The first one determines the action routine to be called, the
 * rest of them are arguments to the action routine
 *
 * if more than one action is to be called a new list is seperated
 * by '\n'
 */
static void
CB_Action (Widget W, XtPointer ClientData, XtPointer CallData)
{
  static char **array = NULL;
  static size_t number = 0;
  int n;
  char *copy, *current, *next, *token;

  /* get a copy of the string and split it */
  copy = MyStrdup ((char *) ClientData, "CB_CallActionWithoutPosition()");

  /* first loop over all action routines;
   * strtok cannot be used in nested loops because it saves
   * a pointer in a private data area which would be corrupted
   * by the inner loop
   */
  for (current = copy; current; current = next)
    {
      /* lookup seperating '\n' character;
       * update pointer if not at the end of the string
       */
      for (next = current; *next && *next != '\n'; next++);
      if (*next)
	{
	  *next = '\0';
	  next++;
	}
      else
	next = NULL;

      token = strtok (current, ",");
      for (n = 0; token; token = strtok (NULL, ","), n++)
	{
	  /* allocate memory if necessary */
	  if (n >= number)
	    {
	      number += 10;
	      array = MyRealloc (array, number * sizeof (char *),
				 "CB_CallActionWithoutPosition()");
	    }
	  array[n] = token;
	}
      /* call action routine */
      CallActionProc (Output.Output, array[0], NULL, &array[1], n - 1);
    }

  /* release memory */
  SaveFree (copy);
}

/* ----------------------------------------------------------------------
 * menu callback interface for misc actions routines that need
 * position information
 */
static void
CB_Position (Widget W, XtPointer ClientData, XtPointer CallData)
{
  if (GetLocation
      ("Move the pointer to the appropriate screen position and press a button."))
    CB_Action (W, ClientData, CallData);
}

/* ----------------------------------------------------------------------
 * menu callback interface for element related actions routines that need
 * position information
 */
static void
CB_ElementPosition (Widget W, XtPointer ClientData, XtPointer CallData)
{
  if (GetLocation ("press a button at the element's location"))
    CB_Action (W, ClientData, CallData);
}

/* ----------------------------------------------------------------------
 * menu callback interface for text related actions routines that need
 * position information
 */
static void
CB_TextPosition (Widget W, XtPointer ClientData, XtPointer CallData)
{
  if (GetLocation ("press a button at the text location"))
    CB_Action (W, ClientData, CallData);
}

/* ----------------------------------------------------------------------
 * menu callback interface for pin/via related actions routines that need
 * position information update
 */
static void
CB_ObjectPosition (Widget W, XtPointer ClientData, XtPointer CallData)
{
  if (GetLocation ("press a button at a 'connecting-objects' location"))
    CB_Action (W, ClientData, CallData);
}

/* ---------------------------------------------------------------------- 
 * called before sizes menu is popped up
 * used to mark the current route style 
 */
static void
CBPOPUP_Sizes (Widget W, XtPointer ClientData, XtPointer CallData)
{
  char menuname[5];

  RemoveCheckFromMenu (&SizesMenu);
  STYLE_LOOP (PCB, 
    {
      if (style->Thick == Settings.LineThickness &&
	  style->Diameter == Settings.ViaThickness &&
	  style->Hole == Settings.ViaDrillingHole)
	{
	  sprintf (menuname, "size%d", n + 1);
	  CheckEntry (&SizesMenu, menuname);
	  break;
	}
    }
  );
}

/* ---------------------------------------------------------------------- 
 * called before display menu is popped up
 * used to mark the current grid-mode, zoom value ...
 */
static void
CBPOPUP_Display (Widget W, XtPointer ClientData, XtPointer CallData)
{
  int zoom;
  RemoveCheckFromMenu (&DisplayMenu);
  XtSetSensitive (XtNameToWidget (DisplayMenu.W, "displayGrid"),
		  GetGridFactor () != 0);
  if (Settings.DrawGrid)
    CheckEntry (&DisplayMenu, "displayGrid");
  if (TEST_FLAG (SHOWMASKFLAG, PCB))
    CheckEntry (&DisplayMenu, "showMask");
  if (Settings.ShowSolderSide)
    CheckEntry (&DisplayMenu, "solderSide");
  zoom = (int)(PCB->Zoom + 0.5);
  if (abs(PCB->Zoom - zoom) > 0.1)
    zoom = -8; /* not close enough to integer value */
  switch (zoom)
    {
    case -4:
      CheckEntry (&DisplayMenu, "zoom25");
      break;
    case -2:
      CheckEntry (&DisplayMenu, "zoom5");
      break;
    case 0:
      CheckEntry (&DisplayMenu, "zoom1");
      break;
    case 2:
      CheckEntry (&DisplayMenu, "zoom2");
      break;
    case 4:
      CheckEntry (&DisplayMenu, "zoom4");
      break;
    case 6:
      CheckEntry (&DisplayMenu, "zoom8");
      break;
    case 8:
      CheckEntry (&DisplayMenu, "zoom16");
      break;
    default:
      break;
    }
  CheckEntry (&DisplayMenu,
	      TEST_FLAG (NAMEONPCBFLAG, PCB) ? "onPCB" :
	      TEST_FLAG (DESCRIPTIONFLAG, PCB) ? "description" : "value");
  if (TEST_FLAG (SHOWNUMBERFLAG, PCB))
    CheckEntry (&DisplayMenu, "pinnum");
}

/* ---------------------------------------------------------------------- 
 * called before settings menu is popped up
 * used to mark the boolean settings
 */
static void
CBPOPUP_Settings (Widget W, XtPointer ClientData, XtPointer CallData)
{
  RemoveCheckFromMenu (&SettingsMenu);
  if (TEST_FLAG (ALLDIRECTIONFLAG, PCB))
    CheckEntry (&SettingsMenu, "toggleAllDirections");
  if (TEST_FLAG (RUBBERBANDFLAG, PCB))
    CheckEntry (&SettingsMenu, "toggleRubberBandMode");
  if (TEST_FLAG (SWAPSTARTDIRFLAG, PCB))
    CheckEntry (&SettingsMenu, "toggleSwapStartDirection");
  if (TEST_FLAG (UNIQUENAMEFLAG, PCB))
    CheckEntry (&SettingsMenu, "toggleUniqueName");
  if (TEST_FLAG (SNAPPINFLAG, PCB))
    CheckEntry (&SettingsMenu, "toggleSnapPin");
  if (TEST_FLAG (LOCALREFFLAG, PCB))
    CheckEntry (&SettingsMenu, "toggleLocalRef");
  if (TEST_FLAG (CLEARNEWFLAG, PCB))
    CheckEntry (&SettingsMenu, "toggleClearLine");
  if (TEST_FLAG (THINDRAWFLAG, PCB))
    CheckEntry (&SettingsMenu, "toggleThindraw");
  if (TEST_FLAG (CHECKPLANESFLAG, PCB))
    CheckEntry (&SettingsMenu, "toggleCheckPlanes");
  if (TEST_FLAG (ORTHOMOVEFLAG, PCB))
    CheckEntry (&SettingsMenu, "toggleOrthoMove");
}


/* ---------------------------------------------------------------------- 
 * called before file menu is popped up
 * enables/disables printing
 */
static void
CBPOPUP_File (Widget W, XtPointer ClientData, XtPointer CallData)
{
  XtSetSensitive (XtNameToWidget (FileMenu.W, "print"),
		  !IsDataEmpty (PCB->Data));
}

/* ---------------------------------------------------------------------- 
 * called before buffer menu is popped up
 */
static void
CBPOPUP_Buffer (Widget W, XtPointer ClientData, XtPointer CallData)
{
  char name[10];

  RemoveCheckFromMenu (&BufferMenu);
  sprintf (name, "buffer%i", Settings.BufferNumber + 1);
  CheckEntry (&BufferMenu, name);
}

/* ---------------------------------------------------------------------- 
 * called before window menu is popped up
 * enables/disables log window 
 */
static void
CBPOPUP_Window (Widget W, XtPointer ClientData, XtPointer CallData)
{
  XtSetSensitive (XtNameToWidget (WindowMenu.W, "log"),
		  Settings.UseLogWindow);
  XtSetSensitive (XtNameToWidget (WindowMenu.W, "netlist"),
		  PCB->NetlistLib.Wind != 0);
}


/* ---------------------------------------------------------------------- 
 * callback routine used by about entry
 */
static void
CB_About (Widget W, XtPointer ClientData, XtPointer CallData)
{
  AboutDialog ();
}

/* ---------------------------------------------------------------------------
 * remove all 'check' symbols from menu entries
 */
void
RemoveCheckFromMenu (PopupMenuTypePtr MenuPtr)
{
  PopupEntryTypePtr entries = MenuPtr->Entries;

  for (; entries->Name; entries++)
    if (entries->Label)
      XtVaSetValues (entries->W, XtNleftBitmap, None, NULL);
}

/* ---------------------------------------------------------------------------
 * add 'check' symbol to menu entry
 */
void
CheckEntry (PopupMenuTypePtr MenuPtr, String WidgetName)
{
  PopupEntryTypePtr entries = MenuPtr->Entries;

  if (InitCheckPixmap ())
    for (; entries->Name; entries++)
      if (entries->Label && !strcmp (entries->Name, (char *) WidgetName))
	{
	  XtVaSetValues (entries->W, XtNleftBitmap, Check, NULL);
	  return;
	}
}

/* ---------------------------------------------------------------------------
 * initializes a menu tree
 * depending on the 'Name' field of the struct either a smeBSB or a
 * smeLine widget is created. If a callback routine is defined for the
 * smeBSB widget it will be registered else the entry will be disabled
 */
static void
InitPopupTree (Widget Parent, PopupEntryTypePtr EntryPtr)
{
  for (; EntryPtr->Name; EntryPtr++)
    {
      /* check if it's only a seperator */
      if (EntryPtr->Label)
	{
	  EntryPtr->W =
	    XtVaCreateManagedWidget (EntryPtr->Name, smeBSBObjectClass,
				     Parent, XtNlabel, EntryPtr->Label,
				     XtNleftMargin, 12, XtNsensitive, True,
				     NULL);
	  if (EntryPtr->Callback)
	    XtAddCallback (EntryPtr->W, XtNcallback,
			   EntryPtr->Callback,
			   (XtPointer) EntryPtr->ClientData);
	  else
	    /* entry is not selectable */
	    XtVaSetValues (EntryPtr->W,
			   XtNsensitive, False, XtNvertSpace, 60, NULL);
	}
      else
	XtVaCreateManagedWidget ("menuLine", smeLineObjectClass, Parent,
				 NULL);
    }
}

/* ---------------------------------------------------------------------------
 * initializes one popup menu
 * create a popup shell, add all entries to it and register the popup and
 * popdown callback functions if required
 */
static void
InitPopupMenu (Widget Parent, PopupMenuTypePtr MenuPtr)
{
  Cursor point = XCreateFontCursor (Dpy, XC_left_ptr);
  MenuPtr->W = XtVaCreatePopupShell (MenuPtr->Name, simpleMenuWidgetClass,
				     Parent,
				     XtNlabel, MenuPtr->Label,
				     XtNsensitive, True,
				     XtNcursor, point, NULL);
  InitPopupTree (MenuPtr->W, MenuPtr->Entries);

  /* install popup and popdown callbacks */
  if (MenuPtr->CB_Popup)
    XtAddCallback (MenuPtr->W, XtNpopupCallback, MenuPtr->CB_Popup, NULL);
  if (MenuPtr->CB_Popdown)
    XtAddCallback (MenuPtr->W, XtNpopupCallback, MenuPtr->CB_Popdown, NULL);
}

/* ---------------------------------------------------------------------------
 * initializes one menubutton plus it's menu
 * create a menu button widget first than add the popup shell to it
 */
Widget
InitMenuButton (Widget Parent,
		MenuButtonTypePtr MenuButtonPtr, Widget Top, Widget Left)
{
  MenuButtonPtr->W =
    XtVaCreateManagedWidget (MenuButtonPtr->Name, menuButtonWidgetClass,
			     Parent, XtNlabel, MenuButtonPtr->Label,
			     XtNmenuName, MenuButtonPtr->PopupMenu->Name,
			     XtNfromHoriz, Left, XtNfromVert, Top, LAYOUT_TOP,
			     NULL);
  XtAddEventHandler (MenuButtonPtr->W, EnterWindowMask, False, CB_StopScroll,
		     NULL);
  InitPopupMenu (MenuButtonPtr->W, MenuButtonPtr->PopupMenu);

  /* return the created button widget to position some others */
  return (MenuButtonPtr->W);
}

/* ---------------------------------------------------------------------------
 * initializes 'check' pixmap if not already done
 */
static Boolean
InitCheckPixmap (void)
{
  if (Check == BadAlloc)
    Check = XCreateBitmapFromData (Dpy,
				   RootWindowOfScreen (XtScreen
						       (Output.Toplevel)),
				   check_icon_bits, check_icon_width,
				   check_icon_height);
  return (Check != BadAlloc ? True : False);
}

/* ---------------------------------------------------------------------------
 * initializes button related menus
 */
void
InitMenu (Widget Parent, Widget Top, Widget Left)
{
  Widget last;

  last = InitMenuButton (Parent, &FileMenuButton, Top, Left);
  last = InitMenuButton (Parent, &EditMenuButton, Top, last);
  last = InitMenuButton (Parent, &DisplayMenuButton, Top, last);
  last = InitializeSizesMenu (Parent, Top, last);
  last = InitMenuButton (Parent, &SettingsMenuButton, Top, last);
  last = InitMenuButton (Parent, &SelectionMenuButton, Top, last);
  last = InitMenuButton (Parent, &BufferMenuButton, Top, last);
  last = InitMenuButton (Parent, &ConnectionMenuButton, Top, last);
  last = InitMenuButton (Parent, &ReportMenuButton, Top, last);
  last = InitMenuButton (Parent, &WindowMenuButton, Top, last);
  InitPopupMenu (Parent, &pMenu);
  InitPopupMenu (Parent, &p2Menu);
  Output.Menu = last;
}
