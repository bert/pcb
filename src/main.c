/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2004 Thomas Nau
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


/* main program, initializes some stuff and handles user input
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define SCROLL_TIME 25

#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <signal.h>
#include <memory.h>

#include "global.h"

#include "action.h"
#include "buffer.h"
#include "create.h"
#include "control.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "file.h"
#include "gui.h"
#include "library.h"
#include "menu.h"
#include "misc.h"
#include "mymem.h"
#include "output.h"
#include "remove.h"
#include "resmenu.h"
#include "set.h"
#include "djopt.h"
#include "resource.h"

#include <X11/cursorfont.h>
#include <X11/Shell.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Porthole.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Simple.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID("$Id$");




/* ---------------------------------------------------------------------------
 * icon data as created by 'bitmap'
 */
#include "icon.data"

#ifdef HAVE_LIBSTROKE
extern void stroke_init (void);
#endif

extern XtActionsRec ActionList[];
extern int ActionListSize;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void CB_Backup (XtPointer, XtIntervalId *);
static void InitShell (int *, char **);
static void InitCursorPosition (Widget, Widget, Widget);
static void InitStatusLine (Widget, Widget, Widget);
static void InitPorthole (Widget, Widget, Widget);
static void InitGC (void);
static void InitIcon (void);
static void InitHandler (void);
static void InitWidgets (void);
static void InitDeviceDriver (void);

/* ---------------------------------------------------------------------------
 *  Stipple patterns with 50 % pixels
 */
#define gray_size 8
typedef char pattern[8];
static pattern gray_bits[] = {
  {0xa5, 0x5a, 0xa5, 0x5a, 0xa5, 0x5a, 0xa5, 0x5a},
  {0x33, 0xcc, 0x33, 0xcc, 0x33, 0xcc, 0x33, 0xcc},
  {0xcc, 0x33, 0xcc, 0x33, 0xcc, 0x33, 0xcc, 0x33},
  {0x88, 0x77, 0x88, 0x77, 0x88, 0x77, 0x88, 0x77},
  {0x77, 0x88, 0x77, 0x88, 0x77, 0x88, 0x77, 0x88},
  {0xee, 0x11, 0xee, 0x11, 0xee, 0x11, 0xee, 0x11},
  {0x11, 0xee, 0x11, 0xee, 0x11, 0xee, 0x11, 0xee},
  {0x96, 0x69, 0x96, 0x69, 0x96, 0x69, 0x96, 0x69},
  {0x69, 0x96, 0x69, 0x96, 0x69, 0x96, 0x69, 0x96}
};

/* ---------------------------------------------------------------------------
 * default translations
 */
static String DefaultTranslations = "";

/* ---------------------------------------------------------------------------
 * fallback resources
 * this is *really* long
 */
