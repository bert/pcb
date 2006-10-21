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
 *  RCS: $Id$
 */

/* definition of types
 */

/* Change History:
 * 10/11/96 11:37 AJF Added support for a Text() driver function.
 * This was done out of a pressing need to force text to be printed on the
 * silkscreen layer. Perhaps the design is not the best.
 */

#ifndef	__GLOBAL_INCLUDED__
#define	__GLOBAL_INCLUDED__

#include "const.h"
#include "macro.h"
#include "polyarea.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include <sys/types.h>

#include "hid.h"

#define _(S) (S)

typedef int LocationType;
typedef int BDimension;		/* big dimension */


#ifndef XtSpecificationRelease
typedef unsigned int Cardinal;
typedef char Boolean;
/*typedef unsigned int	Pixel;*/
typedef char *String;
typedef short Position;
typedef short Dimension;
#endif
typedef unsigned char BYTE;

#define True	1
#define False	0

/* Nobody should know about the internals of this except the macros in
   macros.h that access it.  This structure must be simple-assignable
   for now.  */
typedef struct
{
  unsigned short f;		/* generic flags */
  unsigned char t[(MAX_LAYER + 1) / 2];	/* thermals */
} FlagType, *FlagTypePtr;

#ifndef __GNUC__
#define __FUNCTION1(a,b) a ":" #b
#define __FUNCTION2(a,b) __FUNCTION1(a,b)
#define __FUNCTION__ __FUNCTION2(__FILE__,__LINE__)
#endif

/* ---------------------------------------------------------------------------
 * Do not change the following definitions even if they're not very
 * nice.  It allows us to have functions act on these "base types" and
 * not need to know what kind of actual object they're working on.
 */

/* Any object that uses the "object flags" defined in const.h, or
   exists as an object on the pcb, MUST be defined using this as the
   first fields, either directly or through ANYLINEFIELDS.  */
#define ANYOBJECTFIELDS			\
	BoxType		BoundingBox;	\
	long int	ID;		\
	FlagType	Flags

/* Lines, pads, and rats all use this so they can be cross-cast.  */
#define	ANYLINEFIELDS			\
	ANYOBJECTFIELDS;		\
	BDimension	Thickness,      \
                        Clearance;      \
	PointType	Point1,		\
			Point2

/* ---------------------------------------------------------------------------
 * some useful values of our widgets
 */
typedef struct			/* holds information about output window */
{
#if 0
  GtkWidget *top_window,	/* toplevel widget */
   *drawing_area;		/* PCB drawing area */

  GdkPixmap *pixmap, *mask;
    PangoFontDescription * font_desc;
  PangoLayout *layout;
  int font_size;		/* pin name size levels depending on zoom */

#endif
  hidGC bgGC,			/* background and foreground; */
    fgGC,			/* changed from some routines */
    pmGC,			/* depth 1 pixmap GC to store clip */
    GridGC;			/* for the grid */

  int Width,			/* sizes of output window (porthole) */
    Height;
#if 0
  GdkCursor *XCursor;		/* used X cursor */
  GdkCursorType XCursorShape;	/* and its shape */
  gboolean VisibilityOK,	/* output is completely visible */
    creating, has_entered;
#endif
  int oldObjState,		/* Helpers for GetLocation */
    oldLineState, oldBoxState;
}
OutputType, *OutputTypePtr;

/* ----------------------------------------------------------------------
 * layer group. A layer group identifies layers which are always switched
 * on/off together.
 */
typedef struct
{
  Cardinal Number[MAX_LAYER],	/* number of entries per groups */
    Entries[MAX_LAYER][MAX_LAYER + 2];
} LayerGroupType, *LayerGroupTypePtr;

typedef struct BoxType		/* a bounding box */
{
  LocationType X1, Y1,		/* upper left */
    X2, Y2;			/* and lower right corner */
} BoxType, *BoxTypePtr;

typedef struct
{
  LocationType x, y;
  int width, height;
} RectangleType, *RectangleTypePtr;

typedef struct
{
  char *name;
  char *value;
} AttributeType, *AttributeTypePtr;

typedef struct
{
  int Number, Max;
  AttributeType *List;
} AttributeListType, *AttributeListTypePtr;

/* ---------------------------------------------------------------------------
 * the basic object types supported by PCB
 */

/* All on-pcb objects (elements, lines, pads, vias, rats, etc) are
   based on this. */
