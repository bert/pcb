%{
/* 
 * PCB Edif parser based heavily on:
 *
 *	Header: edif.y,v 1.18 87/12/07 19:59:49 roger Locked 
 */
/************************************************************************
 *									*
 *				edif.y					*
 *									*
 *			EDIF 2.0.0 parser, Level 0			*
 *									*
 *	You are free to copy, distribute, use it, abuse it, make it	*
 *	write bad tracks all over the disk ... or anything else.	*
 *									*
 *	Your friendly neighborhood Rogue Monster - roger@mips.com	*
 *									*
 ************************************************************************/
#include <stdio.h>

/* for malloc, free, atoi */
#include <stdlib.h>

/* for strcpy */
#include <string.h>

#include <ctype.h>

#include "global.h"
#include "data.h"
/* from mymem.h, not include because of the malloc junk */
LibraryMenuType * GetLibraryMenuMemory (LibraryType *);
LibraryEntryType * GetLibraryEntryMemory (LibraryMenuType *);

/*
 *	Local definitions.
 */
#define	IDENT_LENGTH		255
#define	Malloc(s)		malloc(s)
#define	Free(p)			free(p)
#define	Getc(s)			getc(s)
#define	Ungetc(c)		ungetc(c,Input)

 typedef struct _str_pair
 {
     char* str1;
     char* str2;
     struct _str_pair* next;
 } str_pair;

 typedef struct _pair_list
 {
     char* name;
     str_pair* list;
 } pair_list;

 str_pair* new_str_pair(char* s1, char* s2)
 {
   str_pair* ps = (str_pair *)malloc(sizeof(str_pair));
     ps->str1 = s1;
     ps->str2 = s2;
     ps->next = NULL;
     return ps;
 }
 
 pair_list* new_pair_list(str_pair* ps)
 {
   pair_list* pl = (pair_list *)malloc(sizeof(pair_list));
     pl->list = ps;
     pl->name = NULL;
     return pl;
 }

 void str_pair_free(str_pair* ps)
 {
     str_pair* node;
     while ( ps )
     {
	 free(ps->str1);
	 free(ps->str2);
	 node = ps;
	 ps = ps->next;
	 free(node);
     }
 }
  
 void pair_list_free(pair_list* pl)
 {
     str_pair_free(pl->list);
     free(pl->name);
     free(pl);
 }

 void define_pcb_net(str_pair* name, pair_list* nodes)
 {
     int tl;
     str_pair* done_node;
     str_pair* node;
     char* buf;
     char* p;
     LibraryEntryType *entry;
     LibraryMenuType *menu = GetLibraryMenuMemory (&PCB->NetlistLib);

     if ( !name->str1 )
     {
	 /* no net name given, stop now */
	 /* if renamed str2 also exists and must be freed */
	 if ( name->str2 )  free(name->str2);
	 free(name);
	 pair_list_free(nodes);
	 return;
     }
     menu->Name = strdup (name->str1);
     free(name->str1);
     /* if renamed str2 also exists and must be freed */
     if ( name->str2 )  free(name->str2);
     free(name);
     buf = (char *)malloc(256);
     if ( !buf )
     {
	 /* no memory */
	 pair_list_free(nodes);
	 return;
     }

     node = nodes->list;
     free(nodes->name);
     free(nodes);
     while ( node )
     {
	 /* check for node with no instance */
	 if ( !node->str1 )
	 {
	     /* toss it and move on */
	     free(node->str2);
	     done_node = node;
	     node = node->next;
	     free(done_node);
	     continue;
	 }
	 tl = strlen(node->str1) + strlen(node->str2);
	 if ( tl + 3 > 256 )
	 {
	     free(buf);
	     buf = (char *)malloc(tl+3);
	     if ( !buf )
	     {
		 /* no memory */
		 str_pair_free(node);
		 return;
	     }
	 }
	 strcpy(buf,node->str1);
	 /* make all upper case, because of PCB funky behaviour */
	 p=buf;
	 while ( *p ) 
	 {
	     *p = toupper( (int) *p);
	     p++;
	 }
	 /* add dash separating designator from node */
	 *(buf+strlen(node->str1)) = '-';
	 /* check for the edif number prefix */
	 if ( node->str2[0] == '&' )
	 {
	     /* skip number prefix */
	     strcpy(buf+strlen(node->str1)+1,node->str2 +1);
	 }
	 else
	 {
	     strcpy(buf+strlen(node->str1)+1,node->str2);
	 }
	 /* free the strings */
	 free(node->str1);
	 free(node->str2);
	 entry = GetLibraryEntryMemory (menu);
	 entry->ListEntry = strdup(buf);
	 done_node = node;
	 node = node->next;
	 free(done_node);
     }
 }
 
 
/* forward function declarations */
 static int yylex(void);
 static void yyerror(const char *);
 static void PopC(void);
%}

%name-prefix="edif"

%union {
    char* s;
    pair_list* pl;
    str_pair* ps;
}

%type <s> Int Ident Str Keyword Name _Name
%type <ps>  PortRef _PortRef NetNameDef NameDef
%type <ps> Rename _Joined
%type <s>__Rename _Rename NameRef
%type <s> InstanceRef InstNameRef Member PortNameRef
%type <pl> Net _Net Joined

%token	<s>	EDIF_TOK_IDENT
%token	<s>	EDIF_TOK_INT
%token	<s>	EDIF_TOK_KEYWORD
%token	<s>	EDIF_TOK_STR

%token		EDIF_TOK_ANGLE
%token		EDIF_TOK_BEHAVIOR
%token		EDIF_TOK_CALCULATED
%token		EDIF_TOK_CAPACITANCE
%token		EDIF_TOK_CENTERCENTER
%token		EDIF_TOK_CENTERLEFT
%token		EDIF_TOK_CENTERRIGHT
%token		EDIF_TOK_CHARGE
%token		EDIF_TOK_CONDUCTANCE
%token		EDIF_TOK_CURRENT
%token		EDIF_TOK_DISTANCE
%token		EDIF_TOK_DOCUMENT
%token		EDIF_TOK_ENERGY
%token		EDIF_TOK_EXTEND
%token		EDIF_TOK_FLUX
%token		EDIF_TOK_FREQUENCY
%token		EDIF_TOK_GENERIC
%token		EDIF_TOK_GRAPHIC
%token		EDIF_TOK_INDUCTANCE
%token		EDIF_TOK_INOUT
%token		EDIF_TOK_INPUT
%token		EDIF_TOK_LOGICMODEL
%token		EDIF_TOK_LOWERCENTER
%token		EDIF_TOK_LOWERLEFT
%token		EDIF_TOK_LOWERRIGHT
%token		EDIF_TOK_MASKLAYOUT
%token		EDIF_TOK_MASS
%token		EDIF_TOK_MEASURED
%token		EDIF_TOK_MX
%token		EDIF_TOK_MXR90
%token		EDIF_TOK_MY
%token		EDIF_TOK_MYR90
%token		EDIF_TOK_NETLIST
%token		EDIF_TOK_OUTPUT
%token		EDIF_TOK_PCBLAYOUT
%token		EDIF_TOK_POWER
%token		EDIF_TOK_R0
%token		EDIF_TOK_R180
%token		EDIF_TOK_R270
%token		EDIF_TOK_R90
%token		EDIF_TOK_REQUIRED
%token		EDIF_TOK_RESISTANCE
%token		EDIF_TOK_RIPPER
%token		EDIF_TOK_ROUND
%token		EDIF_TOK_SCHEMATIC
%token		EDIF_TOK_STRANGER
%token		EDIF_TOK_SYMBOLIC
%token		EDIF_TOK_TEMPERATURE
%token		EDIF_TOK_TIE
%token		EDIF_TOK_TIME
%token		EDIF_TOK_TRUNCATE
%token		EDIF_TOK_UPPERCENTER
%token		EDIF_TOK_UPPERLEFT
%token		EDIF_TOK_UPPERRIGHT
%token		EDIF_TOK_VOLTAGE

%token		EDIF_TOK_ACLOAD
%token		EDIF_TOK_AFTER
%token		EDIF_TOK_ANNOTATE
%token		EDIF_TOK_APPLY
%token		EDIF_TOK_ARC
%token		EDIF_TOK_ARRAY
%token		EDIF_TOK_ARRAYMACRO
%token		EDIF_TOK_ARRAYRELATEDINFO
%token		EDIF_TOK_ARRAYSITE
%token		EDIF_TOK_ATLEAST
%token		EDIF_TOK_ATMOST
%token		EDIF_TOK_AUTHOR
%token		EDIF_TOK_BASEARRAY
%token		EDIF_TOK_BECOMES
%token		EDIF_TOK_BETWEEN
%token		EDIF_TOK_BOOLEAN
%token		EDIF_TOK_BOOLEANDISPLAY
%token		EDIF_TOK_BOOLEANMAP
%token		EDIF_TOK_BORDERPATTERN
%token		EDIF_TOK_BORDERWIDTH
%token		EDIF_TOK_BOUNDINGBOX
%token		EDIF_TOK_CELL
%token		EDIF_TOK_CELLREF
%token		EDIF_TOK_CELLTYPE
%token		EDIF_TOK_CHANGE
%token		EDIF_TOK_CIRCLE
%token		EDIF_TOK_COLOR
%token		EDIF_TOK_COMMENT
%token		EDIF_TOK_COMMENTGRAPHICS
%token		EDIF_TOK_COMPOUND
%token		EDIF_TOK_CONNECTLOCATION
%token		EDIF_TOK_CONTENTS
%token		EDIF_TOK_CORNERTYPE
%token		EDIF_TOK_CRITICALITY
%token		EDIF_TOK_CURRENTMAP
%token		EDIF_TOK_CURVE
%token		EDIF_TOK_CYCLE
%token		EDIF_TOK_DATAORIGIN
%token		EDIF_TOK_DCFANINLOAD
%token		EDIF_TOK_DCFANOUTLOAD
%token		EDIF_TOK_DCMAXFANIN
%token		EDIF_TOK_DCMAXFANOUT
%token		EDIF_TOK_DELAY
%token		EDIF_TOK_DELTA
%token		EDIF_TOK_DERIVATION
%token		EDIF_TOK_DESIGN
%token		EDIF_TOK_DESIGNATOR
%token		EDIF_TOK_DIFFERENCE
%token		EDIF_TOK_DIRECTION
%token		EDIF_TOK_DISPLAY
%token		EDIF_TOK_DOMINATES
%token		EDIF_TOK_DOT
%token		EDIF_TOK_DURATION
%token		EDIF_TOK_E
%token		EDIF_TOK_EDIF
%token		EDIF_TOK_EDIFLEVEL
%token		EDIF_TOK_EDIFVERSION
%token		EDIF_TOK_ENCLOSUREDISTANCE
%token		EDIF_TOK_ENDTYPE
%token		EDIF_TOK_ENTRY
%token		EDIF_TOK_EVENT
%token		EDIF_TOK_EXACTLY
%token		EDIF_TOK_EXTERNAL
%token		EDIF_TOK_FABRICATE
%token		EDIF_TOK_FALSE
%token		EDIF_TOK_FIGURE
%token		EDIF_TOK_FIGUREAREA
%token		EDIF_TOK_FIGUREGROUP
%token		EDIF_TOK_FIGUREGROUPOBJECT
%token		EDIF_TOK_FIGUREGROUPOVERRIDE
%token		EDIF_TOK_FIGUREGROUPREF
%token		EDIF_TOK_FIGUREPERIMETER
%token		EDIF_TOK_FIGUREWIDTH
%token		EDIF_TOK_FILLPATTERN
%token		EDIF_TOK_FOLLOW
%token		EDIF_TOK_FORBIDDENEVENT
%token		EDIF_TOK_GLOBALPORTREF
%token		EDIF_TOK_GREATERTHAN
%token		EDIF_TOK_GRIDMAP
%token		EDIF_TOK_IGNORE
%token		EDIF_TOK_INCLUDEFIGUREGROUP
%token		EDIF_TOK_INITIAL
%token		EDIF_TOK_INSTANCE
%token		EDIF_TOK_INSTANCEBACKANNOTATE
%token		EDIF_TOK_INSTANCEGROUP
%token		EDIF_TOK_INSTANCEMAP
%token		EDIF_TOK_INSTANCEREF
%token		EDIF_TOK_INTEGER
%token		EDIF_TOK_INTEGERDISPLAY
%token		EDIF_TOK_INTERFACE
%token		EDIF_TOK_INTERFIGUREGROUPSPACING
%token		EDIF_TOK_INTERSECTION
%token		EDIF_TOK_INTRAFIGUREGROUPSPACING
%token		EDIF_TOK_INVERSE
%token		EDIF_TOK_ISOLATED
%token		EDIF_TOK_JOINED
%token		EDIF_TOK_JUSTIFY
%token		EDIF_TOK_KEYWORDDISPLAY
%token		EDIF_TOK_KEYWORDLEVEL
%token		EDIF_TOK_KEYWORDMAP
%token		EDIF_TOK_LESSTHAN
%token		EDIF_TOK_LIBRARY
%token		EDIF_TOK_LIBRARYREF
%token		EDIF_TOK_LISTOFNETS
%token		EDIF_TOK_LISTOFPORTS
%token		EDIF_TOK_LOADDELAY
%token		EDIF_TOK_LOGICASSIGN
%token		EDIF_TOK_LOGICINPUT
%token		EDIF_TOK_LOGICLIST
%token		EDIF_TOK_LOGICMAPINPUT
%token		EDIF_TOK_LOGICMAPOUTPUT
%token		EDIF_TOK_LOGICONEOF
%token		EDIF_TOK_LOGICOUTPUT
%token		EDIF_TOK_LOGICPORT
%token		EDIF_TOK_LOGICREF
%token		EDIF_TOK_LOGICVALUE
%token		EDIF_TOK_LOGICWAVEFORM
%token		EDIF_TOK_MAINTAIN
%token		EDIF_TOK_MATCH
%token		EDIF_TOK_MEMBER
%token		EDIF_TOK_MINOMAX
%token		EDIF_TOK_MINOMAXDISPLAY
%token		EDIF_TOK_MNM
%token		EDIF_TOK_MULTIPLEVALUESET
%token		EDIF_TOK_MUSTJOIN
%token		EDIF_TOK_NAME
%token		EDIF_TOK_NET
%token		EDIF_TOK_NETBACKANNOTATE
%token		EDIF_TOK_NETBUNDLE
%token		EDIF_TOK_NETDELAY
%token		EDIF_TOK_NETGROUP
%token		EDIF_TOK_NETMAP
%token		EDIF_TOK_NETREF
%token		EDIF_TOK_NOCHANGE
%token		EDIF_TOK_NONPERMUTABLE
%token		EDIF_TOK_NOTALLOWED
%token		EDIF_TOK_NOTCHSPACING
%token		EDIF_TOK_NUMBER
%token		EDIF_TOK_NUMBERDEFINITION
%token		EDIF_TOK_NUMBERDISPLAY
%token		EDIF_TOK_OFFPAGECONNECTOR
%token		EDIF_TOK_OFFSETEVENT
%token		EDIF_TOK_OPENSHAPE
%token		EDIF_TOK_ORIENTATION
%token		EDIF_TOK_ORIGIN
%token		EDIF_TOK_OVERHANGDISTANCE
%token		EDIF_TOK_OVERLAPDISTANCE
%token		EDIF_TOK_OVERSIZE
%token		EDIF_TOK_OWNER
%token		EDIF_TOK_PAGE
%token		EDIF_TOK_PAGESIZE
%token		EDIF_TOK_PARAMETER
%token		EDIF_TOK_PARAMETERASSIGN
%token		EDIF_TOK_PARAMETERDISPLAY
%token		EDIF_TOK_PATH
%token		EDIF_TOK_PATHDELAY
%token		EDIF_TOK_PATHWIDTH
%token		EDIF_TOK_PERMUTABLE
%token		EDIF_TOK_PHYSICALDESIGNRULE
%token		EDIF_TOK_PLUG
%token		EDIF_TOK_POINT
%token		EDIF_TOK_POINTDISPLAY
%token		EDIF_TOK_POINTLIST
%token		EDIF_TOK_POLYGON
%token		EDIF_TOK_PORT
%token		EDIF_TOK_PORTBACKANNOTATE
%token		EDIF_TOK_PORTBUNDLE
%token		EDIF_TOK_PORTDELAY
%token		EDIF_TOK_PORTGROUP
%token		EDIF_TOK_PORTIMPLEMENTATION
%token		EDIF_TOK_PORTINSTANCE
%token		EDIF_TOK_PORTLIST
%token		EDIF_TOK_PORTLISTALIAS
%token		EDIF_TOK_PORTMAP
%token		EDIF_TOK_PORTREF
%token		EDIF_TOK_PROGRAM
%token		EDIF_TOK_PROPERTY
%token		EDIF_TOK_PROPERTYDISPLAY
%token		EDIF_TOK_PROTECTIONFRAME
%token		EDIF_TOK_PT
%token		EDIF_TOK_RANGEVECTOR
%token		EDIF_TOK_RECTANGLE
%token		EDIF_TOK_RECTANGLESIZE
%token		EDIF_TOK_RENAME
%token		EDIF_TOK_RESOLVES
%token		EDIF_TOK_SCALE
%token		EDIF_TOK_SCALEX
%token		EDIF_TOK_SCALEY
%token		EDIF_TOK_SECTION
%token		EDIF_TOK_SHAPE
%token		EDIF_TOK_SIMULATE
%token		EDIF_TOK_SIMULATIONINFO
%token		EDIF_TOK_SINGLEVALUESET
%token		EDIF_TOK_SITE
%token		EDIF_TOK_SOCKET
%token		EDIF_TOK_SOCKETSET
%token		EDIF_TOK_STATUS
%token		EDIF_TOK_STEADY
%token		EDIF_TOK_STRING
%token		EDIF_TOK_STRINGDISPLAY
%token		EDIF_TOK_STRONG
%token		EDIF_TOK_SYMBOL
%token		EDIF_TOK_SYMMETRY
%token		EDIF_TOK_TABLE
%token		EDIF_TOK_TABLEDEFAULT
%token		EDIF_TOK_TECHNOLOGY
%token		EDIF_TOK_TEXTHEIGHT
%token		EDIF_TOK_TIMEINTERVAL
%token		EDIF_TOK_TIMESTAMP
%token		EDIF_TOK_TIMING
%token		EDIF_TOK_TRANSFORM
%token		EDIF_TOK_TRANSITION
%token		EDIF_TOK_TRIGGER
%token		EDIF_TOK_TRUE
%token		EDIF_TOK_UNCONSTRAINED
%token		EDIF_TOK_UNDEFINED
%token		EDIF_TOK_UNION
%token		EDIF_TOK_UNIT
%token		EDIF_TOK_UNUSED
%token		EDIF_TOK_USERDATA
%token		EDIF_TOK_VERSION
%token		EDIF_TOK_VIEW
%token		EDIF_TOK_VIEWLIST
%token		EDIF_TOK_VIEWMAP
%token		EDIF_TOK_VIEWREF
%token		EDIF_TOK_VIEWTYPE
%token		EDIF_TOK_VISIBLE
%token		EDIF_TOK_VOLTAGEMAP
%token		EDIF_TOK_WAVEVALUE
%token		EDIF_TOK_WEAK
%token		EDIF_TOK_WEAKJOINED
%token		EDIF_TOK_WHEN
%token		EDIF_TOK_WRITTEN

%start	Edif

%%

PopC :		')' { PopC(); }
     ;

Edif :		EDIF_TOK_EDIF EdifFileName EdifVersion EdifLevel KeywordMap _Edif PopC
     ;

_Edif :
      |		_Edif Status
      |		_Edif External
      |		_Edif Library
      |		_Edif Design
      |		_Edif Comment
      |		_Edif UserData
      ;

EdifFileName :	NameDef { str_pair_free($1); }
	     ;

EdifLevel :	EDIF_TOK_EDIFLEVEL Int PopC { free($2); }
	  ;

EdifVersion :	EDIF_TOK_EDIFVERSION Int Int Int PopC
{ free($2); free($3); free($4); }
	    ;

AcLoad :	EDIF_TOK_ACLOAD _AcLoad PopC
       ;

_AcLoad :	MiNoMaValue
	|	MiNoMaDisp
	;

After :		EDIF_TOK_AFTER _After PopC
      ;

_After :	MiNoMaValue
       |	_After Follow
       |	_After Maintain
       |	_After LogicAssn
       |	_After Comment
       |	_After UserData
       ;

Annotate :	EDIF_TOK_ANNOTATE _Annotate PopC
	 ;

_Annotate :	Str { free($1); }
	  |	StrDisplay
	  ;

Apply :		EDIF_TOK_APPLY _Apply PopC
      ;

_Apply :	Cycle
       |	_Apply LogicIn
       |	_Apply LogicOut
       |	_Apply Comment
       |	_Apply UserData
       ;

Arc :		EDIF_TOK_ARC PointValue PointValue PointValue PopC
    ;

Array :		EDIF_TOK_ARRAY NameDef Int _Array PopC { str_pair_free($2); free($3); }
      ;

_Array :
       |	Int { free($1); }
       ;

ArrayMacro :	EDIF_TOK_ARRAYMACRO Plug PopC
	   ;

ArrayRelInfo :	EDIF_TOK_ARRAYRELATEDINFO _ArrayRelInfo PopC
	     ;

_ArrayRelInfo :	BaseArray
	      |	ArraySite
	      |	ArrayMacro
	      |	_ArrayRelInfo Comment
	      |	_ArrayRelInfo UserData
	      ;

ArraySite :	EDIF_TOK_ARRAYSITE Socket PopC
	  ;

AtLeast :	EDIF_TOK_ATLEAST ScaledInt PopC
	;

AtMost :	EDIF_TOK_ATMOST ScaledInt PopC
       ;

Author :	EDIF_TOK_AUTHOR Str PopC { free($2); }
       ;

BaseArray :	EDIF_TOK_BASEARRAY PopC
	  ;

Becomes :	EDIF_TOK_BECOMES _Becomes PopC
	;

