/*!
 * \file src/hid/xml.c
 *
 * \brief XML file format exporter.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * \author Copyright (C) 2021 PCB Contributors.
 *
 * Licensed under the terms of the GNU General Public
 * License, version 2 or later.
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contact addresses for paper mail and Email:
 * Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 * haceaton@aplcomm.jhuapl.edu
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "global.h"
#include "data.h"
#include "error.h"
#include "misc.h"
#include "layerflags.h"
#include "pcb-printf.h"
#include "file.h"
#include "strflags.h"

#include "hid.h"
#include "hid/common/hidnogui.h"
#include "../hidint.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define F2S(OBJ, TYPE) flags_to_string ((OBJ)->Flags, TYPE)
/*! \todo Move the F2S macro to macro.h, another copy of this macro lives in file.c. */


HID_Attribute xml_attribute_list[] = {

/* %start-doc options "81 XML Export"
@ftable @code
@item --xml-file <string>
Name of the XML output file. Parameter <string> can include a path.
@end ftable
%end-doc
*/
  {"xml-file", "Name of the XML output file",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_xmlfile 0

/* %start-doc options "81 XML Export"
@ftable @code
@item --xml-units <unit>
Unit of dimensions. Defaults to mil.
@end ftable
%end-doc
*/
  {"xml-units", "XML units",
   HID_Unit, 0, 0, {-1, 0, 0}, NULL, 0},
#define HA_unit 1

};


#define NUM_OPTIONS (sizeof(xml_attribute_list)/sizeof(xml_attribute_list[0]))


REGISTER_ATTRIBUTES (xml_attribute_list)


static HID_Attr_Val xml_values[NUM_OPTIONS];
static char *xml_filename;
static Unit *xml_units;


static HID_Attribute *
xml_get_export_options (int *n)
{
  static char *last_xml_filename = NULL;
  static int last_unit_value = -1;

  if (xml_attribute_list[HA_unit].default_val.int_value == last_unit_value)
  {
    if (Settings.grid_unit)
      xml_attribute_list[HA_unit].default_val.int_value = Settings.grid_unit->index;
    else
      xml_attribute_list[HA_unit].default_val.int_value = get_unit_struct ("mil")->index;
    last_unit_value = xml_attribute_list[HA_unit].default_val.int_value;
  }

  if (PCB)
  {
    derive_default_filename (PCB->Filename, &xml_attribute_list[HA_xmlfile], ".xml", &last_xml_filename);
  }

  if (n)
  {
    *n = NUM_OPTIONS;
  }

  return xml_attribute_list;
}


/*!
 * \brief Write a header to the XML output file.
 *
 * \todo Where in the filesystem would a stylesheet be installed ?\n
 * Does the FHS etc. say anything about this ?\n
 *
 * \warning We use /usr/share/pcb for now.
 */
static void
xml_write_header (FILE *fp)
{
  char *str;
  char utcTime[64];
  const char *fmt;
  time_t currenttime;

  if (!fp)
  {
    fprintf (stderr, (_("Error: invalid file pointer received.\n")));
    return;
  }

  /* Write an XML header. */
  fprintf (fp, "<?xml version=\"1.0\" standalone=\"no\"?>\n");

  fprintf (fp, "<?xml-stylesheet type=\"text/xsl\" href=\"pcb.xsl\"?>\n");

  fprintf (fp, "<!DOCTYPE pcb SYSTEM \"/usr/share/pcb/pcb.dtd\">\n");

  fprintf (fp, "<pcb>\n");

  /* PCB specific information. */
  fprintf (fp, "\t<program>%s</program>\n", Progname);

  fprintf (fp, "\t<release>" VERSION "</release>\n");

  fprintf (fp, "\t<file_version>%i</file_version>\n", PCBFileVersionNeeded ());

  if (PCB->Filename)
  {
    fprintf (fp, "\t<filename>%s</filename>\n", PCB->Filename);
  }

  /* Create a portable timestamp. */
  currenttime = time (NULL);

  {
    /* avoid gcc complaints */
    fmt = "%c UTC";
    strftime (utcTime, sizeof (utcTime), fmt, gmtime (&currenttime));
  }

  fprintf (fp, "\t<date>%s</date>\n", utcTime);

  fprintf (fp, "\t<author>%s</author>\n", pcb_author ());

  /* PCB name. */
  if (PCB->Name)
  {
    fprintf (fp, "\t<name>%s</name>\n", UNKNOWN(PCB->Name));
  }

  /*! \todo Do we need global units for the pcb ? */
//  fprintf (fp, "\t\t<grid-units>%s</grid-units>\n", Settings.grid_unit->suffix);

  /* PCB dimensions. */
  fprintf (fp, "\t<dimensions>\n");

  pcb_fprintf (fp, "\t\t<width>%mr</width>\n", PCB->MaxWidth);

  pcb_fprintf (fp, "\t\t<height>%mr</height>\n", PCB->MaxHeight);

  fprintf (fp, "\t</dimensions>\n");

  /* PCB grid information. */
  fprintf (fp, "\t<grid>\n");

  pcb_fprintf (fp, "\t\t<step>%mr</step>\n", PCB->Grid);

  pcb_fprintf (fp, "\t\t<offsetx>%mr</offsetx>\n", PCB->GridOffsetX);

  pcb_fprintf (fp, "\t\t<offsety>%mr</offsety>\n", PCB->GridOffsetY);

  /*! \todo Do we need grid units for the pcb ? */
//  fprintf (fp, "\t\t<grid-units>%s</grid-units>\n", Settings.grid_unit->suffix);

  fprintf (fp, "\t\t<visible>%d</visible>\n", Settings.DrawGrid);

  fprintf (fp, "\t</grid>\n");

  /* Poly area. */
  fprintf (fp, "\t<area>%s</area>\n", c_dtostr (COORD_TO_MIL (COORD_TO_MIL (PCB->IsleArea) * 100) * 100));

  /* Thermal scale. */
  pcb_fprintf (fp, "\t<thermal_scale>%s</thermal_scale>\n", c_dtostr (PCB->ThermScale));

  /* PCB DRC. */
  fprintf (fp, "\t<DRC>\n");

  pcb_fprintf (fp, "\t\t<bloat>%mr</bloat>\n", PCB->Bloat);

  pcb_fprintf (fp, "\t\t<shrink>%mr</shrink>\n", PCB->Shrink);

  pcb_fprintf (fp, "\t\t<line>%mr</line>\n", PCB->minWid);

  pcb_fprintf (fp, "\t\t<silk>%mr</silk>\n", PCB->minSlk);

  pcb_fprintf (fp, "\t\t<drill>%mr</drill>\n", PCB->minDrill);

  pcb_fprintf (fp, "\t\t<ring>%mr</ring>\n", PCB->minRing);

  fprintf (fp, "\t</DRC>\n");

  /* PCB symbolic flags */
  fprintf (fp, "\t<flags>%s</flags>\n", pcbflags_to_string(PCB->Flags));

  /* PCB layer groups. */
  /*! \todo Split layer groups per layer. */
  fprintf (fp, "\t<layer_groups>%s</layer_groups>\n", LayerGroupsToString (&PCB->LayerGroups));

  /* PCB routing styles. */
  /*! \todo Split routing styles per style. */
  str = make_route_string (PCB->RouteStyle, NUM_STYLES);

  fprintf (fp, "\t<styles>%s</styles>\n", str);

  g_free (str);
}


/*!
 * \brief Writes font data of non empty symbols to the XML otput file.
 */
static void
xml_write_font (FILE * fp)
{
  Cardinal i, j;
  LineType *line;
  FontType *font;

  fprintf (fp, "\t<symbols>\n");

  for (font = &PCB->Font, i = 0; i <= MAX_FONTPOSITION; i++)
  {
    if (!font->Symbol[i].Valid)
      continue;

    if (isprint (i))
    {
      fprintf (fp, "\t\t<symbol>\n");

      pcb_fprintf (fp, "\t\t\t<name>'%c'</name>\n", i);

      pcb_fprintf (fp, "\t\t\t<width>%mr</width>\n", font->Symbol[i].Delta);
    }
    else
    {
      fprintf (fp, "\t\t<symbol>\n");

      pcb_fprintf (fp, "\t\t<name>%i</name>\n", i, font->Symbol[i].Delta);

      pcb_fprintf (fp, "\t\t\t<width>%mr</width>\n", font->Symbol[i].Delta);
    }

    fprintf (fp, "\t\t\t<strokes>\n");

    line = font->Symbol[i].Line;

    for (j = font->Symbol[i].LineN; j; j--, line++)

    pcb_fprintf (fp, "\t\t\t\t<stroke>%mr %mr %mr %mr %mr</stroke>\n",
      line->Point1.X,
      line->Point1.Y,
      line->Point2.X,
      line->Point2.Y,
      line->Thickness);

    fprintf (fp, "\t\t\t<strokes>\n");
    fprintf (fp, "\t\t</symbol>\n");
  }

  fprintf (fp, "\t</symbols>\n");
}

/*!
 * \brief Write attributes to the XML output file.
 */
static void
xml_write_attributes (FILE *fp)
{
  int i;

  if (!fp)
  {
    fprintf (stderr, (_("Error: invalid file pointer received.\n")));
    return;
  }

  fprintf (fp, "\t<attributes>\n");

  for (i = 0; i < PCB->Attributes.Number; i++)
  {
    fprintf (fp, "\t\t<attribute>\n");

    fprintf (fp, "\t\t\t<name>%s</name>\n", PCB->Attributes.List[i].name);

    fprintf (fp, "\t\t\t<value>%s</value>\n", PCB->Attributes.List[i].value);

    fprintf (fp, "\t\t<attribute>\n");
  }

  fprintf (fp, "\t</attributes>\n");
}


/*!
 * \brief Write vias to the XML output file.
 */
static void
xml_write_vias (FILE *fp)
{
  if (!fp)
  {
    fprintf (stderr, (_("Error: invalid file pointer received.\n")));
    return;
  }

  fprintf (fp, "\t<vias>\n");

  VIA_LOOP (PCB->Data)
  {

    fprintf (fp, "\t\t<via>\n");

    /* ID of the via. */
    fprintf (fp, "\t\t\t<ID>%ld</ID>\n", via->ID);

    /* X-coordinate of the via (required). */
    pcb_fprintf (fp, "\t\t\t<x>%mr</x>\n", via->X);

    /* Y-coordinate of the via (required). */
    pcb_fprintf (fp, "\t\t\t<y>%mr</y>\n", via->Y);

    /* Thickness of the via (required). */
    pcb_fprintf (fp, "\t\t\t<thickness>%mr</thickness>\n", via->Thickness);

    /* Clearance of the via (required). */
    pcb_fprintf (fp, "\t\t\t<clearance>%mr</clearance>\n", via->Clearance);

    /* Mask of the via (required). */
    pcb_fprintf (fp, "\t\t\t<mask>%mr</mask>\n", via->Mask);

    /* Drilling hole of the via (required). */
    pcb_fprintf (fp, "\t\t\t<drill>%mr</drill>\n", via->DrillingHole);

    /* Blind and buried via information. */
    if ((via->BuriedFrom != 0) || (via->BuriedTo != 0))
    {
      /* Burried from layer (optional). */
      fprintf (fp, "\t\t\t<buried_from>%d</buried_from>\n", via->BuriedFrom);

      /* Burried to layer (optional). */
      fprintf (fp, "\t\t\t<buried_to>%d</buried_to>\n", via->BuriedTo);
    }

    /* Name of the via (required). */
    fprintf (fp, "\t\t\t<name>%s</name>\n", (char *)EMPTY (via->Name));

    /* Flags of the via (required). */
    /*! \todo Symbolic or numerical flags ? */
//    fprintf (fp, "\t\t\t<flags>%ld</flags>\n", via->Flags.f);
//    fprintf (fp, "\t\t\t<flags>0x%02hhx</flags>\n", via->Flags.f);

    fprintf (fp, "\t\t\t<flags>%s</flags>\n", F2S (via, VIA_TYPE));

    fprintf (fp, "\t\t</via>\n");
  }
  END_LOOP;

  fprintf (fp, "\t</vias>\n");
}


/*!
 * \brief Write elements to the XML output file.
 */
static void
xml_write_elements (FILE *fp)
{
  GList *p;
  AttributeType *a;
  PinType *pin;
  PadType *pad;
  LineType *line;
  ArcType *arc;

  if (!fp)
  {
    fprintf (stderr, (_("Error: invalid file pointer received.\n")));
    return;
  }

  fprintf (fp, "\t<elements>\n");

  ELEMENT_LOOP (PCB->Data);
  {
    int i;
    fprintf (fp, "\t\t<element>\n");

    /* ID of the element. */
    fprintf (fp, "\t\t\t<ID>%ld</ID>\n", element->ID);

    /* Flags of the element (optional). */
    /*! \todo Symbolic or numerical flags ? */
    fprintf (fp, "\t\t\t<flags>%s</flags>\n", F2S (element, ELEMENT_TYPE));

    /* Description of the element (required). */
    if (DESCRIPTION_NAME(element))
    {
      fprintf (fp, "\t\t\t<description>%s</description>\n", DESCRIPTION_NAME(element));

      pcb_fprintf  (stderr, "Found an element without a valid description at (%mr,%mr).\n", element->MarkX, element->MarkY);
    }

    /* Name (refdes) of the element (required). */
    if (NAMEONPCB_NAME(element))
    {
      fprintf (fp, "\t\t\t<name>%s</name>\n", NAMEONPCB_NAME(element));

      pcb_fprintf (stderr, "Found an element without a valid reference designator at (%mr,%mr).\n", element->MarkX, element->MarkY);
    }

    /* Value of the element (optional. */
    if (VALUE_NAME(element))
    {
      fprintf (fp, "\t\t\t<value>%s</value>\n", VALUE_NAME(element));
    }

    /* X-coordinate (required).*/
    pcb_fprintf (fp, "\t\t\t<x>%mr</x>\n", DESCRIPTION_TEXT (element).X);

    /* Y-coordinate (required).*/
    pcb_fprintf (fp, "\t\t\t<y>%mr</y>\n", DESCRIPTION_TEXT (element).Y);

    /* Mark X-coordinate (optional).*/
    pcb_fprintf (fp, "\t\t\t<mx>%mr</mx>\n", element->MarkX);

    /* Mark Y-coordinate. (optional)*/
    pcb_fprintf (fp, "\t\t\t<my>%mr</my>\n", element->MarkY);

    /* Text label X-coordinate (required).*/
    pcb_fprintf (fp, "\t\t\t<tx>%mr</tx>\n", DESCRIPTION_TEXT (element).X - element->MarkX);

    /* Text label Y-coordinate (required).*/
    pcb_fprintf (fp, "\t\t\t<ty>%mr</ty>\n", DESCRIPTION_TEXT (element).Y - element->MarkY);

    /* Text direction (required). */
    pcb_fprintf (fp, "\t\t\t<tdir>%mr</tdir>\n", DESCRIPTION_TEXT (element).Direction);

    /* Text scale (required). */
    pcb_fprintf (fp, "\t\t\t<tscale>%mr</tscale>\n", DESCRIPTION_TEXT (element).Scale);

    /* Text flags (required). */
    /*! \todo Symbolic or numerical text flags ? */
    fprintf (fp, "\t\t\t<tflags>%s</tflags>\n", F2S (&(DESCRIPTION_TEXT (element)), ELEMENTNAME_TYPE));

    /* Top or bottom side (required). */
    if (TEST_FLAG (ONSOLDERFLAG, element))
    {
      fprintf (fp, "\t\t\t<side>bottom</side>\n");
    }
    else
    {
      fprintf (fp, "\t\t\t<side>top</side>\n");
    }

    /* Figure out the rotation of the element. */
    if (AttributeGetFromList (&element->Attributes, "xy-fixed-rotation"))
    {
      fprintf (fp, "\t\t\t<rotation>%s</rotation>\n", AttributeGetFromList (&element->Attributes, "xy-fixed-rotation"));
    }
    else
    {
      /*! \todo Figure out the arbitrary angle based the method in PrintBOM () in bom.c. */
    }

    /* Element attributes (optional). */
    fprintf (fp, "\t\t\t<attributes>\n");

    for (i = 0; i < element->Attributes.Number; i++)
    {
      a = element->Attributes.List + i;

      fprintf (fp, "\t\t\t\t<attribute>\n");

      fprintf (fp, "\t\t\t\t\t<name>%s</name>\n", a->name);

      fprintf (fp, "\t\t\t\t\t<value>%s</value>\n", a->value);

      fprintf (fp, "\t\t\t\t</attribute>\n");
    }

    fprintf (fp, "\t\t\t</attributes>\n");

    /* Element pins (optional). */
    if (element->Pin)
    {
      fprintf (fp, "\t\t\t<pins>\n");

      for (p = element->Pin; p != NULL; p = g_list_next (p))
      {
        pin = p->data;

        fprintf (fp, "\t\t\t\t<pin>\n");

        fprintf (fp, "\t\t\t\t\t<ID>%ld</ID>\n", pin->ID);

        if (TEST_FLAG (HOLEFLAG, pin))
        {
          fprintf (fp, "\t\t\t\t\t<type>non-plated</type>\n");
        }
        else
        {
          fprintf (fp, "\t\t\t\t\t<type>plated</type>\n");
        }

        pcb_fprintf (fp, "\t\t\t\t\t<x>%mr</x>\n", pin->X - element->MarkX);

        pcb_fprintf (fp, "\t\t\t\t\t<y>%mr</y>\n", pin->Y - element->MarkY);

        pcb_fprintf (fp, "\t\t\t\t\t<thickness>%mr</thickness>\n", pin->Thickness);

        pcb_fprintf (fp, "\t\t\t\t\t<clearance>%mr</clearance>\n", pin->Clearance);

        pcb_fprintf (fp, "\t\t\t\t\t<mask>%mr</mask>\n", pin->Mask);

        pcb_fprintf (fp, "\t\t\t\t\t<drill>%mr</drill>\n", pin->DrillingHole);

        fprintf (fp, "\t\t\t\t\t<name>%s</name>\n", (char *)EMPTY (pin->Name));

        fprintf (fp, "\t\t\t\t\t<number>%s</number>\n", (char *)EMPTY (pin->Number));

        fprintf (fp, "\t\t\t\t\t<flags>%s</flags>\n", F2S (pin, PIN_TYPE));

        fprintf (fp, "\t\t\t\t</pin>\n");
      }

      fprintf (fp, "\t\t\t</pins>\n");
    }

    /* Element pads (optional). */
    if (element->Pad)
    {
      fprintf (fp, "\t\t\t<pads>\n");

      for (p = element->Pad; p != NULL; p = g_list_next (p))
      {
        pad = p->data;

        fprintf (fp, "\t\t\t\t<pad>\n");

        fprintf (fp, "\t\t\t\t\t<ID>%ld</ID>\n", pad->ID);

        pcb_fprintf (fp, "\t\t\t\t\t<x1>%mr</x1>\n", pad->Point1.X - element->MarkX);

        pcb_fprintf (fp, "\t\t\t\t\t<y1>%mr</y1>\n", pad->Point1.Y - element->MarkY);

        pcb_fprintf (fp, "\t\t\t\t\t<x2>%mr</x2>\n", pad->Point2.X - element->MarkX);

        pcb_fprintf (fp, "\t\t\t\t\t<y2>%mr</y2>\n", pad->Point2.Y - element->MarkY);

        pcb_fprintf (fp, "\t\t\t\t\t<thickness>%mr</thickness>\n", pad->Thickness);

        pcb_fprintf (fp, "\t\t\t\t\t<clearance>%mr</clearance>\n", pad->Clearance);

        pcb_fprintf (fp, "\t\t\t\t\t<mask>%mr</mask>\n", pad->Mask);

        fprintf (fp, "\t\t\t\t\t<name>%s</name>\n", (char *)EMPTY (pad->Name));

        fprintf (fp, "\t\t\t\t\t<number>%s</number>\n", (char *)EMPTY (pad->Number));

        fprintf (fp, "\t\t\t\t\t<flags>%s</flags>\n", F2S (pad, PAD_TYPE));

        fprintf (fp, "\t\t\t\t</pad>\n");
      }

      fprintf (fp, "\t\t\t</pads>\n");
    }

    /* Element lines (optional). */

    if (element->Line)
    {
      fprintf (fp, "\t\t\t<lines>\n");

      for (p = element->Line; p != NULL; p = g_list_next (p))
      {
        line = p->data;
        fprintf (fp, "\t\t\t\t<line>\n");

        fprintf (fp, "\t\t\t\t\t<ID>%ld</ID>\n", line->ID);

        pcb_fprintf (fp, "\t\t\t\t\t<x1>%mr</x1>\n", line->Point1.X - element->MarkX);

        pcb_fprintf (fp, "\t\t\t\t\t<y1>%mr</y1>\n", line->Point1.Y - element->MarkY);

        pcb_fprintf (fp, "\t\t\t\t\t<x2>%mr</x2>\n", line->Point2.X - element->MarkX);

        pcb_fprintf (fp, "\t\t\t\t\t<y2>%mr</y2>\n", line->Point2.Y - element->MarkY);

        pcb_fprintf (fp, "\t\t\t\t\t<thickness>%mr</thickness>\n", line->Thickness);

        fprintf (fp, "\t\t\t\t</line>\n");
      }

      fprintf (fp, "\t\t\t</lines>\n");
    }

    /* Element arcs (optional). */

    if (element->Arc)
    {
      fprintf (fp, "\t\t\t<arcs>\n");

      for (p = element->Arc; p != NULL; p = g_list_next (p))
      {
        arc = p->data;

        fprintf (fp, "\t\t\t\t<arc>\n");

        fprintf (fp, "\t\t\t\t\t<ID>%ld</ID>\n", arc->ID);

        pcb_fprintf (fp, "\t\t\t\t\t<x>%mr</x>\n", arc->X - element->MarkX);

        pcb_fprintf (fp, "\t\t\t\t\t<y>%mr</y>\n", arc->Y - element->MarkY);

        pcb_fprintf (fp, "\t\t\t\t\t<width>%mr</width>\n", arc->Width);

        pcb_fprintf (fp, "\t\t\t\t\t<height>%mr</height>\n", arc->Height);

        pcb_fprintf (fp, "\t\t\t\t\t<start-angle>%mr</start-angle>\n", arc->StartAngle);

        pcb_fprintf (fp, "\t\t\t\t\t<delta-angle>%mr</delta-angle>\n", arc->Delta);

        pcb_fprintf (fp, "\t\t\t\t\t<thickness>%mr</thickness>\n", arc->Thickness);

        fprintf (fp, "\t\t\t\t</arc>\n");
      }

      fprintf (fp, "\t\t\t</arcs>\n");
    }

    fprintf (fp, "\t\t</element>\n");
  }

  END_LOOP;

  fprintf (fp, "\t</elements>\n");
}


/*!
 * \brief Write ratlines to the XML output file.
 */
static void
xml_write_rats (FILE *fp)
{
  GList *iter;
  RatType *line;

  if (!fp)
  {
    fprintf (stderr, (_("Error: invalid file pointer received.\n")));
    return;
  }

  if (PCB->Data->Rat)
  {

    fprintf (fp, "\t<rats>\n");

    for (iter = PCB->Data->Rat; iter != NULL; iter = g_list_next (iter))
    {
      line = iter->data;

      fprintf (fp, "\t\t</rat>\n");

      fprintf (fp, "\t\t\t<ID>%ld</ID>\n", line->ID);

      pcb_fprintf (fp, "\t\t\t<x1>%mr</x1>\n", line->Point1.X);

      pcb_fprintf (fp, "\t\t\t<y1>%mr</y1>\n", line->Point1.Y);

      pcb_fprintf (fp, "\t\t\t<group1>%d</group1>\n", line->group1);

      pcb_fprintf (fp, "\t\t\t<x2>%mr</x2>\n", line->Point2.X);

      pcb_fprintf (fp, "\t\t\t<y2>%mr</y2>\n", line->Point2.Y);

      pcb_fprintf (fp, "\t\t\t<group2>%d</group2>\n", line->group2);

      fprintf (fp, "\t\t\t<flags>%s</flags>\n", F2S (line, RATLINE_TYPE));

      fprintf (fp, "\t\t</rat>\n");
    }

    fprintf (fp, "\t</rats>\n");
  }
}

/*!
 * \brief Write layers to the XML output file.
 */
static void
xml_write_layers (FILE *fp)
{
  int i;
  int j;
  int k;
  int p;
  GList *n;
  AttributeType *a;
  LineType *line;
  ArcType *arc;
  TextType *text;
  PolygonType *polygon;
  Cardinal hole;
  PointType *point;

  if (!fp)
  {
    fprintf (stderr, (_("Error: invalid file pointer received.\n")));

    return;
  }

  fprintf (fp, "\t<layers>\n");

  for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
  {
    fprintf (fp, "\t\t<layer>\n");

    fprintf (fp, "\t\t\t<number>%i</number>\n", (int) i + 1);

    fprintf (fp, "\t\t\t<name>%s</name>\n", (char *)EMPTY ((PCB->Data->Layer[i].Name)));

    fprintf (fp, "\t\t\t<type>%s</type>\n", layertype_to_string (PCB->Data->Layer[i].Type));

    fprintf (fp, "\t\t\t<visible>%d</visible>\n", (PCB->Data->Layer[i].On));

    fprintf (fp, "\t\t\t<color>%s</color>\n", (PCB->Data->Layer[i].Color));

    fprintf (fp, "\t\t\t<no-drc-flag>%d</no-drc-flag>\n", (PCB->Data->Layer[i].no_drc));

    /* Layer attributes (optional). */
    if (PCB->Data->Layer[i].Attributes.Number)
    {
      fprintf (fp, "\t\t\t<attributes>\n");

      for (j = 0; j < (PCB->Data->Layer[i].Attributes.Number); j++)
      {
        a = (PCB->Data->Layer[j].Attributes.List + j);

        fprintf (fp, "\t\t\t\t<attribute>\n");

        fprintf (fp, "\t\t\t\t\t<name>%s</name>\n", a->name);

        fprintf (fp, "\t\t\t\t\t<value>%s</value>\n", a->value);

        fprintf (fp, "\t\t\t\t</attribute>\n");
      }

      fprintf (fp, "\t\t\t</attributes>\n");
    }

    /* Write all lines on layer "i" (optional). */
    if (PCB->Data->Layer[i].Line)
    {
      fprintf (fp, "\t\t\t<lines>\n");

      for (n = PCB->Data->Layer[i].Line; n != NULL; n = g_list_next (n))
      {
        line = n->data;

        fprintf (fp, "\t\t\t\t<line>\n");

        fprintf (fp, "\t\t\t\t\t<ID>%ld</ID>\n", line->ID);

        pcb_fprintf (fp, "\t\t\t\t\t<x1>%mr</x1>\n", line->Point1.X);

        pcb_fprintf (fp, "\t\t\t\t\t<y1>%mr</y1>\n", line->Point1.Y);

        pcb_fprintf (fp, "\t\t\t\t\t<x2>%mr</x2>\n", line->Point2.X);

        pcb_fprintf (fp, "\t\t\t\t\t<y2>%mr</y2>\n", line->Point2.Y);

        pcb_fprintf (fp, "\t\t\t\t\t<thickness>%mr</thickness>\n", line->Thickness);

        pcb_fprintf (fp, "\t\t\t\t\t<clearance>%mr</clearance>\n", line->Clearance);

        pcb_fprintf (fp, "\t\t\t\t\t<flags>%s</flags>\n", F2S (line, LINE_TYPE));

        fprintf (fp, "\t\t\t\t</line>\n");
      }

      fprintf (fp, "\t\t\t</lines>\n");
    }

    /* Write all arcs on layer "i" (optional). */
    if (PCB->Data->Layer[i].Arc)
    {
      fprintf (fp, "\t\t\t<arcs>\n");

      for (n = PCB->Data->Layer[i].Arc; n != NULL; n = g_list_next (n))
      {
        arc = n->data;

        fprintf (fp, "\t\t\t\t<arc>\n");

        fprintf (fp, "\t\t\t\t\t<ID>%ld</ID>\n", arc->ID);

        pcb_fprintf (fp, "\t\t\t\t\t<x>%mr</x>\n", arc->X);

        pcb_fprintf (fp, "\t\t\t\t\t<y>%mr</y>\n", arc->Y);

        pcb_fprintf (fp, "\t\t\t\t\t<width>%mr</width>\n", arc->Width);

        pcb_fprintf (fp, "\t\t\t\t\t<height>%mr</height>\n", arc->Height);

        pcb_fprintf (fp, "\t\t\t\t\t<thickness>%mr</thickness>\n", arc->Thickness);

        pcb_fprintf (fp, "\t\t\t\t\t<clearance>%mr</clearance>\n", arc->Clearance);

        pcb_fprintf (fp, "\t\t\t\t\t<start-angle>%mr</start-angle>\n", arc->StartAngle);

        pcb_fprintf (fp, "\t\t\t\t\t<delta-angle>%mr</delta-angle>\n", arc->Delta);

        fprintf (fp, "\t\t\t\t\t<flags>%s</flags>\n", F2S (arc, ARC_TYPE));

        fprintf (fp, "\t\t\t\t</arc>\n");
      }

      fprintf (fp, "\t\t\t</arcs>\n");
    }

    /* Write all texts on layer "i" (optional). */
    if (PCB->Data->Layer[i].Text)
    {
      fprintf (fp, "\t\t\t<texts>\n");

      for (n = PCB->Data->Layer[i].Text; n != NULL; n = g_list_next (n))
      {
        text = n->data;

        fprintf (fp, "\t\t\t\t<text>\n");

        fprintf (fp, "\t\t\t\t\t<ID>%ld</ID>\n", text->ID);

        pcb_fprintf (fp, "\t\t\t\t\t<x>%mr</x>\n", text->X);

        pcb_fprintf (fp, "\t\t\t\t\t<y>%mr</y>\n", text->Y);

        fprintf (fp, "\t\t\t\t\t<direction>%d</direction>\n", text->Direction);

        fprintf (fp, "\t\t\t\t\t<scale>%d</scale>\n", text->Scale);

        fprintf (fp, "\t\t\t\t\t<value>%s</value>\n", (char *)EMPTY (text->TextString));

        fprintf (fp, "\t\t\t\t\t<flags>%s</flags>\n", F2S (text, TEXT_TYPE));

        fprintf (fp, "\t\t\t\t</text>\n");
      }

      fprintf (fp, "\t\t\t</texts>\n");
    }

    /* Write all polygons on layer "i" (optional). */
    if (PCB->Data->Layer[i].Polygon)
    {
      fprintf (fp, "\t\t\t</polygons>\n");

      for (n = PCB->Data->Layer[i].Polygon; n != NULL; n = g_list_next (n))
      {
        polygon = n->data;

        k = 0;
        p = 0;
        hole = 0;

        fprintf (fp, "\t\t\t\t<polygon>\n");

        fprintf (fp, "\t\t\t\t\t<ID>%ld</ID>\n", polygon->ID);

        fprintf (fp, "\t\t\t\t\t<flags>%s</flags>\n", F2S (polygon, POLYGON_TYPE));

        fprintf (fp, "\t\t\t\t\t<contour>\n");

        for (p = 0; p < polygon->PointN; p++)
        {
          point = &polygon->Points[p];

          if (hole < polygon->HoleIndexN && p == polygon->HoleIndex[hole])
          {
            if (hole > 0)
            {
              fprintf (fp, "\t\t\t\t\t<hole>\n");
            }
            else
            {
              fprintf (fp, "\t\t\t\t\t<contour>\n");
            }

            hole++;
            k = 0;
          }

          if (k++ % 5 == 0)
          {
//            if (hole)
//              fputs ("\t\t\t\t\t<hole>\n", fp);
          }

          fprintf (fp, "\t\t\t\t\t\t<point>\n");

          fprintf (fp, "\t\t\t\t\t\t\t<ID>%ld</ID>\n", point->ID);

          pcb_fprintf (fp, "\t\t\t\t\t\t\t<x>%mr</x>\n", point->X);

          pcb_fprintf (fp, "\t\t\t\t\t\t\t<y>%mr</y>\n", point->Y);

          fprintf (fp, "\t\t\t\t\t\t</point>\n");
        }

        if (hole > 0)
        {
          fprintf (fp, "\t\t\t\t\t</hole>\n)");
        }
        else
        {
          fprintf (fp, "\t\t\t\t\t</contour>\n");
        }

        fprintf (fp, "\t\t\t\t</polygon>\n");
      }

    fprintf (fp, "\t\t\t</polygons>\n");
    }

    fprintf (fp, "\t\t</layer>\n");
  }

  fprintf (fp, "\t</layers>\n");
}


/*!
 * \brief Write netlist to the XML output file.
 */
static void
xml_write_netlist (FILE *fp)
{
  int n;
  int p;
  LibraryMenuType *menu;
  LibraryEntryType *entry;

  if (!fp)
  {
    fprintf (stderr, (_("Error: invalid file pointer received.\n")));
    return;
  }

  if (PCB->NetlistLib.MenuN)
  {
    fprintf (fp, "\t<netlist>\n");

    for (n = 0; n < PCB->NetlistLib.MenuN; n++)
    {
      menu = &PCB->NetlistLib.Menu[n];
      fprintf (fp, "\t\t<net>\n");
      fprintf (fp, "\t\t\t<name>%s</name>\n", &menu->Name[2]);
      fprintf (fp, "\t\t\t<style>%s</style>\n", (char *)UNKNOWN (menu->Style));
      for (p = 0; p < menu->EntryN; p++)
      {
        entry = &menu->Entry[p];
        fprintf (fp, "\t\t\t<node>%s</node>\n", entry->ListEntry);
      }
      fprintf (fp, "\t\t</net>\n");
    }
    fprintf (fp, "\t</netlist>\n");
  }
}


/*!
 * \brief Write a footer to the XML output file.
 */
static void
xml_write_footer (FILE *fp)
{
  if (!fp)
  {
    fprintf (stderr, (_("Error: invalid file pointer received.\n")));
    return;
  }

  fprintf (fp, "</pcb>\n");
}


/*!
 * \brief Write all the selected entities to the xml file.
 */
static int
xml_write_xml (char * xml_filename)
{
  FILE *fp;

  if (xml_filename == NULL)
  {
    Message (_("The XML exporter needs an output filename.\n"));
    return (EXIT_FAILURE);
  }

  fp = fopen (xml_filename, "wb");

  if (!fp)
  {
    fprintf (stderr, (_("Error: could not open %s for writing: %s\n")), xml_filename, strerror (errno));
    return (EXIT_FAILURE);
  }

  xml_write_header (fp);
  xml_write_font (fp);
  xml_write_attributes (fp);
  xml_write_vias (fp);
  xml_write_elements (fp);
  xml_write_rats (fp);
  xml_write_layers (fp);
  xml_write_netlist (fp);
  xml_write_footer (fp);

  fclose (fp);

  return (EXIT_SUCCESS);
}


static void
xml_do_export (HID_Attr_Val * options)
{
  int i;

  if (!options)
    {
      xml_get_export_options (0);

      for (i = 0; i < NUM_OPTIONS; i++)
        xml_values[i] = xml_attribute_list[i].default_val;

      options = xml_values;
    }

  xml_filename = options[HA_xmlfile].str_value;

  if (!xml_filename)
  {
    xml_filename = "pcb-out.xml";
  }

  if (options[HA_unit].int_value)
  {
    xml_units = get_unit_struct ("mm");
  }
  else
  {
    /* options[HA_unit].int_value gets set to the index of the passed unit 
     * in the Units structure (pcb-printf.c) by hid_parse_command_line 
     * (hid/common/hidinit.c), so, this command gets a pointer to the desired 
     * unit structure from that list.
     */
    xml_units = &(get_unit_list ()[options[HA_unit].int_value]);
  }

  xml_write_xml (xml_filename);
}


static void
xml_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (xml_attribute_list,
    sizeof (xml_attribute_list) / sizeof (xml_attribute_list[0]));

  hid_parse_command_line (argc, argv);
}


HID xml_hid;


void
hid_xml_init ()
{
  memset (&xml_hid, 0, sizeof (HID));

  common_nogui_init (&xml_hid);

  xml_hid.struct_size         = sizeof (HID);
  xml_hid.name                = "xml";
  xml_hid.description         = "Exports to an XML file";
  xml_hid.exporter            = 1;

  xml_hid.get_export_options  = xml_get_export_options;
  xml_hid.do_export           = xml_do_export;
  xml_hid.parse_arguments     = xml_parse_arguments;

  hid_register_hid (&xml_hid);
}