typedef struct {
  ANYOBJECTFIELDS;
} AnyObjectType, *AnyObjectTypePtr;

typedef struct			/* a line/polygon point */
{
  LocationType X, Y, X2, Y2;	/* so Point type can be cast as BoxType */
  long int ID;
} PointType, *PointTypePtr;

/* Lines, rats, pads, etc.  */
typedef struct {
  ANYLINEFIELDS;
} AnyLineObjectType, *AnyLineObjectTypePtr;

typedef struct			/* holds information about one line */
{
  ANYLINEFIELDS;
  char *Number;
} LineType, *LineTypePtr;

typedef struct
{
  ANYOBJECTFIELDS;
  BDimension Scale;		/* text scaling in percent */
  LocationType X,		/* origin */
    Y;
  BYTE Direction;
  char *TextString;		/* string */
  void *Element;
} TextType, *TextTypePtr;

typedef struct			/* holds information about a polygon */
{
  ANYOBJECTFIELDS;
  Cardinal PointN,		/* number of points in polygon */
    PointMax;			/* max number from malloc() */
  POLYAREA *Clipped;		/* the clipped region of this polygon */
  PointTypePtr Points;		/* data */
} PolygonType, *PolygonTypePtr;

typedef struct			/* holds information about arcs */
{
  ANYOBJECTFIELDS;
  BDimension Thickness, Clearance;
  LocationType Width,		/* length of axis */
    Height, X,			/* center coordinates */
    Y;
  long int StartAngle,		/* the two limiting angles in degrees */
    Delta;
} ArcType, *ArcTypePtr;

typedef struct
{
  struct rtree_node *root;
  int size;			/* number of entries in tree */
} rtree_t;

typedef struct			/* holds information about one layer */
{
  char *Name;			/* layer name */
  Cardinal LineN,		/* number of lines */
    TextN,			/* labels */
    PolygonN,			/* polygons */
    ArcN,			/* and arcs */
    LineMax,			/* max number from malloc() */
    TextMax, PolygonMax, ArcMax;
  LineTypePtr Line;		/* pointer to additional structures */
  TextTypePtr Text;
  PolygonTypePtr Polygon;
  ArcTypePtr Arc;
  rtree_t *line_tree, *text_tree, *polygon_tree, *arc_tree;
  Boolean On;			/* visible flag */
  char *Color,			/* color */
   *SelectedColor;
}
LayerType, *LayerTypePtr;

typedef struct			/* a rat-line */
{
  ANYLINEFIELDS;
  Cardinal group1, group2;	/* the layer group each point is on */
} RatType, *RatTypePtr;

typedef struct			/* a SMD pad */
{
  ANYLINEFIELDS;
  BDimension Mask;
  char *Name, *Number;		/* 'Line' */
  void *Element;
  void *Spare;
} PadType, *PadTypePtr;

typedef struct
{
  ANYOBJECTFIELDS;
  BDimension Thickness, Clearance, Mask, DrillingHole;
  LocationType X,		/* center and diameter */
    Y;
  char *Name, *Number;
  void *Element;
  void *Spare;
} PinType, *PinTypePtr, **PinTypeHandle;

typedef struct
{
  ANYOBJECTFIELDS;
  TextType Name[MAX_ELEMENTNAMES];	/* the elements names; */
  /* description text */
  /* name on PCB second, */
  /* value third */
  /* see macro.h */
  LocationType MarkX,		/* position mark */
    MarkY;
  Cardinal PinN,		/* number of pins, lines and arcs */
    PinMax, PadN, PadMax, LineN, LineMax, ArcN, ArcMax;
  PinTypePtr Pin;		/* pin description */
  PadTypePtr Pad;		/* pad description of SMD components */
  LineTypePtr Line;
  ArcTypePtr Arc;
  BoxType VBox;
  AttributeListType Attributes;
} ElementType, *ElementTypePtr, **ElementTypeHandle;

/* ---------------------------------------------------------------------------
 * symbol and font related stuff
 */
typedef struct			/* a single symbol */
{
  LineTypePtr Line;
  Boolean Valid;
  Cardinal LineN,		/* number of lines */
    LineMax;
  BDimension Width,		/* size of cell */
    Height, Delta;		/* distance to next symbol in 0.00001'' */
} SymbolType, *SymbolTypePtr;

