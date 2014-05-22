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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* definition of types
 */

/* Change History:
 * 10/11/96 11:37 AJF Added support for a Text() driver function.
 * This was done out of a pressing need to force text to be printed on the
 * silkscreen layer. Perhaps the design is not the best.
 */

#ifndef	PCB_GLOBAL_H
#define	PCB_GLOBAL_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "const.h"
#include "macro.h"

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include <sys/types.h>
#include <stdbool.h>
#include <glib.h>

/* Forward declarations for structures the HIDs need.  */
typedef struct BoxType BoxType;
typedef struct polygon_st PolygonType;
typedef struct pad_st PadType;
typedef struct pin_st PinType;
typedef struct drc_violation_st DrcViolationType;
typedef struct rtree rtree_t;
typedef struct AttributeListType AttributeListType;

typedef struct unit Unit;
typedef struct increments Increments;

typedef COORD_TYPE Coord;	/* pcb base unit */
typedef double Angle;		/* degrees */

#include "hid.h"
#include "polyarea.h"

/* Internationalization support. */
#include "gettext.h"

#if defined (ENABLE_NLS)
/* When an empty string is used for msgid, the functions may return a nonempty
   string. */
# define _(S) (S[0] != '\0') ? gettext(S) : S
# define N_(S) gettext_noop(S)
# define C_(C, S) pgettext(C, S)
#else
# define _(S) S
# define N_(S) S
# define C_(C, S) S
#endif /* ENABLE_NLS */

/* This is used by the lexer/parser */
typedef struct {
  int ival;
  Coord bval;
  double dval;
  char has_units;
} PLMeasure;

#ifndef XtSpecificationRelease
typedef unsigned int Cardinal;
/*typedef unsigned int	Pixel;*/
typedef char *String;
typedef short Position;
typedef short Dimension;
#endif
typedef unsigned char BYTE;

/* Nobody should know about the internals of this except the macros in
   macros.h that access it.  This structure must be simple-assignable
   for now.  */
typedef struct
{
  unsigned int f; /* generic flags */
  unsigned char t[(MAX_LAYER + 1) / 2];	/* thermals */
} FlagType;

#ifndef __GNUC__
#define __FUNCTION1(a,b) a ":" #b
#define __FUNCTION2(a,b) __FUNCTION1(a,b)
#define __FUNCTION__ __FUNCTION2(__FILE__,__LINE__)
#endif


/* ---------------------------------------------------------------------------
 * Macros to annotate branch-prediction information.
 * Taken from GLib 2.16.3 (LGPL 2).G_ / g_ prefixes have
 * been removed to avoid namespace clashes.
 */

/* The LIKELY and UNLIKELY macros let the programmer give hints to
 * the compiler about the expected result of an expression. Some compilers
 * can use this information for optimizations.
 *
 * The PCB_BOOLEAN_EXPR macro is intended to trigger a gcc warning when
 * putting assignments inside the test.
 */
#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#define PCB_BOOLEAN_EXPR(expr)                   \
 __extension__ ({                             \
   int _boolean_var_;                         \
   if (expr)                                  \
      _boolean_var_ = 1;                      \
   else                                       \
      _boolean_var_ = 0;                      \
   _boolean_var_;                             \
})
#define LIKELY(expr) (__builtin_expect (PCB_BOOLEAN_EXPR(expr), 1))
#define UNLIKELY(expr) (__builtin_expect (PCB_BOOLEAN_EXPR(expr), 0))
#else
#define LIKELY(expr) (expr)
#define UNLIKELY(expr) (expr)
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
	FlagType	Flags;		\
	//	struct LibraryEntryType *net

/* Lines, pads, and rats all use this so they can be cross-cast.  */
#define	ANYLINEFIELDS			\
	ANYOBJECTFIELDS;		\
	Coord		Thickness,      \
                        Clearance;      \
	PointType	Point1,		\
			Point2

/* ---------------------------------------------------------------------------
 * some useful values of our widgets
 */
typedef struct			/* holds information about output window */
{
  hidGC bgGC,			/* background and foreground; */
    fgGC,			/* changed from some routines */
    pmGC;			/* depth 1 pixmap GC to store clip */
}
OutputType;