_Becomes :	LogicNameRef
	 |	LogicList
	 |	LogicOneOf
	 ;

Between :	EDIF_TOK_BETWEEN __Between _Between PopC
	;

__Between :	AtLeast
	  |	GreaterThan
	  ;

_Between :	AtMost
	 |	LessThan
	 ;

Boolean :	EDIF_TOK_BOOLEAN _Boolean PopC
	;

_Boolean :
	 |	_Boolean BooleanValue
	 |	_Boolean BooleanDisp
	 |	_Boolean Boolean
	 ;

BooleanDisp :	EDIF_TOK_BOOLEANDISPLAY _BooleanDisp PopC
	    ;

_BooleanDisp :	BooleanValue
	     |	_BooleanDisp Display
	     ;

BooleanMap :	EDIF_TOK_BOOLEANMAP BooleanValue PopC
	   ;

BooleanValue :	True
	     |	False
	     ;

BorderPat :	EDIF_TOK_BORDERPATTERN Int Int Boolean PopC { free($2); free($3); }
	  ;

BorderWidth :	EDIF_TOK_BORDERWIDTH Int PopC { free($2); }
	    ;

BoundBox :	EDIF_TOK_BOUNDINGBOX Rectangle PopC
	 ;

Cell :		EDIF_TOK_CELL CellNameDef _Cell PopC
     ;

_Cell :		CellType
      |		_Cell Status
      |		_Cell ViewMap
      |		_Cell View
      |		_Cell Comment
      |		_Cell UserData
      |		_Cell Property
      ;

CellNameDef :	NameDef { str_pair_free($1); }
	    ;

CellRef :	EDIF_TOK_CELLREF CellNameRef _CellRef PopC
	;

_CellRef :
	 |	LibraryRef
	 ;

CellNameRef :	NameRef { free($1); }
	    ;

CellType :	EDIF_TOK_CELLTYPE _CellType PopC
	 ;

_CellType :	EDIF_TOK_TIE
	  |	EDIF_TOK_RIPPER
	  |	EDIF_TOK_GENERIC
	  ;

Change :	EDIF_TOK_CHANGE __Change _Change PopC
       ;

__Change :	PortNameRef
	 |	PortRef { str_pair_free($1); }
	 |	PortList
	 ;

_Change :
	|	Becomes
	|	Transition
	;

Circle :	EDIF_TOK_CIRCLE PointValue PointValue _Circle PopC
       ;

_Circle :
	|	_Circle Property
	;

Color :		EDIF_TOK_COLOR ScaledInt ScaledInt ScaledInt PopC
      ;

Comment :	EDIF_TOK_COMMENT _Comment PopC
	;

_Comment :
	 |	_Comment Str { free($2); }
	 ;

CommGraph :	EDIF_TOK_COMMENTGRAPHICS _CommGraph PopC
          ;

_CommGraph :
	   |	_CommGraph Annotate
	   |	_CommGraph Figure
	   |	_CommGraph Instance
	   |	_CommGraph BoundBox
	   |	_CommGraph Property
	   |	_CommGraph Comment
	   |	_CommGraph UserData
	   ;

Compound :	EDIF_TOK_COMPOUND LogicNameRef PopC
	 ;

Contents :	EDIF_TOK_CONTENTS _Contents PopC
	 ;

_Contents :
	  |	_Contents Instance
	  |	_Contents OffPageConn
	  |	_Contents Figure
	  |	_Contents Section
	  |	_Contents Net
	  |	_Contents NetBundle
	  |	_Contents Page
	  |	_Contents CommGraph
	  |	_Contents PortImpl
	  |	_Contents Timing
	  |	_Contents Simulate
	  |	_Contents When
	  |	_Contents Follow
	  |	_Contents LogicPort
	  |	_Contents BoundBox
	  |	_Contents Comment
	  |	_Contents UserData
	  ;

ConnectLoc :	EDIF_TOK_CONNECTLOCATION _ConnectLoc PopC
	   ;

_ConnectLoc :
	    |	Figure
	    ;

CornerType :	EDIF_TOK_CORNERTYPE _CornerType PopC
	   ;

_CornerType :	EDIF_TOK_EXTEND
	    |	EDIF_TOK_ROUND
	    |	EDIF_TOK_TRUNCATE
	    ;

Criticality :	EDIF_TOK_CRITICALITY _Criticality PopC
	    ;

_Criticality :	Int { free($1); }
	     |	IntDisplay
	     ;

CurrentMap :	EDIF_TOK_CURRENTMAP MiNoMaValue PopC
	   ;

Curve :		EDIF_TOK_CURVE _Curve PopC
      ;

_Curve :
       |	_Curve Arc
       |	_Curve PointValue
       ;

Cycle :		EDIF_TOK_CYCLE Int _Cycle PopC { free($2); }
      ;

_Cycle :
       |	Duration
       ;

DataOrigin :	EDIF_TOK_DATAORIGIN Str _DataOrigin PopC { free($2); }
	   ;

_DataOrigin :
	    |	Version
	    ;

DcFanInLoad :	EDIF_TOK_DCFANINLOAD _DcFanInLoad PopC
	    ;

_DcFanInLoad :	ScaledInt
	     |	NumbDisplay
	     ;

DcFanOutLoad :	EDIF_TOK_DCFANOUTLOAD _DcFanOutLoad PopC
	     ;

_DcFanOutLoad :	ScaledInt
	      |	NumbDisplay
	      ;

DcMaxFanIn :	EDIF_TOK_DCMAXFANIN _DcMaxFanIn PopC
	   ;

_DcMaxFanIn :	ScaledInt
	    |	NumbDisplay
	    ;

DcMaxFanOut :	EDIF_TOK_DCMAXFANOUT _DcMaxFanOut PopC
	    ;

_DcMaxFanOut :	ScaledInt
	     |	NumbDisplay
	     ;

Delay :		EDIF_TOK_DELAY _Delay PopC
      ;

_Delay :	MiNoMaValue
       |	MiNoMaDisp
       ;

Delta :		EDIF_TOK_DELTA _Delta PopC
      ;

_Delta :
       |	_Delta PointValue
       ;

Derivation :	EDIF_TOK_DERIVATION _Derivation PopC
	   ;

_Derivation :	EDIF_TOK_CALCULATED
	    |	EDIF_TOK_MEASURED
	    |	EDIF_TOK_REQUIRED
	    ;

Design :	EDIF_TOK_DESIGN DesignNameDef _Design PopC
       ;

_Design :	CellRef
	|	_Design Status
	|	_Design Comment
	|	_Design Property
	|	_Design UserData
	;

Designator :	EDIF_TOK_DESIGNATOR _Designator PopC
	   ;

_Designator :	Str { free($1); }
	    |	StrDisplay
	    ;

DesignNameDef :	NameDef { str_pair_free($1); }
	      ;

DesignRule :	EDIF_TOK_PHYSICALDESIGNRULE _DesignRule PopC
	   ;

_DesignRule :
	    |	_DesignRule FigureWidth
	    |	_DesignRule FigureArea
	    |	_DesignRule RectSize
	    |	_DesignRule FigurePerim
	    |	_DesignRule OverlapDist
	    |	_DesignRule OverhngDist
	    |	_DesignRule EncloseDist
	    |	_DesignRule InterFigGrp
	    |	_DesignRule IntraFigGrp
	    |	_DesignRule NotchSpace
	    |	_DesignRule NotAllowed
	    |	_DesignRule FigGrp
	    |	_DesignRule Comment
	    |	_DesignRule UserData
	    ;

Difference :	EDIF_TOK_DIFFERENCE _Difference PopC
	   ;

_Difference :	FigGrpRef
	    |	FigureOp
	    |	_Difference FigGrpRef
	    |	_Difference FigureOp
	    ;

Direction :	EDIF_TOK_DIRECTION _Direction PopC
	  ;

_Direction :	EDIF_TOK_INOUT
	   |	EDIF_TOK_INPUT
	   |	EDIF_TOK_OUTPUT
	   ;

Display :	EDIF_TOK_DISPLAY _Display _DisplayJust _DisplayOrien _DisplayOrg PopC
	;

_Display :	FigGrpNameRef
	 |	FigGrpOver
	 ;

_DisplayJust :
	     |	Justify
	     ;

_DisplayOrien :
	      |	Orientation
	      ;

_DisplayOrg :
	    |	Origin
	    ;

Dominates :	EDIF_TOK_DOMINATES _Dominates PopC
	  ;

_Dominates :
	   |	_Dominates LogicNameRef
	   ;

Dot :		EDIF_TOK_DOT _Dot PopC
    ;

_Dot :		PointValue
     |		_Dot Property
     ;

Duration :	EDIF_TOK_DURATION ScaledInt PopC
	 ;

EncloseDist :	EDIF_TOK_ENCLOSUREDISTANCE RuleNameDef FigGrpObj FigGrpObj _EncloseDist
		PopC
	    ;

_EncloseDist :	Range
	     |	SingleValSet
	     |	_EncloseDist Comment
	     |	_EncloseDist UserData
	     ;

EndType :	EDIF_TOK_ENDTYPE _EndType PopC
	;

_EndType :	EDIF_TOK_EXTEND
	 |	EDIF_TOK_ROUND
	 |	EDIF_TOK_TRUNCATE
	 ;

Entry :		EDIF_TOK_ENTRY ___Entry __Entry _Entry
		PopC
      ;

___Entry :	Match
	 |	Change
	 |	Steady
	 ;

__Entry :	LogicRef
	|	PortRef { str_pair_free($1); }
	|	NoChange
	|	Table
	;

_Entry :
       |	Delay
       |	LoadDelay
       ;

Event :		EDIF_TOK_EVENT _Event PopC
      ;

_Event :	PortRef { str_pair_free($1); }
       |	PortList
       |	PortGroup
       |	NetRef
       |	NetGroup
       |	_Event Transition
       |	_Event Becomes
       ;

Exactly :	EDIF_TOK_EXACTLY ScaledInt PopC
	;

External :	EDIF_TOK_EXTERNAL LibNameDef EdifLevel _External PopC
	 ;

_External :	Technology
	  |	_External Status
	  |	_External Cell
	  |	_External Comment
	  |	_External UserData
	  ;

Fabricate :	EDIF_TOK_FABRICATE LayerNameDef FigGrpNameRef PopC
	  ;

False :		EDIF_TOK_FALSE PopC
      ;

FigGrp :	EDIF_TOK_FIGUREGROUP _FigGrp PopC
       ;

_FigGrp :	FigGrpNameDef
	|	_FigGrp CornerType
	|	_FigGrp EndType
	|	_FigGrp PathWidth
	|	_FigGrp BorderWidth
	|	_FigGrp Color
	|	_FigGrp FillPattern
	|	_FigGrp BorderPat
	|	_FigGrp TextHeight
	|	_FigGrp Visible
	|	_FigGrp Comment
	|	_FigGrp Property
	|	_FigGrp UserData
	|	_FigGrp IncFigGrp
	;

FigGrpNameDef :	NameDef { str_pair_free($1); }
	      ;

FigGrpNameRef :	NameRef { free($1); }
	      ;

FigGrpObj :	EDIF_TOK_FIGUREGROUPOBJECT _FigGrpObj PopC
	  ;

_FigGrpObj :	FigGrpNameRef
	   |	FigGrpRef
	   |	FigureOp
	   ;

FigGrpOver :	EDIF_TOK_FIGUREGROUPOVERRIDE _FigGrpOver PopC
	   ;

_FigGrpOver :	FigGrpNameRef
	    |	_FigGrpOver CornerType
	    |	_FigGrpOver EndType
	    |	_FigGrpOver PathWidth
	    |	_FigGrpOver BorderWidth
	    |	_FigGrpOver Color
	    |	_FigGrpOver FillPattern
	    |	_FigGrpOver BorderPat
	    |	_FigGrpOver TextHeight
	    |	_FigGrpOver Visible
	    |	_FigGrpOver Comment
	    |	_FigGrpOver Property
	    |	_FigGrpOver UserData
	    ;

FigGrpRef :	EDIF_TOK_FIGUREGROUPREF FigGrpNameRef _FigGrpRef PopC
	  ;

_FigGrpRef :
	   |	LibraryRef
	   ;

Figure :	EDIF_TOK_FIGURE _Figure PopC
       ;

_Figure :	FigGrpNameDef
	|	FigGrpOver
	|	_Figure Circle
	|	_Figure Dot
	|	_Figure OpenShape
	|	_Figure Path
	|	_Figure Polygon
	|	_Figure Rectangle
	|	_Figure Shape
	|	_Figure Comment
	|	_Figure UserData
	;

FigureArea :	EDIF_TOK_FIGUREAREA RuleNameDef FigGrpObj _FigureArea PopC
	   ;

_FigureArea :	Range
	    |	SingleValSet
	    |	_FigureArea Comment
	    |	_FigureArea UserData
	    ;

FigureOp :	Intersection
	 |	Union
	 |	Difference
	 |	Inverse
	 |	Oversize
	 ;

FigurePerim :	EDIF_TOK_FIGUREPERIMETER RuleNameDef FigGrpObj _FigurePerim PopC
	    ;

_FigurePerim :	Range
	     |	SingleValSet
	     |	_FigurePerim Comment
	     |	_FigurePerim UserData
	     ;

FigureWidth :	EDIF_TOK_FIGUREWIDTH RuleNameDef FigGrpObj _FigureWidth PopC
	    ;

_FigureWidth :	Range
	     |	SingleValSet
	     |	_FigureWidth Comment
	     |	_FigureWidth UserData
	     ;

FillPattern :	EDIF_TOK_FILLPATTERN Int Int Boolean PopC { free($2); free($3); }
	    ;

Follow :	EDIF_TOK_FOLLOW __Follow _Follow PopC
       ;

__Follow :	PortNameRef
	 |	PortRef { str_pair_free($1); }
	 ;

_Follow :	PortRef { str_pair_free($1); }
	|	Table
	|	_Follow Delay
	|	_Follow LoadDelay
	;

Forbidden :	EDIF_TOK_FORBIDDENEVENT _Forbidden PopC
	  ;

_Forbidden :	TimeIntval
	   |	_Forbidden Event
	   ;

Form :		Keyword _Form ')' { free($1); }
     ;

_Form :
      |		_Form Int { free($2); }
      |		_Form Str { free($2); }
      |		_Form Ident { free($2); }
      |		_Form Form
      ;

GlobPortRef :	EDIF_TOK_GLOBALPORTREF PortNameRef PopC
	    ;

GreaterThan :	EDIF_TOK_GREATERTHAN ScaledInt PopC
	    ;

GridMap :	EDIF_TOK_GRIDMAP ScaledInt ScaledInt PopC
	;

Ignore :	EDIF_TOK_IGNORE PopC
       ;

IncFigGrp :	EDIF_TOK_INCLUDEFIGUREGROUP _IncFigGrp PopC
	  ;

_IncFigGrp :	FigGrpRef
	   |	FigureOp
	   ;

Initial :	EDIF_TOK_INITIAL PopC
	;

Instance :	EDIF_TOK_INSTANCE InstNameDef _Instance PopC
	 ;

_Instance :	ViewRef
	  |	ViewList
	  |	_Instance Transform
	  |	_Instance ParamAssign
	  |	_Instance PortInst
	  |	_Instance Timing
	  |	_Instance Designator
	  |	_Instance Property
	  |	_Instance Comment
	  |	_Instance UserData
	  ;

InstanceRef :	EDIF_TOK_INSTANCEREF InstNameRef _InstanceRef PopC { $$=$2; }
	    ;

_InstanceRef :
|	InstanceRef { free($1); }
	     |	ViewRef
	     ;

InstBackAn :	EDIF_TOK_INSTANCEBACKANNOTATE _InstBackAn PopC
	   ;

_InstBackAn :	InstanceRef { free($1); }
	    |	_InstBackAn Designator
	    |	_InstBackAn Timing
	    |	_InstBackAn Property
	    |	_InstBackAn Comment
	    ;

InstGroup :	EDIF_TOK_INSTANCEGROUP _InstGroup PopC
	  ;

_InstGroup :
	   |	_InstGroup InstanceRef { free($2); }
	   ;

InstMap :	EDIF_TOK_INSTANCEMAP _InstMap PopC
	;

_InstMap :
	 |	_InstMap InstanceRef { free($2); }
	 |	_InstMap InstGroup
	 |	_InstMap Comment
	 |	_InstMap UserData
	 ;

InstNameDef :	NameDef { str_pair_free($1); }
	    |	Array
	    ;

InstNameRef :	NameRef { $$=$1; }
	    |	Member
	    ;

IntDisplay :	EDIF_TOK_INTEGERDISPLAY _IntDisplay PopC
	   ;

_IntDisplay :	Int { free($1); }
	    |	_IntDisplay Display
	    ;

Integer :	EDIF_TOK_INTEGER _Integer PopC
	;

_Integer :
	 |	_Integer Int { free($2); }
	 |	_Integer IntDisplay
	 |	_Integer Integer
	 ;

Interface :	EDIF_TOK_INTERFACE _Interface PopC
	  ;

_Interface :
	   |	_Interface Port
	   |	_Interface PortBundle
	   |	_Interface Symbol
	   |	_Interface ProtectFrame
	   |	_Interface ArrayRelInfo
	   |	_Interface Parameter
	   |	_Interface Joined { pair_list_free($2); }
	   |	_Interface MustJoin
	   |	_Interface WeakJoined
	   |	_Interface Permutable
	   |	_Interface Timing
	   |	_Interface Simulate
	   |	_Interface Designator
	   |	_Interface Property
	   |	_Interface Comment
	   |	_Interface UserData
	   ;

InterFigGrp :	EDIF_TOK_INTERFIGUREGROUPSPACING RuleNameDef FigGrpObj FigGrpObj
		_InterFigGrp PopC
	    ;

_InterFigGrp :	Range
	     |	SingleValSet
	     |	_InterFigGrp Comment
	     |	_InterFigGrp UserData
	     ;

Intersection :	EDIF_TOK_INTERSECTION _Intersection PopC
	     ;

_Intersection :	FigGrpRef
	      |	FigureOp
	      |	_Intersection FigGrpRef
	      |	_Intersection FigureOp
	      ;

IntraFigGrp :	EDIF_TOK_INTRAFIGUREGROUPSPACING RuleNameDef FigGrpObj _IntraFigGrp PopC
	    ;

_IntraFigGrp :	Range
	     |	SingleValSet
	     |	_IntraFigGrp Comment
	     |	_IntraFigGrp UserData
	     ;

Inverse :	EDIF_TOK_INVERSE _Inverse PopC
	;

_Inverse :	FigGrpRef
	 |	FigureOp
	 ;

Isolated :	EDIF_TOK_ISOLATED PopC
	 ;

Joined :	EDIF_TOK_JOINED _Joined PopC { $$ = new_pair_list($2); }
       ;

_Joined : { $$=NULL; }
|	_Joined PortRef { $2->next = $1; $$ = $2; }
	|	_Joined PortList
	|	_Joined GlobPortRef
	;

Justify :	EDIF_TOK_JUSTIFY _Justify PopC
	;

_Justify :	EDIF_TOK_CENTERCENTER
	 |	EDIF_TOK_CENTERLEFT
	 |	EDIF_TOK_CENTERRIGHT
	 |	EDIF_TOK_LOWERCENTER
	 |	EDIF_TOK_LOWERLEFT
	 |	EDIF_TOK_LOWERRIGHT
	 |	EDIF_TOK_UPPERCENTER
	 |	EDIF_TOK_UPPERLEFT
	 |	EDIF_TOK_UPPERRIGHT
	 ;

KeywordDisp :	EDIF_TOK_KEYWORDDISPLAY _KeywordDisp PopC
	    ;

_KeywordDisp :	KeywordName
	     |	_KeywordDisp Display
	     ;

KeywordLevel :	EDIF_TOK_KEYWORDLEVEL Int PopC { free($2); }
	     ;

KeywordMap :	EDIF_TOK_KEYWORDMAP _KeywordMap PopC
	   ;

_KeywordMap :	KeywordLevel
	    |	_KeywordMap Comment
	    ;

KeywordName :	Ident { free($1); }
	    ;

LayerNameDef :	NameDef { str_pair_free($1); }
	     ;

LessThan :	EDIF_TOK_LESSTHAN ScaledInt PopC
	 ;

LibNameDef :	NameDef { str_pair_free($1); }
	   ;

LibNameRef :	NameRef { free($1); }
	   ;

Library :	EDIF_TOK_LIBRARY LibNameDef EdifLevel _Library PopC
	;

_Library :	Technology
	 |	_Library Status
	 |	_Library Cell
	 |	_Library Comment
	 |	_Library UserData
	 ;

LibraryRef :	EDIF_TOK_LIBRARYREF LibNameRef PopC
	   ;

ListOfNets :	EDIF_TOK_LISTOFNETS _ListOfNets PopC
	   ;

_ListOfNets :
	    |	_ListOfNets Net
	    ;

ListOfPorts :	EDIF_TOK_LISTOFPORTS _ListOfPorts PopC
	    ;

_ListOfPorts :
	     |	_ListOfPorts Port
	     |	_ListOfPorts PortBundle
	     ;

LoadDelay :	EDIF_TOK_LOADDELAY _LoadDelay _LoadDelay PopC
	  ;

_LoadDelay :	MiNoMaValue
	   |	MiNoMaDisp
	   ;

LogicAssn :	EDIF_TOK_LOGICASSIGN ___LogicAssn __LogicAssn _LogicAssn PopC
	  ;

___LogicAssn :	PortNameRef
	     |	PortRef { str_pair_free($1); }
	     ;

__LogicAssn :	PortRef { str_pair_free($1); }
	    |	LogicRef
	    |	Table
	    ;

_LogicAssn :
	   |	Delay
	   |	LoadDelay
	   ;

LogicIn :	EDIF_TOK_LOGICINPUT _LogicIn PopC
	;

_LogicIn :	PortList
	 |	PortRef { str_pair_free($1); }
	 |	PortNameRef
	 |	_LogicIn LogicWave
	 ;

