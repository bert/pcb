/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,2005,2006 Thomas Nau
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

/* file save, load, merge ... routines
 * getpid() needs a cast to (int) to get rid of compiler warnings
 * on several architectures
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "global.h"

#include <dirent.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include <time.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <sys/stat.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


#include "buffer.h"
#include "change.h"
#include "create.h"
#include "crosshair.h"
#include "data.h"
#include "edif_parse.h"
#include "error.h"
#include "file.h"
#include "hid.h"
#include "misc.h"
#include "move.h"
#include "mymem.h"
#include "parse_l.h"
#include "polygon.h"
#include "rats.h"
#include "remove.h"
#include "set.h"
#include "strflags.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

#if !defined(HAS_ATEXIT) && !defined(HAS_ON_EXIT)
/* ---------------------------------------------------------------------------
 * some local identifiers for OS without an atexit() or on_exit()
 * call
 */
static char TMPFilename[80];
#endif

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void PrintQuotedString (FILE *, char *);
static void WritePCBInfoHeader (FILE *);
static void WritePCBDataHeader (FILE *);
static void WritePCBFontData (FILE *);
static void WriteViaData (FILE *, DataTypePtr);
static void WritePCBRatData (FILE *);
static void WriteElementData (FILE *, DataTypePtr);
static void WriteLayerData (FILE *, Cardinal, LayerTypePtr);
static int WritePCB (FILE *);
static int WritePCBFile (char *);
static int WritePipe (char *, bool);
static int ParseLibraryTree (void);
static int LoadNewlibFootprintsFromDir(char *path, char *toppath);
static char *pcb_basename (char *p);

/* ---------------------------------------------------------------------------
 * Flag helper functions
 */

#define F2S(OBJ, TYPE) flags_to_string ((OBJ)->Flags, TYPE)

/* --------------------------------------------------------------------------- */

static int
string_cmp (const char *a, const char *b)
{
  while (*a && *b)
    {
      if (isdigit ((int) *a) && isdigit ((int) *b))
	{
	  int ia = atoi (a);
	  int ib = atoi (b);
	  if (ia != ib)
	    return ia - ib;
	  while (isdigit ((int) *a) && *(a+1))
	    a++;
	  while (isdigit ((int) *b) && *(b+1))
	    b++;
	}
      else if (tolower ((int) *a) != tolower ((int) *b))
	return tolower ((int) *a) - tolower ((int) *b);
      a++;
      b++;
    }
  if (*a)
    return 1;
  if (*b)
    return -1;
  return 0;
}

static int netlist_sort_offset = 0;

static int
netlist_sort (const void *va, const void *vb)
{
  LibraryMenuType *am = (LibraryMenuType *) va;
  LibraryMenuType *bm = (LibraryMenuType *) vb;
  char *a = am->Name;
  char *b = bm->Name;
  if (*a == '~')
    a++;
  if (*b == '~')
    b++;
  return string_cmp (a, b);
}

static int
netnode_sort (const void *va, const void *vb)
{
  LibraryEntryType *am = (LibraryEntryType *) va;
  LibraryEntryType *bm = (LibraryEntryType *) vb;
  char *a = am->ListEntry;
  char *b = bm->ListEntry;
  return string_cmp (a, b);
}

static void
sort_library (LibraryTypePtr lib)
{
  int i;
  qsort (lib->Menu, lib->MenuN, sizeof (lib->Menu[0]), netlist_sort);
  for (i = 0; i < lib->MenuN; i++)
    qsort (lib->Menu[i].Entry,
	   lib->Menu[i].EntryN, sizeof (lib->Menu[i].Entry[0]), netnode_sort);
}

void
sort_netlist ()
{
  netlist_sort_offset = 2;
  sort_library (&(PCB->NetlistLib));
  netlist_sort_offset = 0;
}

/* ---------------------------------------------------------------------------
 * opens a file and check if it exists
 */
FILE *
CheckAndOpenFile (char *Filename, bool Confirm, bool AllButton,
		  bool * WasAllButton, bool * WasCancelButton)
{
  FILE *fp = NULL;
  struct stat buffer;
  char message[MAXPATHLEN + 80];
  int response;

  if (Filename && *Filename)
    {
      if (!stat (Filename, &buffer) && Confirm)
	{
	  sprintf (message, _("File '%s' exists, use anyway?"), Filename);
	  if (WasAllButton)
	    *WasAllButton = false;
	  if (WasCancelButton)
	    *WasCancelButton = false;
	  if (AllButton)
	    response =
	      gui->confirm_dialog (message, "Cancel", "Ok",
				   AllButton ? "Sequence OK" : 0);
	  else
	    response =
	      gui->confirm_dialog (message, "Cancel", "Ok", "Sequence OK");

	  switch (response)
	    {
	    case 2:
	      if (WasAllButton)
		*WasAllButton = true;
	      break;
	    case 0:
	      if (WasCancelButton)
		*WasCancelButton = true;
	    }
	}
      if ((fp = fopen (Filename, "w")) == NULL)
	OpenErrorMessage (Filename);
    }
  return (fp);
}

/* ---------------------------------------------------------------------------
 * opens a file for saving connection data
 */
FILE *
OpenConnectionDataFile (void)
{
  char *fname;
  FILE *fp;
  static char * default_file = NULL;
  bool result;		/* not used */

  /* CheckAndOpenFile deals with the case where fname already exists */
  fname = gui->fileselect (_("Save Connection Data As ..."),
			   _("Choose a file to save all connection data to."),
			   default_file, ".net", "connection_data",
			   0);
  if (fname == NULL)
    return NULL;

  if (default_file != NULL)
    {
      free (default_file);
      default_file = NULL;
    }

  if (fname && *fname)
    default_file = strdup (fname);

  fp = CheckAndOpenFile (fname, true, false, &result, NULL);
  free (fname);

  return fp;
}

/* ---------------------------------------------------------------------------
 * save elements in the current buffer
 */
int
SaveBufferElements (char *Filename)
{
  int result;

  if (SWAP_IDENT)
    SwapBuffers ();
  result = WritePipe (Filename, false);
  if (SWAP_IDENT)
    SwapBuffers ();
  return (result);
}

/* ---------------------------------------------------------------------------
 * save PCB
 */
