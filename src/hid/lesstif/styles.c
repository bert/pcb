#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "xincludes.h"

#include "compat.h"
#include "global.h"
#include "data.h"
#include "set.h"
#include "misc.h"
#include "mymem.h"
#include "pcb-printf.h"

#include "hid.h"
#include "../hidint.h"
#include "lesstif.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* There are three places where styles are kept:

   First, the "active" style is in Settings.LineThickness et al.

   Second, there are NUM_STYLES styles in PCB->RouteStyle[].

   Third, there are NUM_STYLES styles in Settings.RouteStyle[]

   Selecting a style copies its values to the active style.  We also
   need a way to modify the active style, copy the active style to
   PCB->RouteStyle[], and copy PCB->RouteStyle[] to
   Settings.RouteStyle[].  Since Lesstif reads the default style from
   .Xdefaults, we can ignore Settings.RouteStyle[] in general, as it's
   only used to initialize new PCB files.

   So, we need to do PCB->RouteStyle <-> active style.
*/

static Arg args[30];
static int n;
#define stdarg(t,v) XtSetArg(args[n], t, v); n++

typedef enum
{
  SSthick, SSdiam, SShole, SSkeep, SSviamask,
  SSNUM
} StyleValues;

static Widget style_dialog = 0;
static Widget style_values[SSNUM];
static Widget style_pb[NUM_STYLES];
static Widget units_pb[SSNUM];
static int name_hashes[NUM_STYLES];
static Widget value_form, value_labels, value_texts, units_form;
static int local_update = 0;
XmString xms_mm, xms_mil;

static const Unit *unit = 0;
static XmString ustr;

static int
hash (char *cp)
{
  int h = 0;
  while (*cp)
    {
      h = h * 13 + *(unsigned char *) cp;
      h ^= (h >> 16);
      cp++;
    }
  return h;
}

typedef struct
{
  Widget w[NUM_STYLES];
} StyleButtons;

static StyleButtons *style_button_list = 0;
static int num_style_buttons = 0;

static char *value_names[] = {
  "Thickness", "Diameter", "Hole", "Keepaway", "ViaMask"
};

static int RouteStylesChanged (int argc, char **argv, Coord x, Coord y);

static void
update_one_value (int i, Coord v)
{
  char buf[100];

  pcb_snprintf (buf, sizeof (buf), "%m+%.2mS", unit->allow, v);
  XmTextSetString (style_values[i], buf);
  n = 0;
  stdarg (XmNlabelString, ustr);
  XtSetValues (units_pb[i], args, n);
}

static void
update_values ()
{
  local_update = 1;
  update_one_value (SSthick, Settings.LineThickness);
  update_one_value (SSdiam, Settings.ViaThickness);
  update_one_value (SShole, Settings.ViaDrillingHole);
  update_one_value (SSkeep, Settings.Keepaway);
  update_one_value (SSviamask, Settings.ViaMaskAperture);
  local_update = 0;
  lesstif_update_status_line ();
}

void
lesstif_styles_update_values ()
{
  if (!style_dialog)
    {
      lesstif_update_status_line ();
      return;
    }
  unit = Settings.grid_unit;
  ustr = XmStringCreateLocalized ((char *)Settings.grid_unit->suffix);
  update_values ();
}

static void
update_style_buttons ()
{
  int i = hid_get_flag ("style");
  int j, n;

  for (n = 0; n < num_style_buttons; n++)
  {
    for (j = 0; j < NUM_STYLES; j++)
	    if (j != i - 1)
	      XmToggleButtonSetState (style_button_list[n].w[j], 0, 0);
	    else
	      XmToggleButtonSetState (style_button_list[n].w[j], 1, 0);
  }
  if (style_dialog)
  {
    for (j = 0; j < NUM_STYLES; j++)
	    if (j != i - 1)
	      XmToggleButtonSetState (style_pb[j], 0, 0);
	    else
	      XmToggleButtonSetState (style_pb[j], 1, 0);
  }
}

