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

#include <stdio.h>
#include <X11/Xmd.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/keysym.h>
typedef int Location;
typedef int BDimension;		/* big dimension */

/* ---------------------------------------------------------------------------
 * do not change the following definition even if it's not very nice.
 * It allows us to cast PadTypePtr and RatTypePtr to LineTypePtr and access
 * their coordinates as a line.
 */
#define	LINESTRUCT			\
	long int		ID,		\
			Flags;		\
	BDimension	Thickness,      \
                        Clearance;      \
	PointType	Point1,		\
			Point2;

/* ---------------------------------------------------------------------------
 * some useful values of our widgets
 */
typedef struct			/* holds information about output window */
{
  Widget Toplevel,		/* toplevel widget */
    StatusLine,			/* label widget of statusline */
    CursorPosition,		/* current cursor position */
    Control,			/* control widget */
    Menu,			/* popup menu */
    Message,			/* fields for user message */
    MasterForm,			/* the widgets thats hols all the others */
    Porthole,			/* porthole widget to pan output */
    panner,			/* panner widget for panning */
    Output;			/* simple widget for drawing */
  Window OutputWindow;		/* window ID of simple widget */
  GC bgGC,			/* background and foreground; */
    fgGC,			/* changed from some routines */
    pmGC,			/* pixmap GC to store clip */
    GridGC;			/* for the grid */
  BDimension cw,		/* canvas width/height */
    ch, Width,			/* sizes of output window (porthole) */
    Height;
  Cursor XCursor;		/* used X cursor */
  unsigned long int XCursorShape;	/* and its shape (cursorfont) */
  Boolean VisibilityOK;		/* output is completely visible */
} OutputType, *OutputTypePtr;

/* ----------------------------------------------------------------------
 * layer group. A layer group identifies layers which are always switched
 * on/off together.
 */
typedef struct
{
  Cardinal Number[MAX_LAYER],	/* number of entries per groups */
    Entries[MAX_LAYER][MAX_LAYER + 2];
} LayerGroupType, *LayerGroupTypePtr;

typedef struct			/* a bounding box */
{
  Location X1, Y1,		/* upper left */
    X2, Y2;			/* and lower right corner */
} BoxType, *BoxTypePtr;

/* ---------------------------------------------------------------------------
 * the basic object types supported by PCB
 */
typedef struct			/* all objects start with a Bounding box and ID */
{
  BoxType BoundingBox;
  long int ID;
} AnyObjectType, *AnyObjectTypePtr;

typedef struct			/* a line/polygon point */
{
  Location X, Y, X2, Y2;	/* so Point type can be cast as BoxType */
  long int ID;
} PointType, *PointTypePtr;

typedef struct			/* holds information about one line */
{
  BoxType BoundingBox;
  LINESTRUCT char *Number;
} LineType, *LineTypePtr;

typedef struct
{
  BoxType BoundingBox;
  long int ID, Flags;
  BDimension Scale;		/* text scaling in percent */
  Location X,			/* origin */
    Y;
  BYTE Direction;
  char *TextString;		/* string */
  void *Element;
} TextType, *TextTypePtr;

typedef struct			/* holds information about a polygon */
{
  BoxType BoundingBox;
  long int ID, Flags;
  Cardinal PointN,		/* number of points in polygon */
    PointMax;			/* max number from malloc() */
  PointTypePtr Points;		/* data */
} PolygonType, *PolygonTypePtr;

typedef struct			/* holds information about arcs */
{
  BoxType BoundingBox;
  long int ID,			/* these fields are unused when contained in elements */
    Flags;
  BDimension Thickness, Clearance;
  Location Width,		/* length of axis */
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
  Pixel Color,			/* color */
    SelectedColor;
} LayerType, *LayerTypePtr;

typedef struct			/* a rat-line */
{
  BoxType BoundingBox;
  LINESTRUCT Cardinal group1, group2;	/* the layer group each point is on */
} RatType, *RatTypePtr;

typedef struct			/* a SMD pad */
{
  BoxType BoundingBox;
  LINESTRUCT BDimension Mask;
  char *Name, *Number;		/* 'Line' */
  void *Element;
  void *Spare;
} PadType, *PadTypePtr;

typedef struct
{
  BoxType BoundingBox;
  long int ID, Flags;
  BDimension Thickness, Clearance, Mask, DrillingHole;
  Location X,			/* center and diameter */
    Y;
  char *Name, *Number;
  void *Element;
  void *Spare;
} PinType, *PinTypePtr, **PinTypeHandle;