typedef struct			/* complete set of symbols */
{
  LocationType MaxHeight,	/* maximum cell width and height */
    MaxWidth;
  BoxType DefaultSymbol;	/* the default symbol is a filled box */
  SymbolType Symbol[MAX_FONTPOSITION + 1];
  Boolean Valid;
} FontType, *FontTypePtr;

typedef struct			/* holds all objects */
{
  Cardinal ViaN,		/* number of vias */
    ViaMax,			/* max number from malloc() */
    ElementN,			/* and elements */
    ElementMax,			/* max number from malloc() */
    RatN,			/* and rat-lines */
    RatMax;
  int LayerN;			/* number of layers in this board */
  PinTypePtr Via;		/* pointer to object data */
  ElementTypePtr Element;
  RatTypePtr Rat;
  rtree_t *via_tree, *element_tree, *pin_tree, *pad_tree, *name_tree[3],	/* for element names */
   *rat_tree;
  void *pcb;
  LayerType Layer[MAX_LAYER + 2];	/* add 2 silkscreen layers */
} DataType, *DataTypePtr;

typedef struct			/* holds drill information */
{
  BDimension DrillSize;		/* this drill's diameter */
  Cardinal ElementN,		/* the number of elements using this drill size */
    ElementMax,			/* max number of elements from malloc() */
    PinCount,			/* number of pins drilled this size */
    ViaCount,			/* number of vias drilled this size */
    UnplatedCount,		/* number of these holes that are unplated */
    PinN,			/* number of drill coordinates in the list */
    PinMax;			/* max number of coordinates from malloc() */
  PinTypePtr *Pin;		/* coordinates to drill */
  ElementTypePtr *Element;	/* a pointer to an array of element pointers */
} DrillType, *DrillTypePtr;

typedef struct			/* holds a range of Drill Infos */
{
  Cardinal DrillN,		/* number of drill sizes */
    DrillMax;			/* max number from malloc() */
  DrillTypePtr Drill;		/* plated holes */
} DrillInfoType, *DrillInfoTypePtr;

typedef struct
{
  BDimension Thick,		/* line thickness */
    Diameter,			/* via diameter */
    Hole,			/* via drill hole */
    Keepaway;			/* min. separation from other nets */
  char *Name;
  int index;
} RouteStyleType, *RouteStyleTypePtr;

/* ---------------------------------------------------------------------------
 * structure used by library routines
 */
typedef struct
{
  char *ListEntry;		/* the string for the selection box */
  char *AllocatedMemory,	/* pointer to allocated memory; all others */
    /* point to parts of the string */
   *Template,			/* m4 template name */
   *Package,			/* package */
   *Value,			/* the value field */
   *Description;		/* some descritional text */
} LibraryEntryType, *LibraryEntryTypePtr;

typedef struct
{
  char *Name,			/* name of the menu entry */
   *directory,			/* Directory name library elements are from */
   *Style;			/* routing style */
  Cardinal EntryN,		/* number of objects */
    EntryMax;			/* number of reserved memory locations */
  LibraryEntryTypePtr Entry;	/* the entries */
  int flag;			/* used by the netlist window to enable/disable nets */
} LibraryMenuType, *LibraryMenuTypePtr;

typedef struct
{
  Cardinal MenuN;               /* number of objects */
  Cardinal MenuMax;             /* number of reserved memory locations */
  LibraryMenuTypePtr Menu;      /* the entries */
} LibraryType, *LibraryTypePtr;


  /* The PCBType struct holds information about board layout most of which is
     |  saved with the layout.  A new PCB layout struct is first initialized
     |  with values from the user configurable Settings struct and then reset
     |  to the saved layout values when a layout is loaded.
     |  This struct is also used for the remove list and for buffer handling
   */