int
SavePCB (char *Filename)
{
  int retcode;
  char *copy;

  if (!(retcode = WritePipe (Filename, true)))
    {
      /* thanks to Nick Bailey for the bug-fix;
       * first of all make a copy of the passed filename because
       * it might be identical to 'PCB->Filename'
       */
      copy = strdup (Filename);
      SaveFree (PCB->Filename);
      PCB->Filename = copy;
      SetChangedFlag (false);
    }
  return (retcode);
}

/* ---------------------------------------------------------------------------
 * set the route style to the first one, if the current one doesn't
 * happen to match any.  This way, "revert" won't change the route
 * style.
 */
static void
set_some_route_style ()
{
  if (hid_get_flag ("style"))
    return;
  SetLineSize (PCB->RouteStyle[0].Thick);
  SetViaSize (PCB->RouteStyle[0].Diameter, true);
  SetViaDrillingHole (PCB->RouteStyle[0].Hole, true);
  SetKeepawayWidth (PCB->RouteStyle[0].Keepaway);
}

/* ---------------------------------------------------------------------------
 * load PCB
 * parse the file with enabled 'PCB mode' (see parser)
 * if successful, update some other stuff
 */
int
LoadPCB (char *Filename)
{
  PCBTypePtr newPCB = CreateNewPCB (false);
  bool units_mm;
#ifdef DEBUG
  double elapsed;
  clock_t start, end;

  start = clock ();
#endif

  /* new data isn't added to the undo list */
  if (!ParsePCB (newPCB, Filename))
    {
      RemovePCB (PCB);
      PCB = newPCB;

      CreateNewPCBPost (PCB, 0);
      ResetStackAndVisibility ();

      /* update cursor location */
      Crosshair.X = MAX (0, MIN (PCB->CursorX, (LocationType) PCB->MaxWidth));
      Crosshair.Y =
	MAX (0, MIN (PCB->CursorY, (LocationType) PCB->MaxHeight));

      Xorig = Crosshair.X - TO_PCB (Output.Width / 2);
      Yorig = Crosshair.Y - TO_PCB (Output.Height / 2);

      /* update cursor confinement and output area (scrollbars) */
      ChangePCBSize (PCB->MaxWidth, PCB->MaxHeight);

      /* create default font if necessary */
      if (!PCB->Font.Valid)
	{
	  Message (_
		   ("File '%s' has no font information, using default font\n"),
		   Filename);
	  CreateDefaultFont ();
	}

      /* clear 'changed flag' */
      SetChangedFlag (false);
      PCB->Filename = strdup (Filename);
      /* just in case a bad file saved file is loaded */

      units_mm = (PCB->Grid != (int) PCB->Grid) ? true : false;

      Settings.grid_units_mm = units_mm;

      sort_netlist ();

      set_some_route_style ();

      hid_action ("PCBChanged");

#ifdef DEBUG
      end = clock ();
      elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
      gui->log ("Loading file %s took %f seconds of CPU time\n",
		Filename, elapsed);
#endif

      return (0);
    }
  hid_action ("PCBChanged");

  /* release unused memory */
  RemovePCB (newPCB);
  return (1);
}

/* ---------------------------------------------------------------------------
 * functions for loading elements-as-pcb
 */

extern	PCBTypePtr		yyPCB;
extern	DataTypePtr		yyData;
extern	FontTypePtr		yyFont;

void
PreLoadElementPCB ()
{

  if (!yyPCB)
    return;

  yyFont = &yyPCB->Font;
  yyData = yyPCB->Data;
  yyData->pcb = (void *)yyPCB;
  yyData->LayerN = 0;
}

void
PostLoadElementPCB ()
{
  PCBTypePtr pcb_save = PCB;
  ElementTypePtr e;

  if (!yyPCB)
    return;

  CreateNewPCBPost (yyPCB, 0);
  ParseGroupString("1,c:2,s", &yyPCB->LayerGroups, yyData->LayerN);
  e = yyPCB->Data->Element; /* we know there's only one */
  PCB = yyPCB;
  MoveElementLowLevel (yyPCB->Data,
		       e, -e->BoundingBox.X1, -e->BoundingBox.Y1);
  PCB = pcb_save;
  yyPCB->MaxWidth = e->BoundingBox.X2;
  yyPCB->MaxHeight = e->BoundingBox.Y2;
}

/* ---------------------------------------------------------------------------
 * writes the quoted string created by another subroutine
 */
static void
PrintQuotedString (FILE * FP, char *S)
{
  static DynamicStringType ds;

  CreateQuotedString (&ds, S);
  fputs (ds.Data, FP);
}

/* ---------------------------------------------------------------------------
 * writes out an attribute list
 */
static void
WriteAttributeList (FILE * FP, AttributeListTypePtr list, char *prefix)
{
  int i;

  for (i = 0; i < list->Number; i++)
    fprintf (FP, "%sAttribute(\"%s\" \"%s\")\n",
	     prefix, list->List[i].name, list->List[i].value);
}

/* ---------------------------------------------------------------------------
 * writes layout header information
 * date, UID and name of user
 */
static void
WritePCBInfoHeader (FILE * FP)
{
#ifdef HAVE_GETPWUID
  struct passwd *pwentry;
#endif

#ifdef HAVE_GETHOSTNAME
  static struct hostent *hostentry = NULL;
  char hostname[256];
#endif
  time_t currenttime;

  /* write some useful comments */
  currenttime = time (NULL);
  fprintf (FP, "# release: %s " VERSION "\n", Progname);
  fprintf (FP, "# date:    %s", asctime (localtime (&currenttime)));

#ifdef HAVE_GETPWUID
  pwentry = getpwuid (getuid ());
  fprintf (FP, "# user:    %s (%s)\n", pwentry->pw_name, pwentry->pw_gecos);
#else
  fprintf (FP, "# user:    Unknown\n");
#endif

#ifdef HAVE_GETHOSTNAME
  if (gethostname (hostname, 255) != -1)
    {
      if (hostentry == NULL)
	hostentry = gethostbyname (hostname);
      fprintf (FP, "# host:    %s\n",
	       hostentry ? hostentry->h_name : hostname);
    }
#else
  fprintf (FP, "# host:    Unknown\n");
#endif
}