LogicList :	EDIF_TOK_LOGICLIST _LogicList PopC
	  ;

_LogicList :
	   |	_LogicList LogicNameRef
	   |	_LogicList LogicOneOf
	   |	_LogicList Ignore
	   ;

LogicMapIn :	EDIF_TOK_LOGICMAPINPUT _LogicMapIn PopC
	   ;

_LogicMapIn :
	    |	_LogicMapIn LogicNameRef
	    ;

LogicMapOut :	EDIF_TOK_LOGICMAPOUTPUT _LogicMapOut PopC
	    ;

_LogicMapOut :
	     |	_LogicMapOut LogicNameRef
	     ;

LogicNameDef :	NameDef { str_pair_free($1); }
	     ;

LogicNameRef :	NameRef { free($1); }
	     ;

LogicOneOf :	EDIF_TOK_LOGICONEOF _LogicOneOf PopC
	   ;

_LogicOneOf :
	    |	_LogicOneOf LogicNameRef
	    |	_LogicOneOf LogicList
	    ;

LogicOut :	EDIF_TOK_LOGICOUTPUT _LogicOut PopC
	 ;

_LogicOut :	PortList
	  |	PortRef { str_pair_free($1); }
	  |	PortNameRef
	  |	_LogicOut LogicWave
	  ;

LogicPort :	EDIF_TOK_LOGICPORT _LogicPort PopC
	  ;

_LogicPort :	PortNameDef
	   |	_LogicPort Property
	   |	_LogicPort Comment
	   |	_LogicPort UserData
	   ;

LogicRef :	EDIF_TOK_LOGICREF LogicNameRef _LogicRef PopC
	 ;

_LogicRef :
	  |	LibraryRef
	  ;

LogicValue :	EDIF_TOK_LOGICVALUE _LogicValue PopC
	   ;

_LogicValue :	LogicNameDef
	    |	_LogicValue VoltageMap
	    |	_LogicValue CurrentMap
	    |	_LogicValue BooleanMap
	    |	_LogicValue Compound
	    |	_LogicValue Weak
	    |	_LogicValue Strong
	    |	_LogicValue Dominates
	    |	_LogicValue LogicMapOut
	    |	_LogicValue LogicMapIn
	    |	_LogicValue Isolated
	    |	_LogicValue Resolves
	    |	_LogicValue Property
	    |	_LogicValue Comment
	    |	_LogicValue UserData
	    ;

LogicWave :	EDIF_TOK_LOGICWAVEFORM _LogicWave PopC
	  ;

_LogicWave :
	   |	_LogicWave LogicNameRef
	   |	_LogicWave LogicList
	   |	_LogicWave LogicOneOf
	   |	_LogicWave Ignore
	   ;

Maintain :	EDIF_TOK_MAINTAIN __Maintain _Maintain PopC
	 ;

__Maintain :	PortNameRef
	   |	PortRef { str_pair_free($1); }
	   ;

_Maintain :
	  |	Delay
	  |	LoadDelay
	  ;

Match :		EDIF_TOK_MATCH __Match _Match PopC
      ;

__Match :	PortNameRef
	|	PortRef { str_pair_free($1); }
	|	PortList
	;

_Match :	LogicNameRef
       |	LogicList
       |	LogicOneOf
       ;

Member :	EDIF_TOK_MEMBER NameRef _Member PopC  { free($2); }
       ;

_Member :	Int { free($1); }
	|	_Member Int { free($2); }
	;

MiNoMa :	EDIF_TOK_MINOMAX _MiNoMa PopC
       ;

_MiNoMa :
	|	_MiNoMa MiNoMaValue
	|	_MiNoMa MiNoMaDisp
	|	_MiNoMa MiNoMa
	;

MiNoMaDisp :	EDIF_TOK_MINOMAXDISPLAY _MiNoMaDisp PopC
	   ;

_MiNoMaDisp :	MiNoMaValue
	    |	_MiNoMaDisp Display
	    ;

MiNoMaValue :	Mnm
	    |	ScaledInt
	    ;

Mnm :		EDIF_TOK_MNM _Mnm _Mnm _Mnm PopC
    ;

_Mnm :		ScaledInt
     |		Undefined
     |		Unconstrained
     ;

MultValSet :	EDIF_TOK_MULTIPLEVALUESET _MultValSet PopC
	   ;

_MultValSet :
	    |	_MultValSet RangeVector
	    ;

MustJoin :	EDIF_TOK_MUSTJOIN _MustJoin PopC
	 ;

_MustJoin :
	  |	_MustJoin PortRef { str_pair_free($2); }
	  |	_MustJoin PortList
	  |	_MustJoin WeakJoined
	  |	_MustJoin Joined { pair_list_free($2); }
	  ;

Name :		EDIF_TOK_NAME _Name PopC { $$=$2; }
     ;

_Name :		Ident { $$=$1; }
      |		_Name Display
      ;

NameDef :	Ident { $$ = new_str_pair($1,NULL); }
	|	Name { $$ = new_str_pair($1,NULL); }
|	Rename { $$=$1; }
	;

NameRef :	Ident { $$=$1; }
	|	Name { $$=$1; }
	;

Net :		EDIF_TOK_NET NetNameDef _Net PopC { define_pcb_net($2, $3); }
    ;

_Net :		Joined { $$=$1; }
     |		_Net Criticality
     |		_Net NetDelay
     |		_Net Figure
     |		_Net Net
     |		_Net Instance
     |		_Net CommGraph
     |		_Net Property
     |		_Net Comment
     |		_Net UserData
     ;

NetBackAn :	EDIF_TOK_NETBACKANNOTATE _NetBackAn PopC
	  ;

_NetBackAn :	NetRef
	   |	_NetBackAn NetDelay
	   |	_NetBackAn Criticality
	   |	_NetBackAn Property
	   |	_NetBackAn Comment
	   ;

NetBundle :	EDIF_TOK_NETBUNDLE NetNameDef _NetBundle PopC { str_pair_free($2); }
	  ;

_NetBundle :	ListOfNets
	   |	_NetBundle Figure
	   |	_NetBundle CommGraph
	   |	_NetBundle Property
	   |	_NetBundle Comment
	   |	_NetBundle UserData
	   ;

NetDelay :	EDIF_TOK_NETDELAY Derivation _NetDelay PopC
	 ;

_NetDelay :	Delay
	  |	_NetDelay Transition
	  |	_NetDelay Becomes
	  ;

NetGroup :	EDIF_TOK_NETGROUP _NetGroup PopC
	 ;

_NetGroup :
	  |	_NetGroup NetNameRef
	  |	_NetGroup NetRef
	  ;

NetMap :	EDIF_TOK_NETMAP _NetMap PopC
       ;

_NetMap :
	|	_NetMap NetRef
	|	_NetMap NetGroup
	|	_NetMap Comment
	|	_NetMap UserData
	;

NetNameDef :	NameDef { $$=$1; }
|	Array { $$=NULL; }

	   ;

NetNameRef :	NameRef { free($1); }
	   |	Member
	   ;

NetRef :	EDIF_TOK_NETREF NetNameRef _NetRef PopC
       ;

_NetRef :
	|	NetRef
	|	InstanceRef { free($1); }
	|	ViewRef
	;

NoChange :	EDIF_TOK_NOCHANGE PopC
	 ;

NonPermut :	EDIF_TOK_NONPERMUTABLE _NonPermut PopC
	  ;

_NonPermut :
	   |	_NonPermut PortRef { str_pair_free($2); }
	   |	_NonPermut Permutable
	   ;

NotAllowed :	EDIF_TOK_NOTALLOWED RuleNameDef _NotAllowed PopC
	   ;

_NotAllowed :	FigGrpObj
	    |	_NotAllowed Comment
	    |	_NotAllowed UserData
	    ;

NotchSpace :	EDIF_TOK_NOTCHSPACING RuleNameDef FigGrpObj _NotchSpace PopC
	   ;

_NotchSpace :	Range
	    |	SingleValSet
	    |	_NotchSpace Comment
	    |	_NotchSpace UserData
	    ;

Number :	EDIF_TOK_NUMBER _Number PopC
       ;

_Number :
	|	_Number ScaledInt
	|	_Number NumbDisplay
	|	_Number Number
	;

NumbDisplay :	EDIF_TOK_NUMBERDISPLAY _NumbDisplay PopC
	    ;

_NumbDisplay :	ScaledInt
	     |	_NumbDisplay Display
	     ;

NumberDefn :	EDIF_TOK_NUMBERDEFINITION _NumberDefn PopC
	   ;

_NumberDefn :
	    |	_NumberDefn Scale
	    |	_NumberDefn GridMap
	    |	_NumberDefn Comment
	    ;

OffPageConn :	EDIF_TOK_OFFPAGECONNECTOR _OffPageConn PopC
	    ;

_OffPageConn :	PortNameDef
	     |	_OffPageConn Unused
	     |	_OffPageConn Property
	     |	_OffPageConn Comment
	     |	_OffPageConn UserData
	     ;

OffsetEvent :	EDIF_TOK_OFFSETEVENT Event ScaledInt PopC
	    ;

OpenShape :	EDIF_TOK_OPENSHAPE _OpenShape PopC
	  ;

_OpenShape :	Curve
	   |	_OpenShape Property
	   ;

Orientation :	EDIF_TOK_ORIENTATION _Orientation PopC
	    ;

_Orientation :	EDIF_TOK_R0
	     |	EDIF_TOK_R90
	     |	EDIF_TOK_R180
	     |	EDIF_TOK_R270
	     |	EDIF_TOK_MX
	     |	EDIF_TOK_MY
	     |	EDIF_TOK_MYR90
	     |	EDIF_TOK_MXR90
	     ;

Origin :	EDIF_TOK_ORIGIN PointValue PopC
       ;

OverhngDist :	EDIF_TOK_OVERHANGDISTANCE RuleNameDef FigGrpObj FigGrpObj _OverhngDist
		PopC
	    ;

_OverhngDist :	Range
	     |	SingleValSet
	     |	_OverhngDist Comment
	     |	_OverhngDist UserData
	     ;

OverlapDist :	EDIF_TOK_OVERLAPDISTANCE RuleNameDef FigGrpObj FigGrpObj _OverlapDist
		PopC
	    ;

_OverlapDist :	Range
	     |	SingleValSet
	     |	_OverlapDist Comment
	     |	_OverlapDist UserData
	     ;

Oversize :	EDIF_TOK_OVERSIZE Int _Oversize CornerType PopC { free($2); }
	 ;

_Oversize :	FigGrpRef
	  |	FigureOp
	  ;

Owner :		EDIF_TOK_OWNER Str PopC { free($2); }
      ;

Page :		EDIF_TOK_PAGE _Page PopC
     ;

_Page :		InstNameDef
      |		_Page Instance
      |		_Page Net
      |		_Page NetBundle
      |		_Page CommGraph
      |		_Page PortImpl
      |		_Page PageSize
      |		_Page BoundBox
      |		_Page Comment
      |		_Page UserData
      ;

PageSize :	EDIF_TOK_PAGESIZE Rectangle PopC
	 ;

ParamDisp :	EDIF_TOK_PARAMETERDISPLAY _ParamDisp PopC
	  ;

_ParamDisp :	ValueNameRef
	   |	_ParamDisp Display
	   ;

Parameter :	EDIF_TOK_PARAMETER ValueNameDef TypedValue _Parameter PopC
	  ;

_Parameter :
	   |	Unit
	   ;

ParamAssign :	EDIF_TOK_PARAMETERASSIGN ValueNameRef TypedValue PopC
	    ;

Path :		EDIF_TOK_PATH _Path PopC
     ;

_Path :		PointList
      |		_Path Property
      ;

PathDelay :	EDIF_TOK_PATHDELAY _PathDelay PopC
	  ;

_PathDelay :	Delay
	   |	_PathDelay Event
	   ;

PathWidth :	EDIF_TOK_PATHWIDTH Int PopC { free($2); }
	  ;

Permutable :	EDIF_TOK_PERMUTABLE _Permutable PopC
	   ;

_Permutable :
	    |	_Permutable PortRef { str_pair_free($2); }
	    |	_Permutable Permutable
	    |	_Permutable NonPermut
	    ;

Plug :		EDIF_TOK_PLUG _Plug PopC
     ;

_Plug :
      |		_Plug SocketSet
      ;

Point :		EDIF_TOK_POINT _Point PopC
      ;

_Point :
       |	_Point PointValue
       |	_Point PointDisp
       |	_Point Point
       ;

PointDisp :	EDIF_TOK_POINTDISPLAY _PointDisp PopC
	  ;

_PointDisp :	PointValue
	   |	_PointDisp Display
	   ;

PointList :	EDIF_TOK_POINTLIST _PointList PopC
	  ;

_PointList :
	   |	_PointList PointValue
	   ;

PointValue : 	EDIF_TOK_PT Int Int PopC { free($2); free($3); }
	   ;

Polygon :	EDIF_TOK_POLYGON _Polygon PopC
	;

_Polygon :	PointList
	 |	_Polygon Property
	 ;

Port :		EDIF_TOK_PORT _Port PopC
     ;

_Port :		PortNameDef
      |		_Port Direction
      |		_Port Unused
      |		_Port PortDelay
      |		_Port Designator
      |		_Port DcFanInLoad
      |		_Port DcFanOutLoad
      |		_Port DcMaxFanIn
      |		_Port DcMaxFanOut
      |		_Port AcLoad
      |		_Port Property
      |		_Port Comment
      |		_Port UserData
      ;

PortBackAn :	EDIF_TOK_PORTBACKANNOTATE _PortBackAn PopC
	   ;

_PortBackAn :	PortRef { str_pair_free($1); }
	    |	_PortBackAn Designator
	    |	_PortBackAn PortDelay
	    |	_PortBackAn DcFanInLoad
	    |	_PortBackAn DcFanOutLoad
	    |	_PortBackAn DcMaxFanIn
	    |	_PortBackAn DcMaxFanOut
	    |	_PortBackAn AcLoad
	    |	_PortBackAn Property
	    |	_PortBackAn Comment
	    ;

PortBundle :	EDIF_TOK_PORTBUNDLE PortNameDef _PortBundle PopC
	   ;

_PortBundle :	ListOfPorts
	    |	_PortBundle Property
	    |	_PortBundle Comment
	    |	_PortBundle UserData
	    ;

PortDelay :	EDIF_TOK_PORTDELAY Derivation _PortDelay PopC
	  ;

_PortDelay :	Delay
	   |	LoadDelay
	   |	_PortDelay Transition
	   |	_PortDelay Becomes
	   ;

PortGroup :	EDIF_TOK_PORTGROUP _PortGroup PopC
	  ;

_PortGroup :
	   |	_PortGroup PortNameRef
	   |	_PortGroup PortRef { str_pair_free($2); }
	   ;

PortImpl :	EDIF_TOK_PORTIMPLEMENTATION _PortImpl PopC
	 ;

_PortImpl :	PortRef { str_pair_free($1); }
	  |	PortNameRef
	  |	_PortImpl ConnectLoc
	  |	_PortImpl Figure
	  |	_PortImpl Instance
	  |	_PortImpl CommGraph
	  |	_PortImpl PropDisplay
	  |	_PortImpl KeywordDisp
	  |	_PortImpl Property
	  |	_PortImpl UserData
	  |	_PortImpl Comment
	  ;

PortInst :	EDIF_TOK_PORTINSTANCE _PortInst PopC
	 ;

_PortInst :	PortRef { str_pair_free($1); }
	  |	PortNameRef
	  |	_PortInst Unused
	  |	_PortInst PortDelay
	  |	_PortInst Designator
	  |	_PortInst DcFanInLoad
	  |	_PortInst DcFanOutLoad
	  |	_PortInst DcMaxFanIn
	  |	_PortInst DcMaxFanOut
	  |	_PortInst AcLoad
	  |	_PortInst Property
	  |	_PortInst Comment
	  |	_PortInst UserData
	  ;

PortList :	EDIF_TOK_PORTLIST _PortList PopC
	 ;

_PortList :
	  |	_PortList PortRef { str_pair_free($2); }
	  |	_PortList PortNameRef
	  ;

PortListAls :	EDIF_TOK_PORTLISTALIAS PortNameDef PortList PopC
	    ;

PortMap :	EDIF_TOK_PORTMAP _PortMap PopC
	;

_PortMap :
	 |	_PortMap PortRef { str_pair_free($2); }
	 |	_PortMap PortGroup
	 |	_PortMap Comment
	 |	_PortMap UserData
	 ;

PortNameDef :	NameDef { str_pair_free($1); }
	    |	Array
	    ;

PortNameRef :	NameRef { $$=$1; }
	    |	Member
	    ;

PortRef :	EDIF_TOK_PORTREF PortNameRef _PortRef PopC 
{ 
    if ($3)
    {
	$$ = new_str_pair($3->str1,$2);
	free($3);
    }
    else 
    {
	/* handle port with no instance by passing up the chain */
	$$ = new_str_pair(NULL,$2);
    }
}
	;

_PortRef : { $$=NULL; }
	 |	PortRef { $$=$1; }
	 |	InstanceRef { $$ = new_str_pair($1,NULL); }
	 |	ViewRef { $$=NULL; }
	 ;

Program :	EDIF_TOK_PROGRAM Str _Program PopC
	;

_Program :
	 |	Version
	 ;

PropDisplay :	EDIF_TOK_PROPERTYDISPLAY _PropDisplay PopC
	    ;

_PropDisplay :	PropNameRef
	     |	_PropDisplay Display
	     ;

Property :	EDIF_TOK_PROPERTY PropNameDef _Property PopC
	 ;

_Property :	TypedValue
	  |	_Property Owner
	  |	_Property Unit
	  |	_Property Property
	  |	_Property Comment
	  ;

PropNameDef :	NameDef { str_pair_free($1); }
	    ;

PropNameRef :	NameRef { free($1); }
	    ;

ProtectFrame :	EDIF_TOK_PROTECTIONFRAME _ProtectFrame PopC
	     ;

_ProtectFrame :
	      |	_ProtectFrame PortImpl
	      |	_ProtectFrame Figure
	      |	_ProtectFrame Instance
	      |	_ProtectFrame CommGraph
	      |	_ProtectFrame BoundBox
	      |	_ProtectFrame PropDisplay
	      |	_ProtectFrame KeywordDisp
	      |	_ProtectFrame ParamDisp
	      |	_ProtectFrame Property
	      |	_ProtectFrame Comment
	      |	_ProtectFrame UserData
	      ;

Range :		LessThan
      |		GreaterThan
      |		AtMost
      |		AtLeast
      |		Exactly
      |		Between
      ;

RangeVector :	EDIF_TOK_RANGEVECTOR _RangeVector PopC
	    ;

_RangeVector :
	     |	_RangeVector Range
	     |	_RangeVector SingleValSet
	     ;

Rectangle :	EDIF_TOK_RECTANGLE PointValue _Rectangle PopC
	  ;

_Rectangle :	PointValue
	   |	_Rectangle Property
	   ;

RectSize :	EDIF_TOK_RECTANGLESIZE RuleNameDef FigGrpObj _RectSize PopC
	 ;

_RectSize :	RangeVector
	  |	MultValSet
	  |	_RectSize Comment
	  |	_RectSize UserData
	  ;

Rename :	EDIF_TOK_RENAME __Rename _Rename PopC
{ $$ = new_str_pair($2,$3); }
       ;

__Rename :	Ident { $$=$1; }
	 |	Name { $$=$1; }
	 ;

_Rename :	Str { $$=$1; }
	|	StrDisplay { $$=NULL; }
	;

Resolves :	EDIF_TOK_RESOLVES _Resolves PopC
	 ;

_Resolves :
	  |	_Resolves LogicNameRef
	  ;

RuleNameDef :	NameDef { str_pair_free($1); }
	    ;

Scale :		EDIF_TOK_SCALE ScaledInt ScaledInt Unit PopC
      ;

ScaledInt :	Int { free($1); }
	  |	EDIF_TOK_E Int Int PopC { free($2); free($3); }
	  ;

ScaleX :	EDIF_TOK_SCALEX Int Int PopC { free($2); free($3); }
       ;

ScaleY :	EDIF_TOK_SCALEY Int Int PopC { free($2); free($3); }
       ;

Section :	EDIF_TOK_SECTION _Section PopC
	;

_Section :	Str { free($1); }
	 |	_Section Section
	 |	_Section Str { free($2); }
	 |	_Section Instance
	 ;

Shape :		EDIF_TOK_SHAPE _Shape PopC
      ;

_Shape :	Curve
       |	_Shape Property
       ;

SimNameDef :	NameDef { str_pair_free($1); }
	   ;

Simulate :	EDIF_TOK_SIMULATE _Simulate PopC
	 ;

_Simulate :	SimNameDef
	  |	_Simulate PortListAls
	  |	_Simulate WaveValue
	  |	_Simulate Apply
	  |	_Simulate Comment
	  |	_Simulate UserData
	  ;

SimulInfo :	EDIF_TOK_SIMULATIONINFO _SimulInfo PopC
	  ;

_SimulInfo :
	   |	_SimulInfo LogicValue
	   |	_SimulInfo Comment
	   |	_SimulInfo UserData
	   ;

SingleValSet :	EDIF_TOK_SINGLEVALUESET _SingleValSet PopC
	     ;

_SingleValSet :
	      |	Range
	      ;

Site :		EDIF_TOK_SITE ViewRef _Site PopC
     ;

_Site :
      |		Transform
      ;

Socket :	EDIF_TOK_SOCKET _Socket PopC
       ;

_Socket :
	|	Symmetry
	;

SocketSet :	EDIF_TOK_SOCKETSET _SocketSet PopC
	  ;

_SocketSet :	Symmetry
	   |	_SocketSet Site
	   ;

Status :	EDIF_TOK_STATUS _Status PopC
       ;

_Status :
	|	_Status Written
	|	_Status Comment
	|	_Status UserData
	;

Steady :	EDIF_TOK_STEADY __Steady _Steady PopC
       ;

__Steady :	PortNameRef
	 |	PortRef { str_pair_free($1); }
	 |	PortList
	 ;