typedef struct
{
  long ID;			/* see macro.h */
  FlagType Flags;
  char *Name,			/* name of board */
   *Filename,			/* name of file (from load) */
   *PrintFilename,		/* from print dialog */
   *Netlistname,		/* name of netlist file */
    ThermStyle;			/* type of thermal to place with thermal tool */
  Boolean Changed,		/* layout has been changed */
    ViaOn,			/* visibility flags */
    ElementOn, RatOn, InvisibleObjectsOn, PinOn, SilkActive,	/* active layer is actually silk */
    RatDraw;			 /* we're drawing rats */
  char *ViaColor,		/* some colors */
   *ViaSelectedColor,
    *PinColor,
    *PinSelectedColor,
    *PinNameColor,
    *ElementColor,
    *RatColor,
    *InvisibleObjectsColor,
    *InvisibleMarkColor,
    *ElementSelectedColor,
    *RatSelectedColor, *ConnectedColor, *WarnColor, *MaskColor;
  long CursorX,			/* cursor position as saved with layout */
    CursorY, Clipping;
  int Bloat,			/* drc sizes saved with layout */
    Shrink, minWid, minSlk, minDrill, minRing;
  int GridOffsetX,		/* as saved with layout */
    GridOffsetY, MaxWidth,	/* allowed size */
    MaxHeight;

  double Grid,			/* used grid with offsets */
    Zoom,			/* zoom factor */
    IsleArea,			/* minimum poly island to retain */
    ThermScale;			/* scale factor used with thermals */
  FontType Font;
  LayerGroupType LayerGroups;
  RouteStyleType RouteStyle[NUM_STYLES];
  LibraryType NetlistLib;
  AttributeListType Attributes;
  DataTypePtr Data;		/* entire database */
}
PCBType, *PCBTypePtr;

typedef struct			/* information about the paste buffer */
{
  LocationType X,		/* offset */
    Y;
  BoxType BoundingBox;
  DataTypePtr Data;		/* data; not all members of PCBType */
  /* are used */
} BufferType, *BufferTypePtr;

/* ---------------------------------------------------------------------------
 * some types for cursor drawing, setting of block and lines
 * as well as for merging of elements
 */
typedef struct			/* rubberband lines for element moves */
{
  LayerTypePtr Layer;		/* layer that holds the line */
  LineTypePtr Line;		/* the line itself */
  PointTypePtr MovedPoint;	/* and finally the point */
} RubberbandType, *RubberbandTypePtr;

typedef struct			/* current marked line */
{
  PointType Point1,		/* start- and end-position */
    Point2;
  long int State;
  Boolean draw;
} AttachedLineType, *AttachedLineTypePtr;

typedef struct			/* currently marked block */
{
  PointType Point1,		/* start- and end-position */
    Point2;
  long int State;
  Boolean otherway;
} AttachedBoxType, *AttachedBoxTypePtr;

typedef struct			/* currently attached object */
{
  LocationType X,		/* saved position when MOVE_MODE */
    Y;				/* was entered */
  BoxType BoundingBox;
  long int Type,		/* object type */
    State;
  void *Ptr1,			/* three pointers to data, see */
   *Ptr2,			/* search.c */
   *Ptr3;
  Cardinal RubberbandN,		/* number of lines in array */
    RubberbandMax;
  RubberbandTypePtr Rubberband;
} AttachedObjectType, *AttachedObjectTypePtr;

typedef struct			/* holds cursor information */
{
  hidGC GC,			/* GC for cursor drawing */
    AttachGC;			/* and for displaying buffer contents */
  LocationType X,		/* position in PCB coordinates */
    Y, MinX,			/* lowest and highest coordinates */
    MinY, MaxX, MaxY;
  Boolean On;			/* flag for 'is visible' */
  AttachedLineType AttachedLine;	/* data of new lines... */
  AttachedBoxType AttachedBox;
  PolygonType AttachedPolygon;
  AttachedObjectType AttachedObject;	/* data of attached objects */
} CrosshairType, *CrosshairTypePtr;

typedef struct
{
  Boolean status;
  long int X, Y;
} MarkType, *MarkTypePtr;

/* ---------------------------------------------------------------------------
 * our resources
 * most of them are used as default when a new design is started
 */