/* ---------------------------------------------------------------------------
 * writes data header
 * the name of the PCB, cursor location, zoom and grid
 * layergroups and some flags
 */
static void
WritePCBDataHeader (FILE * FP)
{
  Cardinal group;

  /*
   * ************************** README *******************
   * ************************** README *******************
   *
   * If the file format is modified in any way, update
   * PCB_FILE_VERSION in file.h
   *  
   * ************************** README *******************
   * ************************** README *******************
   */

  fprintf (FP, "\n# To read pcb files, the pcb version (or the git source date) must be >= the file version\n");
  fprintf (FP, "FileVersion[%i]\n", PCB_FILE_VERSION);

  fputs ("\nPCB[", FP);
  PrintQuotedString (FP, EMPTY (PCB->Name));
  fprintf (FP, " %i %i]\n\n", (int) PCB->MaxWidth, (int) PCB->MaxHeight);
  fprintf (FP, "Grid[%s %i %i %i]\n",
	   c_dtostr (PCB->Grid), (int) PCB->GridOffsetX,
	   (int) PCB->GridOffsetY, (int) Settings.DrawGrid);
  fprintf (FP, "Cursor[%i %i %s]\n", (int) TO_PCB_X (Output.Width / 2),
	   (int) TO_PCB_Y (Output.Height / 2), c_dtostr (PCB->Zoom));
  fprintf (FP, "PolyArea[%s]\n", c_dtostr (PCB->IsleArea));
  fprintf (FP, "Thermal[%s]\n", c_dtostr (PCB->ThermScale));
  fprintf (FP, "DRC[%i %i %i %i %i %i]\n", PCB->Bloat, PCB->Shrink,
	   PCB->minWid, PCB->minSlk, PCB->minDrill, PCB->minRing);
  fprintf (FP, "Flags(%s)\n", pcbflags_to_string(PCB->Flags));
  fprintf (FP, "Groups(\"%s\")\n", LayerGroupsToString (&PCB->LayerGroups));
  fputs ("Styles[\"", FP);
  for (group = 0; group < NUM_STYLES - 1; group++)
    fprintf (FP, "%s,%i,%i,%i,%i:", PCB->RouteStyle[group].Name,
	     PCB->RouteStyle[group].Thick,
	     PCB->RouteStyle[group].Diameter,
	     PCB->RouteStyle[group].Hole, PCB->RouteStyle[group].Keepaway);
  fprintf (FP, "%s,%i,%i,%i,%i\"]\n\n", PCB->RouteStyle[group].Name,
	   PCB->RouteStyle[group].Thick,
	   PCB->RouteStyle[group].Diameter,
	   PCB->RouteStyle[group].Hole, PCB->RouteStyle[group].Keepaway);
}

/* ---------------------------------------------------------------------------
 * writes font data of non empty symbols
 */
static void
WritePCBFontData (FILE * FP)
{
  Cardinal i, j;
  LineTypePtr line;
  FontTypePtr font;

  for (font = &PCB->Font, i = 0; i <= MAX_FONTPOSITION; i++)
    {
      if (!font->Symbol[i].Valid)
	continue;

      if (isprint (i) && font->Symbol[i].Delta % 100 == 0)
	fprintf (FP, "Symbol('%c' %i)\n(\n",
		 (char) i, (int) font->Symbol[i].Delta / 100);
      else if (isprint (i))
	fprintf (FP, "Symbol['%c' %i]\n(\n",
		 (char) i, (int) font->Symbol[i].Delta);
      else
	fprintf (FP, "Symbol[%i %i]\n(\n", i, (int) font->Symbol[i].Delta);

      line = font->Symbol[i].Line;
      for (j = font->Symbol[i].LineN; j; j--, line++)
	{
	  if (line->Point1.X % 100 == 0
	      && line->Point1.Y % 100 == 0
	      && line->Point2.X % 100 == 0
	      && line->Point2.Y % 100 == 0 && line->Thickness % 100 == 0)
	    fprintf (FP, "\tSymbolLine(%i %i %i %i %i)\n",
		     line->Point1.X / 100, line->Point1.Y / 100,
		     line->Point2.X / 100, line->Point2.Y / 100,
		     line->Thickness / 100);
	  else
	    fprintf (FP, "\tSymbolLine[%i %i %i %i %i]\n",
		     line->Point1.X, line->Point1.Y,
		     line->Point2.X, line->Point2.Y, line->Thickness);
	}
      fputs (")\n", FP);
    }
}

/* ---------------------------------------------------------------------------
 * writes via data
 */
static void
WriteViaData (FILE * FP, DataTypePtr Data)
{
  int n;
  /* write information about vias */
  for (n = 0; n < Data->ViaN; n++)
    {
      PinTypePtr via = &Data->Via[n];
      fprintf (FP, "Via[%i %i %i %i %i %i ",
	       via->X, via->Y,
	       via->Thickness, via->Clearance, via->Mask, via->DrillingHole);
      PrintQuotedString (FP, EMPTY (via->Name));
      fprintf (FP, " %s]\n", F2S (via, VIA_TYPE));
    }
}

/* ---------------------------------------------------------------------------
 * writes rat-line data
 */
static void
WritePCBRatData (FILE * FP)
{
  int n;
  /* write information about rats */
  for (n = 0; n < PCB->Data->RatN; n++)
    {
      RatTypePtr line = &PCB->Data->Rat[n];
      fprintf (FP, "Rat[%i %i %i %i %i %i ",
	       (int) line->Point1.X, (int) line->Point1.Y,
	       (int) line->group1, (int) line->Point2.X,
	       (int) line->Point2.Y, (int) line->group2);
      fprintf (FP, " %s]\n", F2S (line, RATLINE_TYPE));
    }
}

/* ---------------------------------------------------------------------------
 * writes netlist data
 */