static String Fallback[] = {
  "Pcb.alignmentDistance:      200",
  "Pcb.allDirectionLines:      Off",
  "Pcb.backupInterval:         60",
  "Pcb.charactersPerLine:      78",
  "Pcb.connectedColor:         green",
  "Pcb.crosshairColor:         yellow",
  "Pcb.crossColor:             red",
  "Pcb.elementColor:           black",
  "Pcb.elementSelectedColor:   cyan",
  "Pcb.elementCommand:         M4PATH='%p';export M4PATH;echo 'include(%f)' | m4",
  "Pcb.elementPath:            .:packages:circuits:/usr/lib/X11/pcb:/usr/lib/X11/pcb/circuits:/usr/lib/X11/pcb/packages",
  "Pcb.fileCommand:            cat '%f'",
  "Pcb.filePath:               .",
  "Pcb.fontCommand:            M4PATH='%p';export M4PATH;echo 'include(%f)' | m4",
  "Pcb.fontFile:               default_font",
  "Pcb.fontPath:               .:/usr/lib/X11/pcb",
  "Pcb.grid:                   10",
  "Pcb.gridColor:		    red",
  "Pcb.warnColor:              hot pink",
  "Pcb.invertedFrameWidth:     500",
  "Pcb.invisibleObjectsColor:  gray80",
  "Pcb.invisibleMarkColor:     gray70",
  "Pcb.layerColor1:            RoyalBlue3",
  "Pcb.layerColor2:            brown4",
  "Pcb.layerColor3:            DodgerBlue4",
  "Pcb.layerColor4:            OrangeRed3",
  "Pcb.layerColor5:            PaleGreen4",
  "Pcb.layerColor6:            burlywood4",
  "Pcb.layerColor7:            turquoise4",
  "Pcb.layerColor8:            forest green",
  "Pcb.layerGroups:            1,s:2,c:3:4:5:6:7:8",
  "Pcb.layerName1:             solder",
  "Pcb.layerName2:             component",
  "Pcb.layerName3:             GND",
  "Pcb.layerName4:             power",
  "Pcb.layerName5:             signal1",
  "Pcb.layerName6:             signal2",
  "Pcb.layerName7:             unused",
  "Pcb.layerName8:             unused",
  "Pcb.layerSelectedColor1:    cyan",
  "Pcb.layerSelectedColor2:    cyan",
  "Pcb.layerSelectedColor3:    cyan",
  "Pcb.layerSelectedColor4:    cyan",
  "Pcb.layerSelectedColor5:    cyan",
  "Pcb.layerSelectedColor6:    cyan",
  "Pcb.layerSelectedColor7:    cyan",
  "Pcb.layerSelectedColor8:    cyan",
  "Pcb.libraryCommand:         /usr/lib/X11/pcb/QueryLibrary.sh '%p' '%f' %a",
  "Pcb.libraryContentsCommand: /usr/lib/X11/pcb/ListLibraryContents.sh '%p' '%f'",
  "Pcb.libraryFilename:        pcblib",
  "Pcb.libraryPath:            .:/usr/lib/X11/pcb",
  "Pcb.lineThickness:          1000",
  "Pcb.media:                  a4",
  "Pcb.offLimitColor:          gray80",
  "Pcb.pinColor:               gray30",
  "Pcb.pinSelectedColor:       cyan",
  "Pcb.pinoutFont0:            -*-courier-bold-r-*-*-24-*-*-*-*-*-*-*",
  "Pcb.pinoutFont1:            -*-courier-bold-r-*-*-12-*-*-*-*-*-*-*",
  "Pcb.pinoutFont2:            -*-courier-bold-r-*-*-8-*-*-*-*-*-*-*",
  "Pcb.pinoutFont3:            -*-courier-bold-r-*-*-4-*-*-*-*-*-*-*",
  "Pcb.pinoutFont4:            -*-courier-bold-r-*-*-2-*-*-*-*-*-*-*",
  "Pcb.pinoutNameLength:       8",
  "Pcb.pinoutOffsetX:          10000",
  "Pcb.pinoutOffsetY:          10000",
  "Pcb.pinoutTextOffsetX:      5000",
  "Pcb.pinoutTextOffsetY:      0",
  "Pcb.pinoutZoom:             2",
  "Pcb.printFile:             %f.output",
  "Pcb.raiseLogWindow:         On",
  "Pcb.ratColor:               DarkGoldenrod",
  "Pcb.ratSelectedColor:       cyan",
  "Pcb.ratThickness:           1000",
  "Pcb.resetAfterElement:      On",
  "Pcb.ringBellWhenFinished:   Off",
  "Pcb.routeStyles:	    Signal,1000,4000,2000,1000:Power,2500,6000,3500"
  ",1000:Fat,4000,6000,3500,1500:Skinny,800,3600,2000,800",
  "Pcb.saveCommand:            cat - > '%f'",
  "Pcb.saveInTMP:              Off",
  "Pcb.saveLastCommand:        Off",
  "Pcb.size:                   6000x5000",
  "Pcb.stipplePolygons:        Off",
  "Pcb.textScale:              100",
  "Pcb.useLogWindow:           On",
  "Pcb.uniqueNames:	       On",
  "Pcb.snapPin:                On",
  "Pcb.viaColor:               gray50",
  "Pcb.viaSelectedColor:       cyan",
  "Pcb.viaThickness:           6000",
  "Pcb.viaDrillingHole:        2800",
  "Pcb.volume:                 100",
  "Pcb.zoom:                   3",
  "Pcb.Bloat:		    699",
  "Pcb.Shrink:		    500",
  "Pcb.minWid:		    800",
  "Pcb.minSlk:		    800",
  "Pcb*beNiceToColormap:			false",
  "Pcb*background:			gray90",
  "Pcb.masterForm*background:	gray90",
  "Pcb*Command.highlightThickness:        1",
  "Pcb*defaultButton.borderWidth:         2",
  "Pcb*defaultButton.highlightThickness:  2",
  "!pcb*font:                              -*-courier-bold-r-*-*-14-*-*-*-*-*-*-*",
  "Pcb*Label.borderWidth:                 0",
  "Pcb*Label.justify:                     center",
  "Pcb*MenuButton.resizable:              off",
  "Pcb*Toggle.borderWidth:                1",
  "Pcb*selector*List.verticalList:        On",
  "Pcb*selector.height:                   240",
  "Pcb*selector.width:                    180",
  "Pcb*horizDistance:                     2",
  "Pcb*defaultButton.accelerators:  #override "
    "  <Key>Return:            set() notify() unset() \n",
  "Pcb*cancelButton.accelerators:  #override "
    "  <Key>Escape:            set() notify() unset() \n",
  "Pcb*baseTranslations: #override "
    "  !Mod1<Key>q:	Quit() \n " "  <Message>WM_PROTOCOLS:   Quit() \n",
  "Pcb*controlMasterForm*font:             -*-courier-bold-r-*-*-12-*-*-*-*-*-*-*",
  "Pcb*controlMasterForm*SimpleMenu.resizeable: True",
  "Pcb*controlMasterForm*shadowWidth:	0",
  "Pcb*controlMasterForm*borderWidth:      0",
  "Pcb*controlMasterForm*SimpleMenu.borderWidth: 1",
  "Pcb*controlMasterForm*horizDistance:	1",
  "Pcb*controlMasterForm*panner.width: 72",
  "Pcb*controlMasterForm*panner.height: 54",
  "Pcb*controlMasterForm*panner.rubberBand: False",
  "Pcb*controlMasterForm*panner.borderWidth: 2",
  "Pcb*controlMasterForm*panner.shadowWidth: 2",
  "Pcb*controlMasterForm*MenuButton.width: 72",
  "Pcb*controlMasterForm*Toggle.width: 72",
  "Pcb*controlMasterForm*Label.height:    12",
  "Pcb*controlMasterForm*Label.width:   72 ",
  "Pcb*controlMasterForm*MenuButton.background:	white",
  "Pcb*controlMasterForm*MenuButton.height:      14",
  "Pcb*controlMasterForm*MenuButton.borderWidth: 1  ",
  "Pcb*controlMasterForm*Toggle.background:	white",
  "Pcb*controlMasterForm*Toggle.height:          14",
  "Pcb*controlMasterForm*Toggle.borderWidth:     1",
  "Pcb.masterForm.Toggle.foreground:	black",
  "!pcb.masterForm.Toggle.internalHeight:   1 ",
  "!pcb.masterForm.Toggle.internalWidth:	1",
  "Pcb.masterForm.Toggle.horizDistance:       3 ",
  "Pcb.masterForm.Toggle.vertDistance:        3",
  "Pcb*fileselectMasterForm*current.vertDistance: 20",
  "Pcb*fileselectMasterForm*input.width:          366",
  "Pcb*fileselectMasterForm*input*background:     gray90",
  "Pcb*fileselectMasterForm.defaultButton.accelerators:  #override "
    "  <Key>Return:   set() notify() unset() \n"
    "  <Btn1Up>(2):   set() notify() unset() \n",
  "Pcb*sizeMasterForm*Label.vertDistance:    10",
  "Pcb*library.iconic:		true",
  "Pcb.library.geometry:         750x250",
  "Pcb.library*type*foreground:  red",
  "Pcb.library.baseTranslations: #override "
    "  <Message>WM_PROTOCOLS:  Bell() \n",
  "Pcb*libraryMasterForm*List.baseTranslations: #replace "
    "  <Btn1Down>,<Btn1Up>:    Set() \n"
    "  <Btn1Up>(2):            Set() Notify() Unset() \n",
  "Pcb*netlist.iconic:	false",
  "Pcb.netlist.geometry:         400x200",
  "Pcb.netlist*net*foreground:	red",
  "Pcb.netlist.baseTranslations: #override "
    "  <Message>WM_PROTOCOLS:  Bell() \n",
  "Pcb*netlistMasterForm*List.baseTranslations: #replace "
    "  <Btn1Down>,<Btn1Up>:    Set() Notify() \n"
    "  <Btn1Up>(2):            ListAct() Set() \n"
    "  None<Key>u:		Undo() \n",
  "Pcb*log.iconic:           true",
  "Pcb.log.geometry:         600x200",
  "Pcb.log.baseTranslations: #override "
    "  <Message>WM_PROTOCOLS:   Bell() \n",
  "Pcb*printMasterForm*Command.width:        100",
  "Pcb*printMasterForm*comment.vertDistance: 20",
  "Pcb*printMasterForm*input.width:          150",
  "Pcb*printMasterForm*Panner.background:    gray90",
  "Pcb*printMasterForm*Toggle.width:         150",
  "Pcb*layerGroupMasterForm*comment.width:             150",
  "Pcb*layerGroupMasterForm*comment.justify:           left",
  "Pcb*layerGroupMasterForm*groupNumber.horizDistance: 6",
  "Pcb*layerGroupMasterForm*groupNumber.width:         20",
  "Pcb*layerGroupMasterForm*layerGroup.width:          150",
  "Pcb*layerGroupMasterForm*layerName.justify:         left",
  "Pcb*layerGroupMasterForm*layerName.width:           150",
  "Pcb*layerGroupMasterForm*Toggle.width:              20",
  "Pcb*pinoutMasterForm*viewport.height:  240",
  "Pcb*pinoutMasterForm*viewport.width:   320",
  "Pcb*pinoutMasterForm.dismiss.accelerators: #override "
    "  <Message>WM_PROTOCOLS:  set() notify() unset() \n",
  "Pcb.masterForm*MenuButton.shadowWidth:	0",
  "Pcb.masterForm*MenuButton.BorderWidth: 0",
  "Pcb*MenuButton.baseTranslations: #override "
    "  <BtnDown>:  set() \n" "  <BtnUp>:    reset() PopupMenu() \n",
  "Pcb*header.foreground:    red",
  "Pcb*SimpleMenu*justify:   left",
  "Pcb*SimpleMenu.baseTranslations: #override "
    "   <Motion>: highlight() \n"
    "   <BtnDown>,<BtnUp>:	MenuPopdown() notify() unhighlight() \n"
    "   <BtnUp>: \n",
  "Pcb.masterForm*cursorPosition.justify:   left",
  "Pcb.masterForm*cursorPosition.width:     300",
  "Pcb.masterForm*inputField*borderWidth:   0",
  "Pcb.masterForm*messageText.foreground:   red",
  "Pcb.masterForm*messageText.justify:      left",
  "Pcb.masterForm*statusLine.justify:       left",
  "Pcb.masterForm*output*background:        gray95",
  "Pcb.masterForm.porthole.height:          600",
  "Pcb.masterForm.porthole.width:           800",
  "Pcb.masterForm*output.baseTranslations:   #override "
    "  <Key>Escape:          Mode(None) \n"
    "  <Key>space:           Mode(Notify) \n"
    "  <Key>colon:           Command() \n"
    "  Shift<Key>BackSpace: Atomic(Save) "
    "			Connection(Reset) "
    "			Atomic(Restore) "
    "                        Unselect(All) "
    "			Atomic(Restore) "
    "                        Connection(Find) "
    "			Atomic(Restore) "
    "                        Select(Connection) "
    "			Atomic(Restore) "
    "                        RemoveSelected() "
    "			Atomic(Restore) "
    "			Connection(Reset) "
    "			Atomic(Restore) "
    "			Unselect(All) "
    "			Atomic(Block) \n"
    "  Shift<Key>Delete:	Atomic(Save) "
    "			Connection(Reset) "
    "			Atomic(Restore) "
    "                        Unselect(All) "
    "			Atomic(Restore) "
    "                        Connection(Find) "
    "			Atomic(Restore) "
    "                        Select(Connection) "
    "			Atomic(Restore) "
    "                        RemoveSelected() "
    "			Atomic(Restore) "
    "			Connection(Reset) "
    "			Atomic(Restore) "
    "			Unselect(All) "
    "			Atomic(Block) \n"
    "  <Key>BackSpace:   Mode(Save) "
    "                        Mode(Remove) "
    "                        Mode(Notify) "
    "                        Mode(Restore) \n"
    "  <Key>Delete:	Mode(Save) "
    "			Mode(Remove) "
    "			Mode(Notify) "
    "			Mode(Restore) \n"
    "  <Key>Insert:      Mode(InsertPoint) \n"
    "  <Key>Tab:         SwapSides() \n"
    "  Ctrl<Key>1: RouteStyle(1) \n"
    "  Ctrl<Key>2: RouteStyle(2) \n"
    "  Ctrl<Key>3: RouteStyle(3) \n"
    "  Ctrl<Key>4: RouteStyle(4) \n"
    "  Shift<Key>1: PasteBuffer(1) \n"
    "  Shift<Key>2: PasteBuffer(2) \n"
    "  Shift<Key>3: PasteBuffer(3) \n"
    "  Shift<Key>4: PasteBuffer(4) \n"
    "  Shift<Key>5: PasteBuffer(5) \n"
    "  <Key>1: SwitchDrawingLayer(1) \n"
    "  <Key>2: SwitchDrawingLayer(2) \n"
    "  <Key>3: SwitchDrawingLayer(3) \n"
    "  <Key>4: SwitchDrawingLayer(4) \n"
    "  <Key>5: SwitchDrawingLayer(5) \n"
    "  <Key>6: SwitchDrawingLayer(6) \n"
    "  <Key>7: SwitchDrawingLayer(7) \n"
    "  <Key>8: SwitchDrawingLayer(8) \n"
    "  <Key>9: SwitchDrawingLayer(9) \n"
    "  Shift Mod1<Key>a:	Unselect(All) \n"
    "  Mod1<Key>a:	Select(All) \n"
    "  Shift<Key>b:		Flip(Selected) \n"
    "  <Key>b:		Flip(Object) \n"
    "  Mod1<Key>c:       PasteBuffer(Clear) "
    "                       PasteBuffer(AddSelected) "
    "                       Unselect(All) \n"
    "  <Key>c:          Display(Center) \n"
    "  Shift<Key>d:        Display(Pinout) \n"
    "  <Key>d:          Display(PinOrPadName) \n"
    "  Shift<Key>e:        DeleteRats(SelectedRats) \n"
    "  <Key>e:          DeleteRats(AllRats) \n"
    "  Ctrl<Key>f:	   Connection(Find) \n"
    "  Shift<Key>f:        Connection(Reset) \n"
    "  <Key>f:          Connection(Reset) "
    "		   Connection(Find) \n"
    "  Shift<Key>g:        SetValue(Grid, -500) \n"
    "  <Key>g:          SetValue(Grid, +500) \n"
    "  Shift<Key>h:        ToggleHideName(SelectedElements) \n"
    "  Ctrl<Key>h:         ChangeHole(ToggleObject) \n"
    "  <Key>h:          ToggleHideName(Object) \n"
    "  <Key>j:		ChangeJoin(Object) \n"
    "  Shift<Key>k:		ChangeClearSize(Object, -200) \n"
    "  <Key>k:		ChangeClearSize(Object, +200) \n"
    "  Shift<Key>l:        SetValue(LineSize, -500) \n"
    "  <Key>l:          SetValue(LineSize, +500) \n"
    "  Shift<Key>m:        MoveToCurrentLayer(SelectedObjects) \n"
    "  Ctrl<Key>m:         MarkCrosshair() \n"
    "  <Key>m:          MoveToCurrentLayer(Object) \n"
    "  <Key>n:          ChangeName(Object) \n"
    "  Shift<Key>o:        Atomic(Save) "
    "		       DeleteRats(AllRats) "
    "		       Atomic(Restore) "
    "                       AddRats(SelectedRats) "
    "		       Atomic(Block) \n"
    "  Ctrl<Key>o:         ChangeOctagon(Object) \n"
    "  <Key>o:          Atomic(Save) "
    "		       DeleteRats(AllRats) "
    "		       Atomic(Restore) "
    "                       AddRats(AllRats) "
    "		       Atomic(Block) \n"
    "  Shift<Key>p:        Polygon(Close) \n"
    "  <Key>p:          Polygon(PreviousPoint) \n"
    "  Mod1<Key>q:	Quit() \n"
    "  <Key>q:          ChangeSquare(ToggleObject) \n"
    "  Shift<Key>r:        Redo() \n"
    "  Ctrl<Key>r:         Report(Object) \n"
    "  <Key>r:          Display(ClearAndRedraw) \n"
    "  Mod1 Shift<Key>s: ChangeDrillSize(Object, -500) \n"
    "  Shift<Key>s:        ChangeSize(Object, -500) \n"
    "  Mod1<Key>s:       ChangeDrillSize(Object, +500) \n"
    "  <Key>s:          ChangeSize(Object, +500) \n"
    "  Shift Ctrl<Key>u:   Undo(ClearList) \n"
    "  <Key>u:          Undo() \n"
    "  Mod1 Shift<Key>v: SetValue(ViaDrillingHole, -500) \n"
    "  Shift<Key>v:        SetValue(ViaSize, -500) \n"
    "  Mod1<Key>v:       SetValue(ViaDrillingHole, +500) \n"
    "  <Key>v:          SetValue(ViaSize, +500) \n"
    "  Shift<Key>w:        AddRats(SelectedRats) \n"
    "  <Key>w:          AddRats(AllRats) \n"
    "  Shift Ctrl<Key>x:   PasteBuffer(Clear) "
    "                       PasteBuffer(AddSelected) "
    "                       RemoveSelected() "
    "                       Mode(PasteBuffer) \n"
    "  Ctrl<Key>x:         PasteBuffer(Clear) "
    "                       PasteBuffer(AddSelected) "
    "                       Mode(PasteBuffer) \n"
    "  Mod1<Key>x:       PasteBuffer(Clear) "
    "                       PasteBuffer(AddSelected) "
    "                       RemoveSelected() \n"
    "  Shift<Key>z:        SetValue(Zoom, +1) \n"
    "  Mod1<Key>z:       Undo() \n"
    "  <Key>z:          SetValue(Zoom, -1) \n"
    "  <Key>.:          Display(Toggle45Degree) \n"
    "  <Key>/:          Display(CycleClip) \n"
    "  Shift<Key>Up:    MovePointer(0, -10) \n"
    "  <Key>Up:      MovePointer(0, -1) \n"
    "  Shift<Key>Down:  MovePointer(0, 10) \n"
    "  <Key>Down:    MovePointer(0, 1) \n"
    "  Shift<Key>Right: MovePointer(10, 0) \n"
    "  <Key>Right:   MovePointer(1, 0) \n"
    "  Shift<Key>Left:  MovePointer(-10, 0) \n"
    "  <Key>Left:    MovePointer(-1, 0) \n"
    "  <Key>F1:   Mode(Via) \n"
    "  <Key>F2:   Mode(Line) \n"
    "  Shift<Key>F3: PasteBuffer(Rotate, 1) \n"
    "  <Key>F3:   Mode(PasteBuffer) \n"
    "  <Key>F4:   Mode(Rectangle) \n"
    "  <Key>F5:   Mode(Text) \n"
    "  <Key>F6:   Mode(Polygon) \n"
    "  <Key>F7:   Mode(Thermal) \n"
    "  <Key>F8:   Mode(Arc) \n"
    "  <Key>F9:	Mode(Rotate) \n"
    "  <Key>F10:  Mode(Arrow) \n"
    "  <Key>[:	Mode(Save) "
    "		Mode(Move) "
    "		Mode(Notify) \n"
    "  <Key>]:	Mode(Notify) "
    "		Mode(Restore) \n"
    "  Shift Ctrl<Btn1Down>: Mode(Save) "
    "                         Mode(Remove) "
    "                         Mode(Notify) "
    "                         Mode(Restore) \n"
    "  Ctrl<Btn1Down>:       Mode(Save) "
    "			 Mode(None) "
    "		         Mode(Restore) "
    "		         Mode(Notify) \n"
    "  <Btn1Down>:        Mode(Notify) \n"
    "  <Btn1Up>:		 Mode(Release) \n"
    "  Shift Mod1<Btn2Down>: Display(ToggleRubberbandMode) "
    "			   Mode(Save) "
    "                           Mode(Move) "
    "                           Mode(Notify) \n"
    "  Mod1<Btn2Down>:       Mode(Save) "
    "                           Mode(Copy) "
    "                           Mode(Notify) \n"
    "  <Btn2Down>:		Mode(Save) "
    "			Mode(Arrow) "
    "			Mode(Notify) \n"
    "  Shift Mod1<Btn2Up>:   Mode(Notify) "
    "                           Mode(Restore) "
    "			   Display(ToggleRubberbandMode) \n"
    "  Mod1<Btn2Up>:            Mode(Notify) "
    "                           Mode(Restore) \n"
    "  <Btn2Up>:		Mode(Release) "
    "			Mode(Restore) \n"
    "  Shift<Btn3Down>:	SetValue(Zoom, +2) \n"
    "  Shift<Btn3Up>:	Display(Center) "
    "			SetValue(Zoom, -2) \n"
    "  None<Btn3Down>:	ButtonThree() \n"
    "  None<Btn3Down>:          XawPositionSimpleMenu(pmenu) "
    "			   XtMenuPopup(pmenu) \n"
    "  Shift <Btn3Down>:	   Mode(Save) "
    "                           Mode(None) "
    "                           Unselect(Block) \n"
    "  Shift<Btn3Up>:	   Unselect(Block) "
    "                           Mode(Restore) \n"
    "  Mod1<Btn3Down>:	Mode(Save) "
    "			Mode(None) "
    "			Select(Block) \n"
    "  Mod1<Btn3Up>:	Select(Block) "
    "			Mode(Restore) \n",
  NULL
};