/* ----------------------------------------------------------------------
 * layer group. A layer group identifies layers which are always switched
 * on/off together.
 */
typedef struct
{
  Cardinal Number[MAX_LAYER],	/* number of entries per groups */
    Entries[MAX_LAYER][MAX_LAYER + 2];
} LayerGroupType;

struct BoxType		/* a bounding box */
{
  Coord X1, Y1;		/* upper left */
  Coord X2, Y2;		/* and lower right corner */
};

typedef struct
{
  Coord x, y;
  Coord width, height;
} RectangleType;

typedef struct
{
  char *name;
  char *value;
} AttributeType;

struct AttributeListType
{
  int Number, Max;
  AttributeType *List;
};

/* ---------------------------------------------------------------------------
 * the basic object types supported by PCB
 */

/* All on-pcb objects (elements, lines, pads, vias, rats, etc) are
   based on this. */
typedef struct {
  ANYOBJECTFIELDS;
} AnyObjectType;

typedef struct			/* a line/polygon point */
{
  Coord X, Y, X2, Y2;	/* so Point type can be cast as BoxType */
  long int ID;
} PointType;

/* Lines, rats, pads, etc.  */
typedef struct {
  ANYLINEFIELDS;
} AnyLineObjectType;

typedef struct			/* holds information about one line */
{
  ANYLINEFIELDS;
  char *Number;
} LineType;

typedef struct
{
  ANYOBJECTFIELDS;
  int Scale;		/* text scaling in percent */
  Coord X, Y;    	/* origin */
  BYTE Direction;
  char *TextString;		/* string */
  void *Element;
} TextType;

struct polygon_st			/* holds information about a polygon */
{
  ANYOBJECTFIELDS;
  Cardinal PointN,		/* number of points in polygon */
    PointMax;			/* max number from malloc() */
  POLYAREA *Clipped;		/* the clipped region of this polygon */
  PLINE *NoHoles;		/* the polygon broken into hole-less regions */
  int NoHolesValid;		/* Is the NoHoles polygon up to date? */
  PointType *Points;		/* data */
  Cardinal *HoleIndex;		/* Index of hole data within the Points array */
  Cardinal HoleIndexN;		/* number of holes in polygon */
  Cardinal HoleIndexMax;	/* max number from malloc() */

};

typedef struct			/* holds information about arcs */
{
  ANYOBJECTFIELDS;
  Coord Thickness, Clearance;
  PointType Point1;
  PointType Point2;
  Coord Width, Height,		/* length of axis */
    X, Y;			/* center coordinates */
  Angle StartAngle, Delta;	/* the two limiting angles in degrees */
} ArcType;

struct rtree
{
  struct rtree_node *root;
  int size;			/* number of entries in tree */
};

typedef struct			/* holds information about one layer */
{
  char *Name;			/* layer name */
  Cardinal LineN,		/* number of lines */
    TextN,			/* labels */
    PolygonN,			/* polygons */
    ArcN;			/* and arcs */
  GList *Line;
  GList *Text;
  GList *Polygon;
  GList *Arc;
  rtree_t *line_tree, *text_tree, *polygon_tree, *arc_tree;
  bool On;			/* visible flag */
  char *Color,			/* color */
   *SelectedColor;
  AttributeListType Attributes;
  int no_drc; /* whether to ignore the layer when checking the design rules */
}
LayerType;

typedef struct			/* a rat-line */
{
  ANYLINEFIELDS;
  Cardinal group1, group2;	/* the layer group each point is on */
} RatType;

struct pad_st			/* a SMD pad */
{
  ANYLINEFIELDS;
  Coord Mask;
  char *Name, *Number;		/* 'Line' */
  void *Element;
  void *Spare;
};

struct pin_st
{
  ANYOBJECTFIELDS;
  Coord Thickness, Clearance, Mask, DrillingHole;
  Coord X, Y;		/* center and diameter */
  char *Name, *Number;
  void *Element;
  void *Spare;
};

/* This is the extents of a Pin or Via, depending on whether it's a
   hole or not.  */
#define PIN_SIZE(pinptr) (TEST_FLAG(HOLEFLAG, (pinptr)) \
			  ? (pinptr)->DrillingHole \
			  : (pinptr)->Thickness)