static void
WritePCBNetlistData (FILE * FP)
{
  /* write out the netlist if it exists */
  if (PCB->NetlistLib.MenuN)
    {
      int n, p;
      fprintf (FP, "NetList()\n(\n");

      for (n = 0; n < PCB->NetlistLib.MenuN; n++)
	{
	  LibraryMenuTypePtr menu = &PCB->NetlistLib.Menu[n];
	  fprintf (FP, "\tNet(");
	  PrintQuotedString(FP, &menu->Name[2]);
	  fprintf (FP, " ");
	  PrintQuotedString(FP, UNKNOWN (menu->Style));
	  fprintf (FP, ")\n\t(\n");
	  for (p = 0; p < menu->EntryN; p++)
	    {
	      LibraryEntryTypePtr entry = &menu->Entry[p];
	      fprintf (FP, "\t\tConnect(");
	      PrintQuotedString (FP, entry->ListEntry);
	      fprintf (FP, ")\n");
	    }
	  fprintf (FP, "\t)\n");
	}
      fprintf (FP, ")\n");
    }
}

/* ---------------------------------------------------------------------------
 * writes element data
 */
static void
WriteElementData (FILE * FP, DataTypePtr Data)
{
  int n, p;
  for (n = 0; n < Data->ElementN; n++)
    {
      ElementTypePtr element = &Data->Element[n];
      /* only non empty elements */
      if (!element->LineN && !element->PinN && !element->ArcN
	  && !element->PadN)
	continue;
      /* the coordinates and text-flags are the same for
       * both names of an element
       */
      fprintf (FP, "\nElement[%s ", F2S (element, ELEMENT_TYPE));
      PrintQuotedString (FP, EMPTY (DESCRIPTION_NAME (element)));
      fputc (' ', FP);
      PrintQuotedString (FP, EMPTY (NAMEONPCB_NAME (element)));
      fputc (' ', FP);
      PrintQuotedString (FP, EMPTY (VALUE_NAME (element)));
      fprintf (FP, " %i %i %i %i %i %i %s]\n(\n",
	       (int) element->MarkX, (int) element->MarkY,
	       (int) (DESCRIPTION_TEXT (element).X -
		      element->MarkX),
	       (int) (DESCRIPTION_TEXT (element).Y -
		      element->MarkY),
	       (int) DESCRIPTION_TEXT (element).Direction,
	       (int) DESCRIPTION_TEXT (element).Scale,
	       F2S (&(DESCRIPTION_TEXT (element)), ELEMENTNAME_TYPE));
      WriteAttributeList (FP, &element->Attributes, "\t");
      for (p = 0; p < element->PinN; p++)
	{
	  PinTypePtr pin = &element->Pin[p];
	  fprintf (FP, "\tPin[%i %i %i %i %i %i ",
		   (int) (pin->X - element->MarkX),
		   (int) (pin->Y - element->MarkY),
		   (int) pin->Thickness, (int) pin->Clearance,
		   (int) pin->Mask, (int) pin->DrillingHole);
	  PrintQuotedString (FP, EMPTY (pin->Name));
	  fprintf (FP, " ");
	  PrintQuotedString (FP, EMPTY (pin->Number));
	  fprintf (FP, " %s]\n", F2S (pin, PIN_TYPE));
	}
      for (p = 0; p < element->PadN; p++)
	{
	  PadTypePtr pad = &element->Pad[p];
	  fprintf (FP, "\tPad[%i %i %i %i %i %i %i ",
		   (int) (pad->Point1.X - element->MarkX),
		   (int) (pad->Point1.Y - element->MarkY),
		   (int) (pad->Point2.X - element->MarkX),
		   (int) (pad->Point2.Y - element->MarkY),
		   (int) pad->Thickness, (int) pad->Clearance,
		   (int) pad->Mask);
	  PrintQuotedString (FP, EMPTY (pad->Name));
	  fprintf (FP, " ");
	  PrintQuotedString (FP, EMPTY (pad->Number));
	  fprintf (FP, " %s]\n", F2S (pad, PAD_TYPE));
	}
      for (p = 0; p < element->LineN; p++)
	{
	  LineTypePtr line = &element->Line[p];
	  fprintf (FP,
		   "\tElementLine [%i %i %i %i %i]\n",
		   (int) (line->Point1.X -
			  element->MarkX),
		   (int) (line->Point1.Y -
			  element->MarkY),
		   (int) (line->Point2.X -
			  element->MarkX),
		   (int) (line->Point2.Y -
			  element->MarkY), (int) line->Thickness);
	}
      for (p = 0; p < element->ArcN; p++)
	{
	  ArcTypePtr arc = &element->Arc[p];
	  fprintf (FP,
		   "\tElementArc [%i %i %i %i %i %i %i]\n",
		   (int) (arc->X - element->MarkX),
		   (int) (arc->Y - element->MarkY),
		   (int) arc->Width, (int) arc->Height,
		   (int) arc->StartAngle, (int) arc->Delta,
		   (int) arc->Thickness);
	}
      fputs ("\n\t)\n", FP);
    }
}

/* ---------------------------------------------------------------------------
 * writes layer data
 */