typedef struct			/* some resources... */
{
  Boolean grid_units_mm;

  int verbose;

  char *BlackColor, *WhiteColor, *BackgroundColor,	/* background and cursor color ... */
   *CrosshairColor,		/* different object colors */
   *CrossColor,
    *ViaColor,
    *ViaSelectedColor,
    *PinColor,
    *PinSelectedColor,
    *PinNameColor,
    *ElementColor,
    *RatColor,
    *InvisibleObjectsColor,
    *InvisibleMarkColor,
    *ElementSelectedColor,
    *RatSelectedColor,
    *ConnectedColor,
    *OffLimitColor,
    *GridColor,
    *LayerColor[MAX_LAYER],
    *LayerSelectedColor[MAX_LAYER], *WarnColor, *MaskColor;
  int ViaThickness,		/* some preset values */
    ViaDrillingHole, LineThickness, RatThickness, Keepaway, MaxWidth,	/* default size of a new layout */
    MaxHeight, TextScale,	/* text scaling in % */
    AlignmentDistance, Bloat,	/* default drc sizes */
    Shrink, minWid, minSlk, minDrill, minRing;
  double Grid,			/* grid 0.001'' */
    IsleArea,    		/* polygon min area */
    grid_increment_mm,		/* key g and <shift>g value for mil units */
    grid_increment_mil,		/* key g and <shift>g value for mil units */
    size_increment_mm,		/* key s and <shift>s value for mil units */
    size_increment_mil,		/* key s and <shift>s value for mil units */
    line_increment_mm, line_increment_mil, clear_increment_mm, clear_increment_mil, Zoom,	/* number of shift operations for zooming */
    PinoutZoom;			/* same for pinout windows */
  int PinoutNameLength,		/* max displayed length of a pinname */
    Volume,			/* the speakers volume -100..100 */
    CharPerLine,		/* width of an output line in characters */
    Mode,			/* currently active mode */
    BufferNumber,		/* number of the current buffer */
    GridFactor;			/* factor used for grid-drawing */
  int BackupInterval;		/* time between two backups in seconds */
  char *DefaultLayerName[MAX_LAYER], *FontCommand,	/* commands for file loading... */
   *FileCommand, *ElementCommand, *PrintFile, *LibraryCommandDir, *LibraryCommand, *LibraryContentsCommand, *LibraryTree,	/* path to library tree */
   *SaveCommand, *LibraryFilename, *FontFile,	/* name of default font file */
   *Groups,			/* string with layergroups */
   *Routes,			/* string with route styles */
   *FilePath, *RatPath, *RatCommand, *FontPath, *PinoutFont, *ElementPath, *LibraryPath, *Size,	/* geometry string for size */
   *Media, *MenuFile,		/* file containing menu definitions */
   *BackgroundImage,		/* PPM file for board background */
   *ScriptFilename,		/* PCB Actions script to execute on startup */
   *ActionString,		/* PCB Actions string to execute on startup */
   *FabAuthor;			/* Full name of author for FAB drawings */
  Boolean DumpMenuFile;		/* dump internal menu definitions */
  LocationType PinoutOffsetX,	/* offset of origin */
    PinoutOffsetY;
  int PinoutTextOffsetX,	/* offset of text from pin center */
    PinoutTextOffsetY;
  RouteStyleType RouteStyle[NUM_STYLES];	/* default routing styles */
  LayerGroupType LayerGroups;	/* default layer groups */
  Boolean ClearLine, UniqueNames,	/* force unique names */
    SnapPin,			/* snap to pins and pads */
    UseLogWindow,		/* FIXME? Used in hid/Xaw code only */
    RaiseLogWindow,		/* raise log window if iconified */
    ShowSolderSide,		/* mirror output */
    SaveLastCommand,		/* save the last command entered by user */
    SaveInTMP,			/* always save data in /tmp */
    DrawGrid,			/* draw grid points */
    RatWarn,			/* rats nest has set warnings */
    StipplePolygons,		/* draw polygons with stipple */
    AllDirectionLines,		/* enable lines to all directions */
    RubberBandMode,		/* move, rotate use rubberband connections */
    SwapStartDirection,		/* change starting direction after each click */
    ShowDRC,			/* show drc region on crosshair */
    AutoDRC,			/* */
    ShowNumber,			/* pinout shows number */
    OrthogonalMoves,		/* */
    ResetAfterElement,		/* reset connections after each element */
    liveRouting,		/* autorouter shows tracks in progress */
    RingBellWhenFinished,	/* flag if a signal should be */
    /* produced when searching of */
    /* connections is done */
    AutoPlace;			/* flag which says we should force placement of the
				   windows on startup */
  int HistorySize,		/* FIXME? Used in hid/xaw code only. */
    init_done;
}
SettingType, *SettingTypePtr;

/* ----------------------------------------------------------------------
 * pointer to low-level copy, move and rotate functions
 */
typedef struct
{
  void *(*Line) (LayerTypePtr, LineTypePtr);
  void *(*Text) (LayerTypePtr, TextTypePtr);
  void *(*Polygon) (LayerTypePtr, PolygonTypePtr);
  void *(*Via) (PinTypePtr);
  void *(*Element) (ElementTypePtr);
  void *(*ElementName) (ElementTypePtr);
  void *(*Pin) (ElementTypePtr, PinTypePtr);
  void *(*Pad) (ElementTypePtr, PadTypePtr);
  void *(*LinePoint) (LayerTypePtr, LineTypePtr, PointTypePtr);
  void *(*Point) (LayerTypePtr, PolygonTypePtr, PointTypePtr);
  void *(*Arc) (LayerTypePtr, ArcTypePtr);
  void *(*Rat) (RatTypePtr);
} ObjectFunctionType, *ObjectFunctionTypePtr;