typedef struct
{
  ANYOBJECTFIELDS;
  TextType Name[MAX_ELEMENTNAMES];	/* the elements names; */
  /* description text */
  /* name on PCB second, */
  /* value third */
  /* see macro.h */
  Coord MarkX, MarkY;		/* position mark */
  Cardinal PinN;		/* number of pins */
  Cardinal PadN;		/* number of pads */
  Cardinal LineN;		/* number of lines */
  Cardinal ArcN;		/* number of arcs */
  GList *Pin;
  GList *Pad;
  GList *Line;
  GList *Arc;
  BoxType VBox;
  AttributeListType Attributes;
} ElementType;

/* ---------------------------------------------------------------------------
 * symbol and font related stuff
 */
typedef struct			/* a single symbol */
{
  LineType *Line;
  bool Valid;
  Cardinal LineN,		/* number of lines */
    LineMax;
  Coord Width, Height,		/* size of cell */
    Delta;			/* distance to next symbol */
} SymbolType;

typedef struct			/* complete set of symbols */
{
  Coord MaxHeight,	/* maximum cell width and height */
    MaxWidth;
  BoxType DefaultSymbol;	/* the default symbol is a filled box */
  SymbolType Symbol[MAX_FONTPOSITION + 1];
  bool Valid;
} FontType;

typedef struct			/* holds all objects */
{
  Cardinal ViaN,		/* number of vias */
    ElementN,			/* and elements */
    RatN;			/* and rat-lines */
  int LayerN;			/* number of layers in this board */
  GList *Via;
  GList *Element;
  GList *Rat;
  rtree_t *via_tree, *element_tree, *pin_tree, *pad_tree, *name_tree[3],	/* for element names */
   *rat_tree;
  struct PCBType *pcb;
  LayerType Layer[MAX_LAYER + 2];	/* add 2 silkscreen layers */
  int polyClip;
} DataType;

typedef struct			/* holds drill information */
{
  Coord DrillSize;		/* this drill's diameter */
  Cardinal ElementN,		/* the number of elements using this drill size */
    ElementMax,			/* max number of elements from malloc() */
    PinCount,			/* number of pins drilled this size */
    ViaCount,			/* number of vias drilled this size */
    UnplatedCount,		/* number of these holes that are unplated */
    PinN,			/* number of drill coordinates in the list */
    PinMax;			/* max number of coordinates from malloc() */
  PinType **Pin;		/* coordinates to drill */
  ElementType **Element;	/* a pointer to an array of element pointers */
} DrillType;

typedef struct			/* holds a range of Drill Infos */
{
  Cardinal DrillN,		/* number of drill sizes */
    DrillMax;			/* max number from malloc() */
  DrillType *Drill;		/* plated holes */
} DrillInfoType;

typedef struct
{
  Coord Thick,			/* line thickness */
    Diameter,			/* via diameter */
    Hole,			/* via drill hole */
    Keepaway;			/* min. separation from other nets */
  char *Name;
  int index;
} RouteStyleType;

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
} LibraryEntryType;

/* If the internal flag is set, the only field that is valid is Name,
   and the struct is allocated with malloc instead of
   CreateLibraryEntry.  These "internal" entries are used for
   electrical paths that aren't yet assigned to a real net.  */

typedef struct
{
  char *Name,			/* name of the menu entry */
   *directory,			/* Directory name library elements are from */
   *Style;			/* routing style */
  Cardinal EntryN,		/* number of objects */
    EntryMax;			/* number of reserved memory locations */
  LibraryEntryType *Entry;	/* the entries */
  char flag;			/* used by the netlist window to enable/disable nets */
  char internal;		/* if set, this is an internal-only entry, not
				   part of the global netlist. */
} LibraryMenuType;

typedef struct
{
  Cardinal MenuN;               /* number of objects */
  Cardinal MenuMax;             /* number of reserved memory locations */
  LibraryMenuType *Menu;      /* the entries */
} LibraryType;


  /* The PCBType struct holds information about board layout most of which is
     |  saved with the layout.  A new PCB layout struct is first initialized
     |  with values from the user configurable Settings struct and then reset
     |  to the saved layout values when a layout is loaded.
     |  This struct is also used for the remove list and for buffer handling
   */