static void
WriteLayerData (FILE * FP, Cardinal Number, LayerTypePtr layer)
{
  int n;
  /* write information about non empty layers */
  if (layer->LineN || layer->ArcN || layer->TextN || layer->PolygonN ||
      (layer->Name && *layer->Name))
    {
      fprintf (FP, "Layer(%i ", (int) Number + 1);
      PrintQuotedString (FP, EMPTY (layer->Name));
      fputs (")\n(\n", FP);
      WriteAttributeList (FP, &layer->Attributes, "\t");

      for (n = 0; n < layer->LineN; n++)
	{
	  LineTypePtr line = &layer->Line[n];
	  fprintf (FP, "\tLine[%i %i %i %i %i %i %s]\n",
		   (int) line->Point1.X, (int) line->Point1.Y,
		   (int) line->Point2.X, (int) line->Point2.Y,
		   (int) line->Thickness, (int) line->Clearance,
		   F2S (line, LINE_TYPE));
	}
      for (n = 0; n < layer->ArcN; n++)
	{
	  ArcTypePtr arc = &layer->Arc[n];
	  fprintf (FP, "\tArc[%i %i %i %i %i %i %i %i %s]\n",
		   (int) arc->X, (int) arc->Y, (int) arc->Width,
		   (int) arc->Height, (int) arc->Thickness,
		   (int) arc->Clearance, (int) arc->StartAngle,
		   (int) arc->Delta, F2S (arc, ARC_TYPE));
	}
      for (n = 0; n < layer->TextN; n++)
	{
	  TextTypePtr text = &layer->Text[n];
	  fprintf (FP, "\tText[%i %i %i %i ",
		   (int) text->X, (int) text->Y,
		   (int) text->Direction, (int) text->Scale);
	  PrintQuotedString (FP, EMPTY (text->TextString));
	  fprintf (FP, " %s]\n", F2S (text, TEXT_TYPE));
	}
      for (n = 0; n < layer->PolygonN; n++)
	{
	  PolygonTypePtr polygon = &layer->Polygon[n];
	  int p, i = 0;
	  Cardinal hole = 0;
	  fprintf (FP, "\tPolygon(%s)\n\t(", F2S (polygon, POLYGON_TYPE));
	  for (p = 0; p < polygon->PointN; p++)
	    {
	      PointTypePtr point = &polygon->Points[p];

	      if (hole < polygon->HoleIndexN &&
		  p == polygon->HoleIndex[hole])
		{
		  if (hole > 0)
		    fputs ("\n\t\t)", FP);
		  fputs ("\n\t\tHole (", FP);
		  hole++;
		  i = 0;
		}

	      if (i++ % 5 == 0)
		{
		  fputs ("\n\t\t", FP);
		  if (hole)
		    fputs ("\t", FP);
		}
	      fprintf (FP, "[%i %i] ", (int) point->X, (int) point->Y);
	    }
	  if (hole > 0)
	    fputs ("\n\t\t)", FP);
	  fputs ("\n\t)\n", FP);
	}
      fputs (")\n", FP);
    }
}

/* ---------------------------------------------------------------------------
 * writes just the elements in the buffer to file
 */
static int
WriteBuffer (FILE * FP)
{
  Cardinal i;

  WriteViaData (FP, PASTEBUFFER->Data);
  WriteElementData (FP, PASTEBUFFER->Data);
  for (i = 0; i < max_copper_layer + 2; i++)
    WriteLayerData (FP, i, &(PASTEBUFFER->Data->Layer[i]));
  return (STATUS_OK);
}

/* ---------------------------------------------------------------------------
 * writes PCB to file
 */
static int
WritePCB (FILE * FP)
{
  Cardinal i;

  WritePCBInfoHeader (FP);
  WritePCBDataHeader (FP);
  WritePCBFontData (FP);
  WriteAttributeList (FP, &PCB->Attributes, "");
  WriteViaData (FP, PCB->Data);
  WriteElementData (FP, PCB->Data);
  WritePCBRatData (FP);
  for (i = 0; i < max_copper_layer + 2; i++)
    WriteLayerData (FP, i, &(PCB->Data->Layer[i]));
  WritePCBNetlistData (FP);

  return (STATUS_OK);
}

/* ---------------------------------------------------------------------------
 * writes PCB to file
 */
static int
WritePCBFile (char *Filename)
{
  FILE *fp;
  int result;

  if ((fp = fopen (Filename, "w")) == NULL)
    {
      OpenErrorMessage (Filename);
      return (STATUS_ERROR);
    }
  result = WritePCB (fp);
  fclose (fp);
  return (result);
}

/* ---------------------------------------------------------------------------
 * writes to pipe using the command defined by Settings.SaveCommand
 * %f are replaced by the passed filename
 */
static int
WritePipe (char *Filename, bool thePcb)
{
  FILE *fp;
  int result;
  char *p;
  static DynamicStringType command;
  int used_popen = 0;

  if (EMPTY_STRING_P (Settings.SaveCommand))
    {
      fp = fopen (Filename, "w");
      if (fp == 0)
	{
	  Message ("Unable to write to file %s\n", Filename);
	  return STATUS_ERROR;
	}
    }
  else
    {
      used_popen = 1;
      /* setup commandline */
      DSClearString (&command);
      for (p = Settings.SaveCommand; *p; p++)
	{
	  /* copy character if not special or add string to command */
	  if (!(*p == '%' && *(p + 1) == 'f'))
	    DSAddCharacter (&command, *p);
	  else
	    {
	      DSAddString (&command, Filename);

	      /* skip the character */
	      p++;
	    }
	}
      DSAddCharacter (&command, '\0');
      printf ("write to pipe \"%s\"\n", command.Data);
      if ((fp = popen (command.Data, "w")) == NULL)
	{
	  PopenErrorMessage (command.Data);
	  return (STATUS_ERROR);
	}
    }
  if (thePcb)
    result = WritePCB (fp);
  else
    result = WriteBuffer (fp);

  if (used_popen)
    return (pclose (fp) ? STATUS_ERROR : result);
  return (fclose (fp) ? STATUS_ERROR : result);
}

/* ---------------------------------------------------------------------------
 * saves the layout in a temporary file
 * this is used for fatal errors and does not call the program specified
 * in 'saveCommand' for safety reasons
 */
void
SaveInTMP (void)
{
  char filename[80];

  /* memory might have been released before this function is called */
  if (PCB && PCB->Changed)
    {
      sprintf (filename, EMERGENCY_NAME, (int) getpid ());
      Message (_("Trying to save your layout in '%s'\n"), filename);
      WritePCBFile (filename);
    }
}

/* ---------------------------------------------------------------------------
 * front-end for 'SaveInTMP()'
 * just makes sure that the routine is only called once
 */
static bool dont_save_any_more = false;
void
EmergencySave (void)
{

  if (!dont_save_any_more)
    {
      SaveInTMP ();
      dont_save_any_more = true;
    }
}

 void 
DisableEmergencySave (void)
{
  dont_save_any_more = true;
}

/* ----------------------------------------------------------------------
 * Callback for the autosave
 */

static hidval backup_timer;

/*  
 * If the backup interval is > 0 then set another timer.  Otherwise
 * we do nothing and it is up to the GUI to call EnableAutosave()
 * after setting Settings.BackupInterval > 0 again.
 */
static void
backup_cb (hidval data)
{
  backup_timer.ptr = NULL;
  Backup ();
  if (Settings.BackupInterval > 0 && gui->add_timer)
    backup_timer = gui->add_timer (backup_cb, 
				   1000 * Settings.BackupInterval, data);
}