static void
style_value_cb (Widget w, int i, void *cbs)
{
  Coord n;
  char *s;

  if (local_update)
    return;
  s = XmTextGetString (w);
  n = GetValueEx (s, NULL, NULL, NULL, unit->suffix);
  switch (i)
    {
    case SSthick:
      Settings.LineThickness = n;
      break;
    case SSdiam:
      Settings.ViaThickness = n;
      break;
    case SShole:
      Settings.ViaDrillingHole = n;
      break;
    case SSkeep:
      Settings.Keepaway = n;
      break;
    case SSviamask:
      Settings.ViaMaskAperture = n;
      break;
    }
  update_style_buttons ();
}

static void
units_cb ()
{
  if (unit == get_unit_struct ("mm"))
    unit = get_unit_struct ("mil");
  else
    unit = get_unit_struct ("mm");
  ustr = XmStringCreateLocalized ((char *)unit->suffix);
  update_values ();
}

static Widget
style_value (int i)
{
  Widget w, l;
  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_POSITION);
  stdarg (XmNtopPosition, i);
  stdarg (XmNbottomAttachment, XmATTACH_POSITION);
  stdarg (XmNbottomPosition, i + 1);
  stdarg (XmNleftAttachment, XmATTACH_FORM);
  stdarg (XmNrightAttachment, XmATTACH_FORM);
  stdarg (XmNalignment, XmALIGNMENT_END);
  l = XmCreateLabel (value_labels, value_names[i], args, n);
  XtManageChild (l);

  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_POSITION);
  stdarg (XmNtopPosition, i);
  stdarg (XmNbottomAttachment, XmATTACH_POSITION);
  stdarg (XmNbottomPosition, i + 1);
  stdarg (XmNleftAttachment, XmATTACH_FORM);
  stdarg (XmNrightAttachment, XmATTACH_FORM);
  stdarg (XmNcolumns, 8);
  w = XmCreateTextField (value_texts, value_names[i], args, n);
  XtAddCallback (w, XmNvalueChangedCallback,
		 (XtCallbackProc) style_value_cb, (XtPointer) (size_t) i);
  XtManageChild (w);

  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_POSITION);
  stdarg (XmNtopPosition, i);
  stdarg (XmNbottomAttachment, XmATTACH_POSITION);
  stdarg (XmNbottomPosition, i + 1);
  stdarg (XmNleftAttachment, XmATTACH_FORM);
  stdarg (XmNrightAttachment, XmATTACH_FORM);
  stdarg (XmNlabelString, ustr);
  units_pb[i] = XmCreatePushButton (units_form, value_names[i], args, n);
  XtAddCallback (units_pb[i], XmNactivateCallback,
		 (XtCallbackProc) units_cb, (XtPointer) (size_t) i);
  XtManageChild (units_pb[i]);

  return w;
}

static void
style_name_cb (Widget w, int i, XmToggleButtonCallbackStruct * cbs)
{
  char *newname = lesstif_prompt_for ("New name", PCB->RouteStyle[i].Name);
  free (PCB->RouteStyle[i].Name);
  PCB->RouteStyle[i].Name = newname;
  RouteStylesChanged (0, 0, 0, 0);
}

static void
style_set_cb (Widget w, int i, XmToggleButtonCallbackStruct * cbs)
{
  PCB->RouteStyle[i].Thick = Settings.LineThickness;
  PCB->RouteStyle[i].Diameter = Settings.ViaThickness;
  PCB->RouteStyle[i].Hole = Settings.ViaDrillingHole;
  PCB->RouteStyle[i].Keepaway = Settings.Keepaway;
  PCB->RouteStyle[i].ViaMask = Settings.ViaMaskAperture;
  update_style_buttons ();
}

