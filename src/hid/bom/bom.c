#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "global.h"
#include "data.h"
#include "error.h"
#include "misc.h"
#include "pcb-printf.h"

#include "hid.h"
#include "hid/common/hidnogui.h"
#include "../hidint.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static HID_Attribute bom_options[] = {
/* %start-doc options "8 BOM Creation"
@ftable @code
@item --bomfile <string>
Name of the BOM output file.
@end ftable
%end-doc
*/
  {"bomfile", "Name of the BOM output file",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_bomfile 0
/* %start-doc options "8 BOM Creation"
@ftable @code
@item --xyfile <string>
Name of the XY output file.
@end ftable
%end-doc
*/
  {"xyfile", "Name of the XY output file",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_xyfile 1

/* %start-doc options "8 BOM Creation"
@ftable @code
@item --xy-unit <unit>
Unit of XY dimensions. Defaults to mil.
@end ftable
%end-doc
*/
  {"xy-unit", "XY units",
   HID_Unit, 0, 0, {-1, 0, 0}, NULL, 0},
#define HA_unit 2
  {"xy-in-mm", ATTR_UNDOCUMENTED,
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_xymm 3
};

#define NUM_OPTIONS (sizeof(bom_options)/sizeof(bom_options[0]))

static HID_Attr_Val bom_values[NUM_OPTIONS];

static const char *bom_filename;
static const char *xy_filename;
static const Unit *xy_unit;

typedef struct _StringList
{
  char *str;
  struct _StringList *next;
} StringList;

typedef struct _BomList
{
  char *descr;
  char *value;
  int num;
  StringList *refdes;
  struct _BomList *next;
} BomList;

static HID_Attribute *
bom_get_export_options (int *n)
{
  static char *last_bom_filename = 0;
  static char *last_xy_filename = 0;
  static int last_unit_value = -1;

  if (bom_options[HA_unit].default_val.int_value == last_unit_value)
    {
      if (Settings.grid_unit)
        bom_options[HA_unit].default_val.int_value = Settings.grid_unit->index;
      else
        bom_options[HA_unit].default_val.int_value = get_unit_struct ("mil")->index;
      last_unit_value = bom_options[HA_unit].default_val.int_value;
    }
  if (PCB) {
    derive_default_filename(PCB->Filename, &bom_options[HA_bomfile], ".bom", &last_bom_filename);
    derive_default_filename(PCB->Filename, &bom_options[HA_xyfile ], ".xy" , &last_xy_filename );
  }

  if (n)
    *n = NUM_OPTIONS;
  return bom_options;
}

/* ---------------------------------------------------------------------------
 * prints a centroid file in a format which includes data needed by a
 * pick and place machine.  Further formatting for a particular factory setup
 * can easily be generated with awk or perl.  In addition, a bill of materials
 * file is generated which can be used for checking stock and purchasing needed
 * materials.
 * returns != zero on error
 */


static char *
CleanBOMString (char *in)
{
  char *out;
  int i;

  if ((out = (char *)malloc ((strlen (in) + 1) * sizeof (char))) == NULL)
    {
      fprintf (stderr, "Error:  CleanBOMString() malloc() failed\n");
      exit (1);
    }

  /* 
   * copy over in to out with some character conversions.
   * Go all the way to then end to get the terminating \0
   */
  for (i = 0; i <= strlen (in); i++)
    {
      switch (in[i])
	{
	case '"':
	  out[i] = '\'';
	  break;
	default:
	  out[i] = in[i];
	}
    }

  return out;
}


static double
xyToAngle (double x, double y)
{
  double theta;

  if ((x > 0.0) && (y >= 0.0))
    theta = 180.0;
  else if ((x <= 0.0) && (y > 0.0))
    theta = 90.0;
  else if ((x < 0.0) && (y <= 0.0))
    theta = 0.0;
  else if ((x >= 0.0) && (y < 0.0))
    theta = 270.0;
  else
    {
      theta = 0.0;
      Message ("xyToAngle(): unable to figure out angle of element\n"
	       "     because the pin is at the centroid of the part.\n"
	       "     This is a BUG!!!\n"
	       "     Setting to %g degrees\n", theta);
    }

  return (theta);
}

static StringList *
string_insert (char *str, StringList * list)
{
  StringList *newlist, *cur;

  if ((newlist = (StringList *) malloc (sizeof (StringList))) == NULL)
    {
      fprintf (stderr, "malloc() failed in string_insert()\n");
      exit (1);
    }

  newlist->next = NULL;
  newlist->str = strdup (str);

  if (list == NULL)
    return (newlist);

  cur = list;
  while (cur->next != NULL)
    cur = cur->next;

  cur->next = newlist;

  return (list);
}

static BomList *
bom_insert (char *refdes, char *descr, char *value, BomList * bom)
{
  BomList *newlist, *cur, *prev = NULL;

  if (bom == NULL)
    {
      /* this is the first element so automatically create an entry */
      if ((newlist = (BomList *) malloc (sizeof (BomList))) == NULL)
	{
	  fprintf (stderr, "malloc() failed in bom_insert()\n");
	  exit (1);
	}

      newlist->next = NULL;
      newlist->descr = strdup (descr);
      newlist->value = strdup (value);
      newlist->num = 1;
      newlist->refdes = string_insert (refdes, NULL);
      return (newlist);
    }

  /* search and see if we already have used one of these
     components */
  cur = bom;
  while (cur != NULL)
    {
      if ((NSTRCMP (descr, cur->descr) == 0) &&
	  (NSTRCMP (value, cur->value) == 0))
	{
	  cur->num++;
	  cur->refdes = string_insert (refdes, cur->refdes);
	  break;
	}
      prev = cur;
      cur = cur->next;
    }

  if (cur == NULL)
    {
      if ((newlist = (BomList *) malloc (sizeof (BomList))) == NULL)
	{
	  fprintf (stderr, "malloc() failed in bom_insert()\n");
	  exit (1);
	}

      prev->next = newlist;

      newlist->next = NULL;
      newlist->descr = strdup (descr);
      newlist->value = strdup (value);
      newlist->num = 1;
      newlist->refdes = string_insert (refdes, NULL);
    }

  return (bom);

}

/* 
 * If fp is not NULL then print out the bill of materials contained in
 * bom.  Either way, free all memory which has been allocated for bom.
 */
static void
print_and_free (FILE *fp, BomList *bom)
{
  BomList *lastb;
  StringList *lasts;
  char *descr, *value;

  while (bom != NULL)
    {
      if (fp)
	{
	  descr = CleanBOMString (bom->descr);
	  value = CleanBOMString (bom->value);
	  fprintf (fp, "%d,\"%s\",\"%s\",", bom->num, descr, value);
	  free (descr);
	  free (value);
	}
      
      while (bom->refdes != NULL)
	{
	  if (fp)
	    {
	      fprintf (fp, "%s ", bom->refdes->str);
	    }
	  free (bom->refdes->str);
	  lasts = bom->refdes;
	  bom->refdes = bom->refdes->next;
	  free (lasts);
	}
      if (fp)
	{
	  fprintf (fp, "\n");
	}
      lastb = bom;
      bom = bom->next;
      free (lastb);
    }
}

static int
PrintBOM (void)
{
  char utcTime[64];
  Coord x, y;
  double theta = 0.0;
  double sumx, sumy;
  double pin1x = 0.0, pin1y = 0.0, pin1angle = 0.0;
  double pin2x = 0.0, pin2y = 0.0;
  int found_pin1;
  int found_pin2;
  int pin_cnt;
  time_t currenttime;
  FILE *fp;
  BomList *bom = NULL;
  char *name, *descr, *value;

  fp = fopen (xy_filename, "w");
  if (!fp)
    {
      gui->log ("Cannot open file %s for writing\n", xy_filename);
      return 1;
    }

  /* Create a portable timestamp. */
  currenttime = time (NULL);
  {
    /* avoid gcc complaints */
    const char *fmt = "%c UTC";
    strftime (utcTime, sizeof (utcTime), fmt, gmtime (&currenttime));
  }
  fprintf (fp, "# PcbXY Version 1.0\n");
  fprintf (fp, "# Date: %s\n", utcTime);
  fprintf (fp, "# Author: %s\n", pcb_author ());
  fprintf (fp, "# Title: %s - PCB X-Y\n", UNKNOWN (PCB->Name));
  fprintf (fp, "# RefDes, Description, Value, X, Y, rotation, top/bottom\n");
  fprintf (fp, "# X,Y in %s.  rotation in degrees.\n", xy_unit->in_suffix);
  fprintf (fp, "# --------------------------------------------\n");

  /*
   * For each element we calculate the centroid of the footprint.
   * In addition, we need to extract some notion of rotation.  
   * While here generate the BOM list
   */

  ELEMENT_LOOP (PCB->Data);
  {

    /* initialize our pin count and our totals for finding the
       centriod */
    pin_cnt = 0;
    sumx = 0.0;
    sumy = 0.0;
    found_pin1 = 0;
    found_pin2 = 0;

    /* insert this component into the bill of materials list */
    bom = bom_insert ((char *)UNKNOWN (NAMEONPCB_NAME (element)),
		      (char *)UNKNOWN (DESCRIPTION_NAME (element)),
		      (char *)UNKNOWN (VALUE_NAME (element)), bom);


    /*
     * iterate over the pins and pads keeping a running count of how
     * many pins/pads total and the sum of x and y coordinates
     * 
     * While we're at it, store the location of pin/pad #1 and #2 if
     * we can find them
     */

    PIN_LOOP (element);
    {
      sumx += (double) pin->X;
      sumy += (double) pin->Y;
      pin_cnt++;

      if (NSTRCMP (pin->Number, "1") == 0)
	{
	  pin1x = (double) pin->X;
	  pin1y = (double) pin->Y;
	  pin1angle = 0.0;	/* pins have no notion of angle */
	  found_pin1 = 1;
	}
      else if (NSTRCMP (pin->Number, "2") == 0)
	{
	  pin2x = (double) pin->X;
	  pin2y = (double) pin->Y;
	  found_pin2 = 1;
	}
    }
    END_LOOP;

    PAD_LOOP (element);
    {
      sumx += (pad->Point1.X + pad->Point2.X) / 2.0;
      sumy += (pad->Point1.Y + pad->Point2.Y) / 2.0;
      pin_cnt++;

      if (NSTRCMP (pad->Number, "1") == 0)
	{
	  pin1x = (double) (pad->Point1.X + pad->Point2.X) / 2.0;
	  pin1y = (double) (pad->Point1.Y + pad->Point2.Y) / 2.0;
	  /*
	   * NOTE:  We swap the Y points because in PCB, the Y-axis
	   * is inverted.  Increasing Y moves down.  We want to deal
	   * in the usual increasing Y moves up coordinates though.
	   */
	  pin1angle = (180.0 / M_PI) * atan2 (pad->Point1.Y - pad->Point2.Y,
					      pad->Point2.X - pad->Point1.X);
	  found_pin1 = 1;
	}
      else if (NSTRCMP (pad->Number, "2") == 0)
	{
	  pin2x = (double) (pad->Point1.X + pad->Point2.X) / 2.0;
	  pin2y = (double) (pad->Point1.Y + pad->Point2.Y) / 2.0;
	  found_pin2 = 1;
	}

    }
    END_LOOP;

    if (pin_cnt > 0)
      {
	x = sumx / (double) pin_cnt;
	y = sumy / (double) pin_cnt;

	if (found_pin1)
	  {
	    /* recenter pin #1 onto the axis which cross at the part
	       centroid */
	    pin1x -= x;
	    pin1y -= y;
	    pin1y = -1.0 * pin1y;

	    /* if only 1 pin, use pin 1's angle */
	    if (pin_cnt == 1)
	      theta = pin1angle;
	    else
	      {
		/* if pin #1 is at (0,0) use pin #2 for rotation */
		if ((pin1x == 0.0) && (pin1y == 0.0))
		  {
		    if (found_pin2)
		      theta = xyToAngle (pin2x, pin2y);
		    else
		      {
			Message
			  ("PrintBOM(): unable to figure out angle of element\n"
			   "     %s because pin #1 is at the centroid of the part.\n"
			   "     and I could not find pin #2's location\n"
			   "     Setting to %g degrees\n",
			   UNKNOWN (NAMEONPCB_NAME (element)), theta);
		      }
		  }
		else
		  theta = xyToAngle (pin1x, pin1y);
	      }
	  }
	/* we did not find pin #1 */
	else
	  {
	    theta = 0.0;
	    Message
	      ("PrintBOM(): unable to figure out angle because I could\n"
	       "     not find pin #1 of element %s\n"
	       "     Setting to %g degrees\n",
	       UNKNOWN (NAMEONPCB_NAME (element)), theta);
	  }

	name = CleanBOMString ((char *)UNKNOWN (NAMEONPCB_NAME (element)));
	descr = CleanBOMString ((char *)UNKNOWN (DESCRIPTION_NAME (element)));
	value = CleanBOMString ((char *)UNKNOWN (VALUE_NAME (element)));

 	y = PCB->MaxHeight - y;
	pcb_fprintf (fp, "%m+%s,\"%s\",\"%s\",%mS,%.2mS,%g,%s\n",
		     xy_unit->allow, name, descr, value, x, y,
		     theta, FRONT (element) == 1 ? "top" : "bottom");
	free (name);
	free (descr);
	free (value);
      }
  }
  END_LOOP;

  fclose (fp);

  /* Now print out a Bill of Materials file */

  fp = fopen (bom_filename, "w");
  if (!fp)
    {
      gui->log ("Cannot open file %s for writing\n", bom_filename);
      print_and_free (NULL, bom);
      return 1;
    }

  fprintf (fp, "# PcbBOM Version 1.0\n");
  fprintf (fp, "# Date: %s\n", utcTime);
  fprintf (fp, "# Author: %s\n", pcb_author ());
  fprintf (fp, "# Title: %s - PCB BOM\n", UNKNOWN (PCB->Name));
  fprintf (fp, "# Quantity, Description, Value, RefDes\n");
  fprintf (fp, "# --------------------------------------------\n");

  print_and_free (fp, bom);

  fclose (fp);

  return (0);
}

static void
bom_do_export (HID_Attr_Val * options)
{
  int i;

  if (!options)
    {
      bom_get_export_options (0);
      for (i = 0; i < NUM_OPTIONS; i++)
	bom_values[i] = bom_options[i].default_val;
      options = bom_values;
    }

  bom_filename = options[HA_bomfile].str_value;
  if (!bom_filename)
    bom_filename = "pcb-out.bom";

  xy_filename = options[HA_xyfile].str_value;
  if (!xy_filename)
    xy_filename = "pcb-out.xy";

  if (options[HA_unit].int_value == -1)
    xy_unit = options[HA_xymm].int_value ? get_unit_struct ("mm")
                                         : get_unit_struct ("mil");
  else
    xy_unit = &get_unit_list ()[options[HA_unit].int_value];
  PrintBOM ();
}

static void
bom_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (bom_options,
			   sizeof (bom_options) / sizeof (bom_options[0]));
  hid_parse_command_line (argc, argv);
}

HID bom_hid;

void
hid_bom_init ()
{
  memset (&bom_hid, 0, sizeof (HID));

  common_nogui_init (&bom_hid);

  bom_hid.struct_size         = sizeof (HID);
  bom_hid.name                = "bom";
  bom_hid.description         = "Exports a Bill of Materials";
  bom_hid.exporter            = 1;

  bom_hid.get_export_options  = bom_get_export_options;
  bom_hid.do_export           = bom_do_export;
  bom_hid.parse_arguments     = bom_parse_arguments;

  hid_register_hid (&bom_hid);
}