typedef struct PCBType
{
  long ID;			/* see macro.h */
  FlagType Flags;
  char *Name,			/* name of board */
   *Filename,			/* name of file (from load) */
   *PrintFilename,		/* from print dialog */
   *Netlistname,		/* name of netlist file */
    ThermStyle;			/* type of thermal to place with thermal tool */
  bool Changed,		/* layout has been changed */
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
    *RatSelectedColor, *ConnectedColor, *FoundColor, *WarnColor, *MaskColor;
  long CursorX,			/* cursor position as saved with layout */
    CursorY, Clipping;
  Coord Bloat,			/* drc sizes saved with layout */
    Shrink, minWid, minSlk, minDrill, minRing;
  Coord GridOffsetX,		/* as saved with layout */
    GridOffsetY;
  /* TODO: Set this always to MAX_COORD, no saving needed.
           Kept for compatibility and until the GUI code can deal
           with the dynamic extent. */
  Coord MaxWidth, MaxHeight;	/* allowed size */
  Coord ExtentMinX, ExtentMinY,	/* extent, defined by the outline layer */
    ExtentMaxX, ExtentMaxY;

  Coord Grid;			/* used grid with offsets */
  double IsleArea,		/* minimum poly island to retain */
    ThermScale;			/* scale factor used with thermals */
  FontType Font;
  LayerGroupType LayerGroups;
  RouteStyleType RouteStyle[NUM_STYLES];
  LibraryType NetlistLib;
  AttributeListType Attributes;
  DataType *Data;		/* entire database */

  bool is_footprint;		/* If set, the user has loaded a footprint, not a pcb. */
}
PCBType;

typedef struct			/* information about the paste buffer */
{
  Coord X, Y;			/* offset */
  BoxType BoundingBox;
  DataType *Data;		/* data; not all members of PCBType */
  /* are used */
} BufferType;

/* ---------------------------------------------------------------------------
 * some types for cursor drawing, setting of block and lines
 * as well as for merging of elements
 */
typedef struct			/* rubberband lines for element moves */
{
  LayerType *Layer;		/* layer that holds the line */
  LineType *Line;		/* the line itself */
  PointType *MovedPoint;	/* and finally the point */
} RubberbandType;

typedef struct			/* current marked line */
{
  PointType Point1,		/* start- and end-position */
    Point2;
  long int State;
  bool draw;
} AttachedLineType;

typedef struct			/* currently marked block */
{
  PointType Point1,		/* start- and end-position */
    Point2;
  long int State;
  bool otherway;
} AttachedBoxType;

typedef struct			/* currently attached object */
{
  Coord X, Y;			/* saved position when MOVE_MODE */
  BoxType BoundingBox;
  long int Type,		/* object type */
    State;
  void *Ptr1,			/* three pointers to data, see */
   *Ptr2,			/* search.c */
   *Ptr3;
  Cardinal RubberbandN,		/* number of lines in array */
    RubberbandMax;
  RubberbandType *Rubberband;
} AttachedObjectType;

enum crosshair_shape
{
  Basic_Crosshair_Shape = 0,  /*  4-ray */
  Union_Jack_Crosshair_Shape, /*  8-ray */
  Dozen_Crosshair_Shape,      /* 12-ray */
  Crosshair_Shapes_Number
};

typedef struct			/* holds cursor information */
{
  hidGC GC,			/* GC for cursor drawing */
    AttachGC;			/* and for displaying buffer contents */
  Coord X, Y,			/* position in PCB coordinates */
    MinX, MinY,			/* lowest and highest coordinates */
    MaxX, MaxY;
  AttachedLineType AttachedLine;	/* data of new lines... */
  AttachedBoxType AttachedBox;
  PolygonType AttachedPolygon;
  AttachedObjectType AttachedObject;	/* data of attached objects */
  enum crosshair_shape shape; 	/* shape of crosshair */
} CrosshairType;

typedef struct
{
  bool status;
  Coord X, Y;
} MarkType;

/* ---------------------------------------------------------------------------
 * our resources
 * most of them are used as default when a new design is started
 */
