/*!
 * \file src/hid/bom_md/bom_md.c
 *
 * \brief Exports a Bill Of Materials in MarkDown format.
 *
 * \copyright (C) 2020 Bert Timmerman
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2020 PCB Contributors
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
 */

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

static HID_Attribute bom_md_options[] = {
/* %start-doc options "82 BOM MarkDown Creation"
@ftable @code
@item --bom_md_file <string>
Name of the BOM MarkDown output file.
Parameter @code{<string>} can include a path.
@end ftable
%end-doc
*/
  {"bomfile", "Name of the BOM MarkDown output file",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_bom_md_file 0

/* %start-doc options "82 BOM MarkDown Creation"
@ftable @code
@item --attrs <string>
Name of the attributes input file.
Parameter @code{<string>} can include a path.
The input file can be any path, the format matches what gschem uses.
One attribute per line, and whitespace is ignored.  Example:
@example
device
manufacturer
manufacturer_part_number
vendor
vendor_part_number
@end example
@end ftable
%end-doc
*/
  {"attrs", "Name of the attributes input file",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_attrs 1

};

#define NUM_OPTIONS (sizeof(bom_md_options)/sizeof(bom_md_options[0]))

static HID_Attr_Val bom_md_values[NUM_OPTIONS];

static const char *bom_md_filename;

static char **attr_list = NULL;
static int attr_count = 0;
static int attr_max = 0;

typedef struct _string_list
{
  char *str;
  struct _string_list *next;
} string_list;

typedef struct _bom_md_list
{
  char *descr;
  char *value;
  int num;
  string_list *refdes;
  char **attrs;
  struct _bom_md_list *next;
} bom_md_list;

/*!
 * \brief Get export options.
 */
static HID_Attribute *
bom_md_get_export_options (int *n)
{
  static char *last_bom_md_filename = 0;

  if (PCB)
    derive_default_filename (PCB->Filename, &bom_md_options[HA_bom_md_file], ".bom.md", &last_bom_md_filename);

  if (!bom_md_options[HA_attrs].default_val.str_value)
    bom_md_options[HA_attrs].default_val.str_value = strdup("attribs");

  if (n)
    *n = NUM_OPTIONS;

  return bom_md_options;
}

/*!
 * \brief Clean up a given string.
 */
static char *
bom_md_clean_string (char *in)
{
  char *out;
  int i;

  if ((out = (char *) malloc ((strlen (in) + 1) * sizeof (char))) == NULL)
    {
      fprintf (stderr, (_("Error in bom_md_clean_string(): malloc() failed\n")));
      exit (1);
    }

  /* 
   * Copy over in to out with some character conversions.
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

static string_list *
bom_md_string_insert (char *str, string_list *list)
{
  string_list *newlist, *cur;

  if ((newlist = (string_list *) malloc (sizeof (string_list))) == NULL)
    {
      fprintf (stderr, (_("Error is bom_md_string_insert(): malloc() failed\n")));
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

/*!
 * \brief Insert in the \c bom_md_list.
 */
static bom_md_list *
bom_md_insert (char *refdes, char *descr, char *value, ElementType *e, bom_md_list * bom_md)
{
  bom_md_list *newlist = NULL, *cur = NULL, *prev = NULL;
  int i;
  char *val;


  if (bom_md != NULL)
    {
      /* search and see if we already have used one of these components. */
      cur = bom_md;

      while (cur != NULL)
      {
        int attr_mismatch = 0;

        for (i=0; i<attr_count; i++)
        {
          val = AttributeGet (e, attr_list[i]);
          val = val ? val : "";

          if (strcmp (val, cur->attrs[i]) != 0)
            attr_mismatch = 1;
        }

        if ((NSTRCMP (descr, cur->descr) == 0)
          && (NSTRCMP (value, cur->value) == 0)
          && !attr_mismatch)
        {
          cur->num++;
          cur->refdes = bom_md_string_insert (refdes, cur->refdes);
          return (bom_md);
        }

        prev = cur;
        cur = cur->next;
      }
    }

  if ((newlist = (bom_md_list *) malloc (sizeof (bom_md_list))) == NULL)
  {
    fprintf (stderr, (_("Error in bom_md_insert(): malloc() failed\n")));
    exit (1);
  }

  if (prev)
    prev->next = newlist;

  newlist->next = NULL;
  newlist->descr = strdup (descr);
  newlist->value = strdup (value);
  newlist->num = 1;
  newlist->refdes = bom_md_string_insert (refdes, NULL);

  if ((newlist->attrs = (char **) malloc (attr_count * sizeof (char *))) == NULL)
  {
    fprintf (stderr, (_("Error in bom_md_insert(): malloc() failed\n")));
    exit (1);
  }

  for (i=0; i<attr_count; i++)
  {
    val = AttributeGet (e, attr_list[i]);
    newlist->attrs[i] = val ? val : "";
  }

  if (bom_md == NULL)
    bom_md = newlist;

  return (bom_md);
}

/*!
 * \brief If \c fp is not NULL then print out the bill of materials
 * contained in \c bom_md.
 * Either way, free all memory which has been allocated for bom_md.
 */
static void
bom_md_print_and_free (FILE *fp, bom_md_list *bom_md)
{
  bom_md_list *lastb;
  string_list *lasts;
  char *descr, *value;

  while (bom_md != NULL)
  {
    if (fp)
    {
      descr = bom_md_clean_string (bom_md->descr);
      value = bom_md_clean_string (bom_md->value);
      fprintf (fp, "| %d | %s | %s |", bom_md->num, descr, value);
      free (descr);
      free (value);
    }

    while (bom_md->refdes != NULL)
    {
      if (fp)
      {
        fprintf (fp, " %s", bom_md->refdes->str);
      }

      free (bom_md->refdes->str);
      lasts = bom_md->refdes;
      bom_md->refdes = bom_md->refdes->next;
      free (lasts);
    }
    fprintf (fp, " |");

    if (fp)
    {
      int i;
      for (i = 0; i <attr_count; i++)
        fprintf (fp, " %s ", bom_md->attrs[i]);

      fprintf (fp, "\n");
    }

    free (bom_md->attrs);
    lastb = bom_md;
    bom_md = bom_md->next;
    free (lastb);
  }
}

/*!
 * \brief Fetch attribute list.
 */
static void
bom_md_fetch_attr_list ()
{
  int i;
  FILE *f;
  char buf[1024];
  char *fname;

  if (attr_list != NULL)
  {
    for (i=0; i<attr_count; i++)
      free (attr_list[i]);

    attr_count = 0;
  }

  fname = (char *)bom_md_options[HA_attrs].value;

  if (!fname)
    fname = (char *)bom_md_options[HA_attrs].default_val.str_value;

  printf((_("reading attrs from %s\n")), fname);

  f = fopen (fname, "r");

  if (f == NULL)
    return;

  while (fgets (buf, 1023, f) != NULL)
  {
      char *c = buf + strlen (buf) - 1;

      while (c >= buf && isspace (*c))
        *c-- = 0;

      c = buf;

      while (*c && isspace (*c))
        c++;

      if (*c)
      {
        if (attr_count == attr_max)
          {
            attr_max += 10;
            attr_list = (char **) realloc (attr_list, attr_max * sizeof (char *));
          }

        attr_list[attr_count++] = strdup (c);
      }
  }

  fclose (f);
}

/*!
 * \brief Print the BOM.
 */
static int
bom_md_print (void)
{
  char utcTime[64];
  time_t currenttime;
  FILE *fp;
  bom_md_list *bom_md = NULL;
  char *name, *descr, *value;
//  int rpindex;
  int i;
//  char fmt[256];

  bom_md_fetch_attr_list ();

  /* Create a portable timestamp. */
  currenttime = time (NULL);
  {
    /* Avoid gcc complaints. */
    const char *fmt = "%c UTC";
    strftime (utcTime, sizeof (utcTime), fmt, gmtime (&currenttime));
  }

  /* Now print out a Bill of Materials file in MarkDown format. */
  fp = fopen (bom_md_filename, "wb");

  if (!fp)
  {
    gui->log ((_("Cannot open file %s for writing\n")), bom_md_filename);
    bom_md_print_and_free (NULL, bom_md);
    return 1;
  }

  fprintf (fp, (_("# PCB Bill Of Materials MarkDown Version 1.0\n\n")));
  fprintf (fp, (_("Date: %s\n\n")), utcTime);
  fprintf (fp, (_("Author: %s\n\n")), pcb_author ());
  fprintf (fp, (_("Title: %s\n\n")), UNKNOWN (PCB->Name));
  fprintf (fp, (_("| Quantity | Description | Value | RefDes")));

  for (i = 0; i < attr_count; i++)
    fprintf (fp, " | %s", attr_list[i]);

  fprintf (fp, " |\n");
  fprintf (fp, "|----------|-------------|-------|--------");

  for (i = 0; i < attr_count; i++)
    fprintf (fp, "|-----");

  fprintf (fp, "|\n");

  ELEMENT_LOOP (PCB->Data);
  {
    /* Insert this component into the bill of materials list. */
    bom_md = bom_md_insert ((char *)UNKNOWN (NAMEONPCB_NAME (element)),
      (char *)UNKNOWN (DESCRIPTION_NAME (element)),
      (char *)UNKNOWN (VALUE_NAME (element)),
      element,
      bom_md);
    name = bom_md_clean_string ((char *)UNKNOWN (NAMEONPCB_NAME (element)));
    descr = bom_md_clean_string ((char *)UNKNOWN (DESCRIPTION_NAME (element)));
    value = bom_md_clean_string ((char *)UNKNOWN (VALUE_NAME (element)));
  }
  END_LOOP;



  bom_md_print_and_free (fp, bom_md);

  fclose (fp);

  return (0);
}

/*!
 * \brief Do export the MarkDown BOM.
 */
static void
bom_md_do_export (HID_Attr_Val * options)
{
  int i;

  if (!options)
  {
    bom_md_get_export_options (0);

    for (i = 0; i < NUM_OPTIONS; i++)
      bom_md_values[i] = bom_md_options[i].default_val;

    options = bom_md_values;
  }

  bom_md_filename = options[HA_bom_md_file].str_value;

  if (!bom_md_filename)
    bom_md_filename = "pcb-out.bom.md";

  bom_md_print ();
}

/*!
 * \brief Parse command line arguments.
 */
static void
bom_md_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (bom_md_options,
    sizeof (bom_md_options) / sizeof (bom_md_options[0]));

  hid_parse_command_line (argc, argv);
}

HID bom_md_hid;

/*!
 * \brief Initialise the exporter HID.
 */
void
hid_bom_md_init ()
{
  memset (&bom_md_hid, 0, sizeof (HID));

  common_nogui_init (&bom_md_hid);

  bom_md_hid.struct_size         = sizeof (HID);
  bom_md_hid.name                = "bom_md";
  bom_md_hid.description         = "Exports a Bill of Materials in MarkDown format";
  bom_md_hid.exporter            = 1;

  bom_md_hid.get_export_options  = bom_md_get_export_options;
  bom_md_hid.do_export           = bom_md_do_export;
  bom_md_hid.parse_arguments     = bom_md_parse_arguments;

  hid_register_hid (&bom_md_hid);
}

/* EOF */