void
EnableAutosave (void)
{
  hidval x;

  x.ptr = NULL;

  /* If we already have a timer going, then cancel it out */
  if (backup_timer.ptr != NULL && gui->stop_timer)
    gui->stop_timer (backup_timer);

  backup_timer.ptr = NULL;
  /* Start up a new timer */
  if (Settings.BackupInterval > 0 && gui->add_timer)
    backup_timer = gui->add_timer (backup_cb, 
				   1000 * Settings.BackupInterval, 
				   x);
}

/* ---------------------------------------------------------------------------
 * creates backup file.  The default is to use the pcb file name with
 * a "-" appended (like "foo.pcb-") and if we don't have a pcb file name
 * then use the template in BACKUP_NAME
 */
void
Backup (void)
{
  char *filename = NULL;

  if( PCB && PCB->Filename )
    {
      filename 	= (char *) malloc (sizeof (char) * (strlen (PCB->Filename) + 2));
      if (filename == NULL)
	{
	  fprintf (stderr, "Backup():  malloc failed\n");
	  exit (1);
	}
      sprintf (filename, "%s-", PCB->Filename);
    }
  else
    {
      /* BACKUP_NAME has %.8i which  will be replaced by the process ID */
      filename 	= (char *) malloc (sizeof (char) * (strlen (BACKUP_NAME) + 8));
      if (filename == NULL)
	{
	  fprintf (stderr, "Backup():  malloc failed\n");
	  exit (1);
	}
      sprintf (filename, BACKUP_NAME, (int) getpid ());
    }

  WritePCBFile (filename);
  free (filename);
}

#if !defined(HAS_ATEXIT) && !defined(HAS_ON_EXIT)
/* ---------------------------------------------------------------------------
 * makes a temporary copy of the data. This is useful for systems which
 * doesn't support calling functions on exit. We use this to save the data
 * before LEX and YACC functions are called because they are able to abort
 * the program.
 */
void
SaveTMPData (void)
{
  sprintf (TMPFilename, EMERGENCY_NAME, (int) getpid ());
  WritePCBFile (TMPFilename);
}

/* ---------------------------------------------------------------------------
 * removes the temporary copy of the data file
 */
void
RemoveTMPData (void)
{
  unlink (TMPFilename);
}
#endif

/* ---------------------------------------------------------------------------
 * Parse the directory tree where newlib footprints are found
 */

/* Helper function for ParseLibraryTree */
static char *
pcb_basename (char *p)
{
  char *rv = strrchr (p, '/');
  if (rv)
    return rv + 1;
  return p;
}

/* This is a helper function for ParseLibrary Tree.   Given a char *path,
 * it finds all newlib footprints in that dir and sticks them into the
 * library menu structure named entry.
 */
static int
LoadNewlibFootprintsFromDir(char *libpath, char *toppath)
{
  char olddir[MAXPATHLEN + 1];    /* The directory we start out in (cwd) */
  char subdir[MAXPATHLEN + 1];    /* The directory holding footprints to load */
  DIR *subdirobj;                 /* Interable object holding all subdir entries */
  struct dirent *subdirentry;     /* Individual subdir entry */
  struct stat buffer;             /* Buffer used in stat */
  LibraryMenuTypePtr menu = NULL; /* Pointer to PCB's library menu structure */
  LibraryEntryTypePtr entry;      /* Pointer to individual menu entry */
  size_t l;
  size_t len;
  int n_footprints = 0;           /* Running count of footprints found in this subdir */

  /* Cache old dir, then cd into subdir because stat is given relative file names. */
  memset (subdir, 0, sizeof subdir);
  memset (olddir, 0, sizeof olddir);
  if (GetWorkingDirectory (olddir) == NULL)
    {
      Message (_("LoadNewlibFootprintsFromDir: Could not determine initial working directory\n"));
      return 0;
    }

  strcpy (subdir, libpath);
  if (chdir (subdir))
    {
      ChdirErrorMessage (subdir);
      return 0;
    }

  /* Determine subdir is abs path */
  if (GetWorkingDirectory (subdir) == NULL)
    {
      Message (_("LoadNewlibFootprintsFromDir: Could not determine new working directory\n"));
      if (chdir (olddir))
        ChdirErrorMessage (olddir);
      return 0;
    }

  /* First try opening the directory specified by path */
  if ( (subdirobj = opendir (subdir)) == NULL )
    {
      OpendirErrorMessage (subdir);
      if (chdir (olddir))
        ChdirErrorMessage (olddir);
      return 0;
    }

  /* Get pointer to memory holding menu */
  menu = GetLibraryMenuMemory (&Library);
  /* Populate menuname and path vars */
  menu->Name = strdup (pcb_basename(subdir));
  menu->directory = strdup (pcb_basename(toppath));

  /* Now loop over files in this directory looking for files.
   * We ignore certain files which are not footprints.
   */
  while ((subdirentry = readdir (subdirobj)) != NULL)
  {
#ifdef DEBUG
/*    printf("...  Examining file %s ... \n", subdirentry->d_name); */
#endif

    /* Ignore non-footprint files found in this directory
     * We're skipping .png and .html because those
     * may exist in a library tree to provide an html browsable
     * index of the library.
     */
    l = strlen (subdirentry->d_name);
    if (!stat (subdirentry->d_name, &buffer) && S_ISREG (buffer.st_mode)
      && subdirentry->d_name[0] != '.'
      && NSTRCMP (subdirentry->d_name, "CVS") != 0
      && NSTRCMP (subdirentry->d_name, "Makefile") != 0
      && NSTRCMP (subdirentry->d_name, "Makefile.am") != 0
      && NSTRCMP (subdirentry->d_name, "Makefile.in") != 0
      && (l < 4 || NSTRCMP(subdirentry->d_name + (l - 4), ".png") != 0) 
      && (l < 5 || NSTRCMP(subdirentry->d_name + (l - 5), ".html") != 0) )
      {
#ifdef DEBUG
/*	printf("...  Found a footprint %s ... \n", subdirentry->d_name); */
#endif
	n_footprints++;
	entry = GetLibraryEntryMemory (menu);

	/* 
	 * entry->AllocatedMemory points to abs path to the footprint.
	 * entry->ListEntry points to fp name itself.
	 */
	len = strlen(subdir) + strlen("/") + strlen(subdirentry->d_name) + 1;
	entry->AllocatedMemory = MyCalloc (1, len, "ParseLibraryTree()");
	strcat (entry->AllocatedMemory, subdir);
	strcat (entry->AllocatedMemory, PCB_DIR_SEPARATOR_S);

	/* store pointer to start of footprint name */
	entry->ListEntry = entry->AllocatedMemory
	    + strlen (entry->AllocatedMemory);

	/* Now place footprint name into AllocatedMemory */
	strcat (entry->AllocatedMemory, subdirentry->d_name);

	/* mark as directory tree (newlib) library */
	entry->Template = (char *) -1;
      }
  }
  /* Done.  Clean up, cd back into old dir, and return */
  closedir (subdirobj);
  if (chdir (olddir))
    ChdirErrorMessage (olddir);
  return n_footprints;
}