/* ---------------------------------------------------------------------------
 * resources to query
 */
static XtResource ToplevelResources[] = {
  {"alignmentDistance", "AlignmentDistance", XtRInt, sizeof (int),
   XtOffsetOf (SettingType, AlignmentDistance), XtRString, "20000"}
  ,
  {"allDirectionLines", "AllDirectionLines", XtRBoolean, sizeof (Boolean),
   XtOffsetOf (SettingType, AllDirectionLines), XtRString, "True"}
  ,
  {"backgroundImage", "BackgroundImage", XtRString, sizeof (String),
   XtOffsetOf (SettingType, BackgroundImage), XtRString, "pcb-background.ppm"}
  ,
  {"backupInterval", "BackupInterval", XtRInt, sizeof (long),
   XtOffsetOf (SettingType, BackupInterval), XtRString, "300"}
  ,
  {"bloat", "Bloat", XtRInt, sizeof (int),
   XtOffsetOf (SettingType, Bloat), XtRString, "699"}
  ,
  {"minWid", "minWid", XtRInt, sizeof (int),
   XtOffsetOf (SettingType, minWid), XtRString, "800"}
  ,
  {"minSlk", "minSlk", XtRInt, sizeof (int),
   XtOffsetOf (SettingType, minSlk), XtRString, "800"}
  ,
  {"charactersPerLine", "CharactersPerLine", XtRInt, sizeof (int),
   XtOffsetOf (SettingType, CharPerLine), XtRString, "80"},
  {"connectedColor", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, ConnectedColor), XtRString, XtDefaultForeground}
  ,
  {"crossColor", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, CrossColor), XtRString, XtDefaultForeground}
  ,
  {"crosshairColor", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, CrosshairColor), XtRString, XtDefaultForeground}
  ,
  {"elementColor", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, ElementColor), XtRString, XtDefaultForeground}
  ,
  {"elementSelectedColor", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, ElementSelectedColor),
   XtRString, XtDefaultForeground}
  ,
  {"elementCommand", "ElementCommand", XtRString, sizeof (String),
   XtOffsetOf (SettingType, ElementCommand), XtRString,
   "M4PATH=\"%p\";export M4PATH;echo 'include(%f)' | " GNUM4}
  ,
  {"elementPath", "ElementPath", XtRString, sizeof (String),
   XtOffsetOf (SettingType, ElementPath), XtRString, PCBLIBDIR}
  ,
  {"fileCommand", "FileCommand", XtRString, sizeof (String),
   XtOffsetOf (SettingType, FileCommand), XtRString, "cat %f"}
  ,
  {"filePath", "FilePath", XtRString, sizeof (String),
   XtOffsetOf (SettingType, FilePath), XtRString, "."}
  ,
  {"fontCommand", "FontCommand", XtRString, sizeof (String),
   XtOffsetOf (SettingType, FontCommand), XtRString, "cat %f"}
  ,
  {"fontFile", "FontFile", XtRString, sizeof (String),
   XtOffsetOf (SettingType, FontFile), XtRString, FONTFILENAME}
  ,
  {"fontPath", "FontPath", XtRString, sizeof (String),
   XtOffsetOf (SettingType, FontPath), XtRString, PCBLIBDIR}
  ,
  {"grid", "Grid", XtRFloat, sizeof (float),
   XtOffsetOf (SettingType, Grid), XtRString, "5000"},
  {"gridColor", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, GridColor), XtRString, XtDefaultForeground}
  ,
  {"historySize", "HistorySize", XtRInt, sizeof (int),
   XtOffsetOf (SettingType, HistorySize), XtRString, "100"}
  ,
  {"invisibleObjectsColor", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, InvisibleObjectsColor),
   XtRString, XtDefaultForeground}
  ,
  {"invisibleMarkColor", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, InvisibleMarkColor),
   XtRString, XtDefaultForeground}
  ,
  {"keepawayWidth", XtCThickness, XtRInt, sizeof (int),
   XtOffsetOf (SettingType, Keepaway), XtRString, "1000"}
  ,
  {"layerColor1", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, LayerColor[0]), XtRString, XtDefaultForeground}
  ,
  {"layerColor2", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, LayerColor[1]), XtRString, XtDefaultForeground}
  ,
  {"layerColor3", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, LayerColor[2]), XtRString, XtDefaultForeground}
  ,
  {"layerColor4", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, LayerColor[3]), XtRString, XtDefaultForeground}
  ,
  {"layerColor5", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, LayerColor[4]), XtRString, XtDefaultForeground}
  ,
  {"layerColor6", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, LayerColor[5]), XtRString, XtDefaultForeground}
  ,
  {"layerColor7", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, LayerColor[6]), XtRString, XtDefaultForeground}
  ,
  {"layerColor8", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, LayerColor[7]), XtRString, XtDefaultForeground}
  ,

  {"layerName1", "LayerName1", XtRString, sizeof (String),
   XtOffsetOf (SettingType, DefaultLayerName[0]), XtRString, NULL}
  ,
  {"layerName2", "LayerName2", XtRString, sizeof (String),
   XtOffsetOf (SettingType, DefaultLayerName[1]), XtRString, NULL}
  ,
  {"layerName3", "LayerName3", XtRString, sizeof (String),
   XtOffsetOf (SettingType, DefaultLayerName[2]), XtRString, NULL}
  ,
  {"layerName4", "LayerName4", XtRString, sizeof (String),
   XtOffsetOf (SettingType, DefaultLayerName[3]), XtRString, NULL}
  ,
  {"layerName5", "LayerName5", XtRString, sizeof (String),
   XtOffsetOf (SettingType, DefaultLayerName[4]), XtRString, NULL}
  ,
  {"layerName6", "LayerName6", XtRString, sizeof (String),
   XtOffsetOf (SettingType, DefaultLayerName[5]), XtRString, NULL}
  ,
  {"layerName7", "LayerName7", XtRString, sizeof (String),
   XtOffsetOf (SettingType, DefaultLayerName[6]), XtRString, NULL}
  ,
  {"layerName8", "LayerName8", XtRString, sizeof (String),
   XtOffsetOf (SettingType, DefaultLayerName[7]), XtRString, NULL}
  ,

  {"layerSelectedColor1", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, LayerSelectedColor[0]),
   XtRString, XtDefaultForeground}
  ,
  {"layerSelectedColor2", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, LayerSelectedColor[1]),
   XtRString, XtDefaultForeground}
  ,
  {"layerSelectedColor3", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, LayerSelectedColor[2]),
   XtRString, XtDefaultForeground}
  ,
  {"layerSelectedColor4", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, LayerSelectedColor[3]),
   XtRString, XtDefaultForeground}
  ,
  {"layerSelectedColor5", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, LayerSelectedColor[4]),
   XtRString, XtDefaultForeground}
  ,
  {"layerSelectedColor6", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, LayerSelectedColor[5]),
   XtRString, XtDefaultForeground}
  ,
  {"layerSelectedColor7", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, LayerSelectedColor[6]),
   XtRString, XtDefaultForeground}
  ,
  {"layerSelectedColor8", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, LayerSelectedColor[7]),
   XtRString, XtDefaultForeground}
  ,

  {"layerGroups", "LayerGroups", XtRString, sizeof (String),
   XtOffsetOf (SettingType, Groups), XtRString, "1:2:3:4:5:6:7:8"}
  ,
  {"libraryCommand", "LibraryCommand", XtRString, sizeof (String),
   XtOffsetOf (SettingType, LibraryCommand), XtRString, "cat %f"}
  ,
  {"libraryContentsCommand", "LibraryContentsCommand", XtRString,
   sizeof (String), XtOffsetOf (SettingType, LibraryContentsCommand),
   XtRString, "cat %f"}
  ,
  {"libraryFilename", "LibraryFilename", XtRString, sizeof (String),
   XtOffsetOf (SettingType, LibraryFilename), XtRString, LIBRARYFILENAME}
  ,
  {"libraryPath", "LibraryPath", XtRString, sizeof (String),
   XtOffsetOf (SettingType, LibraryPath), XtRString, PCBLIBDIR}
  ,
  {"libraryTree", "LibraryTree", XtRString, sizeof (String),
   XtOffsetOf (SettingType, LibraryTree), XtRString, PCBTREEDIR}
  ,
  {"lineThickness", XtCThickness, XtRInt, sizeof (int),
   XtOffsetOf (SettingType, LineThickness), XtRString, "10"}
  ,
  {"media", "Media", XtRString, sizeof (String),
   XtOffsetOf (SettingType, Media), XtRString, DEFAULT_MEDIASIZE}
  ,
  {"menuFile", "MenuFile", XtRString, sizeof (String),
   XtOffsetOf (SettingType, MenuFile), XtRString, PCBLIBDIR "/pcb-menu.res"}
  ,
  {"dumpMenuFile", "DumpMenuFile", XtRBoolean, sizeof (Boolean),
   XtOffsetOf (SettingType, DumpMenuFile), XtRString, "False"}
  ,
  {"offLimitColor", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, OffLimitColor), XtRString, XtDefaultBackground}
  ,
  {"pinColor", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, PinColor), XtRString, XtDefaultForeground}
  ,
  {"pinSelectedColor", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, PinSelectedColor),
   XtRString, XtDefaultForeground}
  ,

  {"pinoutFont0", "Font", XtRFontStruct, sizeof (XFontStruct *),
   XtOffsetOf (SettingType, PinoutFont[0]), XtRFontStruct, XtDefaultFont}
  ,
  {"pinoutFont1", "Font", XtRFontStruct, sizeof (XFontStruct *),
   XtOffsetOf (SettingType, PinoutFont[1]), XtRFontStruct, XtDefaultFont}
  ,
  {"pinoutFont2", "Font", XtRFontStruct, sizeof (XFontStruct *),
   XtOffsetOf (SettingType, PinoutFont[2]), XtRFontStruct, XtDefaultFont}
  ,
  {"pinoutFont3", "Font", XtRFontStruct, sizeof (XFontStruct *),
   XtOffsetOf (SettingType, PinoutFont[3]), XtRFontStruct, XtDefaultFont}
  ,
  {"pinoutFont4", "Font", XtRFontStruct, sizeof (XFontStruct *),
   XtOffsetOf (SettingType, PinoutFont[4]), XtRFontStruct, XtDefaultFont}
  ,

  {"pinoutNameLength", "PinoutNameLength", XtRInt, sizeof (int),
   XtOffsetOf (SettingType, PinoutNameLength), XtRString, "8"},
  {"pinoutOffsetX", "PinoutOffsetX", XtRPosition, sizeof (Position),
   XtOffsetOf (SettingType, PinoutOffsetX), XtRString, "10000"}
  ,
  {"pinoutOffsetY", "PinoutOffsetY", XtRPosition, sizeof (Position),
   XtOffsetOf (SettingType, PinoutOffsetY), XtRString, "10000"}
  ,
  {"pinoutTextOffsetX", "PinoutTextOffsetX", XtRPosition, sizeof (Position),
   XtOffsetOf (SettingType, PinoutTextOffsetX), XtRString, "0"}
  ,
  {"pinoutTextOffsetY", "PinoutTextOffsetY", XtRPosition, sizeof (Position),
   XtOffsetOf (SettingType, PinoutTextOffsetY), XtRString, "0"}
  ,
  {"pinoutZoom", "PinoutZoom", XtRFloat, sizeof (float),
   XtOffsetOf (SettingType, PinoutZoom), XtRString, "2"},
  {"printFile", "PrintFile", XtRString, sizeof (String),
   XtOffsetOf (SettingType, PrintFile), XtRString, "%f.output"}
  ,
  {"raiseLogWindow", "RaiseLogWindow", XtRBoolean, sizeof (Boolean),
   XtOffsetOf (SettingType, RaiseLogWindow), XtRString, "True"}
  ,
  {"ratColor", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, RatColor), XtRString, XtDefaultForeground}
  ,
  {"ratSelectedColor", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, RatSelectedColor),
   XtRString, XtDefaultForeground}
  ,
  {"ratCommand", "RatCommand", XtRString, sizeof (String),
   XtOffsetOf (SettingType, RatCommand), XtRString, "cat %f"}
  ,
  {"ratPath", "RatPath", XtRString, sizeof (String),
   XtOffsetOf (SettingType, RatPath), XtRString, "."}
  ,
  {"ratThickness", XtCThickness, XtRInt, sizeof (int),
   XtOffsetOf (SettingType, RatThickness), XtRString, "500"}
  ,
  {"resetAfterElement", "ResetAfterElement", XtRBoolean, sizeof (Boolean),
   XtOffsetOf (SettingType, ResetAfterElement), XtRString, "False"}
  ,
  {"ringBellWhenFinished", "RingBellWhenFinished", XtRBoolean,
   sizeof (Boolean),
   XtOffsetOf (SettingType, RingBellWhenFinished), XtRString, "True"}
  ,
  {"routeStyles", "RouteStyles", XtRString, sizeof (String),
   XtOffsetOf (SettingType, Routes), XtRString,
   "5000,8000,4000:2000,6000,2800:1000,5000,2800:3000,6500,3500"}
  ,
  {"rubberBandMode", "RubberBandMode", XtRBoolean, sizeof (Boolean),
   XtOffsetOf (SettingType, RubberBandMode), XtRString, "True"}
  ,
  {"saveCommand", "SaveCommand", XtRString, sizeof (String),
   XtOffsetOf (SettingType, SaveCommand), XtRString, "cat - > %f"}
  ,
  {"saveInTMP", "SaveInTMP", XtRBoolean, sizeof (Boolean),
   XtOffsetOf (SettingType, SaveInTMP), XtRString, "True"}
  ,
  {"saveLastCommand", "SaveLastCommand", XtRBoolean, sizeof (Boolean),
   XtOffsetOf (SettingType, SaveLastCommand), XtRString, "False"}
  ,
  {"scriptFilename", "ScriptFilename", XtRString, sizeof (String),
   XtOffsetOf (SettingType, ScriptFilename), XtRString, NULL}
  ,
  {"shrink", "Shrink", XtRInt, sizeof (int),
   XtOffsetOf (SettingType, Shrink), XtRString, "500"}
  ,
  {"size", "Size", XtRString, sizeof (String),
   XtOffsetOf (SettingType, Size), XtRString, DEFAULT_SIZE}
  ,
  {"stipplePolygons", "StipplePolygons", XtRBoolean, sizeof (Boolean),
   XtOffsetOf (SettingType, StipplePolygons), XtRString, "False"}
  ,
  {"swapStartDirection", "SwapStartDirection", XtRBoolean, sizeof (Boolean),
   XtOffsetOf (SettingType, SwapStartDirection), XtRString, "True"}
  ,
  {"textScale", "TextScale", XtRInt, sizeof (BDimension),
   XtOffsetOf (SettingType, TextScale), XtRString, "100"}
  ,
  {"uniqueNames", "UniqueNames", XtRBoolean, sizeof (Boolean),
   XtOffsetOf (SettingType, UniqueNames), XtRString, "True"}
  ,
  {"snapPin", "SnapPin", XtRBoolean, sizeof (Boolean),
   XtOffsetOf (SettingType, SnapPin), XtRString, "True"}
  ,
  {"clearLines", "ClearLines", XtRBoolean, sizeof (Boolean),
   XtOffsetOf (SettingType, ClearLine), XtRString, "True"}
  ,
  {"useLogWindow", "UseLogWindow", XtRBoolean, sizeof (Boolean),
   XtOffsetOf (SettingType, UseLogWindow), XtRString, "True"}
  ,
  {"viaColor", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, ViaColor), XtRString, XtDefaultForeground}
  ,
  {"viaSelectedColor", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, ViaSelectedColor),
   XtRString, XtDefaultForeground}
  ,

  /* a default value of '0' will cause InitShell() to
   * calculate the value from the 'viaThickness' resource
   */
  {"viaDrillingHole", "DrillingHole", XtRInt, sizeof (int),
   XtOffsetOf (SettingType, ViaDrillingHole), XtRString, "2000"}
  ,
  {"viaThickness", XtCThickness, XtRInt, sizeof (int),
   XtOffsetOf (SettingType, ViaThickness), XtRString, "4000"}
  ,
  {"volume", "Volume", XtRInt, sizeof (int),
   XtOffsetOf (SettingType, Volume), XtRString, "100"}
  ,
  {"maskColor", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, MaskColor), XtRString, XtDefaultForeground}
  ,
  {"warnColor", XtCColor, XtRPixel, sizeof (Pixel),
   XtOffsetOf (SettingType, WarnColor), XtRString, XtDefaultForeground}
  ,
  {"zoom", "Zoom", XtRFloat, sizeof (float),
   XtOffsetOf (SettingType, Zoom), XtRString, "3"}
};