typedef struct			/* some resources... */
{
  const Unit *grid_unit;
  Increments *increments;

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
    *FoundColor,
    *OffLimitColor,
    *GridColor,
    *LayerColor[MAX_LAYER],
    *LayerSelectedColor[MAX_LAYER], *WarnColor, *MaskColor;
  Coord ViaThickness,		/* some preset values */
    ViaDrillingHole, LineThickness, RatThickness, Keepaway,	/* default size of a new layout */
    MaxWidth, MaxHeight,
    AlignmentDistance, Bloat,	/* default drc sizes */
    Shrink, minWid, minSlk, minDrill, minRing;
  int TextScale;		/* text scaling in % */
  Coord Grid;			/* grid in pcb-units */
  double IsleArea;		/* polygon min area */
  int PinoutNameLength,		/* max displayed length of a pinname */
    Volume,			/* the speakers volume -100..100 */
    CharPerLine,		/* width of an output line in characters */
    Mode,			/* currently active mode */
    BufferNumber;		/* number of the current buffer */
  int BackupInterval;		/* time between two backups in seconds */
  char *DefaultLayerName[MAX_LAYER], *FontCommand,	/* commands for file loading... */
   *FileCommand, *ElementCommand, *PrintFile, *LibraryCommandDir, *LibraryCommand, *LibraryContentsCommand, *LibraryTree,	/* path to library tree */
   *SaveCommand, *LibraryFilename, *FontFile,	/* name of default font file */
   *Groups,			/* string with layergroups */
   *Routes,			/* string with route styles */
   *FilePath, *RatPath, *RatCommand, *FontPath, *PinoutFont, *ElementPath, *LibraryPath, *Size,	/* geometry string for size */
   *BackgroundImage,		/* PPM file for board background */
   *ScriptFilename,		/* PCB Actions script to execute on startup */
   *ActionString,		/* PCB Actions string to execute on startup */
   *FabAuthor,			/* Full name of author for FAB drawings */
   *GnetlistProgram,		/* gnetlist program name */
   *MakeProgram,		/* make program name */
   *InitialLayerStack;		/* If set, the initial layer stack is set to this */
  Coord PinoutOffsetX,		/* offset of origin */
    PinoutOffsetY;
  Coord PinoutTextOffsetX,	/* offset of text from pin center */
    PinoutTextOffsetY;
  RouteStyleType RouteStyle[NUM_STYLES];	/* default routing styles */
  LayerGroupType LayerGroups;	/* default layer groups */
  bool ClearLine, FullPoly,
    UniqueNames,		/* force unique names */
    SnapPin,			/* snap to pins and pads */
    ShowSolderSide,		/* mirror output */
    SaveLastCommand,		/* save the last command entered by user */
    SaveInTMP,			/* always save data in /tmp */
    SaveMetricOnly,		/* save with mm suffix only, not mil/mm hybrid */
    DrawGrid,			/* draw grid points */
    RatWarn,			/* rats nest has set warnings */
    StipplePolygons,		/* draw polygons with stipple */
    AllDirectionLines,		/* enable lines to all directions */
    RubberBandMode,		/* move, rotate use rubberband connections */
    AllDirectionsRubberBandMode,/* rubberband tracks can move in all directions */
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
}
SettingType;

/* ----------------------------------------------------------------------
 * pointer to low-level copy, move and rotate functions
 */
typedef struct
{
  void *(*Line) (LayerType *, LineType *);
  void *(*Text) (LayerType *, TextType *);
  void *(*Polygon) (LayerType *, PolygonType *);
  void *(*Via) (PinType *);
  void *(*Element) (ElementType *);
  void *(*ElementName) (ElementType *);
  void *(*Pin) (ElementType *, PinType *);
  void *(*Pad) (ElementType *, PadType *);
  void *(*LinePoint) (LayerType *, LineType *, PointType *);
  void *(*Point) (LayerType *, PolygonType *, PointType *);
  void *(*Arc) (LayerType *, ArcType *);
  void *(*Rat) (RatType *);
} ObjectFunctionType;

/* ---------------------------------------------------------------------------
 * structure used by device drivers
 */

