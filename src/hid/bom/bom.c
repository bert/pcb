/* $Id$ */

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

#include "hid.h"
#include "../hidint.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID("$Id$");

static HID_Attribute bom_options[] = {
  {"bomfile", "BOM output file",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_bomfile 0
  {"xyfile", "XY output file",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_xyfile 1
};
#define NUM_OPTIONS (sizeof(bom_options)/sizeof(bom_options[0]))

static HID_Attr_Val bom_values[NUM_OPTIONS];

static char *bom_filename;
static char *xy_filename;

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
  char *buf = 0;

  if (PCB)
    {
      buf = (char *)malloc(strlen(PCB->Filename) + 4);
      if (buf)
	{
	  strcpy(buf, PCB->Filename);
	  if (strcmp (buf + strlen(buf) - 4, ".pcb") == 0)
	    buf[strlen(buf)-4] = 0;
	  strcat(buf, ".bom");
	  if (bom_options[HA_bomfile].default_val.str_value)
	    free (bom_options[HA_bomfile].default_val.str_value);
	  bom_options[HA_bomfile].default_val.str_value = buf;
	}

      buf = (char *)malloc(strlen(PCB->Filename) + 4);
      if (buf)
	{
	  strcpy(buf, PCB->Filename);
	  if (strcmp (buf + strlen(buf) - 4, ".pcb") == 0)
	    buf[strlen(buf)-4] = 0;
	  strcat(buf, ".xy");
	  if (bom_options[HA_xyfile].default_val.str_value)
	    free (bom_options[HA_xyfile].default_val.str_value);
	  bom_options[HA_xyfile].default_val.str_value = buf;
	}
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

  if ( (out = malloc( (strlen(in) + 1)*sizeof(char) ) ) == NULL ) {
    fprintf(stderr, "Error:  CleanBOMString() malloc() failed\n");
    exit (1);
  }

  /* 
   * copy over in to out with some character conversions.
   * Go all the way to then end to get the terminating \0
   */
  for (i = 0; i<=strlen(in); i++) {
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
xyToAngle(double x, double y)
{
  double theta;

  if( ( x > 0.0) && ( y >= 0.0) )
    theta = 180.0;
  else if( ( x <= 0.0) && ( y > 0.0) )
    theta = 90.0;
  else if( ( x < 0.0) && ( y <= 0.0) )
    theta = 0.0;
  else if( ( x >= 0.0) && ( y < 0.0) )
    theta = 270.0;
  else
    {
      theta = 0.0;
      Message ("xyToAngle(): unable to figure out angle of element\n"
	       "     because the pin is at the centroid of the part.\n"
	       "     This is a BUG!!!\n"
	       "     Setting to %g degrees\n",
	       theta );
    }

  return (theta);
}

static StringList *
string_insert(char *str, StringList *list)
{
  StringList *new, *cur;

  if ( (new = (StringList *) malloc (sizeof (StringList) ) ) == NULL )
    {
	  fprintf(stderr, "malloc() failed in string_insert()\n");
	  exit (1);
    }
  
  new->next = NULL;
  new->str = strdup(str);
  
  if (list == NULL)
    return (new);

  cur = list;
  while ( cur->next != NULL )
    cur = cur->next;
  
  cur->next = new;

  return (list);
}

static BomList *
bom_insert (char *refdes, char *descr, char *value, BomList *bom)
{
  BomList *new, *cur, *prev = NULL;

  if (bom == NULL)
    {
      /* this is the first element so automatically create an entry */
      if ( (new = (BomList *) malloc (sizeof (BomList) ) ) == NULL )
	{
	  fprintf(stderr, "malloc() failed in bom_insert()\n");
	  exit (1);
	}

      new->next = NULL;
      new->descr = strdup(descr);
      new->value = strdup(value);
      new->num = 1;
      new->refdes = string_insert(refdes, NULL);
      return (new);
    }

  /* search and see if we already have used one of these
     components */
  cur = bom;
  while (cur != NULL)
    {
      if ( (NSTRCMP(descr, cur->descr) == 0) &&
	   (NSTRCMP(value, cur->value) == 0) )
	{
	  cur->num++;
	  cur->refdes = string_insert(refdes, cur->refdes);
	  break;
	}
      prev = cur;
      cur = cur->next;
    }
  
  if (cur == NULL)
    {
      if ( (new = (BomList *) malloc (sizeof (BomList) ) ) == NULL )
	{
	  fprintf(stderr, "malloc() failed in bom_insert()\n");
	  exit (1);
	}
      
      prev->next = new;
      
      new->next = NULL;
      new->descr = strdup(descr);
      new->value = strdup(value);
      new->num = 1;
      new->refdes = string_insert(refdes, NULL);
    }

  return (bom);

}

static int
PrintBOM (void)
{
  char utcTime[64];
  double x, y, theta = 0.0;
  double sumx, sumy;
  double pin1x = 0.0, pin1y = 0.0, pin1angle = 0.0;
  double pin2x = 0.0, pin2y = 0.0, pin2angle;
  int found_pin1;
  int found_pin2;
  int pin_cnt;
  time_t currenttime;
  FILE *fp;
  BomList *bom = NULL;
  BomList *lastb;
  StringList *lasts;

  fp = fopen(xy_filename, "w");
  if (!fp)
    {
      gui->log("Cannot open file %s for writing\n", xy_filename);
      return 1;
    }

  /* Create a portable timestamp. */
  currenttime = time (NULL);
  strftime (utcTime, sizeof (utcTime), "%c UTC", gmtime (&currenttime));

  fprintf (fp, "# $Id");
  fprintf (fp, "$\n");
  fprintf (fp, "# PcbXY Version 1.0\n");
  fprintf (fp, "# Date: %s\n", utcTime);
  fprintf (fp, "# Author: %s\n", pcb_author());
  fprintf (fp, "# Title: %s - PCB X-Y\n", UNKNOWN (PCB->Name));
  fprintf (fp, "# RefDes, Description, Value, X, Y, rotation, top/bottom\n");
  fprintf (fp, "# X,Y in mils.  rotation in degrees.\n");
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
    bom = bom_insert ( UNKNOWN (NAMEONPCB_NAME (element)),
		       UNKNOWN (DESCRIPTION_NAME (element)),
		       UNKNOWN (VALUE_NAME (element)), bom);
      
    
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

      if (NSTRCMP(pin->Number, "1") == 0)
	{
	  pin1x = (double) pin->X;
	  pin1y = (double) pin->Y;
	  pin1angle = 0.0; /* pins have no notion of angle */
	  found_pin1 = 1;
	}
      else if (NSTRCMP(pin->Number, "2") == 0)
	{
	  pin2x = (double) pin->X;
	  pin2y = (double) pin->Y;
	  pin2angle = 0.0; /* pins have no notion of angle */
	  found_pin2 = 1;
	}
    }
    END_LOOP;
    
    PAD_LOOP (element);
    {
      sumx +=  (pad->Point1.X + pad->Point2.X)/2.0;
      sumy +=  (pad->Point1.Y + pad->Point2.Y)/2.0;
      pin_cnt++;

      if (NSTRCMP(pad->Number, "1") == 0)
	{
	  pin1x = (double) (pad->Point1.X + pad->Point2.X)/2.0;
	  pin1y = (double) (pad->Point1.Y + pad->Point2.Y)/2.0;
	  /*
	   * NOTE:  We swap the Y points because in PCB, the Y-axis
	   * is inverted.  Increasing Y moves down.  We want to deal
	   * in the usual increasing Y moves up coordinates though.
	   */
	  pin1angle = (180.0 / M_PI) * atan2(pad->Point1.Y - pad->Point2.Y ,
					     pad->Point2.X - pad->Point1.X );
	  found_pin1 = 1;
	}
      else if (NSTRCMP(pad->Number, "2") == 0)
	{
	  pin2x = (double) (pad->Point1.X + pad->Point2.X)/2.0;
	  pin2y = (double) (pad->Point1.Y + pad->Point2.Y)/2.0;
	  pin2angle = (180.0 / M_PI) * atan2(pad->Point1.Y - pad->Point2.Y ,
					     pad->Point2.X - pad->Point1.X );
	  found_pin2 = 1;
	}

    }
    END_LOOP;

    if ( pin_cnt > 0) 
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
		if ( (pin1x == 0.0) && (pin1y == 0.0) )
		  {
		    if (found_pin2)
		      theta = xyToAngle( pin2x, pin2y);
		    else
		      {
			Message ("PrintBOM(): unable to figure out angle of element\n"
				 "     %s because pin #1 is at the centroid of the part.\n"
				 "     and I could not find pin #2's location\n"
				 "     Setting to %g degrees\n",
				 UNKNOWN (NAMEONPCB_NAME(element) ), theta );
		      }
		  }
		else
		  theta = xyToAngle( pin1x, pin1y);
	      }
	  }
	/* we did not find pin #1 */
	else
	  {
	    theta = 0.0;
	    Message ("PrintBOM(): unable to figure out angle because I could\n"
		     "     not find pin #1 of element %s\n"
		     "     Setting to %g degrees\n",
		     UNKNOWN (NAMEONPCB_NAME(element) ),
		     theta );
	  }
	
	fprintf (fp, "%s,\"%s\",\"%s\",%.2f,%.2f,%g,%s\n", 
		 CleanBOMString(UNKNOWN (NAMEONPCB_NAME(element) )),
		 CleanBOMString(UNKNOWN (DESCRIPTION_NAME(element) )),
		 CleanBOMString(UNKNOWN (VALUE_NAME(element) )),
#if 0
		 (double) element->MarkX, (double) element->MarkY,
#else
		 0.01*x, 0.01*y,
#endif
		 theta,
		 FRONT(element) == 1 ? "top" : "bottom");
      }
  }
  END_LOOP;
  
  fclose (fp);

  /* Now print out a Bill of Materials file */

  fp = fopen(bom_filename, "w");
  if (!fp)
    {
      gui->log("Cannot open file %s for writing\n", bom_filename);
      return 1;
    }
    
  fprintf (fp, "# $Id");
  fprintf (fp, "$\n");
  fprintf (fp, "# PcbBOM Version 1.0\n");
  fprintf (fp, "# Date: %s\n", utcTime);
  fprintf (fp, "# Author: %s\n", pcb_author());
  fprintf (fp, "# Title: %s - PCB BOM\n", UNKNOWN (PCB->Name));
  fprintf (fp, "# Quantity, Description, Value, RefDes\n");
  fprintf (fp, "# --------------------------------------------\n");
  
  while (bom != NULL)
    {
      fprintf (fp,"%d,\"%s\",\"%s\",", 
	       bom->num, 
	       CleanBOMString(bom->descr), 
	       CleanBOMString(bom->value));
      while (bom->refdes != NULL)
	{
	  fprintf (fp,"%s ", bom->refdes->str);
	  free (bom->refdes->str);
	  lasts = bom->refdes;
	  bom->refdes = bom->refdes->next;
	  free (lasts);
	}
      fprintf (fp, "\n");
      lastb = bom;
      bom = bom->next;
      free (lastb);
    }

  fclose (fp);
  
  return (0);
}

