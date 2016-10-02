/*!
 * \file src/global.h
 *
 * \brief Definition of types.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996, 2004 Thomas Nau
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 *
 * <hr>
 *
 * Change History:
 *
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

#include <locale.h>
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

typedef COORD_TYPE Coord;	/*!< pcb base unit. */
typedef double Angle;		/*!< Degrees. */

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

/*!
 * \brief This is used by the lexer/parser.
 */
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

/*!
 * \brief Nobody should know about the internals of this except the
 * macros in macros.h that access it.
 *
 * This structure must be simple-assignable for now.
 */
typedef struct
{
  unsigned long f;		/* generic flags */
  unsigned char t[(MAX_LAYER + 1) / 2];  /* thermals */
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

/*!
 * \brief Holds information about output window.
 */
typedef struct
{
  hidGC bgGC, /*!< Background; changed from some routines. */
    fgGC, /*!< Foreground; changed from some routines. */
    pmGC; /*!< Depth 1 pixmap GC to store clip. */
}
OutputType;

/*!
 * \brief Layer group.
 *
 * A layer group identifies layers which are always switched on/off
 * together.
 */
typedef struct
{
  Cardinal Number[MAX_GROUP], /*!< Number of entries per groups. */
    Entries[MAX_GROUP][MAX_ALL_LAYER];
} LayerGroupType;

/*!
 * \brief A bounding box.
 */
struct BoxType
{
  Coord X1, Y1; /*!< Upper left corner. */
  Coord X2, Y2; /*!< Lower right corner. */
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

/*!
 * \brief All on-pcb objects (elements, lines, pads, vias, rats, etc)
 * are based on this.
 */
typedef struct {
  ANYOBJECTFIELDS;
} AnyObjectType;

/*!
 * \brief A line/polygon point.
 */
typedef struct
{
  Coord X, Y, X2, Y2; /*!< So Point type can be cast as BoxType. */
  long int ID;
} PointType;

/*!
 * \brief Lines, rats, pads, etc.
 */
typedef struct {
  ANYLINEFIELDS;
} AnyLineObjectType;

/*!
 * \brief Holds information about one line.
 */
typedef struct
{
  ANYLINEFIELDS;
  char *Number;
} LineType;

typedef struct
{
  ANYOBJECTFIELDS;
  int Scale; /*!< Text scaling in percent. */
  Coord X; /*!< X-coordinate of origin. */
  Coord Y; /*!< Y-coordinate of origin. */
  BYTE Direction;
  char *TextString; /*!< String. */
  void *Element;
} TextType;

/*!
 * \brief Holds information about a polygon.
 */
struct polygon_st
{
  ANYOBJECTFIELDS;
  Cardinal PointN; /*!< Number of points in polygon. */
  Cardinal PointMax; /*!< Max number from malloc(). */
  POLYAREA *Clipped; /*!< The clipped region of this polygon. */
  PLINE *NoHoles; /*!< The polygon broken into hole-less regions */
  int NoHolesValid; /*!< Is the NoHoles polygon up to date? */
  PointType *Points; /*!< Data. */
  Cardinal *HoleIndex; /*!< Index of hole data within the Points array. */
  Cardinal HoleIndexN; /*!< Number of holes in polygon. */
  Cardinal HoleIndexMax; /*!< Max number from malloc(). */

};

/*!
 * \brief Holds information about arcs.
 */
typedef struct
{
  ANYOBJECTFIELDS;
  Coord Thickness, Clearance;
  PointType Point1;
  PointType Point2;
  Coord Width; /*!< Length of axis */
  Coord Height; /*!< Width of axis */
  Coord X; /*!< X-value of the center coordinates. */
  Coord Y; /*!< Y-value of the center coordinates. */
  Angle StartAngle; /*!< The start angle in degrees. */
  Angle Delta; /*!< The described angle in degrees. */
} ArcType;

struct rtree
{
  struct rtree_node *root;
  int size; /*!< Number of entries in tree */
};

/*!
 * \brief Holds information about one layer. */
typedef struct
{
  LayertypeType Type; /*!< LT_* from hid.h */
  char *Name; /*!< Layer name. */
  Cardinal LineN; /*!< Number of lines. */
  Cardinal TextN; /*!< Labels. */
  Cardinal PolygonN; /*!< Polygons. */
  Cardinal ArcN; /*!< Arcs. */
  GList *Line;
  GList *Text;
  GList *Polygon;
  GList *Arc;
  rtree_t *line_tree, *text_tree, *polygon_tree, *arc_tree;
  bool On; /*!< Visible flag. */
  char *Color, /*!< Color. */
   *SelectedColor;
  AttributeListType Attributes;
  int no_drc; /*!< Whether to ignore the layer when checking the design
    rules */
}
LayerType;

/*!
 * \brief A rat line.
 */
typedef struct
{
  ANYLINEFIELDS;
  Cardinal group1; /*!< The layer group each point is on. */
  Cardinal group2; /*!< The layer group each point is on. */
} RatType;

/*!
 * \brief A SMD pad.
 */
struct pad_st
{
  ANYLINEFIELDS;
  Coord Mask;
  char *Name, *Number; /*!< 'Line'. */
  void *Element;
  void *Spare;
};

/*!
 * \brief A pin.
 */
struct pin_st
{
  ANYOBJECTFIELDS;
  Coord Thickness;
  Coord Clearance;
  Coord Mask;
  Coord DrillingHole; /*!< Diameter of the drill hole. */
  Coord X; /*!< X-value of the center coordinates. */
  Coord Y; /*!< Y-value of the center coordinates. */
  char *Name;
  char *Number;
  void *Element;
  void *Spare;
};

/* This is the extents of a Pin or Via, depending on whether it's a
   hole or not.  */
#define PIN_SIZE(pinptr) (TEST_FLAG(HOLEFLAG, (pinptr)) \
			  ? (pinptr)->DrillingHole \
			  : (pinptr)->Thickness)

/*!
 * \brief An element.
 */
typedef struct
{
  ANYOBJECTFIELDS;
  TextType Name[MAX_ELEMENTNAMES];
    /*!< The elements names;
     * - description text,
     * - name on PCB second,
     * - value third.
     * see macro.h.
     */
  Coord MarkX; /*!< X-value of the position mark. */
  Coord MarkY; /*!< Y-value of the position mark. */
  Cardinal PinN; /*!< Number of pins. */
  Cardinal PadN; /*!< Number of pads. */
  Cardinal LineN; /*!< Number of lines. */
  Cardinal ArcN; /*!< Number of arcs. */
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
/*!
 * \brief A single symbol.
 */
typedef struct
{
  LineType *Line;
  bool Valid;
  Cardinal LineN; /*!< Number of lines. */
  Cardinal LineMax;
  Coord Width; /*!< Width of cell. */
  Coord Height; /*!< Height of cell. */
  Coord Delta; /*!< Distance to next symbol. */
} SymbolType;

/*!
 * \brief Complete set of symbols.
 */
typedef struct
{
  char * Name;
  Coord MaxHeight; /*!< Maximum cell width. */
  Coord MaxWidth; /*!< Maximum cell height. */
  BoxType DefaultSymbol; /*!< The default symbol is a filled box. */
  SymbolType Symbol[MAX_FONTPOSITION + 1];
  bool Valid;
} FontType;

/*!
 * \brief Holds all objects.
 */
typedef struct
{
  Cardinal ViaN; /*!< Number of vias. */
  Cardinal ElementN; /*!< Number of elements. */
  Cardinal RatN; /*!< Number of rat-lines. */
  int LayerN; /*!< Number of layers in this board. */
  GList *Via;
  GList *Element;
  GList *Rat;
  rtree_t *via_tree, *element_tree, *pin_tree, *pad_tree, *name_tree[3],	/* for element names */
   *rat_tree;
  struct PCBType *pcb;
  LayerType Layer[MAX_ALL_LAYER];
  int polyClip;
} DataType;

/*!
 * \brief Holds drill information.
 */
typedef struct
{
  Coord DrillSize; /*!< This drill's diameter. */
  Cardinal ElementN; /*!< The number of elements using this drill size. */
  Cardinal ElementMax; /*!< Max. number of elements from malloc(). */
  Cardinal PinCount; /*!< Number of pins drilled this size. */
  Cardinal ViaCount; /*!< Number of vias drilled this size. */
  Cardinal UnplatedCount; /*!< Number of these holes that are unplated. */
  Cardinal PinN; /*!< Number of drill coordinates in the list. */
  Cardinal PinMax; /*!< Max. number of coordinates from malloc(). */
  PinType **Pin; /*!< Coordinates to drill. */
  ElementType **Element; /*!< A pointer to an array of element pointers. */
} DrillType;

/*!
 * \brief Holds a range of Drill Infos.
 */
typedef struct
{
  Cardinal DrillN; /*!< Number of drill sizes. */
  Cardinal DrillMax; /*!< Max. number from malloc(). */
  DrillType *Drill; /*!< Plated holes. */
} DrillInfoType;

typedef struct
{
  Coord Thick; /*!< Line thickness. */
  Coord Diameter; /*!< Via diameter. */
  Coord Hole; /*!< Via drill hole. */
  Coord Keepaway; /*!< Min. separation from other nets. */
  char *Name;
  int index;
} RouteStyleType;

/*!
 * \brief Structure used by library routines.
 */
typedef struct
{
  char *ListEntry; /*!< The string for the selection box. */
  char *AllocatedMemory; /*!< Pointer to allocated memory;
    all others point to parts of the string. */
  char *Template; /*!< m4 template name. */
  char *Package; /*!< Package. */
  char *Value; /*!< The value field. */
  char *Description; /*!< Some descriptional text. */
} LibraryEntryType;

/*!
 * \brief .
 *
 * If the internal flag is set, the only field that is valid is Name,
 * and the struct is allocated with malloc instead of
 * CreateLibraryEntry.  These "internal" entries are used for
 * electrical paths that aren't yet assigned to a real net.
 */
typedef struct
{
  char *Name; /*!< Name of the menu entry. */
  char *directory; /*!< Directory name library elements are from. */
  char *Style; /*!< Routing style. */
  Cardinal EntryN; /*!< Number of objects. */
  Cardinal EntryMax; /*!< Number of reserved memory locations. */
  LibraryEntryType *Entry; /*!< The entries. */
  char flag; /*!< Used by the netlist window to enable/disable nets. */
  char internal; /*!< If set, this is an internal-only entry, not
                  * part of the global netlist. */
} LibraryMenuType;

typedef struct
{
  Cardinal MenuN; /*!< Number of objects. */
  Cardinal MenuMax; /*!< Number of reserved memory locations. */
  LibraryMenuType *Menu; /*!< The entries. */
} LibraryType;


/*!
 * \brief The PCBType struct holds information about board layout most
 * of which is saved with the layout.
 *
 * A new PCB layout struct is first initialized with values from the
 * user configurable \c Settings struct and then reset to the saved
 * layout values when a layout is loaded.
 *
 * This struct is also used for the remove list and for buffer handling.
 */
typedef struct PCBType
{
  long ID; /*!< See macro.h. */
  FlagType Flags;
  char *Name, /*!< Name of board. */
   *Filename, /*!< Name of file (from load). */
   *PrintFilename, /*!< From print dialog. */
   *Netlistname, /*!< Name of netlist file. */
    ThermStyle; /*!< Type of thermal to place with thermal tool. */
  bool Changed, /*!< Layout has been changed. */
    ViaOn, /*!< Visibility flag for vias. */
    ElementOn, /*!< Visibility flag for elements. */
    RatOn, /*!< Visibility flag for rat lines. */
    InvisibleObjectsOn,
    PinOn, /*!< Visibility flag for pins. */
    SilkActive, /*!< Active layer is actually silk. */
    RatDraw; /*!< We're drawing rats. */
  char *ViaColor, /*!< Via color. */
   *ViaSelectedColor, /*!< Selected via color. */
    *PinColor, /*!< Pin color. */
    *PinSelectedColor, /*!< Selected pin color. */
    *PinNameColor, /*!< Pin name color. */
    *ElementColor, /*!< Element color. */
    *RatColor, /*!< Rat line color. */
    *InvisibleObjectsColor, /*!< Invisible objects color. */
    *InvisibleMarkColor, /*!< Invisible mark color. */
    *ElementSelectedColor, /*!< Selected elements color. */
    *RatSelectedColor, /*!< Selected rat line color. */
    *ConnectedColor, /*!< Connected color. */
    *FoundColor, /*!< Found color. */
    *WarnColor, /*!< Warning color. */
    *MaskColor; /*!< Mask color. */
  long CursorX, /*!< Cursor position as saved with layout (X value). */
    CursorY, /*!< Cursor position as saved with layout (Y value). */
    Clipping;
  Coord Bloat, /*!< DRC bloat size saved with layout. */
    Shrink, /*!< DRC shrink size saved with layout. */
    minWid, /*!< DRC minimum width size saved with layout. */
    minSlk, /*!< DRC minimum silk size saved with layout. */
    minDrill, /*!< DRC minimum drill size saved with layout. */
    minRing; /*!< DRC minimum annular ring size saved with layout. */
  Coord GridOffsetX, /*!< As saved with layout (X value). */
    GridOffsetY, /*!< As saved with layout (Y value). */
    MaxWidth, /*!< Maximum allowed width size. */
    MaxHeight; /*!< Maximum allowed height size. */

  Coord Grid; /*!< Used grid with offsets. */
  double IsleArea, /*!< Minimum poly island to retain. */
    ThermScale; /*!< Scale factor used with thermals. */
  char * DefaultFontName; /* Default font for read objects that don't specify */
  GSList * FontLibrary;
  LayerGroupType LayerGroups;
  RouteStyleType RouteStyle[NUM_STYLES];
  LibraryType NetlistLib;
  AttributeListType Attributes;
  DataType *Data; /*!< Entire database. */

  bool is_footprint; /*!< If set, the user has loaded a footprint, not a pcb. */
}
PCBType;

/*!
 * \brief Information about the paste buffer.
 */
typedef struct
{
  Coord X; /*!< Offset (X value). */
  Coord Y; /*!< Offset (Y value). */
  BoxType BoundingBox;
  DataType *Data; /*!< Data; not all members of PCBType are used. */
} BufferType;

/* ---------------------------------------------------------------------------
 * some types for cursor drawing, setting of block and lines
 * as well as for merging of elements
 */

/*!
 * \brief Rubberband lines for element moves.
 */
typedef struct
{
  LayerType *Layer; /*!< Layer that holds the line. */
  LineType *Line; /*!< The line itself. */
  PointType *MovedPoint; /*!< And finally the point. */
} RubberbandType;

/*!
 * \brief Current marked line.
 */
typedef struct
{
  PointType Point1; /*!< Start position. */
  PointType Point2; /*!< End position. */
  long int State;
  bool draw;
} AttachedLineType;

/*!
 * \brief Currently marked block.
 */
typedef struct
{
  PointType Point1; /*!< Start position. */
  PointType Point2; /*!< End position. */
  long int State;
  bool otherway;
} AttachedBoxType;

/*!
 * \brief Currently attached object.
 */
typedef struct
{
  Coord X; /*!< Saved position when MOVE_MODE (X value). */
  Coord Y; /*!< Saved position when MOVE_MODE (Y value). */
  BoxType BoundingBox;
  long int Type, /*!< Object type. */
    State;
  void *Ptr1; /*!< Pointer to data, see search.c. */
  void *Ptr2; /*!< Pointer to data, see search.c. */
  void *Ptr3; /*!< Pointer to data, see search.c. */
  Cardinal RubberbandN, /*!< Number of lines in array. */
    RubberbandMax;
  RubberbandType *Rubberband;
} AttachedObjectType;

enum crosshair_shape
{
  Basic_Crosshair_Shape = 0,  /*!< 4-ray. */
  Union_Jack_Crosshair_Shape, /*!< 8-ray. */
  Dozen_Crosshair_Shape,      /*!< 12-ray. */
  Crosshair_Shapes_Number
};

/*!
 * \brief Holds cursor information.
 */
typedef struct
{
  hidGC GC; /*!< GC for cursor drawing. */
  hidGC AttachGC; /*!< GC for displaying buffer contents. */
  Coord X; /*!< Position in PCB coordinates (X value). */
  Coord Y; /*!< Position in PCB coordinates (Y value). */
  Coord MinX; /*!< Lowest coordinates (X value). */
  Coord MinY; /*!< Lowest coordinates (Y value). */
  Coord MaxX; /*!< Highest coordinates (X value). */
  Coord MaxY; /*!< Highest coordinates (Y value). */
  AttachedLineType AttachedLine; /*!< Data of new lines. */
  AttachedBoxType AttachedBox;
  PolygonType AttachedPolygon;
  AttachedObjectType AttachedObject; /*!< Data of attached objects. */
  enum crosshair_shape shape; /*!< Shape of Crosshair. */
} CrosshairType;

typedef struct
{
  bool status;
  Coord X, Y;
} MarkType;

/*!
 * \brief Our resources.
 *
 * Most of them are used as default when a new design is started.
 */
typedef struct
{
  const Unit *grid_unit;
  Increments *increments;

  int verbose;

  char *BlackColor,
    *WhiteColor,
    *BackgroundColor, /*!< Background color. */
    *CrosshairColor, /*!< Crosshair color. */
    *CrossColor, /*!< Cross color. */
    *ViaColor, /*!< Via color. */
    *ViaSelectedColor, /*!< Selected via color. */
    *PinColor, /*!< Pin color. */
    *PinSelectedColor, /*!< Selected pin color. */
    *PinNameColor, /*!< Pin name color. */
    *ElementColor, /*!< Element color. */
    *RatColor, /*!< Rat color. */
    *InvisibleObjectsColor, /*!< Invisible objects color. */
    *InvisibleMarkColor, /*!< Invisible mark color. */
    *ElementSelectedColor, /*!< Selected element color. */
    *RatSelectedColor, /*!< Selected rat color. */
    *ConnectedColor, /*!< Connected color. */
    *FoundColor, /*!< Found color. */
    *OffLimitColor,
    *GridColor, /*!< Grid color. */
    *LayerColor[MAX_LAYER],
    *LayerSelectedColor[MAX_LAYER],
    *WarnColor, /*!< Warning color. */
    *MaskColor; /*!< Mask color. */
  Coord ViaThickness, /*!< Default via thickness value. */
    ViaDrillingHole, /*!< Default via drill hole value. */
    LineThickness, /*!< Default line thickness value. */
    RatThickness, /*!< Default rat thickness value. */
    Keepaway, /*!< Default keepaway value. */
    MaxWidth, /*!< Default size of a new layout (X value). */
    MaxHeight, /*!< Default size of a new layout (Y value). */
    AlignmentDistance,
    Bloat, /*!< Default drc size for bloat. */
    Shrink, /*!< Default drc size for shrink. */
    minWid, /*!< Default drc size for minimum trace width. */
    minSlk, /*!< Default drc size for minumum silk width. */
    minDrill, /*!< Default drc size for minimum drill size. */
    minRing; /*!< Default drc size for minimum annular ring. */
  int TextScale; /*!< Text scaling in %. */
  Coord Grid; /*!< Grid in pcb-units. */
  double IsleArea; /*!< Polygon min area. */
  int PinoutNameLength, /*!< Max displayed length of a pinname. */
    Volume, /*!< The speakers volume -100 .. 100. */
    CharPerLine, /*!< Width of an output line in characters. */
    Mode, /*!< Currently active mode. */
    BufferNumber; /*!< Number of the current buffer. */
  int BackupInterval; /*!< Time between two backups in seconds. */
  char *DefaultLayerName[MAX_LAYER],
   *FontCommand, /*!< Command for font file loading. */
   *FileCommand, /*!< Command for file loading. */
   *ElementCommand, /*!< Command for element file loading. */
   *PrintFile,
   *LibraryCommandDir,
   *LibraryCommand,
   *LibraryContentsCommand,
   *LibraryTree, /*!< Path to library tree. */
   *SaveCommand,
   *LibraryFilename,
   *FontFile, /*!< Name of default font file. */
   *Groups, /*!< String with layergroups. */
   *Routes, /*!< String with route styles. */
   *FilePath,
   *RatPath,
   *RatCommand,
   *FontPath,
   *PinoutFont,
   *ElementPath,
   *LibraryPath,
   *Size, /*!< Geometry string for size. */
   *BackgroundImage, /*!< PPM file for board background. */
   *ScriptFilename, /*!< PCB Actions script to execute on startup. */
   *ActionString, /*!< PCB Actions string to execute on startup. */
   *FabAuthor, /*!< Full name of author for FAB drawings. */
   *GnetlistProgram, /*!< gnetlist program name. */
   *MakeProgram, /*!< make program name. */
   *InitialLayerStack; /*!< If set, the initial layer stack is set to this. */
  Coord PinoutOffsetX; /*!< Offset of origin (X value). */
  Coord PinoutOffsetY; /*!< Offset of origin (Y value). */
  Coord PinoutTextOffsetX; /*!< Offset of text from pin center (X value). */
  Coord PinoutTextOffsetY; /*!< Offset of text from pin center (Y value). */
  RouteStyleType RouteStyle[NUM_STYLES]; /*!< Default routing styles. */
  LayerGroupType LayerGroups; /*!< Default layer groups. */
  GSList * FontLibrary;
  FontType * Font;
  bool ClearLine,
    FullPoly,
    UniqueNames, /*!< Force unique names. */
    SnapPin, /*!< Snap to pins and pads. */
    ShowBottomSide, /*!< Mirror output. */
    SaveLastCommand, /*!< Save the last command entered by user. */
    SaveInTMP, /*!< Always save data in /tmp. */
    SaveMetricOnly, /*!< Save with mm suffix only, not mil/mm hybrid. */
    DrawGrid, /*!< Draw grid points. */
    RatWarn, /*!< Rats nest has set warnings. */
    StipplePolygons, /*!< Draw polygons with stipple. */
    AllDirectionLines, /*!< Enable lines to all directions. */
    RubberBandMode, /*!< Move, rotate use rubberband connections. */
    SwapStartDirection,/*!< Change starting direction after each click. */
    ShowDRC, /*!< Show drc region on crosshair. */
    AutoDRC, /*!< . */
    ShowNumber, /*!< Pinout shows number. */
    OrthogonalMoves, /*!< . */
    ResetAfterElement, /*!< Reset connections after each element. */
    liveRouting, /*!< Autorouter shows tracks in progress. */
    RingBellWhenFinished,
      /*!< flag if a signal should be produced when searching of
       * connections is done. */
    AutoPlace;
      /*!< Flag which says we should force placement of the windows on
       * startup. */
}
SettingType;

/*!
 * \brief Pointer to low-level copy, move and rotate functions.
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


/*!
 * \brief Holds a connection.
 */
typedef struct
{
  Coord X; /*!< Coordinate of connection (X value). */
  Coord Y; /*!< Coordinate of connection (Y value). */
  long int type; /*!< Type of object in ptr1 - 3. */
  void *ptr1; /*!< The object of the connection. */
  void *ptr2; /*!< The object of the connection. */
  Cardinal group; /*!< The layer group of the connection. */
  LibraryMenuType *menu; /*!< The netmenu this *SHOULD* belong to. */
} ConnectionType;

/*!
 * \brief Holds a net of connections.
 */
typedef struct
{
  Cardinal ConnectionN; /*!< The number of connections contained. */
  Cardinal ConnectionMax; /*!< Max connections from malloc. */
  ConnectionType *Connection;
  RouteStyleType *Style;
} NetType;

/*!
 * \brief Holds a list of nets.
 */
typedef struct
{
  Cardinal NetN; /*!< The number of subnets contained. */
  Cardinal NetMax; /*!< Max subnets from malloc. */
  NetType *Net;
} NetListType;

/*!
 * \brief Holds a list of net lists.
 */
typedef struct
{
  Cardinal NetListN; /*!< The number of net lists contained. */
  Cardinal NetListMax; /*!< Max net lists from malloc. */
  NetListType *NetList;
} NetListListType;

/*!
 * \brief Holds a generic list of pointers.
 */
typedef struct
{
  Cardinal PtrN; /*!< The number of pointers contained. */
  Cardinal PtrMax; /*!< Max subnets from malloc. */
  void **Ptr;
} PointerListType;

typedef struct
{
  Cardinal BoxN; /*!< The number of boxes contained. */
  Cardinal BoxMax; /*!< Max boxes from malloc. */
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
#define	UNDO_CHANGENAME			0x0001	/*!< Change of names. */
#define	UNDO_MOVE			0x0002	/*!< Moving objects. */
#define	UNDO_REMOVE			0x0004	/*!< Removing objects. */
#define	UNDO_REMOVE_POINT		0x0008	/*!< Removing polygon/... points. */
#define	UNDO_INSERT_POINT		0x0010	/*!< Inserting polygon/... points. */
#define	UNDO_REMOVE_CONTOUR		0x0020	/*!< Removing a contour from a polygon. */
#define	UNDO_INSERT_CONTOUR		0x0040	/*!< Inserting a contour from a polygon. */
#define	UNDO_ROTATE			0x0080	/*!< Rotations. */
#define	UNDO_CREATE			0x0100	/*!< Creation of objects. */
#define	UNDO_MOVETOLAYER		0x0200	/*!< Moving objects to. */
#define	UNDO_FLAG			0x0400	/*!< Toggling SELECTED flag. */
#define	UNDO_CHANGESIZE			0x0800	/*!< Change size of object. */
#define	UNDO_CHANGE2NDSIZE		0x1000	/*!< Change 2ndSize of object. */
#define	UNDO_MIRROR			0x2000	/*!< Change side of board. */
#define	UNDO_CHANGECLEARSIZE		0x4000	/*!< Change clearance size. */
#define	UNDO_CHANGEMASKSIZE		0x8000	/*!< Change mask size. */
#define	UNDO_CHANGEANGLES	       0x10000	/*!< Change arc angles. */
#define	UNDO_LAYERCHANGE	       0x20000	/*!< Layer new/delete/move. */
#define	UNDO_CLEAR		       0x40000	/*!< Clear/restore to polygons. */
#define	UNDO_NETLISTCHANGE	       0x80000	/*!< Netlist change. */

/* ---------------------------------------------------------------------------
 */
#if (__GNUC__ * 1000 + __GNUC_MINOR__) > 2007
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
extern char *bindir;       /*!< The dir in which PCB installation was found. */
extern char *pcblibdir;    /*!< The system M4 fp directory. */
extern char *pcblibpath;   /*!< The search path for M4 fps. */
extern char *pcbtreedir;   /*!< The system newlib fp directory. */
extern char *pcbtreepath;  /*!< The search path for newlib fps. */
extern char *exec_prefix;
extern char *homedir;

#endif /* PCB_GLOBAL_H  */