/* ---------------------------------------------------------------------------
 * additional command line arguments
 */
static XrmOptionDescRec CommandLineOptions[] = {
  {"-alldirections", "allDirectionLines", XrmoptionNoArg, (caddr_t) "True"},
  {"+alldirections", "allDirectionLines", XrmoptionNoArg, (caddr_t) "False"},
  {"-rubberband", "rubberBandMode", XrmoptionNoArg, (caddr_t) "False"},
  {"+rubberband", "rubberBandMode", XrmoptionNoArg, (caddr_t) "True"},
  {"-background", "backgroundImage", XrmoptionSepArg, (caddr_t) NULL},
  {"-backup", "backupInterval", XrmoptionSepArg, (caddr_t) NULL},
  {"-c", "charactersPerLine", XrmoptionSepArg, (caddr_t) NULL},
  {"-dumpmenu", "dumpMenuFile", XrmoptionNoArg, (caddr_t) "True"},
  {"-fontfile", "fontFile", XrmoptionSepArg, (caddr_t) NULL},
  {"-laperture", "apertureCommand", XrmoptionSepArg, (caddr_t) NULL},
  {"-lelement", "elementCommand", XrmoptionSepArg, (caddr_t) NULL},
  {"-lfile", "fileCommand", XrmoptionSepArg, (caddr_t) NULL},
  {"-lfont", "fontCommand", XrmoptionSepArg, (caddr_t) NULL},
  {"-lg", "layerGroups", XrmoptionSepArg, (caddr_t) NULL},
  {"-libname", "libraryFilename", XrmoptionSepArg, (caddr_t) NULL},
  {"-libpath", "libraryPath", XrmoptionSepArg, (caddr_t) NULL},
  {"-libtree", "libraryTree", XrmoptionSepArg, (caddr_t) NULL},
  {"-llib", "libraryCommand", XrmoptionSepArg, (caddr_t) NULL},
  {"-llibcont", "libraryContentsCommand", XrmoptionSepArg, (caddr_t) NULL},
  {"-log", "useLogWindow", XrmoptionNoArg, (caddr_t) "False"},
  {"-loggeometry", "log.geometry", XrmoptionSepArg, (caddr_t) NULL},
  {"-pnl", "pinoutNameLength", XrmoptionSepArg, (caddr_t) NULL},
  {"-pz", "pinoutZoom", XrmoptionSepArg, (caddr_t) NULL},
  {"-reset", "resetAfterElement", XrmoptionNoArg, (caddr_t) "True"},
  {"+reset", "resetAfterElement", XrmoptionNoArg, (caddr_t) "False"},
  {"-ring", "ringBellWhenFinished", XrmoptionNoArg, (caddr_t) "True"},
  {"+ring", "ringBellWhenFinished", XrmoptionNoArg, (caddr_t) "False"},
  {"-rs", "routeStyles", XrmoptionSepArg, (caddr_t) NULL},
  {"-s", "saveLastCommand", XrmoptionNoArg, (caddr_t) "True"},
  {"+s", "saveLastCommand", XrmoptionNoArg, (caddr_t) "False"},
  {"-save", "saveInTMP", XrmoptionNoArg, (caddr_t) "True"},
  {"+save", "saveInTMP", XrmoptionNoArg, (caddr_t) "False"},
  {"-script", "scriptFilename", XrmoptionSepArg, (caddr_t) NULL},
  {"-size", "size", XrmoptionSepArg, (caddr_t) NULL},
  {"-sfile", "saveCommand", XrmoptionSepArg, (caddr_t) NULL},
  {"-v", "volume", XrmoptionSepArg, (caddr_t) NULL}
};