typedef struct
{
  BoxType BoundingBox;
  long int ID, Flags;
  TextType Name[MAX_ELEMENTNAMES];	/* the elements names; */
  /* description text */
  /* name on PCB second, */
  /* value third */
  /* see macro.h */
  Location MarkX,		/* position mark */
    MarkY;
  Cardinal PinN,		/* number of pins, lines and arcs */
    PinMax, PadN, PadMax, LineN, LineMax, ArcN, ArcMax;
  PinTypePtr Pin;		/* pin description */
  PadTypePtr Pad;		/* pad description of SMD components */
  LineTypePtr Line;
  ArcTypePtr Arc;
  BoxType VBox;
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
  Location MaxHeight,		/* maximum cell width and height */
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
  PinTypePtr Via;		/* pointer to object data */
  ElementTypePtr Element;
  RatTypePtr Rat;
  rtree_t *via_tree, *element_tree, *pin_tree, *pad_tree, *name_tree[3],	/* for element names */
   *rat_tree;
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
   *Style;			/* routing style */
  Cardinal EntryN,		/* number of objects */
    EntryMax;			/* number of reserved memory locations */
  LibraryEntryTypePtr Entry;	/* the entries */
} LibraryMenuType, *LibraryMenuTypePtr;

typedef struct
{
  Cardinal MenuN, MenuMax;
  LibraryMenuTypePtr Menu;
  Window Wind;
} LibraryType, *LibraryTypePtr;

typedef struct			/* holds information about board layout */
{				/* most of the information is saved with layout */
  /* and initialized from resources when */
  /* a new board design is started */
  /* this struct is also used for the remove list */
  /* and for buffer handling */
  long int ID,			/* see macro.h */
    Flags;
  char *Name,			/* name of board */
   *Filename,			/* name of file (from load) */
   *PrintFilename,		/* from print dialog */
   *Netlistname;		/* name of netlist file */
  Boolean Changed,		/* layout has been changed */
    ViaOn,			/* visibility flags */
    ElementOn, RatOn, InvisibleObjectsOn, PinOn, SilkActive,	/* active layer is actually silk */
    RatDraw;			/* we're drawing rats */
  Pixel ViaColor,		/* some colors */
   
    ViaSelectedColor,
    PinColor,
    PinSelectedColor,
    ElementColor,
    RatColor,
    InvisibleObjectsColor,
    InvisibleMarkColor,
    ElementSelectedColor,
    RatSelectedColor, ConnectedColor, WarnColor, MaskColor;
  long int CursorX,		/* cursor position as saved with layout */
    CursorY, Clipping;
  BDimension GridOffsetX,	/* as saves with layout */
    GridOffsetY, MaxWidth,	/* allowed size */
    MaxHeight;
  float Grid,			/* used grid with offsets */
    Zoom,			/* zoom factor */
    ThermScale;			/* scale factor used with thermals */
  FontType Font;
  LayerGroupType LayerGroups;
  RouteStyleType RouteStyle[NUM_STYLES];
  LibraryType NetlistLib;
  DataTypePtr Data;		/* entire database */
} PCBType, *PCBTypePtr;