_Steady :	Duration
	|	_Steady Transition
	|	_Steady Becomes
	;

StrDisplay :	EDIF_TOK_STRINGDISPLAY _StrDisplay PopC
	   ;

String :	EDIF_TOK_STRING _String PopC
       ;

_String :
	|	_String Str { free($2); }
	|	_String StrDisplay
	|	_String String
	;

_StrDisplay :	Str { free($1); }
	    |	_StrDisplay Display
	    ;

Strong :	EDIF_TOK_STRONG LogicNameRef PopC
       ;

Symbol :	EDIF_TOK_SYMBOL _Symbol PopC
       ;

_Symbol :
	|	_Symbol PortImpl
	|	_Symbol Figure
	|	_Symbol Instance
	|	_Symbol CommGraph
	|	_Symbol Annotate
	|	_Symbol PageSize
	|	_Symbol BoundBox
	|	_Symbol PropDisplay
	|	_Symbol KeywordDisp
	|	_Symbol ParamDisp
	|	_Symbol Property
	|	_Symbol Comment
	|	_Symbol UserData
	;

Symmetry :	EDIF_TOK_SYMMETRY _Symmetry PopC
	 ;

_Symmetry :
	  |	_Symmetry Transform
	  ;

Table :		EDIF_TOK_TABLE _Table PopC
      ;

_Table :
       |	_Table Entry
       |	_Table TableDeflt
       ;

TableDeflt :	EDIF_TOK_TABLEDEFAULT __TableDeflt _TableDeflt PopC
	   ;

__TableDeflt :	LogicRef
	     |	PortRef { str_pair_free($1); }
	     |	NoChange
	     |	Table
	     ;

_TableDeflt :
	    |	Delay
	    |	LoadDelay
	    ;

Technology :	EDIF_TOK_TECHNOLOGY _Technology PopC
	   ;

_Technology :	NumberDefn
	    |	_Technology FigGrp
	    |	_Technology Fabricate
	    |	_Technology SimulInfo
	    |	_Technology DesignRule
	    |	_Technology Comment
	    |	_Technology UserData
	    ;

TextHeight :	EDIF_TOK_TEXTHEIGHT Int PopC { free($2); }
	   ;

TimeIntval :	EDIF_TOK_TIMEINTERVAL __TimeIntval _TimeIntval PopC
	   ;

__TimeIntval :	Event
	     |	OffsetEvent
	     ;

_TimeIntval :	Event
	    |	OffsetEvent
	    |	Duration
	    ;

TimeStamp :	EDIF_TOK_TIMESTAMP Int Int Int Int Int Int PopC
{ free($2); free($3); free($4); free($5); free($6); free($7); }
	  ;

Timing :	EDIF_TOK_TIMING _Timing PopC
       ;

_Timing :	Derivation
	|	_Timing PathDelay
	|	_Timing Forbidden
	|	_Timing Comment
	|	_Timing UserData
	;

Transform :	EDIF_TOK_TRANSFORM _TransX _TransY _TransDelta _TransOrien _TransOrg
		PopC
	  ;

_TransX :
	|	ScaleX
	;

_TransY :
	|	ScaleY
	;

_TransDelta :
	    |	Delta
	    ;

_TransOrien :
	    |	Orientation
	    ;

_TransOrg :
	  |	Origin
	  ;

Transition :	EDIF_TOK_TRANSITION _Transition _Transition PopC
	   ;

_Transition :	LogicNameRef
	    |	LogicList
	    |	LogicOneOf
	    ;

Trigger :	EDIF_TOK_TRIGGER _Trigger PopC
	;

_Trigger :
	 |	_Trigger Change
	 |	_Trigger Steady
	 |	_Trigger Initial
	 ;

True :		EDIF_TOK_TRUE PopC
     ;

TypedValue :	Boolean
	   |	Integer
	   |	MiNoMa
	   |	Number
	   |	Point
	   |	String
	   ;

Unconstrained :	EDIF_TOK_UNCONSTRAINED PopC
	      ;

Undefined :	EDIF_TOK_UNDEFINED PopC
	  ;

Union :		EDIF_TOK_UNION _Union PopC
      ;

_Union :	FigGrpRef
       |	FigureOp
       |	_Union FigGrpRef
       |	_Union FigureOp
       ;

Unit :		EDIF_TOK_UNIT _Unit PopC
     ;

_Unit :		EDIF_TOK_DISTANCE
      |		EDIF_TOK_CAPACITANCE
      |		EDIF_TOK_CURRENT
      |		EDIF_TOK_RESISTANCE
      |		EDIF_TOK_TEMPERATURE
      |		EDIF_TOK_TIME
      |		EDIF_TOK_VOLTAGE
      |		EDIF_TOK_MASS
      |		EDIF_TOK_FREQUENCY
      |		EDIF_TOK_INDUCTANCE
      |		EDIF_TOK_ENERGY
      |		EDIF_TOK_POWER
      |		EDIF_TOK_CHARGE
      |		EDIF_TOK_CONDUCTANCE
      |		EDIF_TOK_FLUX
      |		EDIF_TOK_ANGLE
      ;

Unused :	EDIF_TOK_UNUSED PopC
       ;

UserData :	EDIF_TOK_USERDATA _UserData PopC
	 ;

_UserData :	Ident { free($1); }
	  |	_UserData Int { free($2); }
	  |	_UserData Str { free($2); }
	  |	_UserData Ident { free($2); }
	  |	_UserData Form
	  ;

ValueNameDef :	NameDef { str_pair_free($1); }
	     |	Array
	     ;

ValueNameRef :	NameRef { free($1); }
	     |	Member
	     ;

Version :	EDIF_TOK_VERSION Str PopC { free($2); }
	;

View :		EDIF_TOK_VIEW ViewNameDef ViewType _View PopC
     ;

_View :		Interface
      |		_View Status
      |		_View Contents
      |		_View Comment
      |		_View Property
      |		_View UserData
      ;

ViewList :	EDIF_TOK_VIEWLIST _ViewList PopC
	 ;

_ViewList :
	  |	_ViewList ViewRef
	  |	_ViewList ViewList
	  ;

ViewMap :	EDIF_TOK_VIEWMAP _ViewMap PopC
	;

_ViewMap :
	 |	_ViewMap PortMap
	 |	_ViewMap PortBackAn
	 |	_ViewMap InstMap
	 |	_ViewMap InstBackAn
	 |	_ViewMap NetMap
	 |	_ViewMap NetBackAn
	 |	_ViewMap Comment
	 |	_ViewMap UserData
	 ;

ViewNameDef :	NameDef { str_pair_free($1); }
	    ;

ViewNameRef :	NameRef { free($1); }
	    ;

ViewRef :	EDIF_TOK_VIEWREF ViewNameRef _ViewRef PopC
	;

_ViewRef :
	 |	CellRef
	 ;

ViewType :	EDIF_TOK_VIEWTYPE _ViewType PopC
	 ;

_ViewType :	EDIF_TOK_MASKLAYOUT
	  |	EDIF_TOK_PCBLAYOUT
	  |	EDIF_TOK_NETLIST
	  |	EDIF_TOK_SCHEMATIC
	  |	EDIF_TOK_SYMBOLIC
	  |	EDIF_TOK_BEHAVIOR
	  |	EDIF_TOK_LOGICMODEL
	  |	EDIF_TOK_DOCUMENT
	  |	EDIF_TOK_GRAPHIC
	  |	EDIF_TOK_STRANGER
	  ;

Visible :	EDIF_TOK_VISIBLE BooleanValue PopC
	;

VoltageMap :	EDIF_TOK_VOLTAGEMAP MiNoMaValue PopC
	   ;

WaveValue :	EDIF_TOK_WAVEVALUE LogicNameDef ScaledInt LogicWave PopC
	  ;

Weak :		EDIF_TOK_WEAK LogicNameRef PopC
     ;

WeakJoined :	EDIF_TOK_WEAKJOINED _WeakJoined PopC
	   ;

_WeakJoined :
	    |	_WeakJoined PortRef { str_pair_free($2); }
	    |	_WeakJoined PortList
	    |	_WeakJoined Joined { pair_list_free($2); }
	    ;

When :		EDIF_TOK_WHEN _When PopC
     ;

_When :		Trigger
      |		_When After
      |		_When Follow
      |		_When Maintain
      |		_When LogicAssn
      |		_When Comment
      |		_When UserData
      ;

Written :	EDIF_TOK_WRITTEN _Written PopC
	;

_Written :	TimeStamp
	 |	_Written Author
	 |	_Written Program
	 |	_Written DataOrigin
	 |	_Written Property
	 |	_Written Comment
	 |	_Written UserData
	 ;

Ident :		EDIF_TOK_IDENT { $$=$1; }
      ;

Str :		EDIF_TOK_STR { $$=$1; }
    ;

Int :		EDIF_TOK_INT { $$=$1; }
    ;

Keyword :	EDIF_TOK_KEYWORD	{ $$=$1; }
	;

%%
/*
 *	xmalloc:
 *
 *	  Garbage function for 'alloca()'.
 */
char *xmalloc(int siz)
{
  return ((char *)Malloc(siz));
}
/*
 *	Token & context carriers:
 *
 *	  These are the linkage pointers for threading this context garbage
 *	for converting identifiers into parser tokens.
 */
typedef struct TokenCar {
  struct TokenCar *Next;	/* pointer to next carrier */
  struct Token *Token;		/* associated token */
} TokenCar;
typedef struct UsedCar {
  struct UsedCar *Next;		/* pointer to next carrier */
  short Code;			/* used '%token' value */
} UsedCar;
typedef struct ContextCar {
  struct ContextCar *Next;	/* pointer to next carrier */
  struct Context *Context;	/* associated context */
  union {
    int Single;			/* single usage flag (context tree) */
    struct UsedCar *Used;	/* single used list (context stack) */
  } u;
} ContextCar;
/*
 *	Token definitions:
 *
 *	  This associates the '%token' codings with strings which are to
 *	be free standing tokens. Doesn't have to be in sorted order but the
 *	strings must be in lower case.
 */
typedef struct Token {
  char *Name;			/* token name */
  int Code;			/* '%token' value */
  struct Token *Next;		/* hash table linkage */
} Token;
static Token TokenDef[] = {
  {"angle",		EDIF_TOK_ANGLE},
  {"behavior",		EDIF_TOK_BEHAVIOR},
  {"calculated",	EDIF_TOK_CALCULATED},
  {"capacitance",	EDIF_TOK_CAPACITANCE},
  {"centercenter",	EDIF_TOK_CENTERCENTER},
  {"centerleft",	EDIF_TOK_CENTERLEFT},
  {"centerright",	EDIF_TOK_CENTERRIGHT},
  {"charge",		EDIF_TOK_CHARGE},
  {"conductance",	EDIF_TOK_CONDUCTANCE},
  {"current",		EDIF_TOK_CURRENT},
  {"distance",		EDIF_TOK_DISTANCE},
  {"document",		EDIF_TOK_DOCUMENT},
  {"energy",		EDIF_TOK_ENERGY},
  {"extend",		EDIF_TOK_EXTEND},
  {"flux",		EDIF_TOK_FLUX},
  {"frequency",		EDIF_TOK_FREQUENCY},
  {"generic",		EDIF_TOK_GENERIC},
  {"graphic",		EDIF_TOK_GRAPHIC},
  {"inductance",	EDIF_TOK_INDUCTANCE},
  {"inout",		EDIF_TOK_INOUT},
  {"input",		EDIF_TOK_INPUT},
  {"logicmodel",	EDIF_TOK_LOGICMODEL},
  {"lowercenter",	EDIF_TOK_LOWERCENTER},
  {"lowerleft",		EDIF_TOK_LOWERLEFT},
  {"lowerright",	EDIF_TOK_LOWERRIGHT},
  {"masklayout",	EDIF_TOK_MASKLAYOUT},
  {"mass",		EDIF_TOK_MASS},
  {"measured",		EDIF_TOK_MEASURED},
  {"mx",		EDIF_TOK_MX},
  {"mxr90",		EDIF_TOK_MXR90},
  {"my",		EDIF_TOK_MY},
  {"myr90",		EDIF_TOK_MYR90},
  {"netlist",		EDIF_TOK_NETLIST},
  {"output",		EDIF_TOK_OUTPUT},
  {"pcblayout",		EDIF_TOK_PCBLAYOUT},
  {"power",		EDIF_TOK_POWER},
  {"r0",		EDIF_TOK_R0},
  {"r180",		EDIF_TOK_R180},
  {"r270",		EDIF_TOK_R270},
  {"r90",		EDIF_TOK_R90},
  {"required",		EDIF_TOK_REQUIRED},
  {"resistance",	EDIF_TOK_RESISTANCE},
  {"ripper",		EDIF_TOK_RIPPER},
  {"round",		EDIF_TOK_ROUND},
  {"schematic",		EDIF_TOK_SCHEMATIC},
  {"stranger",		EDIF_TOK_STRANGER},
  {"symbolic",		EDIF_TOK_SYMBOLIC},
  {"temperature",	EDIF_TOK_TEMPERATURE},
  {"tie",		EDIF_TOK_TIE},
  {"time",		EDIF_TOK_TIME},
  {"truncate",		EDIF_TOK_TRUNCATE},
  {"uppercenter",	EDIF_TOK_UPPERCENTER},
  {"upperleft",		EDIF_TOK_UPPERLEFT},
  {"upperright",	EDIF_TOK_UPPERRIGHT},
  {"voltage",		EDIF_TOK_VOLTAGE}
};
static int TokenDefSize = sizeof(TokenDef) / sizeof(Token);
/*
 *	Token enable definitions:
 *
 *	  There is one array for each set of tokens enabled by a
 *	particular context (barf). Another array is used to bind
 *	these arrays to a context.
 */
static short e_CellType[] = {EDIF_TOK_TIE, EDIF_TOK_RIPPER, EDIF_TOK_GENERIC};
static short e_CornerType[] = {EDIF_TOK_EXTEND, EDIF_TOK_TRUNCATE,
			       EDIF_TOK_ROUND};
static short e_Derivation[] = {EDIF_TOK_CALCULATED, EDIF_TOK_MEASURED,
			       EDIF_TOK_REQUIRED};
static short e_Direction[] = {EDIF_TOK_INPUT, EDIF_TOK_OUTPUT,
			      EDIF_TOK_INOUT};
static short e_EndType[] = {EDIF_TOK_EXTEND, EDIF_TOK_TRUNCATE,
			    EDIF_TOK_ROUND};
static short e_Justify[] = {EDIF_TOK_CENTERCENTER, EDIF_TOK_CENTERLEFT,
			    EDIF_TOK_CENTERRIGHT, EDIF_TOK_LOWERCENTER,
			    EDIF_TOK_LOWERLEFT, EDIF_TOK_LOWERRIGHT,
			    EDIF_TOK_UPPERCENTER, EDIF_TOK_UPPERLEFT,
			    EDIF_TOK_UPPERRIGHT};
static short e_Orientation[] = {EDIF_TOK_R0, EDIF_TOK_R90, EDIF_TOK_R180,
				EDIF_TOK_R270, EDIF_TOK_MX, EDIF_TOK_MY,
				EDIF_TOK_MXR90, EDIF_TOK_MYR90};
static short e_Unit[] = {EDIF_TOK_DISTANCE, EDIF_TOK_CAPACITANCE,
			 EDIF_TOK_CURRENT, EDIF_TOK_RESISTANCE,
			 EDIF_TOK_TEMPERATURE, EDIF_TOK_TIME,
			 EDIF_TOK_VOLTAGE, EDIF_TOK_MASS, EDIF_TOK_FREQUENCY,
			 EDIF_TOK_INDUCTANCE, EDIF_TOK_ENERGY,
			 EDIF_TOK_POWER, EDIF_TOK_CHARGE,
			 EDIF_TOK_CONDUCTANCE, EDIF_TOK_FLUX, EDIF_TOK_ANGLE};
static short e_ViewType[] = {EDIF_TOK_MASKLAYOUT, EDIF_TOK_PCBLAYOUT,
			     EDIF_TOK_NETLIST, EDIF_TOK_SCHEMATIC,
			     EDIF_TOK_SYMBOLIC, EDIF_TOK_BEHAVIOR,
			     EDIF_TOK_LOGICMODEL, EDIF_TOK_DOCUMENT,
			     EDIF_TOK_GRAPHIC, EDIF_TOK_STRANGER};
/*
 *	Token tying table:
 *
 *	  This binds enabled tokens to a context.
 */
typedef struct Tie {
  short *Enable;		/* pointer to enable array */
  short Origin;			/* '%token' value of context */
  short EnableSize;		/* size of enabled array */
} Tie;
#define	TE(e,o)			{e,o,sizeof(e)/sizeof(short)}
static Tie TieDef[] = {
  TE(e_CellType,	EDIF_TOK_CELLTYPE),
  TE(e_CornerType,	EDIF_TOK_CORNERTYPE),
  TE(e_Derivation,	EDIF_TOK_DERIVATION),
  TE(e_Direction,	EDIF_TOK_DIRECTION),
  TE(e_EndType,		EDIF_TOK_ENDTYPE),
  TE(e_Justify,		EDIF_TOK_JUSTIFY),
  TE(e_Orientation,	EDIF_TOK_ORIENTATION),
  TE(e_Unit,		EDIF_TOK_UNIT),
  TE(e_ViewType,	EDIF_TOK_VIEWTYPE)
};
static int TieDefSize = sizeof(TieDef) / sizeof(Tie);
/*
 *	Context definitions:
 *
 *	  This associates keyword strings with '%token' values. It
 *	also creates a pretty much empty header for later building of
 *	the context tree. Again they needn't be sorted, but strings
 *	must be lower case.
 */