static void
style_selected (Widget w, int i, XmToggleButtonCallbackStruct * cbs)
{
  RouteStyleType *style;
  int j, n;
  if (cbs && cbs->set == 0)
    {
      XmToggleButtonSetState (w, 1, 0);
      return;
    }
  style = PCB->RouteStyle + i;
  SetLineSize (style->Thick);
  SetViaSize (style->Diameter, true);
  SetViaDrillingHole (style->Hole, true);
  SetKeepawayWidth (style->Keepaway);
  SetViaMaskAperture(style->ViaMask);
  if (style_dialog)
    {
      for (j = 0; j < NUM_STYLES; j++)
	if (j != i)
	  XmToggleButtonSetState (style_pb[j], 0, 0);
	else
	  XmToggleButtonSetState (style_pb[j], 1, 0);
      update_values ();
    }
  else
    lesstif_update_status_line ();
  for (n = 0; n < num_style_buttons; n++)
    {
      for (j = 0; j < NUM_STYLES; j++)
	if (j != i)
	  XmToggleButtonSetState (style_button_list[n].w[j], 0, 0);
	else
	  XmToggleButtonSetState (style_button_list[n].w[j], 1, 0);
    }
}

static Widget
style_button (int i)
{
  Widget pb, set;

  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_WIDGET);
  stdarg (XmNtopWidget, i ? style_pb[i - 1] : value_form);
  stdarg (XmNleftAttachment, XmATTACH_FORM);
  stdarg (XmNlabelString, XmStringCreatePCB ("Name"));
  set = XmCreatePushButton (style_dialog, "style", args, n);
  XtManageChild (set);
  XtAddCallback (set, XmNactivateCallback,
		 (XtCallbackProc) style_name_cb, (XtPointer) (size_t) i);

  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_WIDGET);
  stdarg (XmNtopWidget, i ? style_pb[i - 1] : value_form);
  stdarg (XmNleftAttachment, XmATTACH_WIDGET);
  stdarg (XmNleftWidget, set);
  stdarg (XmNlabelString, XmStringCreatePCB ("Set"));
  set = XmCreatePushButton (style_dialog, "style", args, n);
  XtManageChild (set);
  XtAddCallback (set, XmNactivateCallback,
		 (XtCallbackProc) style_set_cb, (XtPointer) (size_t) i);

  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_WIDGET);
  stdarg (XmNtopWidget, i ? style_pb[i - 1] : value_form);
  stdarg (XmNrightAttachment, XmATTACH_FORM);
  stdarg (XmNleftAttachment, XmATTACH_WIDGET);
  stdarg (XmNleftWidget, set);
  stdarg (XmNlabelString, XmStringCreatePCB (PCB->RouteStyle[i].Name));
  stdarg (XmNindicatorType, XmONE_OF_MANY);
  stdarg (XmNalignment, XmALIGNMENT_BEGINNING);
  pb = XmCreateToggleButton (style_dialog, "style", args, n);
  XtManageChild (pb);
  XtAddCallback (pb, XmNvalueChangedCallback,
		 (XtCallbackProc) style_selected, (XtPointer) (size_t) i);
  return pb;
}

static const char adjuststyle_syntax[] =
"AdjustStyle()";

static const char adjuststyle_help[] =
"Displays the route style adjustment window.";

/* %start-doc actions AdjustStyle

%end-doc */