/* This function loads the newlib footprints into the Library.
 * It examines all directories pointed to by Settings.LibraryTree.
 * In each directory specified there, it looks both in that directory,
 * as well as *one* level down.  It calls the subfunction 
 * LoadNewlibFootprintsFromDir to put the footprints into PCB's internal
 * datastructures.
 */
static int
ParseLibraryTree (void)
{
  char toppath[MAXPATHLEN + 1];    /* String holding abs path to top level library dir */
  char working[MAXPATHLEN + 1];    /* String holding abs path to working dir */
  char *libpaths;                  /* String holding list of library paths to search */
  char *p;                         /* Helper string used in iteration */
  DIR *dirobj;                     /* Iterable directory object */
  struct dirent *direntry = NULL;  /* Object holding individual directory entries */
  struct stat buffer;              /* buffer used in stat */
  int n_footprints = 0;            /* Running count of footprints found */

  /* Initialize path, working by writing 0 into every byte. */
  memset (toppath, 0, sizeof toppath);
  memset (working, 0, sizeof working);

  /* Save the current working directory as an absolute path.
   * This fcn writes the abs path into the memory pointed to by the input arg.
   */
  if (GetWorkingDirectory (working) == NULL)
    {
      Message (_("ParseLibraryTree: Could not determine initial working directory\n"));
      return 0;
    }

  /* Additional loop to allow for multiple 'newlib' style library directories 
   * called out in Settings.LibraryTree
   */
  libpaths = strdup (Settings.LibraryTree);
  for (p = strtok (libpaths, PCB_PATH_DELIMETER); p && *p; p = strtok (NULL, PCB_PATH_DELIMETER))
    {
      /* remove trailing path delimeter */
      strncpy (toppath, p, sizeof (toppath) - 1);

      /* start out in the working directory in case the path is a
       * relative path 
       */
      if (chdir (working))
        {
          ChdirErrorMessage (working);
          free (libpaths);
          return 0;
        }

      /*
       * Next change to the directory which is the top of the library tree
       * and extract its abs path.
       */
      if (chdir (toppath))
        {
          ChdirErrorMessage (toppath);
          continue;
        }

      if (GetWorkingDirectory (toppath) == NULL)
        {
          Message (_("ParseLibraryTree: Could not determine new working directory\n"));
          continue;
        }

#ifdef DEBUG
      printf("In ParseLibraryTree, looking for newlib footprints inside top level directory %s ... \n", 
	     toppath);
#endif

      /* Next read in any footprints in the top level dir */
      n_footprints += LoadNewlibFootprintsFromDir("(local)", toppath);

      /* Then open this dir so we can loop over its contents. */
      if ((dirobj = opendir (toppath)) == NULL)
	{
	  OpendirErrorMessage (toppath);
	  continue;
	}

      /* Now loop over files in this directory looking for subdirs.
       * For each direntry which is a valid subdirectory,
       * try to load newlib footprints inside it.
       */
      while ((direntry = readdir (dirobj)) != NULL)
	{
#ifdef DEBUG
	  printf("In ParseLibraryTree loop examining 2nd level direntry %s ... \n", direntry->d_name);
#endif
	  /* Find subdirectories.  Ignore entries beginning with "." and CVS
	   * directories.
	   */
	  if (!stat (direntry->d_name, &buffer)
	      && S_ISDIR (buffer.st_mode) 
	      && direntry->d_name[0] != '.'
	      && NSTRCMP (direntry->d_name, "CVS") != 0)
	    {
	      /* Found a valid subdirectory.  Try to load footprints from it.
	       */
	      n_footprints += LoadNewlibFootprintsFromDir(direntry->d_name, toppath);
	    }
	}
      closedir (dirobj);
    }

  /* restore the original working directory */
  if (chdir (working))
    ChdirErrorMessage (working);

#ifdef DEBUG
  printf("Leaving ParseLibraryTree, found %d footprints.\n", n_footprints);
#endif

  return n_footprints;
}

/* ---------------------------------------------------------------------------
 * Read contents of the library description file (for M4)
 * and then read in M4 libs.  Then call a fcn to read the newlib
 * footprints.
 */