/* ---------------------------------------------------------------------------
 * actions
 */
static XtActionsRec Actions[] = {
  {"AutoPlaceSelected", ActionAutoPlaceSelected},
  {"AutoRoute", ActionAutoRoute},
  {"ButtonThree", ActionButton3},
  {"SetSame", ActionSetSame},
  {"MovePointer", ActionMovePointer},
  {"ToggleHideName", ActionToggleHideName},
  {"ChangeHole", ActionChangeHole},
  {"ToggleThermal", ActionToggleThermal},
  {"Atomic", ActionAtomic},
  {"AdjustStyle", ActionAdjustStyle},
  {"RouteStyle", ActionRouteStyle},
  {"DRC", ActionDRCheck},
  {"Flip", ActionFlip},
  {"SetValue", ActionSetValue},
  {"FinishInputDialog", ActionFinishInputDialog},
  {"Quit", ActionQuit},
  {"Connection", ActionConnection},
  {"Command", ActionCommand},
  {"Display", ActionDisplay},
  {"Report", ActionReport},
  {"ListAct", ActionListAct},
  {"Mode", ActionMode},
  {"RemoveSelected", ActionRemoveSelected},
  {"DeleteRats", ActionDeleteRats},
  {"AddRats", ActionAddRats},
  {"MarkCrosshair", ActionMarkCrosshair},
  {"ChangeSize", ActionChangeSize},
  {"ChangeClearSize", ActionChangeClearSize},
  {"ChangeDrillSize", ActionChange2ndSize},
  {"ChangeName", ActionChangeName},
  {"ChangeSquare", ActionChangeSquare},
  {"ChangeOctagon", ActionChangeOctagon},
  {"ChangeJoin", ActionChangeJoin},
  {"Select", ActionSelect},
  {"Unselect", ActionUnselect},
  {"Save", ActionSave},
  {"Load", ActionLoad},
  {"Print", ActionPrint},
  {"New", ActionNew},
  {"SwapSides", ActionSwapSides},
  {"Bell", ActionBell},
  {"PasteBuffer", ActionPasteBuffer},
  {"Undo", ActionUndo},
  {"Redo", ActionRedo},
  {"RipUp", ActionRipUp},
  {"Polygon", ActionPolygon},
  {"EditLayerGroups", ActionEditLayerGroups},
  {"MoveToCurrentLayer", ActionMoveToCurrentLayer},
  {"SwitchDrawingLayer", ActionSwitchDrawingLayer},
  {"ToggleVisibility", ActionToggleVisibility},
  {"MoveObject", ActionMoveObject},
  {"djopt", ActionDJopt},
  {"GetLoc", ActionGetLocation},
  {"SetFlag", ActionSetFlag},
  {"ClrFlag", ActionClrFlag},
  {"ChangeFlag", ActionChangeFlag}
};