static int
AdjustStyle (int argc, char **argv, Coord x, Coord y)
{
  if (!mainwind)
    return 1;
  if (style_dialog == 0)
    {
      int i;

      unit = Settings.grid_unit;
      ustr = XmStringCreateLocalized ((char *)unit->suffix);

      n = 0;
      stdarg (XmNautoUnmanage, False);
      stdarg (XmNtitle, "Route Styles");
      style_dialog = XmCreateFormDialog (mainwind, "style", args, n);

      n = 0;
      stdarg (XmNtopAttachment, XmATTACH_FORM);
      stdarg (XmNleftAttachment, XmATTACH_FORM);
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      value_form = XmCreateForm (style_dialog, "values", args, n);
      XtManageChild (value_form);

      n = 0;
      stdarg (XmNtopAttachment, XmATTACH_FORM);
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      stdarg (XmNbottomAttachment, XmATTACH_FORM);
      stdarg (XmNfractionBase, SSNUM);
      stdarg (XmNresizePolicy, XmRESIZE_GROW);
      units_form = XmCreateForm (value_form, "units", args, n);
      XtManageChild (units_form);

      n = 0;
      stdarg (XmNtopAttachment, XmATTACH_FORM);
      stdarg (XmNbottomAttachment, XmATTACH_FORM);
      stdarg (XmNleftAttachment, XmATTACH_FORM);
      stdarg (XmNfractionBase, SSNUM);
      value_labels = XmCreateForm (value_form, "values", args, n);
      XtManageChild (value_labels);

      n = 0;
      stdarg (XmNtopAttachment, XmATTACH_FORM);
      stdarg (XmNbottomAttachment, XmATTACH_FORM);
      stdarg (XmNrightAttachment, XmATTACH_WIDGET);
      stdarg (XmNrightWidget, units_form);
      stdarg (XmNleftAttachment, XmATTACH_WIDGET);
      stdarg (XmNleftWidget, value_labels);
      stdarg (XmNfractionBase, SSNUM);
      value_texts = XmCreateForm (value_form, "values", args, n);
      XtManageChild (value_texts);

      for (i = 0; i < SSNUM; i++)
      {
	      style_values[i] = style_value (i);
      }
      
      for (i = 0; i < NUM_STYLES; i++)
      {
        name_hashes[i] = hash (PCB->RouteStyle[i].Name);
        style_pb[i] = style_button (i);
      }
      update_values ();
      update_style_buttons ();
    }
  XtManageChild (style_dialog);
  return 0;
}

static int
RouteStylesChanged (int argc, char **argv, Coord x, Coord y)
{
  int i, j, h;
  if (!PCB || !PCB->RouteStyle[0].Name)
    return 0;
  update_style_buttons ();
  if (!style_dialog)
    return 0;
  for (j = 0; j < NUM_STYLES; j++)
    {
      h = hash (PCB->RouteStyle[j].Name);
      if (name_hashes[j] == h)
	continue;
      name_hashes[j] = h;
      n = 0;
      stdarg (XmNlabelString,
	      XmStringCreatePCB (PCB->RouteStyle[j].Name));
      if (style_dialog)
	XtSetValues (style_pb[j], args, n);
      for (i = 0; i < num_style_buttons; i++)
	XtSetValues (style_button_list[i].w[j], args, n);
    }
  update_values ();
  return 0;
}

void
lesstif_insert_style_buttons (Widget menu)
{
  StyleButtons *sb;
  int s, i;

  num_style_buttons++;
  s = num_style_buttons * sizeof (StyleButtons);
  style_button_list = (StyleButtons *) realloc (style_button_list, s);
  sb = style_button_list + num_style_buttons - 1;

  for (i = 0; i < NUM_STYLES; i++)
    {
      Widget btn;
      n = 0;
      stdarg (XmNindicatorType, XmONE_OF_MANY);
      stdarg (XmNlabelString,
	      XmStringCreatePCB (PCB->RouteStyle[i].Name));
      btn = XmCreateToggleButton (menu, "style", args, n);
      XtManageChild (btn);
      XtAddCallback (btn, XmNvalueChangedCallback,
		     (XtCallbackProc) style_selected, (XtPointer) (size_t) i);
      sb->w[i] = btn;
    }
  update_style_buttons ();
}

HID_Action lesstif_styles_action_list[] = {
  {"AdjustStyle", 0, AdjustStyle,
   adjuststyle_help, adjuststyle_syntax},
  {"RouteStylesChanged", 0, RouteStylesChanged,
   routestyleschanged_help, routestyleschanged_syntax}
};

REGISTER_ACTIONS (lesstif_styles_action_list)