int
ReadLibraryContents (void)
{
  static char *command = NULL;
  char inputline[MAX_LIBRARY_LINE_LENGTH + 1];
  FILE *resultFP = NULL;
  LibraryMenuTypePtr menu = NULL;
  LibraryEntryTypePtr entry;


  /*  First load the M4 stuff.  The variable Settings.LibraryPath
   *  points to it.
   */
  MYFREE (command);
  command = EvaluateFilename (Settings.LibraryContentsCommand,
			      Settings.LibraryPath, Settings.LibraryFilename,
			      NULL);

#ifdef DEBUG
  printf("In ReadLibraryContents, about to execute command %s\n", command);
#endif

  /* This uses a pipe to execute a shell script which provides the names of
   * all M4 libs and footprints.  The results are placed in resultFP.
   */
  if (command && *command && (resultFP = popen (command, "r")) == NULL)
    {
      PopenErrorMessage (command);
    }

  /* the M4 library contents are separated by colons;
   * template : package : name : description
   */
  while (resultFP != NULL && fgets (inputline, MAX_LIBRARY_LINE_LENGTH, resultFP))
    {
      size_t len = strlen (inputline);

      /* check for maximum linelength */
      if (len)
	{
	  len--;
	  if (inputline[len] != '\n')
	    Message
	      ("linelength (%i) exceeded; following characters will be ignored\n",
	       MAX_LIBRARY_LINE_LENGTH);
	  else
	    inputline[len] = '\0';
	}

      /* if the line defines a menu */
      if (!strncmp (inputline, "TYPE=", 5))
	{
	  menu = GetLibraryMenuMemory (&Library);
	  menu->Name = strdup (UNKNOWN (&inputline[5]));
	  menu->directory = strdup (Settings.LibraryFilename);
	}
      else
	{
	  /* allocate a new menu entry if not already done */
	  if (!menu)
	    {
	      menu = GetLibraryMenuMemory (&Library);
	      menu->Name = strdup (UNKNOWN ((char *) NULL));
	      menu->directory = strdup (Settings.LibraryFilename);
	    }
	  entry = GetLibraryEntryMemory (menu);
	  entry->AllocatedMemory = strdup (inputline);

	  /* now break the line into pieces separated by colons */
	  if ((entry->Template = strtok (entry->AllocatedMemory, ":")) !=
	      NULL)
	    if ((entry->Package = strtok (NULL, ":")) != NULL)
	      if ((entry->Value = strtok (NULL, ":")) != NULL)
		entry->Description = strtok (NULL, ":");

	  /* create the list entry */
	  len = strlen (EMPTY (entry->Value)) +
	    strlen (EMPTY (entry->Description)) + 4;
	  entry->ListEntry = MyCalloc (len, sizeof (char),
				       "ReadLibraryDescription()");
	  sprintf (entry->ListEntry,
		   "%s, %s", EMPTY (entry->Value),
		   EMPTY (entry->Description));
	}
    }
  if (resultFP != NULL)
    pclose (resultFP);

  /* Now after reading in the M4 libs, call a function to
   * read the newlib footprint libraries.  Then sort the whole
   * library.
   */
  if (ParseLibraryTree () > 0 || resultFP != NULL)
    {
      sort_library (&Library);
      return 0;
    }
  
  return (1);
}

#define BLANK(x) ((x) == ' ' || (x) == '\t' || (x) == '\n' \
		|| (x) == '\0')

/* ---------------------------------------------------------------------------
 * Read in a netlist and store it in the netlist menu 
 */

int
ReadNetlist (char *filename)
{
  static char *command = NULL;
  char inputline[MAX_NETLIST_LINE_LENGTH + 1];
  char temp[MAX_NETLIST_LINE_LENGTH + 1];
  FILE *fp;
  LibraryMenuTypePtr menu = NULL;
  LibraryEntryTypePtr entry;
  int i, j, lines, kind;
  bool continued;
  int used_popen = 0;

  if (!filename)
    return (1);			/* nothing to do */

  Message (_("Importing PCB netlist %s\n"), filename);

  if (EMPTY_STRING_P (Settings.RatCommand))
    {
      fp = fopen (filename, "r");
      if (!fp)
	{
	  Message("Cannot open %s for reading", filename);
	  return 1;
	}
    }
  else
    {
      used_popen = 1;
      MYFREE (command);
      command = EvaluateFilename (Settings.RatCommand,
				  Settings.RatPath, filename, NULL);

      /* open pipe to stdout of command */
      if (*command == '\0' || (fp = popen (command, "r")) == NULL)
	{
	  PopenErrorMessage (command);
	  return (1);
	}
    }
  lines = 0;
  /* kind = 0  is net name
   * kind = 1  is route style name
   * kind = 2  is connection
   */
  kind = 0;
  while (fgets (inputline, MAX_NETLIST_LINE_LENGTH, fp))
    {
      size_t len = strlen (inputline);
      /* check for maximum length line */
      if (len)
	{
	  if (inputline[--len] != '\n')
	    Message (_("Line length (%i) exceeded in netlist file.\n"
		       "additional characters will be ignored.\n"),
		     MAX_NETLIST_LINE_LENGTH);
	  else
	    inputline[len] = '\0';
	}
      continued = (inputline[len - 1] == '\\') ? true : false;
      if (continued)
	inputline[len - 1] = '\0';
      lines++;
      i = 0;
      while (inputline[i] != '\0')
	{
	  j = 0;
	  /* skip leading blanks */
	  while (inputline[i] != '\0' && BLANK (inputline[i]))
	    i++;
	  if (kind == 0)
	    {
	      /* add two spaces for included/unincluded */
	      temp[j++] = ' ';
	      temp[j++] = ' ';
	    }
	  while (!BLANK (inputline[i]))
	    temp[j++] = inputline[i++];
	  temp[j] = '\0';
	  while (inputline[i] != '\0' && BLANK (inputline[i]))
	    i++;
	  if (kind == 0)
	    {
	      menu = GetLibraryMenuMemory (&PCB->NetlistLib);
	      menu->Name = strdup (temp);
	      menu->flag = 1;
	      kind++;
	    }
	  else
	    {
	      if (kind == 1 && strchr (temp, '-') == NULL)
		{
		  kind++;
		  menu->Style = strdup (temp);
		}
	      else
		{
		  entry = GetLibraryEntryMemory (menu);
		  entry->ListEntry = strdup (temp);
		}
	    }
	}
      if (!continued)
	kind = 0;
    }
  if (!lines)
    {
      Message (_("Empty netlist file!\n"));
      pclose (fp);
      return (1);
    }
  if (used_popen)
    pclose (fp);
  else
    fclose (fp);
  sort_netlist ();
  return (0);
}

static int ReadEdifNetlist (char *filename);

int ImportNetlist (char *filename)
{
  FILE *fp;
  char buf[16];
  int i;
  char* p;
  

  if (!filename) return (1);			/* nothing to do */
  fp = fopen (filename, "r");
  if (!fp) return (1);			/* bad filename */
  i = fread (buf, 1, sizeof(buf)-1, fp);
  fclose(fp);
  buf[i] = '\0';
  p=buf;
  while ( *p )
  {
      *p = tolower ((int) *p);
      p++;
  }
  p = strstr (buf, "edif");
  if (!p) return ReadNetlist (filename);
  else return ReadEdifNetlist (filename);
}

static int ReadEdifNetlist (char *filename)
{
    Message (_("Importing edif netlist %s\n"), filename);
    ParseEDIF(filename, NULL);
    
    return 0;
}