/* ---------------------------------------------------------------------------
 * handles callbacks from timer,
 * used to backup data
 */
static void
CB_Backup (XtPointer ClientData, XtIntervalId * ID)
{
  Backup ();

  /* restart timer */
  XtAppAddTimeOut (Context, 1000 * Settings.BackupInterval, CB_Backup, NULL);
}

/* ---------------------------------------------------------------------------
 * init toplevel shell and get application resources
 */
static void
InitShell (int *Argc, char **Argv)
{
  int i, x, y;
  unsigned int width, height;

  /* init application toplevel window, get resources */
/*
	Output.Toplevel = XtOpenApplication(&Context, "Pcb",
		CommandLineOptions, XtNumber(CommandLineOptions),
		Argc, Argv, Fallback, sessionShellWidgetClass, NULL, 0);
*/
  Output.Toplevel = XtAppInitialize (&Context, "Pcb",
				     CommandLineOptions,
				     XtNumber (CommandLineOptions), Argc,
				     Argv, Fallback, NULL, 0);

  XtVaSetValues (Output.Toplevel, XtNmappedWhenManaged, False, NULL);
  Dpy = XtDisplay (Output.Toplevel);

  /* clear structure and get resources */
  memset (&Settings, 0, sizeof (SettingType));
  XtGetApplicationResources (Output.Toplevel, &Settings,
			     ToplevelResources, XtNumber (ToplevelResources),
			     NULL, 0);
  if (Settings.LineThickness > MAX_LINESIZE
      || Settings.LineThickness < MIN_LINESIZE)
    Settings.LineThickness = 1000;
  if (Settings.ViaThickness > MAX_PINORVIASIZE ||
      Settings.ViaThickness < MIN_PINORVIASIZE)
    Settings.ViaThickness = 4000;
  if (Settings.ViaDrillingHole <= 0)
    Settings.ViaDrillingHole =
      DEFAULT_DRILLINGHOLE * Settings.ViaThickness / 100;

  /* parse geometry string, ignore offsets */
  i = XParseGeometry (Settings.Size, &x, &y, &width, &height);
  if (!(i & WidthValue) || !(i & HeightValue))
    {
      /* failed; use default setting */
      XParseGeometry (DEFAULT_SIZE, &x, &y, &width, &height);
    }
  Settings.MaxWidth =
    MIN (MAX_COORD, MAX ((BDimension) width * 100, MIN_SIZE));
  Settings.MaxHeight =
    MIN (MAX_COORD, MAX ((BDimension) height * 100, MIN_SIZE));
  Settings.Volume = MIN (100, MAX (-100, Settings.Volume));
  ParseGroupString (Settings.Groups, &Settings.LayerGroups);
  ParseRouteString (Settings.Routes, &Settings.RouteStyle[0], 1);
  InitGui ();
}