static void
bom_do_export (HID_Attr_Val *options)
{
  int i;

  if (!options)
    {
      bom_get_export_options (0);
      for (i=0; i<NUM_OPTIONS; i++)
	bom_values[i] = bom_options[i].default_val;
      options = bom_values;
    }

  bom_filename = options[HA_bomfile].str_value;
  if (!bom_filename)
    bom_filename = "pcb-out.bom";

  xy_filename = options[HA_xyfile].str_value;
  if (!xy_filename)
    xy_filename = "pcb-out.xy";

  PrintBOM();
}

static void
bom_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (bom_options, sizeof(bom_options)/sizeof(bom_options[0]));
  hid_parse_command_line (argc, argv);
}

HID bom_hid = {
  "bom",
  "Exports a Bill of Materials",
  0, 0, 1, 0, 0,
  bom_get_export_options,
  bom_do_export,
  bom_parse_arguments,
  0, /* bom_invalidate_wh */
  0, /* bom_invalidate_lr */
  0, /* bom_invalidate_all */
  0, /* bom_set_layer */
  0, /* bom_make_gc */
  0, /* bom_destroy_gc */
  0, /* bom_use_mask */
  0, /* bom_set_color */
  0, /* bom_set_line_cap */
  0, /* bom_set_line_width */
  0, /* bom_set_draw_xor */
  0, /* bom_set_draw_faded */
  0, /* bom_set_line_cap_angle */
  0, /* bom_draw_line */
  0, /* bom_draw_arc */
  0, /* bom_draw_rect */
  0, /* bom_fill_circle */
  0, /* bom_fill_polygon */
  0, /* bom_fill_rect */
  0, /* bom_calibrate */
  0, /* bom_shift_is_pressed */
  0, /* bom_control_is_pressed */
  0, /* bom_get_coords */
  0, /* bom_set_crosshair */
  0, /* bom_add_timer */
  0, /* bom_stop_timer */
  0, /* bom_log */
  0, /* bom_logv */
  0, /* bom_confirm_dialog */
  0, /* bom_report_dialog */
  0, /* bom_prompt_for */
  0, /* bom_attribute_dialog */
  0, /* bom_show_item */
  0, /* bom_bee */
};

void
hid_bom_init()
{
  apply_default_hid(&bom_hid, 0);
  hid_register_hid (&bom_hid);
}