typedef struct Context {
  char *Name;			/* keyword name */
  short Code;			/* '%token' value */
  short Flags;			/* special operation flags */
  struct ContextCar *Context;	/* contexts which can be moved to */
  struct TokenCar *Token;	/* active tokens */
  struct Context *Next;		/* hash table linkage */
} Context;
static Context ContextDef[] = {
  {"",				0},		/* start context */
  {"acload",			EDIF_TOK_ACLOAD},
  {"after",			EDIF_TOK_AFTER},
  {"annotate",			EDIF_TOK_ANNOTATE},
  {"apply",			EDIF_TOK_APPLY},
  {"arc",			EDIF_TOK_ARC},
  {"array",			EDIF_TOK_ARRAY},
  {"arraymacro",		EDIF_TOK_ARRAYMACRO},
  {"arrayrelatedinfo",		EDIF_TOK_ARRAYRELATEDINFO},
  {"arraysite",			EDIF_TOK_ARRAYSITE},
  {"atleast",			EDIF_TOK_ATLEAST},
  {"atmost",			EDIF_TOK_ATMOST},
  {"author",			EDIF_TOK_AUTHOR},
  {"basearray",			EDIF_TOK_BASEARRAY},
  {"becomes",			EDIF_TOK_BECOMES},
  {"between",			EDIF_TOK_BETWEEN},
  {"boolean",			EDIF_TOK_BOOLEAN},
  {"booleandisplay",		EDIF_TOK_BOOLEANDISPLAY},
  {"booleanmap",		EDIF_TOK_BOOLEANMAP},
  {"borderpattern",		EDIF_TOK_BORDERPATTERN},
  {"borderwidth",		EDIF_TOK_BORDERWIDTH},
  {"boundingbox",		EDIF_TOK_BOUNDINGBOX},
  {"cell",			EDIF_TOK_CELL},
  {"cellref",			EDIF_TOK_CELLREF},
  {"celltype",			EDIF_TOK_CELLTYPE},
  {"change",			EDIF_TOK_CHANGE},
  {"circle",			EDIF_TOK_CIRCLE},
  {"color",			EDIF_TOK_COLOR},
  {"comment",			EDIF_TOK_COMMENT},
  {"commentgraphics",		EDIF_TOK_COMMENTGRAPHICS},
  {"compound",			EDIF_TOK_COMPOUND},
  {"connectlocation",		EDIF_TOK_CONNECTLOCATION},
  {"contents",			EDIF_TOK_CONTENTS},
  {"cornertype",		EDIF_TOK_CORNERTYPE},
  {"criticality",		EDIF_TOK_CRITICALITY},
  {"currentmap",		EDIF_TOK_CURRENTMAP},
  {"curve",			EDIF_TOK_CURVE},
  {"cycle",			EDIF_TOK_CYCLE},
  {"dataorigin",		EDIF_TOK_DATAORIGIN},
  {"dcfaninload",		EDIF_TOK_DCFANINLOAD},
  {"dcfanoutload",		EDIF_TOK_DCFANOUTLOAD},
  {"dcmaxfanin",		EDIF_TOK_DCMAXFANIN},
  {"dcmaxfanout",		EDIF_TOK_DCMAXFANOUT},
  {"delay",			EDIF_TOK_DELAY},
  {"delta",			EDIF_TOK_DELTA},
  {"derivation",		EDIF_TOK_DERIVATION},
  {"design",			EDIF_TOK_DESIGN},
  {"designator",		EDIF_TOK_DESIGNATOR},
  {"difference",		EDIF_TOK_DIFFERENCE},
  {"direction",			EDIF_TOK_DIRECTION},
  {"display",			EDIF_TOK_DISPLAY},
  {"dominates",			EDIF_TOK_DOMINATES},
  {"dot",			EDIF_TOK_DOT},
  {"duration",			EDIF_TOK_DURATION},
  {"e",				EDIF_TOK_E},
  {"edif",			EDIF_TOK_EDIF},
  {"ediflevel",			EDIF_TOK_EDIFLEVEL},
  {"edifversion",		EDIF_TOK_EDIFVERSION},
  {"enclosuredistance",		EDIF_TOK_ENCLOSUREDISTANCE},
  {"endtype",			EDIF_TOK_ENDTYPE},
  {"entry",			EDIF_TOK_ENTRY},
  {"exactly",			EDIF_TOK_EXACTLY},
  {"external",			EDIF_TOK_EXTERNAL},
  {"fabricate",			EDIF_TOK_FABRICATE},
  {"false",			EDIF_TOK_FALSE},
  {"figure",			EDIF_TOK_FIGURE},
  {"figurearea",		EDIF_TOK_FIGUREAREA},
  {"figuregroup",		EDIF_TOK_FIGUREGROUP},
  {"figuregroupobject",		EDIF_TOK_FIGUREGROUPOBJECT},
  {"figuregroupoverride",	EDIF_TOK_FIGUREGROUPOVERRIDE},
  {"figuregroupref",		EDIF_TOK_FIGUREGROUPREF},
  {"figureperimeter",		EDIF_TOK_FIGUREPERIMETER},
  {"figurewidth",		EDIF_TOK_FIGUREWIDTH},
  {"fillpattern",		EDIF_TOK_FILLPATTERN},
  {"follow",			EDIF_TOK_FOLLOW},
  {"forbiddenevent",		EDIF_TOK_FORBIDDENEVENT},
  {"globalportref",		EDIF_TOK_GLOBALPORTREF},
  {"greaterthan",		EDIF_TOK_GREATERTHAN},
  {"gridmap",			EDIF_TOK_GRIDMAP},
  {"ignore",			EDIF_TOK_IGNORE},
  {"includefiguregroup",	EDIF_TOK_INCLUDEFIGUREGROUP},
  {"initial",			EDIF_TOK_INITIAL},
  {"instance",			EDIF_TOK_INSTANCE},
  {"instancebackannotate",	EDIF_TOK_INSTANCEBACKANNOTATE},
  {"instancegroup",		EDIF_TOK_INSTANCEGROUP},
  {"instancemap",		EDIF_TOK_INSTANCEMAP},
  {"instanceref",		EDIF_TOK_INSTANCEREF},
  {"integer",			EDIF_TOK_INTEGER},
  {"integerdisplay",		EDIF_TOK_INTEGERDISPLAY},
  {"interface",			EDIF_TOK_INTERFACE},
  {"interfiguregroupspacing",	EDIF_TOK_INTERFIGUREGROUPSPACING},
  {"intersection",		EDIF_TOK_INTERSECTION},
  {"intrafiguregroupspacing",	EDIF_TOK_INTRAFIGUREGROUPSPACING},
  {"inverse",			EDIF_TOK_INVERSE},
  {"isolated",			EDIF_TOK_ISOLATED},
  {"joined",			EDIF_TOK_JOINED},
  {"justify",			EDIF_TOK_JUSTIFY},
  {"keyworddisplay",		EDIF_TOK_KEYWORDDISPLAY},
  {"keywordlevel",		EDIF_TOK_KEYWORDLEVEL},
  {"keywordmap",		EDIF_TOK_KEYWORDMAP},
  {"lessthan",			EDIF_TOK_LESSTHAN},
  {"library",			EDIF_TOK_LIBRARY},
  {"libraryref",		EDIF_TOK_LIBRARYREF},
  {"listofnets",		EDIF_TOK_LISTOFNETS},
  {"listofports",		EDIF_TOK_LISTOFPORTS},
  {"loaddelay",			EDIF_TOK_LOADDELAY},
  {"logicassign",		EDIF_TOK_LOGICASSIGN},
  {"logicinput",		EDIF_TOK_LOGICINPUT},
  {"logiclist",			EDIF_TOK_LOGICLIST},
  {"logicmapinput",		EDIF_TOK_LOGICMAPINPUT},
  {"logicmapoutput",		EDIF_TOK_LOGICMAPOUTPUT},
  {"logiconeof",		EDIF_TOK_LOGICONEOF},
  {"logicoutput",		EDIF_TOK_LOGICOUTPUT},
  {"logicport",			EDIF_TOK_LOGICPORT},
  {"logicref",			EDIF_TOK_LOGICREF},
  {"logicvalue",		EDIF_TOK_LOGICVALUE},
  {"logicwaveform",		EDIF_TOK_LOGICWAVEFORM},
  {"maintain",			EDIF_TOK_MAINTAIN},
  {"match",			EDIF_TOK_MATCH},
  {"member",			EDIF_TOK_MEMBER},
  {"minomax",			EDIF_TOK_MINOMAX},
  {"minomaxdisplay",		EDIF_TOK_MINOMAXDISPLAY},
  {"mnm",			EDIF_TOK_MNM},
  {"multiplevalueset",		EDIF_TOK_MULTIPLEVALUESET},
  {"mustjoin",			EDIF_TOK_MUSTJOIN},
  {"name",			EDIF_TOK_NAME},
  {"net",			EDIF_TOK_NET},
  {"netbackannotate",		EDIF_TOK_NETBACKANNOTATE},
  {"netbundle",			EDIF_TOK_NETBUNDLE},
  {"netdelay",			EDIF_TOK_NETDELAY},
  {"netgroup",			EDIF_TOK_NETGROUP},
  {"netmap",			EDIF_TOK_NETMAP},
  {"netref",			EDIF_TOK_NETREF},
  {"nochange",			EDIF_TOK_NOCHANGE},
  {"nonpermutable",		EDIF_TOK_NONPERMUTABLE},
  {"notallowed",		EDIF_TOK_NOTALLOWED},
  {"notchspacing",		EDIF_TOK_NOTCHSPACING},
  {"number",			EDIF_TOK_NUMBER},
  {"numberdefinition",		EDIF_TOK_NUMBERDEFINITION},
  {"numberdisplay",		EDIF_TOK_NUMBERDISPLAY},
  {"offpageconnector",		EDIF_TOK_OFFPAGECONNECTOR},
  {"offsetevent",		EDIF_TOK_OFFSETEVENT},
  {"openshape",			EDIF_TOK_OPENSHAPE},
  {"orientation",		EDIF_TOK_ORIENTATION},
  {"origin",			EDIF_TOK_ORIGIN},
  {"overhangdistance",		EDIF_TOK_OVERHANGDISTANCE},
  {"overlapdistance",		EDIF_TOK_OVERLAPDISTANCE},
  {"oversize",			EDIF_TOK_OVERSIZE},
  {"owner",			EDIF_TOK_OWNER},
  {"page",			EDIF_TOK_PAGE},
  {"pagesize",			EDIF_TOK_PAGESIZE},
  {"parameter",			EDIF_TOK_PARAMETER},
  {"parameterassign",		EDIF_TOK_PARAMETERASSIGN},
  {"parameterdisplay",		EDIF_TOK_PARAMETERDISPLAY},
  {"path",			EDIF_TOK_PATH},
  {"pathdelay",			EDIF_TOK_PATHDELAY},
  {"pathwidth",			EDIF_TOK_PATHWIDTH},
  {"permutable",		EDIF_TOK_PERMUTABLE},
  {"physicaldesignrule",	EDIF_TOK_PHYSICALDESIGNRULE},
  {"plug",			EDIF_TOK_PLUG},
  {"point",			EDIF_TOK_POINT},
  {"pointdisplay",		EDIF_TOK_POINTDISPLAY},
  {"pointlist",			EDIF_TOK_POINTLIST},
  {"polygon",			EDIF_TOK_POLYGON},
  {"port",			EDIF_TOK_PORT},
  {"portbackannotate",		EDIF_TOK_PORTBACKANNOTATE},
  {"portbundle",		EDIF_TOK_PORTBUNDLE},
  {"portdelay",			EDIF_TOK_PORTDELAY},
  {"portgroup",			EDIF_TOK_PORTGROUP},
  {"portimplementation",	EDIF_TOK_PORTIMPLEMENTATION},
  {"portinstance",		EDIF_TOK_PORTINSTANCE},
  {"portlist",			EDIF_TOK_PORTLIST},
  {"portlistalias",		EDIF_TOK_PORTLISTALIAS},
  {"portmap",			EDIF_TOK_PORTMAP},
  {"portref",			EDIF_TOK_PORTREF},
  {"program",			EDIF_TOK_PROGRAM},
  {"property",			EDIF_TOK_PROPERTY},
  {"propertydisplay",		EDIF_TOK_PROPERTYDISPLAY},
  {"protectionframe",		EDIF_TOK_PROTECTIONFRAME},
  {"pt",			EDIF_TOK_PT},
  {"rangevector",		EDIF_TOK_RANGEVECTOR},
  {"rectangle",			EDIF_TOK_RECTANGLE},
  {"rectanglesize",		EDIF_TOK_RECTANGLESIZE},
  {"rename",			EDIF_TOK_RENAME},
  {"resolves",			EDIF_TOK_RESOLVES},
  {"scale",			EDIF_TOK_SCALE},
  {"scalex",			EDIF_TOK_SCALEX},
  {"scaley",			EDIF_TOK_SCALEY},
  {"section",			EDIF_TOK_SECTION},
  {"shape",			EDIF_TOK_SHAPE},
  {"simulate",			EDIF_TOK_SIMULATE},
  {"simulationinfo",		EDIF_TOK_SIMULATIONINFO},
  {"singlevalueset",		EDIF_TOK_SINGLEVALUESET},
  {"site",			EDIF_TOK_SITE},
  {"socket",			EDIF_TOK_SOCKET},
  {"socketset",			EDIF_TOK_SOCKETSET},
  {"status",			EDIF_TOK_STATUS},
  {"steady",			EDIF_TOK_STEADY},
  {"string",			EDIF_TOK_STRING},
  {"stringdisplay",		EDIF_TOK_STRINGDISPLAY},
  {"strong",			EDIF_TOK_STRONG},
  {"symbol",			EDIF_TOK_SYMBOL},
  {"symmetry",			EDIF_TOK_SYMMETRY},
  {"table",			EDIF_TOK_TABLE},
  {"tabledefault",		EDIF_TOK_TABLEDEFAULT},
  {"technology",		EDIF_TOK_TECHNOLOGY},
  {"textheight",		EDIF_TOK_TEXTHEIGHT},
  {"timeinterval",		EDIF_TOK_TIMEINTERVAL},
  {"timestamp",			EDIF_TOK_TIMESTAMP},
  {"timing",			EDIF_TOK_TIMING},
  {"transform",			EDIF_TOK_TRANSFORM},
  {"transition",		EDIF_TOK_TRANSITION},
  {"trigger",			EDIF_TOK_TRIGGER},
  {"true",			EDIF_TOK_TRUE},
  {"unconstrained",		EDIF_TOK_UNCONSTRAINED},
  {"undefined",			EDIF_TOK_UNDEFINED},
  {"union",			EDIF_TOK_UNION},
  {"unit",			EDIF_TOK_UNIT},
  {"unused",			EDIF_TOK_UNUSED},
  {"userdata",			EDIF_TOK_USERDATA},
  {"version",			EDIF_TOK_VERSION},
  {"view",			EDIF_TOK_VIEW},
  {"viewlist",			EDIF_TOK_VIEWLIST},
  {"viewmap",			EDIF_TOK_VIEWMAP},
  {"viewref",			EDIF_TOK_VIEWREF},
  {"viewtype",			EDIF_TOK_VIEWTYPE},
  {"visible",			EDIF_TOK_VISIBLE},
  {"voltagemap",		EDIF_TOK_VOLTAGEMAP},
  {"wavevalue",			EDIF_TOK_WAVEVALUE},
  {"weak",			EDIF_TOK_WEAK},
  {"weakjoined",		EDIF_TOK_WEAKJOINED},
  {"when",			EDIF_TOK_WHEN},
  {"written",			EDIF_TOK_WRITTEN}
};
static int ContextDefSize = sizeof(ContextDef) / sizeof(Context);
/*
 *	Context follower tables:
 *
 *	  This is pretty ugly, an array is defined for each context
 *	which has following context levels. Yet another table is used
 *	to bind these arrays to the originating contexts.
 *	  Arrays are declared as:
 *
 *		static short f_<Context name>[] = { ... };
 *
 *	The array entries are the '%token' values for all keywords which
 *	can be reached from the <Context name> context. Like I said, ugly,
 *	but it works.
 *	  A negative entry means that the follow can only occur once within
 *	the specified context.
 */
static short f_NULL[] = {EDIF_TOK_EDIF};
static short f_Edif[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME, EDIF_TOK_EDIFVERSION,
			 EDIF_TOK_EDIFLEVEL, EDIF_TOK_KEYWORDMAP,
			 -EDIF_TOK_STATUS, EDIF_TOK_EXTERNAL,
			 EDIF_TOK_LIBRARY, EDIF_TOK_DESIGN, EDIF_TOK_COMMENT,
			 EDIF_TOK_USERDATA};