/* ---------------------------------------------------------------------------
 * initializes cursor position
 */
static void
InitCursorPosition (Widget Parent, Widget Top, Widget Left)
{
  Output.CursorPosition = XtVaCreateManagedWidget ("cursorPosition",
						   labelWidgetClass,
						   Parent,
						   XtNlabel, "",
						   XtNfromVert, Top,
						   XtNfromHoriz, Left,
						   LAYOUT_TOP, NULL);
}

/* ---------------------------------------------------------------------------
 * initializes statusline
 */
static void
InitStatusLine (Widget Parent, Widget Top, Widget Left)
{
  Output.StatusLine = XtVaCreateManagedWidget ("statusLine", labelWidgetClass,
					       Parent,
					       XtNlabel, "",
					       XtNresizable, True,
					       XtNresize, True,
					       XtNfromVert, Top,
					       XtNfromHoriz, Left,
					       LAYOUT_TOP, NULL);
}

/* ---------------------------------------------------------------------------
 * initialize output and porthole widget
 */
static void
InitPorthole (Widget Parent, Widget Top, Widget Left)
{
  /* porthole widget to handle panning */
  Output.Porthole = XtVaCreateManagedWidget ("porthole", portholeWidgetClass,
					     Parent,
					     XtNfromHoriz, Left,
					     XtNfromVert, Top,
					     LAYOUT_NORMAL, NULL);

  /* create simple widget to which I draw to (handled by porthole) */
  Output.Output = XtVaCreateManagedWidget ("output", simpleWidgetClass,
					   Output.Porthole,
					   XtNresizable, True,
					   XtNheight,
					   TO_SCREEN (Settings.MaxHeight),
					   XtNwidth,
					   TO_SCREEN (Settings.MaxWidth),
					   NULL);

  /* add event handlers */
  XtAddEventHandler (Output.Output,
		     ExposureMask | LeaveWindowMask | EnterWindowMask |
		     StructureNotifyMask | PointerMotionMask | KeyPressMask |
		     KeyReleaseMask, False, (XtEventHandler) OutputEvent,
		     NULL);
  XtAddEventHandler (Output.Porthole,
		     StructureNotifyMask | VisibilityChangeMask, False,
		     (XtEventHandler) PortholeEvent, NULL);
}

/* ----------------------------------------------------------------------
 * initializes icon pixmap and also cursor bit maps
 */
static void
InitIcon (void)
{
  Screen *screen;

  /* initialize icon pixmap */
  screen = XtScreen (Output.Toplevel);
  XtVaSetValues (Output.Toplevel,
		 XtNiconPixmap, XCreatePixmapFromBitmapData (Dpy,
							     XtWindow
							     (Output.
							      Toplevel),
							     icon_bits,
							     icon_width,
							     icon_height,
							     BlackPixelOfScreen
							     (screen),
							     WhitePixelOfScreen
							     (screen),
							     DefaultDepthOfScreen
							     (screen)), NULL);
  XC_clock_source =
    XCreateBitmapFromData (Dpy, XtWindow (Output.Toplevel), rotateIcon_bits,
			   rotateIcon_width, rotateIcon_height);
  XC_clock_mask =
    XCreateBitmapFromData (Dpy, XtWindow (Output.Toplevel), rotateMask_bits,
			   rotateMask_width, rotateMask_height);

  XC_hand_source =
    XCreateBitmapFromData (Dpy, XtWindow (Output.Toplevel), handIcon_bits,
			   handIcon_width, handIcon_height);
  XC_hand_mask =
    XCreateBitmapFromData (Dpy, XtWindow (Output.Toplevel), handMask_bits,
			   handMask_width, handMask_height);

  XC_lock_source =
    XCreateBitmapFromData (Dpy, XtWindow (Output.Toplevel), lockIcon_bits,
			   lockIcon_width, lockIcon_height);
  XC_lock_mask =
    XCreateBitmapFromData (Dpy, XtWindow (Output.Toplevel), lockMask_bits,
			   lockMask_width, lockMask_height);
}

/* ----------------------------------------------------------------------
 * initializes GC 
 * create a public GC for drawing with background color, font
 * and fill rules for polygons
 * the foreground color is set by some routines in draw.c
 */