/* ---------------------------------------------------------------------------
 * structure used by device drivers
 */

typedef struct			/* holds a connection */
{
  LocationType X, Y;		/* coordinate of connection */
  long int type;		/* type of object in ptr1 - 3 */
  void *ptr1, *ptr2;		/* the object of the connection */
  Cardinal group;		/* the layer group of the connection */
  LibraryMenuType *menu;	/* the netmenu this *SHOULD* belong too */
} ConnectionType, *ConnectionTypePtr;

typedef struct			/* holds a net of connections */
{
  Cardinal ConnectionN,		/* the number of connections contained */
    ConnectionMax;		/* max connections from malloc */
  ConnectionTypePtr Connection;
  RouteStyleTypePtr Style;
} NetType, *NetTypePtr;

typedef struct			/* holds a list of nets */
{
  Cardinal NetN,		/* the number of subnets contained */
    NetMax;			/* max subnets from malloc */
  NetTypePtr Net;
} NetListType, *NetListTypePtr;

typedef struct			/* holds a list of net lists */
{
  Cardinal NetListN,		/* the number of net lists contained */
    NetListMax;			/* max net lists from malloc */
  NetListTypePtr NetList;
} NetListListType, *NetListListTypePtr;

typedef struct			/* holds a generic list of pointers */
{
  Cardinal PtrN,		/* the number of pointers contained */
    PtrMax;			/* max subnets from malloc */
  void **Ptr;
} PointerListType, *PointerListTypePtr;

typedef struct
{
  Cardinal BoxN,		/* the number of boxes contained */
    BoxMax;			/* max boxes from malloc */
  BoxTypePtr Box;

} BoxListType, *BoxListTypePtr;


/* ---------------------------------------------------------------------------
 * define supported types of undo operations
 * note these must be separate bits now
 */
#define	UNDO_CHANGENAME			0x0001	/* change of names */
#define	UNDO_MOVE			0x0002	/* moving objects */
#define	UNDO_REMOVE			0x0004	/* removing objects */
#define	UNDO_REMOVE_POINT		0x0008	/* removing polygon/... points */
#define	UNDO_INSERT_POINT		0x0010	/* inserting polygon/... points */
#define	UNDO_ROTATE			0x0020	/* rotations */
#define	UNDO_CREATE			0x0040	/* creation of objects */
#define	UNDO_MOVETOLAYER		0x0080	/* moving objects to */
#define UNDO_FLAG			0x0100	/* toggling SELECTED flag */
#define UNDO_CHANGESIZE			0x0200	/* change size of object */
#define UNDO_CHANGE2NDSIZE		0x0400	/* change 2ndSize of object */
#define UNDO_MIRROR			0x0800	/* change side of board */
#define UNDO_CHANGECLEARSIZE		0x1000	/* change clearance size */
#define UNDO_CHANGEMASKSIZE		0x2000	/* change mask size */
#define UNDO_CHANGEANGLES		0x4000	/* change arc angles */
#define UNDO_LAYERCHANGE		0x8000	/* layer new/delete/move */
#define UNDO_CLEAR		       0x10000  /* clear/restore to polygons */
#define UNDO_NETLISTCHANGE     	       0x20000	/* netlist change */


/* ---------------------------------------------------------------------------
 * add a macro for wrapping RCS ID's in so that ident will still work
 * but we won't get as many compiler warnings
 */

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 1000 + __GNUC_MINOR__)
#endif /* GCC_VERSION */

#if GCC_VERSION > 2007
#define ATTRIBUTE_UNUSED __attribute__((unused))
#else
#define ATTRIBUTE_UNUSED
#endif

#define RCSID(x) static char *rcsid  ATTRIBUTE_UNUSED = x

/* ---------------------------------------------------------------------------
 * Macros called by various action routines to show usage or to report
 * a syntax error and fail
 */
#define AUSAGE(x) Message ("Usage:\n%s\n", (x##_syntax))
#define AFAIL(x) { Message ("Syntax error.  Usage:\n%s\n", (x##_syntax)); return 1; }

#endif /* __GLOBAL_INCLUDED__  */