typedef struct			/* holds a connection */
{
  Coord X, Y;			/* coordinate of connection */
  long int type;		/* type of object in ptr1 - 3 */
  void *ptr1, *ptr2;		/* the object of the connection */
  Cardinal group;		/* the layer group of the connection */
  LibraryMenuType *menu;	/* the netmenu this *SHOULD* belong too */
} ConnectionType;

typedef struct			/* holds a net of connections */
{
  Cardinal ConnectionN,		/* the number of connections contained */
    ConnectionMax;		/* max connections from malloc */
  ConnectionType *Connection;
  RouteStyleType *Style;
} NetType;

typedef struct			/* holds a list of nets */
{
  Cardinal NetN,		/* the number of subnets contained */
    NetMax;			/* max subnets from malloc */
  NetType *Net;
} NetListType;

typedef struct			/* holds a list of net lists */
{
  Cardinal NetListN,		/* the number of net lists contained */
    NetListMax;			/* max net lists from malloc */
  NetListType *NetList;
} NetListListType;

typedef struct			/* holds a generic list of pointers */
{
  Cardinal PtrN,		/* the number of pointers contained */
    PtrMax;			/* max subnets from malloc */
  void **Ptr;
} PointerListType;

typedef struct
{
  Cardinal BoxN,		/* the number of boxes contained */
    BoxMax;			/* max boxes from malloc */
  BoxType *Box;

} BoxListType;

struct drc_violation_st
{
  char *title;
  char *explanation;
  Coord x, y;
  Angle angle;
  int have_measured;
  Coord measured_value;
  Coord required_value;
  int object_count;
  long int *object_id_list;
  int *object_type_list;
};

/* ---------------------------------------------------------------------------
 * define supported types of undo operations
 * note these must be separate bits now
 */
#define	UNDO_CHANGENAME			0x0001	/* change of names */
#define	UNDO_MOVE			0x0002	/* moving objects */
#define	UNDO_REMOVE			0x0004	/* removing objects */
#define	UNDO_REMOVE_POINT		0x0008	/* removing polygon/... points */
#define	UNDO_INSERT_POINT		0x0010	/* inserting polygon/... points */
#define	UNDO_REMOVE_CONTOUR		0x0020	/* removing a contour from a polygon */
#define	UNDO_INSERT_CONTOUR		0x0040	/* inserting a contour from a polygon */
#define	UNDO_ROTATE			0x0080	/* rotations */
#define	UNDO_CREATE			0x0100	/* creation of objects */
#define	UNDO_MOVETOLAYER		0x0200	/* moving objects to */
#define	UNDO_FLAG			0x0400	/* toggling SELECTED flag */
#define	UNDO_CHANGESIZE			0x0800	/* change size of object */
#define	UNDO_CHANGE2NDSIZE		0x1000	/* change 2ndSize of object */
#define	UNDO_MIRROR			0x2000	/* change side of board */
#define	UNDO_CHANGECLEARSIZE		0x4000	/* change clearance size */
#define	UNDO_CHANGEMASKSIZE		0x8000	/* change mask size */
#define	UNDO_CHANGEANGLES	       0x10000	/* change arc angles */
#define	UNDO_LAYERCHANGE	       0x20000	/* layer new/delete/move */
#define	UNDO_CLEAR		       0x40000	/* clear/restore to polygons */
#define	UNDO_NETLISTCHANGE	       0x80000	/* netlist change */


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

/* ---------------------------------------------------------------------------
 * Macros called by various action routines to show usage or to report
 * a syntax error and fail
 */
#define AUSAGE(x) Message ("Usage:\n%s\n", (x##_syntax))
#define AFAIL(x) { Message ("Syntax error.  Usage:\n%s\n", (x##_syntax)); return 1; }

/* ---------------------------------------------------------------------------
 * Variables with absolute paths to various directories.  These are deduced
 * at runtime to allow pcb to be relocatable
 */
extern char *bindir;       /* The dir in which PCB installation was found */
extern char *pcblibdir;    /* The system M4 fp directory */
extern char *pcblibpath;   /* The search path for M4 fps */
extern char *pcbtreedir;   /* The system newlib fp directory */
extern char *pcbtreepath;  /* The search path for newlib fps */
extern char *exec_prefix;
extern char *homedir;

#endif /* PCB_GLOBAL_H  */