static void
InitGC (void)
{
  Cardinal i;

  XtVaGetValues (Output.Output, XtNbackground, &Settings.bgColor, NULL);
  Output.fgGC = XCreateGC (Dpy, Output.OutputWindow, 0, NULL);
  Output.bgGC = XCreateGC (Dpy, Output.OutputWindow, 0, NULL);
  Output.pmGC = (GC) 0;
  Output.GridGC = XCreateGC (Dpy, Output.OutputWindow, 0, NULL);
  if (!VALID_GC ((long int) Output.fgGC) ||
      !VALID_GC ((long int) Output.bgGC) ||
      !VALID_GC ((long int) Output.GridGC
		 || !VALID_GC ((long int) Output.pmGC)))
    MyFatal ("can't create default GC\n");

  XSetForeground (Dpy, Output.bgGC, Settings.bgColor);
  XSetFillRule (Dpy, Output.fgGC, WindingRule);
  XSetFillStyle (Dpy, Output.fgGC, FillSolid);
  Stipples = MyCalloc (9, sizeof (Pixmap), "main()");
  for (i = 0; i < 9; i++)
    {
      Stipples[i] = XCreateBitmapFromData (Dpy, Output.OutputWindow,
					   gray_bits[i], gray_size,
					   gray_size);
    }
  XSetState (Dpy, Output.GridGC, Settings.GridColor, Settings.bgColor,
	     GXinvert, AllPlanes);
  XSetLineAttributes (Dpy, Output.GridGC, 2, LineSolid, CapButt, JoinMiter);

  /* dont create graphic expose events from XCopyArea() */
  XSetGraphicsExposures (Dpy, Output.fgGC, False);
}

/* ----------------------------------------------------------------------
 * initialize signal and error handlers
 */
static void
InitHandler (void)
{
  /* install a new error handler and catch terminating signals */
  XtAppSetErrorHandler (Context, X11ErrorHandler);

/*
	signal(SIGHUP, CatchSignal);
	signal(SIGQUIT, CatchSignal);
	signal(SIGABRT, CatchSignal);
	signal(SIGSEGV, CatchSignal);
	signal(SIGTERM, CatchSignal);
	signal(SIGINT, CatchSignal);
*/

  /* calling external program by popen() may cause a PIPE signal,
   * so we ignore it
   */
  signal (SIGPIPE, SIG_IGN);
}

/* ---------------------------------------------------------------------- 
 * initialize widgets
 */
static void
InitWidgets (void)
{
  Screen *screen;

  screen = XtScreen (Output.Toplevel);
  /* this form widget is a container for most of the other widgets */
  Output.MasterForm = XtVaCreateManagedWidget ("masterForm", formWidgetClass,
					       Output.Toplevel,
					       XtNresizable, True, NULL);

  XtAppAddActions (Context, Actions, XtNumber (Actions));
  XtAppAddActions (Context, ActionList, ActionListSize);

  /* init all other widgets as childs of the masterform */
  InitMenu (Output.MasterForm, NULL, NULL);
  InitControlPanel (Output.MasterForm, Output.Menu, NULL);
  InitCursorPosition (Output.MasterForm, NULL, Output.Menu);
  InitPorthole (Output.MasterForm, Output.Menu, Output.Control);
  InitStatusLine (Output.MasterForm, Output.Porthole, Output.Control);

  /* set actions and install translations;
   * the menu must be initialized before the actions are added
   * for XawPositionSimpleMenu() to be available
   */
  XtAugmentTranslations (Output.Toplevel,
			 XtParseTranslationTable (DefaultTranslations));

  MenuSetAccelerators (Output.Output);

  /* realize the tree, get the IDs of the output window
   * and initialize some identifiers in draw.c
   */
  XtRealizeWidget (Output.Toplevel);

/* the Mode buttons need the Toplevel widget to be realized before
 * they can be initialized
 */
  InitModeButtons (Output.MasterForm, Output.Control, NULL);
  Output.OutputWindow = XtWindow (Output.Output);
/* start at full screen size */
  XtMakeResizeRequest (Output.Toplevel, WidthOfScreen (screen) - 10,
		       HeightOfScreen (screen) - 25, NULL, NULL);
  InitGC ();
  InitCrosshair ();
  InitIcon ();
  InitHandler ();
  InitBuffers ();
  SetMode (NO_MODE);

  /* set window manager property to get 'delete window' messages */
  WMDeleteWindowAtom = XInternAtom (Dpy, "WM_DELETE_WINDOW", True);
  if (WMDeleteWindowAtom != None)
    XSetWMProtocols (Dpy, XtWindow (Output.Toplevel), &WMDeleteWindowAtom, 1);
}

/* ---------------------------------------------------------------------------
 * init device drivers for printing
 */
static void
InitDeviceDriver (void)
{
  DeviceInfoTypePtr ptr = PrintingDevice;

  while (ptr->Query)
    {
      ptr->Info = ptr->Query ();
      ptr++;
    }
}

/* ---------------------------------------------------------------------- 
 * main program
 */
int
main (int argc, char *argv[])
{
  Cardinal i;
  XRectangle Big;

  /* init application:
   * - make program name available for error handlers
   * - evaluate special options
   * - initialize toplevel shell and resources
   * - create an empty PCB with default symbols
   * - initialize all other widgets
   * - update screen and get size of drawing area
   * - evaluate command-line arguments
   * - register 'call on exit()' function
   */
  if ((Progname = strrchr (*argv, '/')) == NULL)
    Progname = *argv;
  else
    Progname++;

  /* take care of special options */
  if (argc == 2)
    {
      if (!strcmp ("-help", argv[1]) ||
	  !strcmp ("--help", argv[1]) )
	Usage ();
      if (!strcmp ("-copyright", argv[1]) ||
	  !strcmp ("--copyright", argv[1]) )
	Copyright ();
      if (!strcmp ("-version", argv[1]) ||
	  !strcmp ("--version", argv[1]) )
	{
	  puts (VERSION);
	  exit (0);
	}
    }

  InitShell (&argc, argv);
  if (Settings.DumpMenuFile)
    DumpMenu();

  InitDeviceDriver ();
  PCB = CreateNewPCB (True);
  ResetStackAndVisibility ();
  CreateDefaultFont ();
  InitWidgets ();

  /* update zoom and grid... */
  UpdateSettingsOnScreen ();
  GetSizeOfDrawingArea ();
  UpRegion = XCreateRegion ();
  FullRegion = XCreateRegion ();
  Big.x = Big.y = 0;
  Big.width = Big.height = TO_SCREEN(MAX_COORD);
  XUnionRectWithRegion (&Big, FullRegion, FullRegion);
  InitErrorLog ();
  switch (--argc)
    {
    case 0:			/* only program name */
      SetZoom(PCB->Zoom);
      break;

    case 1:			/* load an initial layout;
				 * abort if filename looks like an option
				 */
      ++argv;
      if (**argv == '-')
	Usage ();

      /* keep filename even if initial load command failed;
       * file might not exist
       */
      if (LoadPCB (*argv))
	PCB->Filename = MyStrdup (*argv, "main()");
      break;

    default:			/* error */
      Usage ();
      break;
    }
  LoadBackgroundImage (Settings.BackgroundImage);

  /* Register a function to be called when the program terminates.
   * This makes sure that data is saved even if LEX/YACC routines
   * abort the program.
   * If the OS doesn't have at least one of them,
   * the critical sections will be handled by parse_l.l
   */
#ifdef HAS_ON_EXIT
  on_exit (GlueEmergencySave, NULL);
#else
#ifdef HAS_ATEXIT
  atexit (EmergencySave);
#endif
#endif

  /* popup main window and initialize error log window
   * If the window isn't mapped the statusline won't be resizable
   * (dont know why)
   */
  XMapWindow (Dpy, XtWindow (Output.Toplevel));

  /* read the library file and display it if it's not empty */
  if (!ReadLibraryContents () && Library.MenuN)
    InitLibraryWindow (Output.Toplevel);

  /* start backup timer */
  XtAppAddTimeOut (Context, 1000 * Settings.BackupInterval, CB_Backup, NULL);
#ifdef HAVE_LIBSTROKE
  stroke_init ();
#endif


  /*
   * see if we have a startup actions file to execute.  This
   * may be from the -script flag or an Xresources setting
   */

  if (Settings.ScriptFilename != NULL)
  {
	String Param[1];
	Cardinal nparam=1;

	Message("Executing startup script file %s\n", 
		Settings.ScriptFilename);
	Param[0] = Settings.ScriptFilename;
	ActionExecuteFile(Output.Output, NULL, Param, &nparam);
  }

  XtAppMainLoop (Context);
  for (i = 0; i < 9; i++)
    XFreePixmap (Dpy, Stipples[i]);
  return (0);
}