static short f_AcLoad[] = {EDIF_TOK_MNM, EDIF_TOK_E, EDIF_TOK_MINOMAXDISPLAY};
static short f_After[] = {EDIF_TOK_MNM, EDIF_TOK_E, EDIF_TOK_FOLLOW,
			  EDIF_TOK_MAINTAIN, EDIF_TOK_LOGICASSIGN,
			  EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_Annotate[] = {EDIF_TOK_STRINGDISPLAY};
static short f_Apply[] = {EDIF_TOK_CYCLE, EDIF_TOK_LOGICINPUT,
			  EDIF_TOK_LOGICOUTPUT, EDIF_TOK_COMMENT,
			  EDIF_TOK_USERDATA};
static short f_Arc[] = {EDIF_TOK_PT};
static short f_Array[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME};
static short f_ArrayMacro[] = {EDIF_TOK_PLUG};
static short f_ArrayRelatedInfo[] = {EDIF_TOK_BASEARRAY, EDIF_TOK_ARRAYSITE,
				     EDIF_TOK_ARRAYMACRO, EDIF_TOK_COMMENT,
				     EDIF_TOK_USERDATA};
static short f_ArraySite[] = {EDIF_TOK_SOCKET};
static short f_AtLeast[] = {EDIF_TOK_E};
static short f_AtMost[] = {EDIF_TOK_E};
static short f_Becomes[] = {EDIF_TOK_NAME, EDIF_TOK_LOGICLIST,
			    EDIF_TOK_LOGICONEOF};
/*
static short f_Between[] = {EDIF_TOK_ATLEAST, EDIF_TOK_GREATERTHAN,
			    EDIF_TOK_ATMOST, EDIF_TOK_LESSTHAN};
*/
static short f_Boolean[] = {EDIF_TOK_FALSE, EDIF_TOK_TRUE,
			    EDIF_TOK_BOOLEANDISPLAY, EDIF_TOK_BOOLEAN};
static short f_BooleanDisplay[] = {EDIF_TOK_FALSE, EDIF_TOK_TRUE,
				   EDIF_TOK_DISPLAY};
static short f_BooleanMap[] = {EDIF_TOK_FALSE, EDIF_TOK_TRUE};
static short f_BorderPattern[] = {EDIF_TOK_BOOLEAN};
static short f_BoundingBox[] = {EDIF_TOK_RECTANGLE};
static short f_Cell[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME, EDIF_TOK_CELLTYPE,
			 -EDIF_TOK_STATUS, -EDIF_TOK_VIEWMAP, EDIF_TOK_VIEW,
			 EDIF_TOK_COMMENT, EDIF_TOK_USERDATA,
			 EDIF_TOK_PROPERTY};
static short f_CellRef[] = {EDIF_TOK_NAME, EDIF_TOK_LIBRARYREF};
static short f_Change[] = {EDIF_TOK_NAME, EDIF_TOK_PORTREF, EDIF_TOK_PORTLIST,
			   EDIF_TOK_BECOMES, EDIF_TOK_TRANSITION};
static short f_Circle[] = {EDIF_TOK_PT, EDIF_TOK_PROPERTY};
static short f_Color[] = {EDIF_TOK_E};
static short f_CommentGraphics[] = {EDIF_TOK_ANNOTATE, EDIF_TOK_FIGURE,
				    EDIF_TOK_INSTANCE, -EDIF_TOK_BOUNDINGBOX,
				    EDIF_TOK_PROPERTY, EDIF_TOK_COMMENT,
				    EDIF_TOK_USERDATA};
static short f_Compound[] = {EDIF_TOK_NAME};
static short f_ConnectLocation[] = {EDIF_TOK_FIGURE};
static short f_Contents[] = {EDIF_TOK_INSTANCE, EDIF_TOK_OFFPAGECONNECTOR,
			     EDIF_TOK_FIGURE, EDIF_TOK_SECTION, EDIF_TOK_NET,
			     EDIF_TOK_NETBUNDLE, EDIF_TOK_PAGE,
			     EDIF_TOK_COMMENTGRAPHICS,
			     EDIF_TOK_PORTIMPLEMENTATION,
			     EDIF_TOK_TIMING, EDIF_TOK_SIMULATE,
			     EDIF_TOK_WHEN, EDIF_TOK_FOLLOW,
			     EDIF_TOK_LOGICPORT, -EDIF_TOK_BOUNDINGBOX,
			     EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_Criticality[] = {EDIF_TOK_INTEGERDISPLAY};
static short f_CurrentMap[] = {EDIF_TOK_MNM, EDIF_TOK_E};
static short f_Curve[] = {EDIF_TOK_ARC, EDIF_TOK_PT};
static short f_Cycle[] = {EDIF_TOK_DURATION};
static short f_DataOrigin[] = {EDIF_TOK_VERSION};
static short f_DcFanInLoad[] = {EDIF_TOK_E, EDIF_TOK_NUMBERDISPLAY};
static short f_DcFanOutLoad[] = {EDIF_TOK_E, EDIF_TOK_NUMBERDISPLAY};
static short f_DcMaxFanIn[] = {EDIF_TOK_E, EDIF_TOK_NUMBERDISPLAY};
static short f_DcMaxFanOut[] = {EDIF_TOK_E, EDIF_TOK_NUMBERDISPLAY};
static short f_Delay[] = {EDIF_TOK_MNM, EDIF_TOK_E};
static short f_Delta[] = {EDIF_TOK_PT};
static short f_Design[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME, EDIF_TOK_CELLREF,
			   EDIF_TOK_STATUS, EDIF_TOK_COMMENT,
			   EDIF_TOK_PROPERTY, EDIF_TOK_USERDATA};
static short f_Designator[] = {EDIF_TOK_STRINGDISPLAY};
static short f_Difference[] = {EDIF_TOK_FIGUREGROUPREF, EDIF_TOK_INTERSECTION,
			       EDIF_TOK_UNION, EDIF_TOK_DIFFERENCE,
			       EDIF_TOK_INVERSE, EDIF_TOK_OVERSIZE};
static short f_Display[] = {EDIF_TOK_NAME, EDIF_TOK_FIGUREGROUPOVERRIDE,
			    EDIF_TOK_JUSTIFY, EDIF_TOK_ORIENTATION,
			    EDIF_TOK_ORIGIN};
static short f_Dominates[] = {EDIF_TOK_NAME};
static short f_Dot[] = {EDIF_TOK_PT, EDIF_TOK_PROPERTY};
static short f_Duration[] = {EDIF_TOK_E};
static short f_EnclosureDistance[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME,
				      EDIF_TOK_FIGUREGROUPOBJECT,
				      EDIF_TOK_LESSTHAN, EDIF_TOK_GREATERTHAN,
				      EDIF_TOK_ATMOST, EDIF_TOK_ATLEAST,
				      EDIF_TOK_EXACTLY, EDIF_TOK_BETWEEN,
				      EDIF_TOK_SINGLEVALUESET,
				      EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_Entry[] = {EDIF_TOK_MATCH, EDIF_TOK_CHANGE, EDIF_TOK_STEADY,
			  EDIF_TOK_LOGICREF, EDIF_TOK_PORTREF,
			  EDIF_TOK_NOCHANGE, EDIF_TOK_TABLE,
			  EDIF_TOK_DELAY, EDIF_TOK_LOADDELAY};
static short f_Exactly[] = {EDIF_TOK_E};
static short f_External[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME,
			     EDIF_TOK_EDIFLEVEL, EDIF_TOK_TECHNOLOGY,
			     -EDIF_TOK_STATUS, EDIF_TOK_CELL, EDIF_TOK_COMMENT,
			     EDIF_TOK_USERDATA};
static short f_Fabricate[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME};
static short f_Figure[] = {EDIF_TOK_NAME, EDIF_TOK_FIGUREGROUPOVERRIDE,
			   EDIF_TOK_CIRCLE, EDIF_TOK_DOT, EDIF_TOK_OPENSHAPE,
			   EDIF_TOK_PATH, EDIF_TOK_POLYGON,
			   EDIF_TOK_RECTANGLE, EDIF_TOK_SHAPE,
			   EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_FigureArea[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME,
			       EDIF_TOK_FIGUREGROUPOBJECT, EDIF_TOK_LESSTHAN,
			       EDIF_TOK_GREATERTHAN, EDIF_TOK_ATMOST,
			       EDIF_TOK_ATLEAST, EDIF_TOK_EXACTLY,
			       EDIF_TOK_BETWEEN, EDIF_TOK_SINGLEVALUESET,
			       EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_FigureGroup[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME,
				-EDIF_TOK_CORNERTYPE, -EDIF_TOK_ENDTYPE,
				-EDIF_TOK_PATHWIDTH, -EDIF_TOK_BORDERWIDTH,
				-EDIF_TOK_COLOR, -EDIF_TOK_FILLPATTERN,
				-EDIF_TOK_BORDERPATTERN, -EDIF_TOK_TEXTHEIGHT,
				-EDIF_TOK_VISIBLE, EDIF_TOK_INCLUDEFIGUREGROUP,
				EDIF_TOK_COMMENT, EDIF_TOK_PROPERTY,
				EDIF_TOK_USERDATA};
static short f_FigureGroupObject[] = {EDIF_TOK_NAME,
				      EDIF_TOK_FIGUREGROUPOBJECT,
				      EDIF_TOK_INTERSECTION, EDIF_TOK_UNION,
				      EDIF_TOK_DIFFERENCE, EDIF_TOK_INVERSE,
				      EDIF_TOK_OVERSIZE};
static short f_FigureGroupOverride[] = {EDIF_TOK_NAME, -EDIF_TOK_CORNERTYPE,
					-EDIF_TOK_ENDTYPE, -EDIF_TOK_PATHWIDTH,
					-EDIF_TOK_BORDERWIDTH, -EDIF_TOK_COLOR,
					-EDIF_TOK_FILLPATTERN,
					-EDIF_TOK_TEXTHEIGHT,
					-EDIF_TOK_BORDERPATTERN,
					EDIF_TOK_VISIBLE, EDIF_TOK_COMMENT,
					EDIF_TOK_PROPERTY, EDIF_TOK_USERDATA};
static short f_FigureGroupRef[] = {EDIF_TOK_NAME, EDIF_TOK_LIBRARYREF};
static short f_FigurePerimeter[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME,
				    EDIF_TOK_FIGUREGROUPOBJECT,
				    EDIF_TOK_LESSTHAN, EDIF_TOK_GREATERTHAN,
				    EDIF_TOK_ATMOST, EDIF_TOK_ATLEAST,
				    EDIF_TOK_EXACTLY, EDIF_TOK_BETWEEN,
				    EDIF_TOK_SINGLEVALUESET, EDIF_TOK_COMMENT,
				    EDIF_TOK_USERDATA};
static short f_FigureWidth[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME,
				EDIF_TOK_FIGUREGROUPOBJECT, EDIF_TOK_LESSTHAN,
				EDIF_TOK_GREATERTHAN, EDIF_TOK_ATMOST,
				EDIF_TOK_ATLEAST, EDIF_TOK_EXACTLY,
				EDIF_TOK_BETWEEN, EDIF_TOK_SINGLEVALUESET,
				EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_FillPattern[] = {EDIF_TOK_BOOLEAN};
static short f_Follow[] = {EDIF_TOK_NAME, EDIF_TOK_PORTREF, EDIF_TOK_TABLE,
			   EDIF_TOK_DELAY, EDIF_TOK_LOADDELAY};
static short f_ForbiddenEvent[] = {EDIF_TOK_TIMEINTERVAL, EDIF_TOK_EVENT};
static short f_GlobalPortRef[] = {EDIF_TOK_NAME};
static short f_GreaterThan[] = {EDIF_TOK_E};
static short f_GridMap[] = {EDIF_TOK_E};
static short f_IncludeFigureGroup[] = {EDIF_TOK_FIGUREGROUPREF,
				       EDIF_TOK_INTERSECTION, EDIF_TOK_UNION,
				       EDIF_TOK_DIFFERENCE, EDIF_TOK_INVERSE,
				       EDIF_TOK_OVERSIZE};
static short f_Instance[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME, EDIF_TOK_ARRAY,
			     EDIF_TOK_VIEWREF, EDIF_TOK_VIEWLIST,
			     -EDIF_TOK_TRANSFORM, EDIF_TOK_PARAMETERASSIGN,
			     EDIF_TOK_PORTINSTANCE, EDIF_TOK_TIMING,
			     -EDIF_TOK_DESIGNATOR, EDIF_TOK_PROPERTY,
			     EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_InstanceBackAnnotate[] = {EDIF_TOK_INSTANCEREF,
					 -EDIF_TOK_DESIGNATOR, EDIF_TOK_TIMING,
					 EDIF_TOK_PROPERTY, EDIF_TOK_COMMENT};
static short f_InstanceGroup[] = {EDIF_TOK_INSTANCEREF};
static short f_InstanceMap[] = {EDIF_TOK_INSTANCEREF, EDIF_TOK_INSTANCEGROUP,
				EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_InstanceRef[] = {EDIF_TOK_NAME, EDIF_TOK_MEMBER,
				EDIF_TOK_INSTANCEREF, EDIF_TOK_VIEWREF};
static short f_Integer[] = {EDIF_TOK_INTEGERDISPLAY, EDIF_TOK_INTEGER};
static short f_IntegerDisplay[] = {EDIF_TOK_DISPLAY};
static short f_Interface[] = {EDIF_TOK_PORT, EDIF_TOK_PORTBUNDLE,
			      -EDIF_TOK_SYMBOL, -EDIF_TOK_PROTECTIONFRAME,
			      -EDIF_TOK_ARRAYRELATEDINFO, EDIF_TOK_PARAMETER,
			      EDIF_TOK_JOINED, EDIF_TOK_MUSTJOIN,
			      EDIF_TOK_WEAKJOINED, EDIF_TOK_PERMUTABLE,
			      EDIF_TOK_TIMING, EDIF_TOK_SIMULATE,
			      -EDIF_TOK_DESIGNATOR, EDIF_TOK_PROPERTY,
			      EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_InterFigureGroupSpacing[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME,
					    EDIF_TOK_FIGUREGROUPOBJECT,
					    EDIF_TOK_LESSTHAN,
					    EDIF_TOK_GREATERTHAN,
					    EDIF_TOK_ATMOST,
					    EDIF_TOK_ATLEAST, EDIF_TOK_EXACTLY,
					    EDIF_TOK_BETWEEN,
					    EDIF_TOK_SINGLEVALUESET,
					    EDIF_TOK_COMMENT,
					    EDIF_TOK_USERDATA};
static short f_Intersection[] = {EDIF_TOK_FIGUREGROUPREF,
				 EDIF_TOK_INTERSECTION, EDIF_TOK_UNION,
				 EDIF_TOK_DIFFERENCE, EDIF_TOK_INVERSE,
				 EDIF_TOK_OVERSIZE};
static short f_IntraFigureGroupSpacing[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME,
					    EDIF_TOK_FIGUREGROUPOBJECT,
					    EDIF_TOK_LESSTHAN,
					    EDIF_TOK_GREATERTHAN,
					    EDIF_TOK_ATMOST, EDIF_TOK_ATLEAST,
					    EDIF_TOK_EXACTLY, EDIF_TOK_BETWEEN,
					    EDIF_TOK_SINGLEVALUESET,
					    EDIF_TOK_COMMENT,
					    EDIF_TOK_USERDATA};
static short f_Inverse[] = {EDIF_TOK_FIGUREGROUPREF, EDIF_TOK_INTERSECTION,
			    EDIF_TOK_UNION, EDIF_TOK_DIFFERENCE,
			    EDIF_TOK_INVERSE, EDIF_TOK_OVERSIZE};
static short f_Joined[] = {EDIF_TOK_PORTREF, EDIF_TOK_PORTLIST,
			   EDIF_TOK_GLOBALPORTREF};
static short f_KeywordDisplay[] = {EDIF_TOK_DISPLAY};
static short f_KeywordMap[] = {EDIF_TOK_KEYWORDLEVEL, EDIF_TOK_COMMENT};
static short f_LessThan[] = {EDIF_TOK_E};
static short f_Library[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME, EDIF_TOK_EDIFLEVEL,
			    EDIF_TOK_TECHNOLOGY, -EDIF_TOK_STATUS,
			    EDIF_TOK_CELL, EDIF_TOK_COMMENT,
			    EDIF_TOK_USERDATA};
static short f_LibraryRef[] = {EDIF_TOK_NAME};
static short f_ListOfNets[] = {EDIF_TOK_NET};
static short f_ListOfPorts[] = {EDIF_TOK_PORT, EDIF_TOK_PORTBUNDLE};
static short f_LoadDelay[] = {EDIF_TOK_MNM, EDIF_TOK_E, EDIF_TOK_MINOMAXDISPLAY};
static short f_LogicAssign[] = {EDIF_TOK_NAME, EDIF_TOK_PORTREF,
				EDIF_TOK_LOGICREF, EDIF_TOK_TABLE,
				EDIF_TOK_DELAY, EDIF_TOK_LOADDELAY};
static short f_LogicInput[] = {EDIF_TOK_PORTLIST, EDIF_TOK_PORTREF,
			       EDIF_TOK_NAME, EDIF_TOK_LOGICWAVEFORM};
static short f_LogicList[] = {EDIF_TOK_NAME, EDIF_TOK_LOGICONEOF,
			      EDIF_TOK_IGNORE};
static short f_LogicMapInput[] = {EDIF_TOK_LOGICREF};
static short f_LogicMapOutput[] = {EDIF_TOK_LOGICREF};
static short f_LogicOneOf[] = {EDIF_TOK_NAME, EDIF_TOK_LOGICLIST};
static short f_LogicOutput[] = {EDIF_TOK_PORTLIST, EDIF_TOK_PORTREF,
				EDIF_TOK_NAME, EDIF_TOK_LOGICWAVEFORM};
static short f_LogicPort[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME,
			      EDIF_TOK_PROPERTY, EDIF_TOK_COMMENT,
			      EDIF_TOK_USERDATA};
static short f_LogicRef[] = {EDIF_TOK_NAME, EDIF_TOK_LIBRARYREF};
static short f_LogicValue[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME,
			       -EDIF_TOK_VOLTAGEMAP, -EDIF_TOK_CURRENTMAP,
			       -EDIF_TOK_BOOLEANMAP, -EDIF_TOK_COMPOUND,
			       -EDIF_TOK_WEAK ,-EDIF_TOK_STRONG,
			       -EDIF_TOK_DOMINATES, -EDIF_TOK_LOGICMAPOUTPUT,
			       -EDIF_TOK_LOGICMAPINPUT,
			       -EDIF_TOK_ISOLATED, EDIF_TOK_RESOLVES,
			       EDIF_TOK_PROPERTY, EDIF_TOK_COMMENT,
			       EDIF_TOK_USERDATA};
static short f_LogicWaveform[] = {EDIF_TOK_NAME, EDIF_TOK_LOGICLIST,
				  EDIF_TOK_LOGICONEOF, EDIF_TOK_IGNORE};
static short f_Maintain[] = {EDIF_TOK_NAME, EDIF_TOK_PORTREF, EDIF_TOK_DELAY,
			     EDIF_TOK_LOADDELAY};
static short f_Match[] = {EDIF_TOK_NAME, EDIF_TOK_PORTREF, EDIF_TOK_PORTLIST,
			  EDIF_TOK_LOGICLIST, EDIF_TOK_LOGICONEOF};
static short f_Member[] = {EDIF_TOK_NAME};
static short f_MiNoMax[] = {EDIF_TOK_MNM, EDIF_TOK_E, EDIF_TOK_MINOMAXDISPLAY,
			    EDIF_TOK_MINOMAX};
static short f_MiNoMaxDisplay[] = {EDIF_TOK_MNM, EDIF_TOK_E, EDIF_TOK_DISPLAY};
static short f_Mnm[] = {EDIF_TOK_E, EDIF_TOK_UNDEFINED,
			EDIF_TOK_UNCONSTRAINED};
static short f_MultipleValueSet[] = {EDIF_TOK_RANGEVECTOR};
static short f_MustJoin[] = {EDIF_TOK_PORTREF, EDIF_TOK_PORTLIST,
			     EDIF_TOK_WEAKJOINED, EDIF_TOK_JOINED};
static short f_Name[] = {EDIF_TOK_DISPLAY};
static short f_Net[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME, -EDIF_TOK_CRITICALITY,
			EDIF_TOK_NETDELAY, EDIF_TOK_FIGURE, EDIF_TOK_NET,
			EDIF_TOK_INSTANCE, EDIF_TOK_COMMENTGRAPHICS,
			EDIF_TOK_PROPERTY, EDIF_TOK_COMMENT,
			EDIF_TOK_USERDATA, EDIF_TOK_JOINED, EDIF_TOK_ARRAY};
static short f_NetBackAnnotate[] = {EDIF_TOK_NETREF, EDIF_TOK_NETDELAY,
				    -EDIF_TOK_CRITICALITY, EDIF_TOK_PROPERTY,
				    EDIF_TOK_COMMENT};
static short f_NetBundle[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME, EDIF_TOK_ARRAY,
			      EDIF_TOK_LISTOFNETS, EDIF_TOK_FIGURE,
			      EDIF_TOK_COMMENTGRAPHICS, EDIF_TOK_PROPERTY,
			      EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_NetDelay[] = {EDIF_TOK_DERIVATION, EDIF_TOK_DELAY,
			     EDIF_TOK_TRANSITION, EDIF_TOK_BECOMES};
static short f_NetGroup[] = {EDIF_TOK_NAME, EDIF_TOK_MEMBER, EDIF_TOK_NETREF};
static short f_NetMap[] = {EDIF_TOK_NETREF, EDIF_TOK_NETGROUP,
			   EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_NetRef[] = {EDIF_TOK_NAME, EDIF_TOK_MEMBER, EDIF_TOK_NETREF,
			   EDIF_TOK_INSTANCEREF, EDIF_TOK_VIEWREF};
static short f_NonPermutable[] = {EDIF_TOK_PORTREF, EDIF_TOK_PERMUTABLE};
static short f_NotAllowed[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME,
			       EDIF_TOK_FIGUREGROUPOBJECT, EDIF_TOK_COMMENT,
			       EDIF_TOK_USERDATA};
static short f_NotchSpacing[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME,
				 EDIF_TOK_FIGUREGROUPOBJECT, EDIF_TOK_LESSTHAN,
				 EDIF_TOK_GREATERTHAN, EDIF_TOK_ATMOST,
				 EDIF_TOK_ATLEAST, EDIF_TOK_EXACTLY,
				 EDIF_TOK_BETWEEN, EDIF_TOK_SINGLEVALUESET,
				 EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_Number[] = {EDIF_TOK_E, EDIF_TOK_NUMBERDISPLAY, EDIF_TOK_NUMBER};
static short f_NumberDefinition[] = {EDIF_TOK_SCALE, -EDIF_TOK_GRIDMAP,
				     EDIF_TOK_COMMENT};
static short f_NumberDisplay[] = {EDIF_TOK_E, EDIF_TOK_DISPLAY};
static short f_OffPageConnector[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME,
				     -EDIF_TOK_UNUSED, EDIF_TOK_PROPERTY,
				     EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_OffsetEvent[] = {EDIF_TOK_EVENT, EDIF_TOK_E};
static short f_OpenShape[] = {EDIF_TOK_CURVE, EDIF_TOK_PROPERTY};
static short f_Origin[] = {EDIF_TOK_PT};
static short f_OverhangDistance[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME,
				     EDIF_TOK_FIGUREGROUPOBJECT, EDIF_TOK_LESSTHAN,
				     EDIF_TOK_GREATERTHAN, EDIF_TOK_ATMOST,
				     EDIF_TOK_ATLEAST, EDIF_TOK_EXACTLY,
				     EDIF_TOK_BETWEEN, EDIF_TOK_SINGLEVALUESET,
				     EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_OverlapDistance[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME,
				    EDIF_TOK_FIGUREGROUPOBJECT, EDIF_TOK_LESSTHAN,
				    EDIF_TOK_GREATERTHAN, EDIF_TOK_ATMOST,
				    EDIF_TOK_ATLEAST, EDIF_TOK_EXACTLY,
				    EDIF_TOK_BETWEEN, EDIF_TOK_SINGLEVALUESET,
				    EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_Oversize[] = {EDIF_TOK_FIGUREGROUPREF, EDIF_TOK_INTERSECTION,
			     EDIF_TOK_UNION, EDIF_TOK_DIFFERENCE,
			     EDIF_TOK_INVERSE, EDIF_TOK_OVERSIZE,
			     EDIF_TOK_CORNERTYPE};
static short f_Page[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME, EDIF_TOK_ARRAY,
			 EDIF_TOK_INSTANCE, EDIF_TOK_NET, EDIF_TOK_NETBUNDLE,
			 EDIF_TOK_COMMENTGRAPHICS, EDIF_TOK_PORTIMPLEMENTATION,
			 -EDIF_TOK_PAGESIZE, -EDIF_TOK_BOUNDINGBOX,
			 EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_PageSize[] = {EDIF_TOK_RECTANGLE};
static short f_Parameter[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME, EDIF_TOK_ARRAY,
			      EDIF_TOK_BOOLEAN, EDIF_TOK_INTEGER,
			      EDIF_TOK_MINOMAX, EDIF_TOK_NUMBER,
			      EDIF_TOK_POINT, EDIF_TOK_STRING};
static short f_ParameterAssign[] = {EDIF_TOK_NAME, EDIF_TOK_MEMBER,
				    EDIF_TOK_BOOLEAN, EDIF_TOK_INTEGER,
				    EDIF_TOK_MINOMAX, EDIF_TOK_NUMBER, EDIF_TOK_POINT,
				    EDIF_TOK_STRING};
static short f_ParameterDisplay[] = {EDIF_TOK_NAME, EDIF_TOK_MEMBER,
				     EDIF_TOK_DISPLAY};
static short f_Path[] = {EDIF_TOK_POINTLIST, EDIF_TOK_PROPERTY};
static short f_PathDelay[] = {EDIF_TOK_DELAY, EDIF_TOK_EVENT};
static short f_Permutable[] = {EDIF_TOK_PORTREF, EDIF_TOK_PERMUTABLE,
			       EDIF_TOK_NONPERMUTABLE};
static short f_PhysicalDesignRule[] = {EDIF_TOK_FIGUREWIDTH,
				       EDIF_TOK_FIGUREAREA,
				       EDIF_TOK_RECTANGLESIZE,
				       EDIF_TOK_FIGUREPERIMETER,
				       EDIF_TOK_OVERLAPDISTANCE,
				       EDIF_TOK_OVERHANGDISTANCE,
				       EDIF_TOK_ENCLOSUREDISTANCE,
				       EDIF_TOK_INTERFIGUREGROUPSPACING,
				       EDIF_TOK_NOTCHSPACING,
				       EDIF_TOK_INTRAFIGUREGROUPSPACING,
				       EDIF_TOK_NOTALLOWED,
				       EDIF_TOK_FIGUREGROUP, EDIF_TOK_COMMENT,
				       EDIF_TOK_USERDATA};
static short f_Plug[] = {EDIF_TOK_SOCKETSET};
static short f_Point[] = {EDIF_TOK_PT, EDIF_TOK_POINTDISPLAY,
			  EDIF_TOK_POINT};
static short f_PointDisplay[] = {EDIF_TOK_PT, EDIF_TOK_DISPLAY};
static short f_PointList[] = {EDIF_TOK_PT};
static short f_Polygon[] = {EDIF_TOK_POINTLIST, EDIF_TOK_PROPERTY};
static short f_Port[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME, EDIF_TOK_ARRAY,
			 -EDIF_TOK_DIRECTION, -EDIF_TOK_UNUSED,
			 EDIF_TOK_PORTDELAY, -EDIF_TOK_DESIGNATOR,
			 -EDIF_TOK_DCFANINLOAD, -EDIF_TOK_DCFANOUTLOAD,
			 -EDIF_TOK_DCMAXFANIN, -EDIF_TOK_DCMAXFANOUT,
			 -EDIF_TOK_ACLOAD, EDIF_TOK_PROPERTY,
			 EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_PortBackAnnotate[] = {EDIF_TOK_PORTREF, -EDIF_TOK_DESIGNATOR,
				     EDIF_TOK_PORTDELAY, -EDIF_TOK_DCFANINLOAD,
				     -EDIF_TOK_DCFANOUTLOAD,
				     -EDIF_TOK_DCMAXFANIN,
				     -EDIF_TOK_DCMAXFANOUT, -EDIF_TOK_ACLOAD,
				     EDIF_TOK_PROPERTY, EDIF_TOK_COMMENT};
static short f_PortBundle[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME, EDIF_TOK_ARRAY,
			       EDIF_TOK_LISTOFPORTS, EDIF_TOK_PROPERTY,
			       EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_PortDelay[] = {EDIF_TOK_DERIVATION, EDIF_TOK_DELAY,
			      EDIF_TOK_LOADDELAY, EDIF_TOK_TRANSITION,
			      EDIF_TOK_BECOMES};
static short f_PortGroup[] = {EDIF_TOK_NAME, EDIF_TOK_MEMBER,
			      EDIF_TOK_PORTREF};
static short f_PortImplementation[] = {EDIF_TOK_PORTREF, EDIF_TOK_NAME, EDIF_TOK_MEMBER,
				       -EDIF_TOK_CONNECTLOCATION,
				       EDIF_TOK_FIGURE, EDIF_TOK_INSTANCE,
				       EDIF_TOK_COMMENTGRAPHICS,
				       EDIF_TOK_PROPERTYDISPLAY,
				       EDIF_TOK_KEYWORDDISPLAY,
				       EDIF_TOK_PROPERTY,
				       EDIF_TOK_USERDATA, EDIF_TOK_COMMENT};
static short f_PortInstance[] = {EDIF_TOK_PORTREF, EDIF_TOK_NAME,
				 EDIF_TOK_MEMBER, -EDIF_TOK_UNUSED,
				 EDIF_TOK_PORTDELAY, -EDIF_TOK_DESIGNATOR,
				 -EDIF_TOK_DCFANINLOAD,
				 -EDIF_TOK_DCFANOUTLOAD, -EDIF_TOK_DCMAXFANIN,
				 -EDIF_TOK_DCMAXFANOUT, -EDIF_TOK_ACLOAD,
				 EDIF_TOK_PROPERTY, EDIF_TOK_COMMENT,
				 EDIF_TOK_USERDATA};
static short f_PortList[] = {EDIF_TOK_PORTREF, EDIF_TOK_NAME,
			     EDIF_TOK_MEMBER};
static short f_PortListAlias[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME,
				  EDIF_TOK_ARRAY, EDIF_TOK_PORTLIST};
static short f_PortMap[] = {EDIF_TOK_PORTREF, EDIF_TOK_PORTGROUP,
			    EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_PortRef[] = {EDIF_TOK_NAME, EDIF_TOK_MEMBER,
			    EDIF_TOK_PORTREF, EDIF_TOK_INSTANCEREF,
			    EDIF_TOK_VIEWREF};
static short f_Program[] = {EDIF_TOK_VERSION};
static short f_Property[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME, EDIF_TOK_BOOLEAN,
			     EDIF_TOK_INTEGER, EDIF_TOK_MINOMAX,
			     EDIF_TOK_NUMBER, EDIF_TOK_POINT, EDIF_TOK_STRING,
			     -EDIF_TOK_OWNER, -EDIF_TOK_UNIT,
			     EDIF_TOK_PROPERTY, EDIF_TOK_COMMENT};
static short f_PropertyDisplay[] = {EDIF_TOK_NAME, EDIF_TOK_DISPLAY};
static short f_ProtectionFrame[] = {EDIF_TOK_PORTIMPLEMENTATION,
				    EDIF_TOK_FIGURE, EDIF_TOK_INSTANCE,
				    EDIF_TOK_COMMENTGRAPHICS,
				    -EDIF_TOK_BOUNDINGBOX,
				    EDIF_TOK_PROPERTYDISPLAY,
				    EDIF_TOK_KEYWORDDISPLAY,
				    EDIF_TOK_PARAMETERDISPLAY,
				    EDIF_TOK_PROPERTY, EDIF_TOK_COMMENT,
				    EDIF_TOK_USERDATA};
static short f_RangeVector[] = {EDIF_TOK_LESSTHAN, EDIF_TOK_GREATERTHAN,
				EDIF_TOK_ATMOST, EDIF_TOK_ATLEAST,
				EDIF_TOK_EXACTLY, EDIF_TOK_BETWEEN,
				EDIF_TOK_SINGLEVALUESET};
static short f_Rectangle[] = {EDIF_TOK_PT, EDIF_TOK_PROPERTY};
static short f_RectangleSize[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME,
				  EDIF_TOK_FIGUREGROUPOBJECT,
				  EDIF_TOK_RANGEVECTOR,
				  EDIF_TOK_MULTIPLEVALUESET,EDIF_TOK_COMMENT,
				  EDIF_TOK_USERDATA};
static short f_Rename[] = {EDIF_TOK_NAME, EDIF_TOK_STRINGDISPLAY};
static short f_Resolves[] = {EDIF_TOK_NAME};
static short f_Scale[] = {EDIF_TOK_E, EDIF_TOK_UNIT};
static short f_Section[] = {EDIF_TOK_SECTION, EDIF_TOK_INSTANCE};
static short f_Shape[] = {EDIF_TOK_CURVE, EDIF_TOK_PROPERTY};
static short f_Simulate[] = {EDIF_TOK_NAME, EDIF_TOK_PORTLISTALIAS,
			     EDIF_TOK_WAVEVALUE, EDIF_TOK_APPLY,
			     EDIF_TOK_COMMENT, EDIF_TOK_USERDATA};
static short f_SimulationInfo[] = {EDIF_TOK_LOGICVALUE, EDIF_TOK_COMMENT,
				   EDIF_TOK_USERDATA};
static short f_SingleValueSet[] = {EDIF_TOK_LESSTHAN, EDIF_TOK_GREATERTHAN,
				   EDIF_TOK_ATMOST, EDIF_TOK_ATLEAST,
				   EDIF_TOK_EXACTLY, EDIF_TOK_BETWEEN};
static short f_Site[] = {EDIF_TOK_VIEWREF, EDIF_TOK_TRANSFORM};
static short f_Socket[] = {EDIF_TOK_SYMMETRY};
static short f_SocketSet[] = {EDIF_TOK_SYMMETRY, EDIF_TOK_SITE};
static short f_Status[] = {EDIF_TOK_WRITTEN, EDIF_TOK_COMMENT,
			   EDIF_TOK_USERDATA};
static short f_Steady[] = {EDIF_TOK_NAME, EDIF_TOK_MEMBER, EDIF_TOK_PORTREF,
			   EDIF_TOK_PORTLIST, EDIF_TOK_DURATION,
			   EDIF_TOK_TRANSITION, EDIF_TOK_BECOMES};
static short f_String[] = {EDIF_TOK_STRINGDISPLAY, EDIF_TOK_STRING};
static short f_StringDisplay[] = {EDIF_TOK_DISPLAY};
static short f_Strong[] = {EDIF_TOK_NAME};
static short f_Symbol[] = {EDIF_TOK_PORTIMPLEMENTATION, EDIF_TOK_FIGURE,
			   EDIF_TOK_INSTANCE, EDIF_TOK_COMMENTGRAPHICS,
			   EDIF_TOK_ANNOTATE, -EDIF_TOK_PAGESIZE,
			   -EDIF_TOK_BOUNDINGBOX, EDIF_TOK_PROPERTYDISPLAY,
			   EDIF_TOK_KEYWORDDISPLAY, EDIF_TOK_PARAMETERDISPLAY,
			   EDIF_TOK_PROPERTY, EDIF_TOK_COMMENT,
			   EDIF_TOK_USERDATA};
static short f_Symmetry[] = {EDIF_TOK_TRANSFORM};
static short f_Table[] = {EDIF_TOK_ENTRY, EDIF_TOK_TABLEDEFAULT};
static short f_TableDefault[] = {EDIF_TOK_LOGICREF, EDIF_TOK_PORTREF,
				 EDIF_TOK_NOCHANGE, EDIF_TOK_TABLE,
				 EDIF_TOK_DELAY, EDIF_TOK_LOADDELAY};
static short f_Technology[] = {EDIF_TOK_NUMBERDEFINITION, EDIF_TOK_FIGUREGROUP,
			       EDIF_TOK_FABRICATE, -EDIF_TOK_SIMULATIONINFO,
			       EDIF_TOK_COMMENT, EDIF_TOK_USERDATA,
			       -EDIF_TOK_PHYSICALDESIGNRULE};
static short f_TimeInterval[] = {EDIF_TOK_EVENT, EDIF_TOK_OFFSETEVENT,
				 EDIF_TOK_DURATION};
static short f_Timing[] = {EDIF_TOK_DERIVATION, EDIF_TOK_PATHDELAY,
			   EDIF_TOK_FORBIDDENEVENT, EDIF_TOK_COMMENT,
			   EDIF_TOK_USERDATA};
static short f_Transform[] = {EDIF_TOK_SCALEX, EDIF_TOK_SCALEY, EDIF_TOK_DELTA,
			      EDIF_TOK_ORIENTATION, EDIF_TOK_ORIGIN};
static short f_Transition[] = {EDIF_TOK_NAME, EDIF_TOK_LOGICLIST,
			       EDIF_TOK_LOGICONEOF};
static short f_Trigger[] = {EDIF_TOK_CHANGE, EDIF_TOK_STEADY,
			    EDIF_TOK_INITIAL};
static short f_Union[] = {EDIF_TOK_FIGUREGROUPREF, EDIF_TOK_INTERSECTION,
			  EDIF_TOK_UNION, EDIF_TOK_DIFFERENCE,
			  EDIF_TOK_INVERSE, EDIF_TOK_OVERSIZE};
static short f_View[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME, EDIF_TOK_VIEWTYPE,
			 EDIF_TOK_INTERFACE, -EDIF_TOK_STATUS,
			 -EDIF_TOK_CONTENTS, EDIF_TOK_COMMENT,
			 EDIF_TOK_PROPERTY, EDIF_TOK_USERDATA};
static short f_ViewList[] = {EDIF_TOK_VIEWREF, EDIF_TOK_VIEWLIST};
static short f_ViewMap[] = {EDIF_TOK_PORTMAP, EDIF_TOK_PORTBACKANNOTATE,
			    EDIF_TOK_INSTANCEMAP,
			    EDIF_TOK_INSTANCEBACKANNOTATE, EDIF_TOK_NETMAP,
			    EDIF_TOK_NETBACKANNOTATE, EDIF_TOK_COMMENT,
			    EDIF_TOK_USERDATA};
static short f_ViewRef[] = {EDIF_TOK_NAME, EDIF_TOK_CELLREF};
static short f_Visible[] = {EDIF_TOK_FALSE, EDIF_TOK_TRUE};
static short f_VoltageMap[] = {EDIF_TOK_MNM, EDIF_TOK_E};
static short f_WaveValue[] = {EDIF_TOK_NAME, EDIF_TOK_RENAME, EDIF_TOK_E,
			      EDIF_TOK_LOGICWAVEFORM};
static short f_Weak[] = {EDIF_TOK_NAME};
static short f_WeakJoined[] = {EDIF_TOK_PORTREF, EDIF_TOK_PORTLIST,
			       EDIF_TOK_JOINED};
static short f_When[] = {EDIF_TOK_TRIGGER, EDIF_TOK_AFTER,
			 EDIF_TOK_FOLLOW, EDIF_TOK_MAINTAIN,
			 EDIF_TOK_LOGICASSIGN, EDIF_TOK_COMMENT,
			 EDIF_TOK_USERDATA};
static short f_Written[] = {EDIF_TOK_TIMESTAMP, EDIF_TOK_AUTHOR,
			    EDIF_TOK_PROGRAM, EDIF_TOK_DATAORIGIN,
			    EDIF_TOK_PROPERTY, EDIF_TOK_COMMENT,
			    EDIF_TOK_USERDATA};
/*
 *	Context binding table:
 *
 *	  This binds context follower arrays to their originating context.
 */
typedef struct Binder {
  short *Follower;		/* pointer to follower array */
  short Origin;			/* '%token' value of origin */
  short FollowerSize;		/* size of follower array */
} Binder;
#define	BE(f,o)			{f,o,sizeof(f)/sizeof(short)}
static Binder BinderDef[] = {
  BE(f_NULL,			0),
  BE(f_Edif,			EDIF_TOK_EDIF),
  BE(f_AcLoad,			EDIF_TOK_ACLOAD),
  BE(f_After,			EDIF_TOK_AFTER),
  BE(f_Annotate,		EDIF_TOK_ANNOTATE),
  BE(f_Apply,			EDIF_TOK_APPLY),
  BE(f_Arc,			EDIF_TOK_ARC),
  BE(f_Array,			EDIF_TOK_ARRAY),
  BE(f_ArrayMacro,		EDIF_TOK_ARRAYMACRO),
  BE(f_ArrayRelatedInfo,	EDIF_TOK_ARRAYRELATEDINFO),
  BE(f_ArraySite,		EDIF_TOK_ARRAYSITE),
  BE(f_AtLeast,			EDIF_TOK_ATLEAST),
  BE(f_AtMost,			EDIF_TOK_ATMOST),
  BE(f_Becomes,			EDIF_TOK_BECOMES),
  BE(f_Boolean,			EDIF_TOK_BOOLEAN),
  BE(f_BooleanDisplay,		EDIF_TOK_BOOLEANDISPLAY),
  BE(f_BooleanMap,		EDIF_TOK_BOOLEANMAP),
  BE(f_BorderPattern,		EDIF_TOK_BORDERPATTERN),
  BE(f_BoundingBox,		EDIF_TOK_BOUNDINGBOX),
  BE(f_Cell,			EDIF_TOK_CELL),
  BE(f_CellRef,			EDIF_TOK_CELLREF),
  BE(f_Change,			EDIF_TOK_CHANGE),
  BE(f_Circle,			EDIF_TOK_CIRCLE),
  BE(f_Color,			EDIF_TOK_COLOR),
  BE(f_CommentGraphics,		EDIF_TOK_COMMENTGRAPHICS),
  BE(f_Compound,		EDIF_TOK_COMPOUND),
  BE(f_ConnectLocation,		EDIF_TOK_CONNECTLOCATION),
  BE(f_Contents,		EDIF_TOK_CONTENTS),
  BE(f_Criticality,		EDIF_TOK_CRITICALITY),
  BE(f_CurrentMap,		EDIF_TOK_CURRENTMAP),
  BE(f_Curve,			EDIF_TOK_CURVE),
  BE(f_Cycle,			EDIF_TOK_CYCLE),
  BE(f_DataOrigin,		EDIF_TOK_DATAORIGIN),
  BE(f_DcFanInLoad,		EDIF_TOK_DCFANINLOAD),
  BE(f_DcFanOutLoad,		EDIF_TOK_DCFANOUTLOAD),
  BE(f_DcMaxFanIn,		EDIF_TOK_DCMAXFANIN),
  BE(f_DcMaxFanOut,		EDIF_TOK_DCMAXFANOUT),
  BE(f_Delay,			EDIF_TOK_DELAY),
  BE(f_Delta,			EDIF_TOK_DELTA),
  BE(f_Design,			EDIF_TOK_DESIGN),
  BE(f_Designator,		EDIF_TOK_DESIGNATOR),
  BE(f_Difference,		EDIF_TOK_DIFFERENCE),
  BE(f_Display,			EDIF_TOK_DISPLAY),
  BE(f_Dominates,		EDIF_TOK_DOMINATES),
  BE(f_Dot,			EDIF_TOK_DOT),
  BE(f_Duration,		EDIF_TOK_DURATION),
  BE(f_EnclosureDistance,	EDIF_TOK_ENCLOSUREDISTANCE),
  BE(f_Entry,			EDIF_TOK_ENTRY),
  BE(f_Exactly,			EDIF_TOK_EXACTLY),
  BE(f_External,		EDIF_TOK_EXTERNAL),
  BE(f_Fabricate,		EDIF_TOK_FABRICATE),
  BE(f_Figure,			EDIF_TOK_FIGURE),
  BE(f_FigureArea,		EDIF_TOK_FIGUREAREA),
  BE(f_FigureGroup,		EDIF_TOK_FIGUREGROUP),
  BE(f_FigureGroupObject,	EDIF_TOK_FIGUREGROUPOBJECT),
  BE(f_FigureGroupOverride,	EDIF_TOK_FIGUREGROUPOVERRIDE),
  BE(f_FigureGroupRef,		EDIF_TOK_FIGUREGROUPREF),
  BE(f_FigurePerimeter,		EDIF_TOK_FIGUREPERIMETER),
  BE(f_FigureWidth,		EDIF_TOK_FIGUREWIDTH),
  BE(f_FillPattern,		EDIF_TOK_FILLPATTERN),
  BE(f_Follow,			EDIF_TOK_FOLLOW),
  BE(f_ForbiddenEvent,		EDIF_TOK_FORBIDDENEVENT),
  BE(f_GlobalPortRef,		EDIF_TOK_GLOBALPORTREF),
  BE(f_GreaterThan,		EDIF_TOK_GREATERTHAN),
  BE(f_GridMap,			EDIF_TOK_GRIDMAP),
  BE(f_IncludeFigureGroup,	EDIF_TOK_INCLUDEFIGUREGROUP),
  BE(f_Instance,		EDIF_TOK_INSTANCE),
  BE(f_InstanceBackAnnotate,	EDIF_TOK_INSTANCEBACKANNOTATE),
  BE(f_InstanceGroup,		EDIF_TOK_INSTANCEGROUP),
  BE(f_InstanceMap,		EDIF_TOK_INSTANCEMAP),
  BE(f_InstanceRef,		EDIF_TOK_INSTANCEREF),
  BE(f_Integer,			EDIF_TOK_INTEGER),
  BE(f_IntegerDisplay,		EDIF_TOK_INTEGERDISPLAY),
  BE(f_InterFigureGroupSpacing,	EDIF_TOK_INTERFIGUREGROUPSPACING),
  BE(f_Interface,		EDIF_TOK_INTERFACE),
  BE(f_Intersection,		EDIF_TOK_INTERSECTION),
  BE(f_IntraFigureGroupSpacing,	EDIF_TOK_INTRAFIGUREGROUPSPACING),
  BE(f_Inverse,			EDIF_TOK_INVERSE),
  BE(f_Joined,			EDIF_TOK_JOINED),
  BE(f_KeywordDisplay,		EDIF_TOK_KEYWORDDISPLAY),
  BE(f_KeywordMap,		EDIF_TOK_KEYWORDMAP),
  BE(f_LessThan,		EDIF_TOK_LESSTHAN),
  BE(f_Library,			EDIF_TOK_LIBRARY),
  BE(f_LibraryRef,		EDIF_TOK_LIBRARYREF),
  BE(f_ListOfNets,		EDIF_TOK_LISTOFNETS),
  BE(f_ListOfPorts,		EDIF_TOK_LISTOFPORTS),
  BE(f_LoadDelay,		EDIF_TOK_LOADDELAY),
  BE(f_LogicAssign,		EDIF_TOK_LOGICASSIGN),
  BE(f_LogicInput,		EDIF_TOK_LOGICINPUT),
  BE(f_LogicList,		EDIF_TOK_LOGICLIST),
  BE(f_LogicMapInput,		EDIF_TOK_LOGICMAPINPUT),
  BE(f_LogicMapOutput,		EDIF_TOK_LOGICMAPOUTPUT),
  BE(f_LogicOneOf,		EDIF_TOK_LOGICONEOF),
  BE(f_LogicOutput,		EDIF_TOK_LOGICOUTPUT),
  BE(f_LogicPort,		EDIF_TOK_LOGICPORT),
  BE(f_LogicRef,		EDIF_TOK_LOGICREF),
  BE(f_LogicValue,		EDIF_TOK_LOGICVALUE),
  BE(f_LogicWaveform,		EDIF_TOK_LOGICWAVEFORM),
  BE(f_Maintain,		EDIF_TOK_MAINTAIN),
  BE(f_Match,			EDIF_TOK_MATCH),
  BE(f_Member,			EDIF_TOK_MEMBER),
  BE(f_MiNoMax,			EDIF_TOK_MINOMAX),
  BE(f_MiNoMaxDisplay,		EDIF_TOK_MINOMAXDISPLAY),
  BE(f_Mnm,			EDIF_TOK_MNM),
  BE(f_MultipleValueSet,	EDIF_TOK_MULTIPLEVALUESET),
  BE(f_MustJoin,		EDIF_TOK_MUSTJOIN),
  BE(f_Name,			EDIF_TOK_NAME),
  BE(f_Net,			EDIF_TOK_NET),
  BE(f_NetBackAnnotate,		EDIF_TOK_NETBACKANNOTATE),
  BE(f_NetBundle,		EDIF_TOK_NETBUNDLE),
  BE(f_NetDelay,		EDIF_TOK_NETDELAY),
  BE(f_NetGroup,		EDIF_TOK_NETGROUP),
  BE(f_NetMap,			EDIF_TOK_NETMAP),
  BE(f_NetRef,			EDIF_TOK_NETREF),
  BE(f_NonPermutable,		EDIF_TOK_NONPERMUTABLE),
  BE(f_NotAllowed,		EDIF_TOK_NOTALLOWED),
  BE(f_NotchSpacing,		EDIF_TOK_NOTCHSPACING),
  BE(f_Number,			EDIF_TOK_NUMBER),
  BE(f_NumberDefinition,	EDIF_TOK_NUMBERDEFINITION),
  BE(f_NumberDisplay,		EDIF_TOK_NUMBERDISPLAY),
  BE(f_OffPageConnector,	EDIF_TOK_OFFPAGECONNECTOR),
  BE(f_OffsetEvent,		EDIF_TOK_OFFSETEVENT),
  BE(f_OpenShape,		EDIF_TOK_OPENSHAPE),
  BE(f_Origin,			EDIF_TOK_ORIGIN),
  BE(f_OverhangDistance,	EDIF_TOK_OVERHANGDISTANCE),
  BE(f_OverlapDistance,		EDIF_TOK_OVERLAPDISTANCE),
  BE(f_Oversize,		EDIF_TOK_OVERSIZE),
  BE(f_Page,			EDIF_TOK_PAGE),
  BE(f_PageSize,		EDIF_TOK_PAGESIZE),
  BE(f_Parameter,		EDIF_TOK_PARAMETER),
  BE(f_ParameterAssign,		EDIF_TOK_PARAMETERASSIGN),
  BE(f_ParameterDisplay,	EDIF_TOK_PARAMETERDISPLAY),
  BE(f_Path,			EDIF_TOK_PATH),
  BE(f_PathDelay,		EDIF_TOK_PATHDELAY),
  BE(f_Permutable,		EDIF_TOK_PERMUTABLE),
  BE(f_PhysicalDesignRule,	EDIF_TOK_PHYSICALDESIGNRULE),
  BE(f_Plug,			EDIF_TOK_PLUG),
  BE(f_Point,			EDIF_TOK_POINT),
  BE(f_PointDisplay,		EDIF_TOK_POINTDISPLAY),
  BE(f_PointList,		EDIF_TOK_POINTLIST),
  BE(f_Polygon,			EDIF_TOK_POLYGON),
  BE(f_Port,			EDIF_TOK_PORT),
  BE(f_PortBackAnnotate,	EDIF_TOK_PORTBACKANNOTATE),
  BE(f_PortBundle,		EDIF_TOK_PORTBUNDLE),
  BE(f_PortDelay,		EDIF_TOK_PORTDELAY),
  BE(f_PortGroup,		EDIF_TOK_PORTGROUP),
  BE(f_PortImplementation,	EDIF_TOK_PORTIMPLEMENTATION),
  BE(f_PortInstance,		EDIF_TOK_PORTINSTANCE),
  BE(f_PortList,		EDIF_TOK_PORTLIST),
  BE(f_PortListAlias,		EDIF_TOK_PORTLISTALIAS),
  BE(f_PortMap,			EDIF_TOK_PORTMAP),
  BE(f_PortRef,			EDIF_TOK_PORTREF),
  BE(f_Program,			EDIF_TOK_PROGRAM),
  BE(f_Property,		EDIF_TOK_PROPERTY),
  BE(f_PropertyDisplay,		EDIF_TOK_PROPERTYDISPLAY),
  BE(f_ProtectionFrame,		EDIF_TOK_PROTECTIONFRAME),
  BE(f_RangeVector,		EDIF_TOK_RANGEVECTOR),
  BE(f_Rectangle,		EDIF_TOK_RECTANGLE),
  BE(f_RectangleSize,		EDIF_TOK_RECTANGLESIZE),
  BE(f_Rename,			EDIF_TOK_RENAME),
  BE(f_Resolves,		EDIF_TOK_RESOLVES),
  BE(f_Scale,			EDIF_TOK_SCALE),
  BE(f_Section,			EDIF_TOK_SECTION),
  BE(f_Shape,			EDIF_TOK_SHAPE),
  BE(f_Simulate,		EDIF_TOK_SIMULATE),
  BE(f_SimulationInfo,		EDIF_TOK_SIMULATIONINFO),
  BE(f_SingleValueSet,		EDIF_TOK_SINGLEVALUESET),
  BE(f_Site,			EDIF_TOK_SITE),
  BE(f_Socket,			EDIF_TOK_SOCKET),
  BE(f_SocketSet,		EDIF_TOK_SOCKETSET),
  BE(f_Status,			EDIF_TOK_STATUS),
  BE(f_Steady,			EDIF_TOK_STEADY),
  BE(f_String,			EDIF_TOK_STRING),
  BE(f_StringDisplay,		EDIF_TOK_STRINGDISPLAY),
  BE(f_Strong,			EDIF_TOK_STRONG),
  BE(f_Symbol,			EDIF_TOK_SYMBOL),
  BE(f_Symmetry,		EDIF_TOK_SYMMETRY),
  BE(f_Table,			EDIF_TOK_TABLE),
  BE(f_TableDefault,		EDIF_TOK_TABLEDEFAULT),
  BE(f_Technology,		EDIF_TOK_TECHNOLOGY),
  BE(f_TimeInterval,		EDIF_TOK_TIMEINTERVAL),
  BE(f_Timing,			EDIF_TOK_TIMING),
  BE(f_Transform,		EDIF_TOK_TRANSFORM),
  BE(f_Transition,		EDIF_TOK_TRANSITION),
  BE(f_Trigger,			EDIF_TOK_TRIGGER),
  BE(f_Union,			EDIF_TOK_UNION),
  BE(f_View,			EDIF_TOK_VIEW),
  BE(f_ViewList,		EDIF_TOK_VIEWLIST),
  BE(f_ViewMap,			EDIF_TOK_VIEWMAP),
  BE(f_ViewRef,			EDIF_TOK_VIEWREF),
  BE(f_Visible,			EDIF_TOK_VISIBLE),
  BE(f_VoltageMap,		EDIF_TOK_VOLTAGEMAP),
  BE(f_WaveValue,		EDIF_TOK_WAVEVALUE),
  BE(f_Weak,			EDIF_TOK_WEAK),
  BE(f_WeakJoined,		EDIF_TOK_WEAKJOINED),
  BE(f_When,			EDIF_TOK_WHEN),
  BE(f_Written,			EDIF_TOK_WRITTEN)
};
static int BinderDefSize = sizeof(BinderDef) / sizeof(Binder);
/*
 *	Keyword table:
 *
 *	  This hash table holds all strings which may have to be matched
 *	to. WARNING: it is assumed that there is no overlap of the 'token'
 *	and 'context' strings.
 */
typedef struct Keyword {
  struct Keyword *Next;	 	/* pointer to next entry */
  char *String;			/* pointer to associated string */
} Keyword;
#define	KEYWORD_HASH	127	/* hash table size */
static Keyword *KeywordTable[KEYWORD_HASH];
/*
 *	Enter keyword:
 *
 *	  The passed string is entered into the keyword hash table.
 */
static void EnterKeyword(char * str)
{
  /* 
   *	Locals.
   */
  register Keyword *key;
  register unsigned int hsh;
  register char *cp;
  /*
   *	Create the hash code, and add an entry to the table.
   */
  for (hsh = 0, cp = str; *cp; hsh += hsh + *cp++);
  hsh %= KEYWORD_HASH;
  key = (Keyword *) Malloc(sizeof(Keyword));
  key->Next = KeywordTable[hsh];
  (KeywordTable[hsh] = key)->String = str;
}
/*
 *	Find keyword:
 *
 *	  The passed string is located within the keyword table. If an
 *	entry exists, then the value of the keyword string is returned. This
 *	is real useful for doing string comparisons by pointer value later.
 *	If there is no match, a NULL is returned.
 */
static char *FindKeyword(char * str)
{
  /*
   *	Locals.
   */
  register Keyword *wlk,*owk;
  register unsigned int hsh;
  register char *cp;
  char lower[IDENT_LENGTH + 1];
  /*
   *	Create a lower case copy of the string.
   */
  for (cp = lower; *str;)
    if (isupper( (int) *str))
      *cp++ = tolower( (int) *str++);
    else
      *cp++ = *str++;
  *cp = '\0';
  /*
   *	Search the hash table for a match.
   */
  for (hsh = 0, cp = lower; *cp; hsh += hsh + *cp++);
  hsh %= KEYWORD_HASH;
  for (owk = NULL, wlk = KeywordTable[hsh]; wlk; wlk = (owk = wlk)->Next)
    if (!strcmp(wlk->String,lower)){
      /*
       *	Readjust the LRU.
       */
      if (owk){
      	owk->Next = wlk->Next;
      	wlk->Next = KeywordTable[hsh];
      	KeywordTable[hsh] = wlk;
      }
      return (wlk->String);
    }
  return (NULL);
}
/*
 *	Token hash table.
 */
#define	TOKEN_HASH	51
static Token *TokenHash[TOKEN_HASH];
/*
 *	Find token:
 *
 *	  A pointer to the token of the passed code is returned. If
 *	no such beastie is present a NULL is returned instead.
 */
static Token *FindToken(register int cod)
{
  /*
   *	Locals.
   */
  register Token *wlk,*owk;
  register unsigned int hsh;
  /*
   *	Search the hash table for a matching token.
   */
  hsh = cod % TOKEN_HASH;
  for (owk = NULL, wlk = TokenHash[hsh]; wlk; wlk = (owk = wlk)->Next)
    if (cod == wlk->Code){
      if (owk){
      	owk->Next = wlk->Next;
      	wlk->Next = TokenHash[hsh];
      	TokenHash[hsh] = wlk;
      }
      break;
    }
  return (wlk);
}
/*
 *	Context hash table.
 */
#define	CONTEXT_HASH	127
static Context *ContextHash[CONTEXT_HASH];
/*
 *	Find context:
 *
 *	  A pointer to the context of the passed code is returned. If
 *	no such beastie is present a NULL is returned instead.
 */
static Context *FindContext(register int cod)
{
  /*
   *	Locals.
   */
  register Context *wlk,*owk;
  register unsigned int hsh;
  /*
   *	Search the hash table for a matching context.
   */
  hsh = cod % CONTEXT_HASH;
  for (owk = NULL, wlk = ContextHash[hsh]; wlk; wlk = (owk = wlk)->Next)
    if (cod == wlk->Code){
      if (owk){
      	owk->Next = wlk->Next;
      	wlk->Next = ContextHash[hsh];
      	ContextHash[hsh] = wlk;
      }
      break;
    }
  return (wlk);
}
/*
 *	Parser state variables.
 */
static FILE *Input = NULL;		/* input stream */
static FILE *Error = NULL;		/* error stream */
static char *InFile;			/* file name on the input stream */
static long LineNumber;			/* current input line number */
static ContextCar *CSP = NULL;		/* top of context stack */
static char yytext[IDENT_LENGTH + 1];	/* token buffer */
static char CharBuf[IDENT_LENGTH + 1];	/* garbage buffer */
/*
 *	Token stacking variables.
 */
#ifdef	DEBUG
#define	TS_DEPTH	8
#define	TS_MASK		(TS_DEPTH - 1)
static unsigned int TSP = 0;		/* token stack pointer */
static char *TokenStack[TS_DEPTH];	/* token name strings */
static short TokenType[TS_DEPTH];	/* token types */
/*
 *	Stack:
 *
 *	  Add a token to the debug stack. The passed string and type are
 *	what is to be pushed.
 */
static Stack(char * str, int typ)
{
  /*
   *	Free any previous string, then push.
   */
  if (TokenStack[TSP & TS_MASK])
    Free(TokenStack[TSP & TS_MASK]);
  TokenStack[TSP & TS_MASK] = strcpy((char *)Malloc(strlen(str) + 1),str);
  TokenType[TSP & TS_MASK] = typ;
  TSP += 1;
}
/*
 *	Dump stack:
 *
 *	  This displays the last set of accumulated tokens.
 */
static DumpStack()
{
  /*
   *	Locals.
   */
  register int i;
  register Context *cxt;
  register Token *tok;
  register char *nam;
  /*
   *	Run through the list displaying the oldest first.
   */
  fprintf(Error,"\n\n");
  for (i = 0; i < TS_DEPTH; i += 1)
    if (TokenStack[(TSP + i) & TS_MASK]){
      /*
       *	Get the type name string.
       */
      if (cxt = FindContext(TokenType[(TSP + i) & TS_MASK]))
        nam = cxt->Name;
      else if (tok = FindToken(TokenType[(TSP + i) & TS_MASK]))
        nam = tok->Name;
      else switch (TokenType[(TSP + i) & TS_MASK]){
      	case EDIF_TOK_IDENT:	nam = "IDENT";		break;
      	case EDIF_TOK_INT:	    nam = "INT";		break;
      	case EDIF_TOK_KEYWORD:	nam = "KEYWORD";	break;
      	case EDIF_TOK_STR:	    nam = "STR";		break;
      	default:	            nam = "?";	    	break;
      }
      /*
       *	Now print the token state.
       */
      fprintf(Error,"%2d %-16.16s '%s'\n",TS_DEPTH - i,nam,
        TokenStack[(TSP + i) & TS_MASK]);
    }
  fprintf(Error,"\n");
}
#else
#define	Stack(s,t)
#endif	/* DEBUG */
/*
 *	yyerror:
 *
 *	  Standard error reporter, it prints out the passed string
 *	preceeded by the current filename and line number.
 */
static void yyerror(const char *ers)
{
#ifdef	DEBUG
  DumpStack();
#endif	/* DEBUG */
  fprintf(Error,"%s, line %ld: %s\n",InFile,LineNumber,ers);
}
/*
 *	String bucket definitions.
 */
#define	BUCKET_SIZE	64
typedef struct Bucket {
  struct Bucket *Next;			/* pointer to next bucket */
  int Index;				/* pointer to next free slot */
  char Data[BUCKET_SIZE];		/* string data */
} Bucket;
static Bucket *CurrentBucket = NULL;	/* string bucket list */
static int StringSize = 0;		/* current string length */
/*
 *	Push string:
 *
 *	  This adds the passed charater to the current string bucket.
 */
static void PushString(char chr)
{
  /*
   *	Locals.
   */
  register Bucket *bck;
  /*
   *	Make sure there is room for the push.
   */
  if ((bck = CurrentBucket)->Index >= BUCKET_SIZE){
    bck = (Bucket *) Malloc(sizeof(Bucket));
    bck->Next = CurrentBucket;
    (CurrentBucket = bck)->Index = 0;
  }
  /*
   *	Push the character.
   */
  bck->Data[bck->Index++] = chr;
  StringSize += 1;
}
/*
 *	Form string:
 *
 *	  This converts the current string bucket into a real live string,
 *	whose pointer is returned.
 */
static char *FormString()
{
  /*
   *	Locals.
   */
  register Bucket *bck;
  register char *cp;
  /*
   *	Allocate space for the string, set the pointer at the end.
   */
  cp = (char *) Malloc(StringSize + 1);
  
  cp += StringSize;
  *cp-- = '\0';
  /*
   *	Yank characters out of the bucket.
   */
  for (bck = CurrentBucket; bck->Index || bck->Next;){
    if (!bck->Index){
      CurrentBucket = bck->Next;
      Free(bck);
      bck = CurrentBucket;
    }
    *cp-- = bck->Data[--bck->Index];
  }
  /* reset buffer size  to zero */
  StringSize =0;
  return (cp + 1);
}
/*
 *	Parse EDIF:
 *
 *	  This builds the context tree and then calls the real parser.
 *	It is passed two file streams, the first is where the input comes
 *	from; the second is where error messages get printed.
 */
void ParseEDIF(char* filename,FILE* err)
{
  /*
   *	Locals.
   */
  register int i;
  static int ContextDefined = 1;
  /*
   *	Set up the file state to something useful.
   */
  InFile = filename;
  Input = fopen(filename,"r");
  Error = err;
  LineNumber = 1;
  /*
   *	Define both the enabled token and context strings.
   */
  if (ContextDefined){
    for (i = TokenDefSize; i--; EnterKeyword(TokenDef[i].Name)){
      register unsigned int hsh;
      hsh = TokenDef[i].Code % TOKEN_HASH;
      TokenDef[i].Next = TokenHash[hsh];
      TokenHash[hsh] = &TokenDef[i];
    }
    for (i = ContextDefSize; i--; EnterKeyword(ContextDef[i].Name)){
      register unsigned int hsh;
      hsh = ContextDef[i].Code % CONTEXT_HASH;
      ContextDef[i].Next = ContextHash[hsh];
      ContextHash[hsh] = &ContextDef[i];
    }
    /*
     *	Build the context tree.
     */
    for (i = BinderDefSize; i--;){
      register Context *cxt;
      register int j;
      /*
       *	Define the current context to have carriers bound to it.
       */
      cxt = FindContext(BinderDef[i].Origin);
      for (j = BinderDef[i].FollowerSize; j--;){
        register ContextCar *cc;
        /*
         *	Add carriers to the current context.
         */
        cc = (ContextCar *) Malloc(sizeof(ContextCar));
        cc->Next = cxt->Context;
        (cxt->Context = cc)->Context =
          FindContext(ABS(BinderDef[i].Follower[j]));
        cc->u.Single = BinderDef[i].Follower[j] < 0;
      }
    }
    /*
     *	Build the token tree.
     */
    for (i = TieDefSize; i--;){
      register Context *cxt;
      register int j;
      /*
       *	Define the current context to have carriers bound to it.
       */
      cxt = FindContext(TieDef[i].Origin);
      for (j = TieDef[i].EnableSize; j--;){
        register TokenCar *tc;
        /*
         *	Add carriers to the current context.
         */
        tc = (TokenCar *) Malloc(sizeof(TokenCar));
        tc->Next = cxt->Token;
        (cxt->Token = tc)->Token = FindToken(TieDef[i].Enable[j]);
      }
    }
    /*
     *	Put a bogus context on the stack which has 'EDIF' as its
     *	follower.
     */
    CSP = (ContextCar *) Malloc(sizeof(ContextCar));
    CSP->Next = NULL;
    CSP->Context = FindContext(0);
    CSP->u.Used = NULL;
    ContextDefined = 0;
  }
  /*
   *	Create an initial, empty string bucket.
   */
  CurrentBucket = (Bucket *) Malloc(sizeof(Bucket));
  CurrentBucket->Next = 0;
  CurrentBucket->Index = 0;
  /*
   *	Fill the token stack with NULLs if debugging is enabled.
   */
#ifdef	DEBUG
  for (i = TS_DEPTH; i--; TokenStack[i] = NULL)
    if (TokenStack[i])
      Free(TokenStack[i]);
  TSP = 0;
#endif	/* DEBUG */
  /*
   *	Go parse things!
   */
  edifparse();
}
/*
 *	Match token:
 *
 *	  The passed string is looked up in the current context's token
 *	list to see if it is enabled. If so the token value is returned,
 *	if not then zero.
 */
static int MatchToken(register char * str)
{
  /*
   *	Locals.
   */
  register TokenCar *wlk,*owk;
  /*
   *	Convert the string to the proper form, then search the token
   *	carrier list for a match.
   */
  str = FindKeyword(str);
  for (owk = NULL, wlk = CSP->Context->Token; wlk; wlk = (owk = wlk)->Next)
    if (str == wlk->Token->Name){
      if (owk){
        owk->Next = wlk->Next;
        wlk->Next = CSP->Context->Token;
        CSP->Context->Token = wlk;
      }
      return (wlk->Token->Code);
    }
  return (0);
}
/*
 *	Match context:
 *
 *	  If the passed keyword string is within the current context, the
 *	new context is pushed and token value is returned. A zero otherwise.
 */
static int MatchContext(register char * str)
{
  /*
   *	Locals.
   */
  register ContextCar *wlk,*owk;
  /*
   *	See if the context is present.
   */
  str = FindKeyword(str);
  for (owk = NULL, wlk = CSP->Context->Context; wlk; wlk = (owk = wlk)->Next)
    if (str == wlk->Context->Name){
      if (owk){
      	owk->Next = wlk->Next;
      	wlk->Next = CSP->Context->Context;
      	CSP->Context->Context = wlk;
      }
      /*
       *	If a single context, make sure it isn't already used.
       */
      if (wlk->u.Single){
      	register UsedCar *usc;
      	for (usc = CSP->u.Used; usc; usc = usc->Next)
      	  if (usc->Code == wlk->Context->Code)
      	    break;
      	if (usc){
      	  sprintf(CharBuf,"'%s' is used more than once within '%s'",
      	    str,CSP->Context->Name);
      	  yyerror(CharBuf);
      	} else {
      	  usc = (UsedCar *) Malloc(sizeof(UsedCar));
      	  usc->Next = CSP->u.Used;
      	  (CSP->u.Used = usc)->Code = wlk->Context->Code;
      	}
      }
      /*
       *	Push the new context.
       */
      owk = (ContextCar *) Malloc(sizeof(ContextCar));
      owk->Next = CSP;
      (CSP = owk)->Context = wlk->Context;
      owk->u.Used = NULL;
      return (wlk->Context->Code);
    }
  return (0);
}
/*
 *	PopC:
 *
 *	  This pops the current context.
 */
static void PopC()
{
  /*
   *	Locals.
   */
  register UsedCar *usc;
  register ContextCar *csp;
  /*
   *	Release single markers and pop context.
   */
  while ( (usc = CSP->u.Used) ){
    CSP->u.Used = usc->Next;
    Free(usc);
  }
  csp = CSP->Next;
  Free(CSP);
  CSP = csp;
}
/*
 *	Lexical analyzer states.
 */
#define	L_START		0
#define	L_INT		1
#define	L_IDENT		2
#define	L_KEYWORD	3
#define	L_STRING	4
#define	L_KEYWORD2	5
#define	L_ASCIICHAR	6
#define	L_ASCIICHAR2	7
/*
 *	yylex:
 *
 *	  This is the lexical analyzer called by the YACC/BISON parser.
 *	It returns a pretty restricted set of token types and does the
 *	context movement when acceptable keywords are found. The token
 *	value returned is a NULL terminated string to allocated storage
 *	(ie - it should get released some time) with some restrictions.
 *	  The token value for integers is strips a leading '+' if present.
 *	String token values have the leading and trailing '"'-s stripped.
 *	'%' conversion characters in string values are passed converted.
 *	The '(' and ')' characters do not have a token value.
 */
static int yylex()
{
  /*
   *	Locals.
   */
  register int c,s,l;
  /*
   *	Keep on sucking up characters until we find something which
   *	explicitly forces us out of this function.
   */
  for (s = L_START, l = 0; 1;){
    yytext[l++] = c = Getc(Input);
    switch (s){
      /*
       *	Starting state, look for something resembling a token.
       */
      case L_START:
        if (isdigit(c) || c == '-')
          s = L_INT;
        else if (isalpha(c) || c == '&')
          s = L_IDENT;
        else if (isspace(c)){
          if (c == '\n')
            LineNumber += 1;
          l = 0;
        } else if (c == '('){
          l = 0;
          s = L_KEYWORD;
        } else if (c == '"')
          s = L_STRING;
        else if (c == '+'){
          l = 0;				/* strip '+' */
          s = L_INT;
        } else if (c == EOF)
          return ('\0');
        else {
          yytext[1] = '\0';
          Stack(yytext,c);
          return (c);
        }
        break;
      /*
       *	Suck up the integer digits.
       */
      case L_INT:
        if (isdigit(c))
          break;
        Ungetc(c);
        yytext[--l] = '\0';
        yylval.s = strcpy((char *)Malloc(l + 1),yytext);
        Stack(yytext,EDIF_TOK_INT);
        return (EDIF_TOK_INT);
      /*
       *	Grab an identifier, see if the current context enables
       *	it with a specific token value.
       */
      case L_IDENT:
        if (isalpha(c) || isdigit(c) || c == '_')
          break;
        Ungetc(c);
        yytext[--l] = '\0';
        if (CSP->Context->Token && (c = MatchToken(yytext))){
          Stack(yytext,c);
          return (c);
        }
        yylval.s = strcpy((char *)Malloc(l + 1),yytext);
        Stack(yytext, EDIF_TOK_IDENT);
        return (EDIF_TOK_IDENT);
      /*
       *	Scan until you find the start of an identifier, discard
       *	any whitespace found. On no identifier, return a '('.
       */
      case L_KEYWORD:
        if (isalpha(c) || c == '&'){
          s = L_KEYWORD2;
          break;
        } else if (isspace(c)){
          l = 0;
          break;
        }
        Ungetc(c);
        Stack("(",'(');
        return ('(');
      /*
       *	Suck up the keyword identifier, if it matches the set of
       *	allowable contexts then return its token value and push
       *	the context, otherwise just return the identifier string.
       */
      case L_KEYWORD2:
        if (isalpha(c) || isdigit(c) || c == '_')
          break;
        Ungetc(c);
        yytext[--l] = '\0';
        if ( (c = MatchContext(yytext)) ){
          Stack(yytext,c);
          return (c);
        }
        yylval.s = strcpy((char *)Malloc(l + 1),yytext);
        Stack(yytext, EDIF_TOK_KEYWORD);
        return (EDIF_TOK_KEYWORD);
      /*
       *	Suck up string characters but once resolved they should
       *	be deposited in the string bucket because they can be
       *	arbitrarily long.
       */
      case L_STRING:
        if (c == '\n')
          LineNumber += 1;
        else if (c == '\r')
          ;
        else if (c == '"' || c == EOF){
          yylval.s = FormString();
          Stack(yylval.s, EDIF_TOK_STR);
          return (EDIF_TOK_STR);
        } else if (c == '%')
          s = L_ASCIICHAR;
        else
          PushString(c);
        l = 0;
        break;
      /*
       *	Skip white space and look for integers to be pushed
       *	as characters.
       */
      case L_ASCIICHAR:
        if (isdigit(c)){
          s = L_ASCIICHAR2;
          break;
        } else if (c == '%' || c == EOF)
          s = L_STRING;
        else if (c == '\n')
          LineNumber += 1;
        l = 0;
        break;
      /*
       *	Convert the accumulated integer into a char and push.
       */
      case L_ASCIICHAR2:
        if (isdigit(c))
          break;
        Ungetc(c);
        yytext[--l] = '\0';
        PushString(atoi(yytext));
        s = L_ASCIICHAR;
        l = 0;
        break;
    }
  }
}