typedef struct			/* information about the paste buffer */
{
  Location X,			/* offset */
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
  Location X,			/* saved position when MOVE_MODE */
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
  GC GC,			/* GC for cursor drawing */
    AttachGC;			/* and for displaying buffer contents */
  Location X,			/* position in PCB coordinates */
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
 * most of them is used as default when a new design is started
 */
typedef struct			/* some resources... */
{
  Pixel bgColor,		/* background and cursor color ... */
    CrosshairColor,		/* different object colors */
   
    CrossColor,
    ViaColor,
    ViaSelectedColor,
    PinColor,
    PinSelectedColor,
    ElementColor,
    RatColor,
    InvisibleObjectsColor,
    InvisibleMarkColor,
    ElementSelectedColor,
    RatSelectedColor,
    ConnectedColor,
    OffLimitColor,
    GridColor,
    LayerColor[MAX_LAYER],
    LayerSelectedColor[MAX_LAYER], WarnColor, MaskColor;
  BDimension ViaThickness,	/* some preset values */
    ViaDrillingHole, LineThickness, RatThickness, Keepaway, MaxWidth,	/* maximum size of a layout */
    MaxHeight, TextScale,	/* text scaling in % */
    AlignmentDistance, Bloat,	/* drc sizes */
    Shrink, minWid, minSlk;
  float Grid,			/* grid 0.001'' */
    Zoom,			/* number of shift operations for zooming */
    PinoutZoom;			/* same for pinout windows */
  long int PinoutNameLength,	/* max displayed length of a pinname */
    Volume,			/* the speakers volume -100..100 */
    CharPerLine,		/* width of an output line in characters */
    Mode,			/* currently active mode */
    BufferNumber,		/* number of the current buffer */
    GridFactor;			/* factor used for grid-drawing */
  long BackupInterval;		/* time between two backups in seconds */
  String DefaultLayerName[MAX_LAYER], FontCommand,	/* commands for file loading... */
    FileCommand, ElementCommand, PrintFile, LibraryCommand, LibraryContentsCommand, LibraryTree,	/* path to library tree */
    SaveCommand, LibraryFilename, FontFile,	/* name of default font file */
    Groups,			/* string with layergroups */
    Routes,			/* string with route styles */
    FilePath, RatPath, RatCommand, FontPath, ElementPath, LibraryPath, Size,	/* geometry string for size */
    Media,			/* type of output media */
    MenuFile,			/* file containing menu definitions */
    BackgroundImage,		/* PPM file for board background */
    ScriptFilename;             /* PCB Actions script to execute on startup */
  Boolean DumpMenuFile;		/* dump internal menu definitions */
  Location PinoutOffsetX,	/* offset of origin */
    PinoutOffsetY;
  Position PinoutTextOffsetX,	/* offset of text from pin center */
    PinoutTextOffsetY;
  RouteStyleType RouteStyle[NUM_STYLES];	/* default routing styles */
  LayerGroupType LayerGroups;	/* default layer groups */
  Boolean ClearLine, UniqueNames,	/* force unique names */
    SnapPin,			/* snap to pins and pads */
    UseLogWindow,		/* use log window instead of dialog box */
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
    ResetAfterElement,		/* reset connections after each element */
    liveRouting,		/* autorouter shows tracks in progress */
    RingBellWhenFinished;	/* flag if a signal should be */
  /* produced when searching of */
  /* connections is done */
  XFontStruct *PinoutFont[MAX_ZOOM + 1];	/* font ID used for pin names */
  int HistorySize;
} SettingType, *SettingTypePtr;

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
typedef struct			/* media description */
{
  String Name;			/* name and size (in mil*100) */
  BDimension Width, Height;
  Location MarginX, MarginY;
} MediaType, *MediaTypePtr;

typedef struct			/* needs and abilities of a driver */
{
  MediaTypePtr SelectedMedia;	/* media to be used */
  FILE *FP;			/* file pointers */
  Boolean MirrorFlag,		/* several flags */
    RotateFlag, InvertFlag;
  BDimension OffsetX,		/* offset from lower/left corner */
    OffsetY;
  float Scale;			/* scaleing */
  BoxType BoundingBox;		/* bounding box of output */
} PrintInitType, *PrintInitTypePtr;

typedef struct			/* functions of a print driver */
{
  char *Name,			/* name of driver */
   *Suffix;			/* filename suffix */
  void (*init) (PrintInitTypePtr);	/* initializes driver */
  void (*Exit) (void);		/* exit code */
  char *(*Preamble) (PrintInitTypePtr, char *);	/* initializes file */
  void (*Postamble) (void);	/* exit code */
  void (*SetColor) (XColor);	/* set color */
  void (*Polarity) (int);	/* control drawing polarity */
  void (*Line) (LineTypePtr, Boolean);	/* draw a line (Thick/Clear) */
  void (*Arc) (ArcTypePtr, Boolean);	/* draw an arc (Thick/Clear) */
  void (*Poly) (PolygonTypePtr);	/* draw a polygon */
  void (*Text) (TextTypePtr);	/* draw vector text */
  void (*Pad) (PadTypePtr, int);	/* pad thick=0 clear=1 mask=2 */
  void (*PinOrVia) (PinTypePtr, int);	/* pin diam=0 clear=1 mask=2 */
  void (*ElementPackage) (ElementTypePtr);
  void (*Drill) (PinTypePtr, Cardinal);	/* drilling information */
  void (*Outline) (Location, Location, Location, Location);
  void (*Alignment) (Location, Location, Location, Location);
  void (*DrillHelper) (PinTypePtr, int);
  void (*GroupID) (int);	/* comments group info */
  Boolean HandlesColor,		/* colored output */
    HandlesDrill,		/* able to produce drill info */
    HandlesMedia,		/* able to handle different media */
    HandlesMirror,		/* allows mirror flag to be set */
    HandlesRotate,		/* allows the rotate flag to be set */
    HandlesScale;
} PrintDeviceType, *PrintDeviceTypePtr;

typedef struct
{
  PrintDeviceTypePtr (*Query) (void);	/* query function */
  PrintDeviceTypePtr Info;	/* data returned by Query() */
} DeviceInfoType, *DeviceInfoTypePtr;

typedef struct			/* holds a connection */
{
  Location X, Y;		/* coordinate of connection */
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


/* ---------------------------------------------------------------------------
 * add a macro for wrapping RCS ID's in so that ident will still work
 * but we won't get as many compiler warnings
 */

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 1000 + __GNUC_MINOR__)
#endif /* GCC_VERSION */

#if GCC_VERSION > 2007
#define ATTRIBUTE_UNUSED __attribute__((unused)) 
#endif 

#define RCSID(x) static char *rcsid  ATTRIBUTE_UNUSED = x

#endif /* __GLOBAL_INCLUDED__  */

