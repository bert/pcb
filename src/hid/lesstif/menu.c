#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "xincludes.h"

#include "global.h"
#include "data.h"
#include "error.h"
#include "misc.h"
#include "pcb-printf.h"

#include "hid.h"
#include "../hidint.h"
#include "hid/common/hid_resource.h"
#include "resource.h"
#include "lesstif.h"
#include "mymem.h"
#include "layerflags.h"

#include "pcb-menu.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#ifndef R_OK
/* Common value for systems that don't define it.  */
#define R_OK 4
#endif

static Colormap cmap;

static Arg args[30];
static int n;
#define stdarg(t,v) XtSetArg(args[n], t, v), n++

static void note_accelerator (char *acc, Resource * node);
static void note_widget_flag (Widget w, char *type, char *name);

static const char getxy_syntax[] =
"GetXY()";

static const char getxy_help[] =
"Get a coordinate.";

/*!
 * \brief Get a coordinate.
 *
 * Each action is registered with a flag for the core ("need_coord_msg")
 * that says whether it requires an X,Y location from the user, and if
 * so, provides a prompt for that X,Y location.
 *
 * The GUIs display that prompt while waiting for the user to click.
 *
 * GetXY() is a "filler" action which exists *only* to request an X,Y
 * click from the user, for actions that don't (or optionally) need
 * coords, or don't have a message appropriate for the case being issued
 * (i.e. if there are multiple actions to be executed, the GetXY() might
 * be appropriate for the *group* results, not the first action that
 * happens to need a coordinate).
 *
 * The catch is that the core asks for the X,Y click *before* the action
 * is called, so there's no way for any argument to GetXY() to have any
 * effect.
 *
 * The string "printed" is the empty one in the struct where GetXY() is
 * registered.
 */
/* %start-doc actions GetXY

Prompts the user for a coordinate, if one is not already selected.

%end-doc */

static int
GetXY (int argc, char **argv, Coord x, Coord y)
{
  return 0;
}

static const char debug_syntax[] =
"Debug(...)";

static const char debug_help[] =
"Debug action.";

/* %start-doc actions Debug

This action exists to help debug scripts; it simply prints all its
arguments to stdout.

%end-doc */

static const char debugxy_syntax[] =
"DebugXY(...)";

static const char debugxy_help[] =
"Debug action, with coordinates";

/* %start-doc actions DebugXY

Like @code{Debug}, but requires a coordinate.  If the user hasn't yet
indicated a location on the board, the user will be prompted to click
on one.

%end-doc */

static int
Debug (int argc, char **argv, Coord x, Coord y)
{
  int i;
  printf ("Debug:");
  for (i = 0; i < argc; i++)
    printf (" [%d] `%s'", i, argv[i]);
  pcb_printf (" x,y %$mD\n", x, y);
  for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
    {
      printf("0x%08x %s (%s)\n",
             PCB->Data->Layer[i].Type,
             PCB->Data->Layer[i].Name,
             layertype_to_string (PCB->Data->Layer[i].Type));
    }
  return 0;
}

static const char return_syntax[] =
"Return(0|1)";

static const char return_help[] =
"Simulate a passing or failing action.";

/* %start-doc actions Return

This is for testing.  If passed a 0, does nothing and succeeds.  If
passed a 1, does nothing but pretends to fail.

%end-doc */

static int
Return (int argc, char **argv, Coord x, Coord y)
{
  if (argc < 1)
    return 0;
  else if (argc == 1)
    return atoi (argv[0]);

  return 1;
}

static const char dumpkeys_syntax[] =
"DumpKeys()";

static const char dumpkeys_help[] =
"Dump Lesstif key bindings.";

/* %start-doc actions DumpKeys

Causes the list of key bindings (from @code{pcb-menu.res}) to be
dumped to stdout.  This is most useful when invoked from the command
line like this:

@example
pcb --action-string DumpKeys
@end example

%end-doc */

static int do_dump_keys = 0;
static int
DumpKeys (int argc, char **argv, Coord x, Coord y)
{
  do_dump_keys = 1;
  return 0;
}

/*-----------------------------------------------------------------------------*/

#define LB_SILK (MAX_LAYER + BOTTOM_SILK_LAYER)
#define LB_RATS (MAX_LAYER + 1)
#define LB_NUMPICK (LB_RATS+1)
/* more */
#define LB_PINS (MAX_ALL_LAYER)
#define LB_VIAS (MAX_ALL_LAYER + 1)
#define LB_BACK (MAX_ALL_LAYER + 2)
#define LB_MASK (MAX_ALL_LAYER + 3)
#define LB_NUM  (MAX_ALL_LAYER + 4)

typedef struct
{
  Widget w[LB_NUM];
  int is_pick;
} LayerButtons;

static LayerButtons *layer_button_list = 0;
static int num_layer_buttons = 0;
static int fg_colors[LB_NUM];
static int bg_color;

extern Widget lesstif_m_layer;

static int
LayersChanged (int argc, char **argv, Coord x, Coord y)
{
  int l, i, set;
  char *name;
  int current_layer;

  if (!layer_button_list)
    return 0;
  if (PCB && PCB->Data)
    {
      DataType *d = PCB->Data;
      for (i = 0; i < MAX_LAYER; i++)
	fg_colors[i] = lesstif_parse_color (d->Layer[i].Color);
      fg_colors[LB_SILK] = lesstif_parse_color (PCB->ElementColor);
      fg_colors[LB_RATS] = lesstif_parse_color (PCB->RatColor);
      fg_colors[LB_PINS] = lesstif_parse_color (PCB->PinColor);
      fg_colors[LB_VIAS] = lesstif_parse_color (PCB->ViaColor);
      fg_colors[LB_BACK] =
	lesstif_parse_color (PCB->InvisibleObjectsColor);
      fg_colors[LB_MASK] = lesstif_parse_color (PCB->MaskColor);
      bg_color = lesstif_parse_color (Settings.BackgroundColor);
    }
  else
    {
      for (i = 0; i < MAX_LAYER; i++)
	fg_colors[i] = lesstif_parse_color (Settings.LayerColor[i]);
      fg_colors[LB_SILK] = lesstif_parse_color (Settings.ElementColor);
      fg_colors[LB_RATS] = lesstif_parse_color (Settings.RatColor);
      fg_colors[LB_PINS] = lesstif_parse_color (Settings.PinColor);
      fg_colors[LB_VIAS] = lesstif_parse_color (Settings.ViaColor);
      fg_colors[LB_BACK] =
	lesstif_parse_color (Settings.InvisibleObjectsColor);
      fg_colors[LB_MASK] = lesstif_parse_color (Settings.MaskColor);
      bg_color = lesstif_parse_color (Settings.BackgroundColor);
    }

  if (PCB->RatDraw)
    current_layer = LB_RATS;
  else if (PCB->SilkActive)
    current_layer = LB_SILK;
  else
    current_layer = LayerStack[0];

  for (l = 0; l < num_layer_buttons; l++)
    {
      LayerButtons *lb = layer_button_list + l;
      for (i = 0; i < (lb->is_pick ? LB_NUMPICK : LB_NUM); i++)
	{
	  switch (i)
	    {
	    case LB_SILK:
	      set = PCB->ElementOn;
	      break;
	    case LB_RATS:
	      set = PCB->RatOn;
	      break;
	    case LB_PINS:
	      set = PCB->PinOn;
	      break;
	    case LB_VIAS:
	      set = PCB->ViaOn;
	      break;
	    case LB_BACK:
	      set = PCB->InvisibleObjectsOn;
	      break;
	    case LB_MASK:
	      set = TEST_FLAG (SHOWMASKFLAG, PCB);
	      break;
	    default:		/* layers */
	      set = PCB->Data->Layer[i].On;
	      break;
	    }

	  n = 0;
	  if (i < MAX_LAYER && PCB->Data->Layer[i].Name)
	    {
	      XmString s = XmStringCreatePCB (PCB->Data->Layer[i].Name);
	      stdarg (XmNlabelString, s);
	    }
	  if (!lb->is_pick)
	    {
	      if (set)
		{
		  stdarg (XmNforeground, bg_color);
		  stdarg (XmNbackground, fg_colors[i]);
		}
	      else
		{
		  stdarg (XmNforeground, fg_colors[i]);
		  stdarg (XmNbackground, bg_color);
		}
	      stdarg (XmNset, set);
	    }
	  else
	    {
	      stdarg (XmNforeground, bg_color);
	      stdarg (XmNbackground, fg_colors[i]);
	      stdarg (XmNset, current_layer == i ? True : False);
	    }
	  XtSetValues (lb->w[i], args, n);

	  if (i >= max_copper_layer && i < MAX_LAYER)
	    XtUnmanageChild(lb->w[i]);
	  else
	    XtManageChild(lb->w[i]);
	}
    }
  if (lesstif_m_layer)
    {
      switch (current_layer)
	{
	case LB_RATS:
	  name = "Rats";
	  break;
	case LB_SILK:
	  name = "Silk";
	  break;
	default:
	  name = PCB->Data->Layer[current_layer].Name;
	  break;
	}
      n = 0;
      stdarg (XmNbackground, fg_colors[current_layer]);
      stdarg (XmNforeground, bg_color);
      stdarg (XmNlabelString, XmStringCreatePCB (name));
      XtSetValues (lesstif_m_layer, args, n);
    }

  lesstif_update_layer_groups ();

  return 0;
}

static void
show_one_layer_button (int layer, int set)
{
  int l;
  n = 0;
  if (set)
    {
      stdarg (XmNforeground, bg_color);
      stdarg (XmNbackground, fg_colors[layer]);
    }
  else
    {
      stdarg (XmNforeground, fg_colors[layer]);
      stdarg (XmNbackground, bg_color);
    }
  stdarg (XmNset, set);

  for (l = 0; l < num_layer_buttons; l++)
    {
      LayerButtons *lb = layer_button_list + l;
      if (!lb->is_pick)
	XtSetValues (lb->w[layer], args, n);
    }
}

static void
layer_button_callback (Widget w, int layer, XmPushButtonCallbackStruct * pbcs)
{
  int l, set;
  switch (layer)
    {
    case LB_SILK:
      set = PCB->ElementOn = !PCB->ElementOn;
      PCB->Data->SILKLAYER.On = set;
      PCB->Data->BACKSILKLAYER.On = set;
      break;
    case LB_RATS:
      set = PCB->RatOn = !PCB->RatOn;
      break;
    case LB_PINS:
      set = PCB->PinOn = !PCB->PinOn;
      break;
    case LB_VIAS:
      set = PCB->ViaOn = !PCB->ViaOn;
      break;
    case LB_BACK:
      set = PCB->InvisibleObjectsOn = !PCB->InvisibleObjectsOn;
      break;
    case LB_MASK:
      TOGGLE_FLAG (SHOWMASKFLAG, PCB);
      set = TEST_FLAG (SHOWMASKFLAG, PCB);
      break;
    default:			/* layers */
      set = PCB->Data->Layer[layer].On = !PCB->Data->Layer[layer].On;
      break;
    }

  show_one_layer_button (layer, set);
  if (layer < max_copper_layer)
    {
      int i;
      int group = GetLayerGroupNumberByNumber (layer);
      for (i = 0; i < PCB->LayerGroups.Number[group]; i++)
	{
	  l = PCB->LayerGroups.Entries[group][i];
	  if (l != layer && l < max_copper_layer)
	    {
	      show_one_layer_button (l, set);
	      PCB->Data->Layer[l].On = set;
	    }
	}
    }
  lesstif_invalidate_all ();
}

static void
layerpick_button_callback (Widget w, int layer,
			   XmPushButtonCallbackStruct * pbcs)
{
  int l, i;
  char *name;
  PCB->RatDraw = (layer == LB_RATS);
  PCB->SilkActive = (layer == LB_SILK);
  if (layer < max_copper_layer)
    ChangeGroupVisibility (layer, 1, 1);
  for (l = 0; l < num_layer_buttons; l++)
    {
      LayerButtons *lb = layer_button_list + l;
      if (!lb->is_pick)
	continue;
      for (i = 0; i < LB_NUMPICK; i++)
	XmToggleButtonSetState (lb->w[i], layer == i, False);
    }
  switch (layer)
    {
    case LB_RATS:
      name = "Rats";
      break;
    case LB_SILK:
      name = "Silk";
      break;
    default:
      name = PCB->Data->Layer[layer].Name;
      break;
    }
  n = 0;
  stdarg (XmNbackground, fg_colors[layer]);
  stdarg (XmNforeground, bg_color);
  stdarg (XmNlabelString, XmStringCreatePCB (name));
  XtSetValues (lesstif_m_layer, args, n);
  lesstif_invalidate_all ();
}

static const char selectlayer_syntax[] =
"SelectLayer(1..MAXLAYER|Silk|Rats)";

static const char selectlayer_help[] =
"Select which layer is the current layer.";

/* %start-doc actions SelectLayer

The specified layer becomes the currently active layer.  It is made
visible if it is not already visible

%end-doc */

static int
SelectLayer (int argc, char **argv, Coord x, Coord y)
{
  int i;
  int newl = -1;
  if (argc == 0)
    return 1;

  for (i = 0; i < max_copper_layer; ++i)
    if (strcasecmp (argv[0], PCB->Data->Layer[i].Name) == 0)
      newl = i;

  if (strcasecmp (argv[0], "silk") == 0)
    newl = LB_SILK;
  else if (strcasecmp (argv[0], "rats") == 0)
    newl = LB_RATS;
  else if (newl == -1)
    newl = atoi (argv[0]) - 1;
  layerpick_button_callback (0, newl, 0);
  return 0;
}

static const char toggleview_syntax[] =
"ToggleView(1..MAXLAYER)\n"
"ToggleView(layername)\n"
"ToggleView(Silk|Rats|Pins|Vias|Mask|BackSide)";

static const char toggleview_help[] =
"Toggle the visibility of the specified layer or layer group.";

/* %start-doc actions ToggleView

If you pass an integer, that layer is specified by index (the first
layer is @code{1}, etc).  If you pass a layer name, that layer is
specified by name.  When a layer is specified, the visibility of the
layer group containing that layer is toggled.

If you pass a special layer name, the visibility of those components
(silk, rats, etc) is toggled.  Note that if you have a layer named
the same as a special layer, the layer is chosen over the special layer.

%end-doc */

static int
ToggleView (int argc, char **argv, Coord x, Coord y)
{
  int i, l;

  if (argc == 0)
    return 1;
  if (isdigit ((int) argv[0][0]))
    {
      l = atoi (argv[0]) - 1;
      layer_button_callback (0, l, 0);
    }
  else if (strcmp (argv[0], "Silk") == 0)
    layer_button_callback (0, LB_SILK, 0);
  else if (strcmp (argv[0], "Rats") == 0)
    layer_button_callback (0, LB_RATS, 0);
  else if (strcmp (argv[0], "Pins") == 0)
    layer_button_callback (0, LB_PINS, 0);
  else if (strcmp (argv[0], "Vias") == 0)
    layer_button_callback (0, LB_VIAS, 0);
  else if (strcmp (argv[0], "Mask") == 0)
    layer_button_callback (0, LB_MASK, 0);
  else if (strcmp (argv[0], "BackSide") == 0)
    layer_button_callback (0, LB_BACK, 0);
  else
    {
      l = -1;
      for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
	if (strcmp (argv[0], PCB->Data->Layer[i].Name) == 0)
	  {
	    l = i;
	    break;
	  }
      if (l == -1)
	return 1;
      layer_button_callback (0, l, 0);
    }
  return 0;
}

static void
insert_layerview_buttons (Widget menu)
{
  int i, s;
  LayerButtons *lb;

  num_layer_buttons++;
  s = num_layer_buttons * sizeof (LayerButtons);
  if (layer_button_list)
    layer_button_list = (LayerButtons *) realloc (layer_button_list, s);
  else
    layer_button_list = (LayerButtons *) malloc (s);
  lb = layer_button_list + num_layer_buttons - 1;

  for (i = 0; i < LB_NUM; i++)
    {
      static char namestr[] = "Label ";
      char *name = namestr;
      int accel_idx = i;
      Widget btn;
      name[5] = 'A' + i;
      switch (i)
	{
	case LB_SILK:
	  name = "Silk";
          accel_idx = max_copper_layer;
	  break;
	case LB_RATS:
	  name = "Rat Lines";
          accel_idx = max_copper_layer + 1;
	  break;
	case LB_PINS:
	  name = "Pins/Pads";
	  break;
	case LB_VIAS:
	  name = "Vias";
	  break;
	case LB_BACK:
	  name = "Far Side";
	  break;
	case LB_MASK:
	  name = "Solder Mask";
	  break;
	}
      n = 0;
      if (accel_idx < 9)
	{
	  char buf[20], av[30];
	  Resource *ar;
	  XmString as;
	  sprintf (buf, "Ctrl-%d", accel_idx + 1);
	  as = XmStringCreatePCB (buf);
	  stdarg (XmNacceleratorText, as);
	  ar = resource_create (0);
	  sprintf (av, "ToggleView(%d)", i + 1);
	  resource_add_val (ar, 0, strdup (av), 0);
	  resource_add_val (ar, 0, strdup (av), 0);
	  ar->flags |= FLAG_V;
	  sprintf (av, "Ctrl<Key>%d", accel_idx + 1);
	  note_accelerator (av, ar);
	  stdarg (XmNmnemonic, accel_idx + '1');
	}
      btn = XmCreateToggleButton (menu, name, args, n);
      XtManageChild (btn);
      XtAddCallback (btn, XmNvalueChangedCallback,
		     (XtCallbackProc) layer_button_callback, (XtPointer) (size_t) i);
      lb->w[i] = btn;

      if (i == LB_MASK)
	note_widget_flag (btn, XmNset, "showmask");
    }
  lb->is_pick = 0;
  LayersChanged (0, 0, 0, 0);
}

static void
insert_layerpick_buttons (Widget menu)
{
  int i, s;
  LayerButtons *lb;

  num_layer_buttons++;
  s = num_layer_buttons * sizeof (LayerButtons);
  if (layer_button_list)
    layer_button_list = (LayerButtons *) realloc (layer_button_list, s);
  else
    layer_button_list = (LayerButtons *) malloc (s);
  lb = layer_button_list + num_layer_buttons - 1;

  for (i = 0; i < LB_NUMPICK; i++)
    {
      static char namestr[] = "Label ";
      char *name = namestr;
      int accel_idx = i;
      char buf[20], av[30];
      Widget btn;
      name[5] = 'A' + i;
      switch (i)
	{
	case LB_SILK:
	  name = "Silk";
          accel_idx = max_copper_layer;
	  strcpy (av, "SelectLayer(Silk)");
	  break;
	case LB_RATS:
	  name = "Rat Lines";
          accel_idx = max_copper_layer + 1;
          strcpy (av, "SelectLayer(Rats)");
          break;
        default:
          sprintf (av, "SelectLayer(%d)", i + 1);
          break;
	}
      n = 0;
      if (accel_idx < 9)
        {
	  Resource *ar;
	  XmString as;
	  ar = resource_create (0);
	  resource_add_val (ar, 0, strdup (av), 0);
	  resource_add_val (ar, 0, strdup (av), 0);
	  ar->flags |= FLAG_V;
	  sprintf (buf, "%d", i + 1);
	  as = XmStringCreatePCB (buf);
	  stdarg (XmNacceleratorText, as);
	  sprintf (av, "<Key>%d", accel_idx + 1);
	  note_accelerator (av, ar);
	  stdarg (XmNmnemonic, accel_idx + '1');
        }
      stdarg (XmNindicatorType, XmONE_OF_MANY);
      btn = XmCreateToggleButton (menu, name, args, n);
      XtManageChild (btn);
      XtAddCallback (btn, XmNvalueChangedCallback,
		     (XtCallbackProc) layerpick_button_callback,
		     (XtPointer) (size_t) i);
      lb->w[i] = btn;
    }
  lb->is_pick = 1;
  LayersChanged (0, 0, 0, 0);
}

/*-----------------------------------------------------------------------------*/

typedef struct
{
  Widget w;
  const char *flagname;
  int oldval;
  char *xres;
} WidgetFlagType;

static WidgetFlagType *wflags = 0;
static int n_wflags = 0;
static int max_wflags = 0;

static void
note_widget_flag (Widget w, char *type, char *name)
{
  if (n_wflags >= max_wflags)
    {
      max_wflags += 20;
      wflags = (WidgetFlagType *) realloc (wflags, max_wflags * sizeof (WidgetFlagType));
    }
  wflags[n_wflags].w = w;
  wflags[n_wflags].flagname = name;
  wflags[n_wflags].oldval = -1;
  wflags[n_wflags].xres = type;
  n_wflags++;
}

void
lesstif_update_widget_flags ()
{
  int i;

  for (i = 0; i < n_wflags; i++)
    {
      int v = hid_get_flag (wflags[i].flagname);
      Arg args[1];
      XtSetArg (args[0], wflags[i].xres, v ? 1 : 0);
      XtSetValues (wflags[i].w, args, 1);
      wflags[i].oldval = v;
    }
}

/*-----------------------------------------------------------------------------*/

HID_Action lesstif_menu_action_list[] = {
  {"DumpKeys", 0, DumpKeys,
   dumpkeys_help, dumpkeys_syntax},
  {"Debug", 0, Debug,
   debug_help, debug_syntax},
  {"DebugXY", "Click X,Y for Debug", Debug,
   debugxy_help, debugxy_syntax},
  {"GetXY", "", GetXY,
   getxy_help, getxy_syntax},
  {"Return", 0, Return,
   return_help, return_syntax},
  {"LayersChanged", 0, LayersChanged,
   layerschanged_help, layerschanged_syntax},
  {"ToggleView", 0, ToggleView,
   toggleview_help, toggleview_syntax},
  {"SelectLayer", 0, SelectLayer,
   selectlayer_help, selectlayer_syntax}
};

REGISTER_ACTIONS (lesstif_menu_action_list)

#if 0
static void
do_color (char *value, char *which)
{
  XColor color;
  if (XParseColor (display, cmap, value, &color))
    if (XAllocColor (display, cmap, &color))
      {
	stdarg (which, color.pixel);
      }
}
#endif

typedef struct ToggleItem
{
  struct ToggleItem *next;
  Widget w;
  char *group, *item;
  XtCallbackProc callback;
  Resource *node;
} ToggleItem;
static ToggleItem *toggle_items = 0;

static int need_xy = 0, have_xy = 0, action_x, action_y;

static void
radio_callback (Widget toggle, ToggleItem * me,
		XmToggleButtonCallbackStruct * cbs)
{
  if (!cbs->set)		/* uh uh, can't turn it off */
    XmToggleButtonSetState (toggle, 1, 0);
  else
    {
      ToggleItem *ti;
      for (ti = toggle_items; ti; ti = ti->next)
	if (strcmp (me->group, ti->group) == 0)
	  {
	    if (me->item == ti->item || strcmp (me->item, ti->item) == 0)
	      XmToggleButtonSetState (ti->w, 1, 0);
	    else
	      XmToggleButtonSetState (ti->w, 0, 0);
	  }
      me->callback (toggle, me->node, cbs);
    }
}

int
lesstif_button_event (Widget w, XEvent * e)
{
  have_xy = 1;
  action_x = e->xbutton.x;
  action_y = e->xbutton.y;
  if (!need_xy)
    return 0;
  if (w != work_area)
    return 1;
  return 0;
}

void
lesstif_get_xy (const char *message)
{
  XmString ls = XmStringCreatePCB ((char *)message);

  XtManageChild (m_click);
  n = 0;
  stdarg (XmNlabelString, ls);
  XtSetValues (m_click, args, n);
  //printf("need xy: msg `%s'\n", msg);
  need_xy = 1;
  XBell (display, 100);
  while (!have_xy)
    {
      XEvent e;
      XtAppNextEvent (app_context, &e);
      XtDispatchEvent (&e);
    }
  need_xy = 0;
  have_xy = 1;
  XtUnmanageChild (m_click);
}

void
lesstif_get_coords (const char *msg, Coord *px, Coord *py)
{
  if (!have_xy && msg)
    lesstif_get_xy (msg);
  if (have_xy)
    lesstif_coords_to_pcb (action_x, action_y, px, py);
}

static void
callback (Widget w, Resource * node, XmPushButtonCallbackStruct * pbcs)
{
  int vi;
  have_xy = 0;
  lesstif_show_crosshair (0);
  if (pbcs->event && pbcs->event->type == KeyPress)
    {
      Dimension wx, wy;
      Widget aw = XtWindowToWidget (display, pbcs->event->xkey.window);
      action_x = pbcs->event->xkey.x;
      action_y = pbcs->event->xkey.y;
      if (aw)
	{
	  Widget p = work_area;
	  while (p && p != aw)
	    {
	      n = 0;
	      stdarg (XmNx, &wx);
	      stdarg (XmNy, &wy);
	      XtGetValues (p, args, n);
	      action_x -= wx;
	      action_y -= wy;
	      p = XtParent (p);
	    }
	  if (p == aw)
	    have_xy = 1;
	}
      //pcb_printf("have xy from %s: %$mD\n", XtName(aw), action_x, action_y);
    }

  lesstif_need_idle_proc ();
  for (vi = 1; vi < node->c; vi++)
    if (resource_type (node->v[vi]) == 10)
      if (hid_parse_actions (node->v[vi].value))
	return;
}

typedef struct acc_table_t
{
  char mods;
  char key_char;
  union {
    /* If M_Multi is set in mods, these are used to chain to the next
       attribute table for multi-key accelerators.  */
    struct {
      int n_chain;
      struct acc_table_t *chain;
    } c;
    /* If M_Multi isn't set, these are used to map a single key to an
       event.  */
    struct {
      KeySym key;
      Resource *node;
    } a;
  } u;
} acc_table_t;

static acc_table_t *acc_table;
static int acc_num = 0;

static int
acc_sort (const void *va, const void *vb)
{
  acc_table_t *a = (acc_table_t *) va;
  acc_table_t *b = (acc_table_t *) vb;
  if (a->key_char != b->key_char)
    return a->key_char - b->key_char;
  if (!(a->mods & M_Multi))
    if (a->u.a.key != b->u.a.key)
      return a->u.a.key - b->u.a.key;
  return a->mods - b->mods;
}

static int
DumpKeys2 ()
{
  int i;
  char ch[2];
  printf ("in dumpkeys! %d\n", acc_num);
  qsort (acc_table, acc_num, sizeof (acc_table_t), acc_sort);
  ch[1] = 0;
  for (i = 0; i < acc_num; i++)
    {
      char mod[16];
      int vi;
      char *tabs = "";

      sprintf (mod, "%s%s%s",
	       acc_table[i].mods & M_Alt ? "Alt-" : "",
	       acc_table[i].mods & M_Ctrl ? "Ctrl-" : "",
	       acc_table[i].mods & M_Shift ? "Shift-" : "");
      ch[0] = toupper ((int)  acc_table[i].key_char);
      printf ("%16s%s\t", mod,
	      acc_table[i].key_char ? ch : XKeysymToString (acc_table[i].
							    u.a.key));

      for (vi = 1; vi < acc_table[i].u.a.node->c; vi++)
	if (resource_type (acc_table[i].u.a.node->v[vi]) == 10)
	  {
	    printf ("%s%s", tabs, acc_table[i].u.a.node->v[vi].value);
	    tabs = "\n\t\t\t  ";
	  }

      printf ("\n");
    }
  exit (0);
}

static acc_table_t *
find_or_create_acc (char mods, char key, KeySym sym,
		    acc_table_t **table, int *n_ents)
{
  int i, max;
  acc_table_t *a;

  if (*table)
    for (i=(*n_ents)-1; i>=0; i--)
      {
	a = & (*table)[i];
	if (a->mods == mods
	    && a->key_char == key
	    && (mods & M_Multi || a->u.a.key == sym))
	  return a;
      }

  (*n_ents) ++;
  max = (*n_ents + 16) & ~15;

  if (*table)
    *table = (acc_table_t *) realloc (*table, max * sizeof (acc_table_t));
  else
    *table = (acc_table_t *) malloc (max * sizeof (acc_table_t));

  a = & ((*table)[(*n_ents)-1]);
  memset (a, 0, sizeof(acc_table_t));

  a->mods = mods;
  a->key_char = key;
  if (!(mods & M_Multi))
    a->u.a.key = sym;

  return a;
}

static void
note_accelerator (char *acc, Resource * node)
{
  char *orig_acc = acc;
  int mods = 0;
  acc_table_t *a;
  char key_char = 0;
  KeySym key = 0;
  int multi_key = 0;

  while (isalpha ((int) acc[0]))
    {
      if (strncmp (acc, "Shift", 5) == 0)
	{
	  mods |= M_Shift;
	  acc += 5;
	}
      else if (strncmp (acc, "Ctrl", 4) == 0)
	{
	  mods |= M_Ctrl;
	  acc += 4;
	}
      else if (strncmp (acc, "Alt", 3) == 0)
	{
	  mods |= M_Alt;
	  acc += 3;
	}
      else
	{
	  printf ("Must be Shift/Ctrl/Alt: %s\n", acc);
	  return;
	}
      while (*acc == ' ')
	acc++;
    }
  if (strncmp (acc, "<Keys>", 6) == 0)
    {
      multi_key = 1;
      acc ++;
    }
  else if (strncmp (acc, "<Key>", 5))
    {
      fprintf (stderr, "accelerator \"%s\" not <Key> or <Keys>\n", orig_acc);
      return;
    }

  /* We have a hard time specifying the Enter key the "usual" way.  */
  if (strcmp (acc, "<Key>Enter") == 0)
    acc = "<Key>\r";

  acc += 5;
  if (acc[0] && acc[1] == 0)
    {
      key_char = acc[0];
      a = find_or_create_acc (mods, key_char, 0, &acc_table, &acc_num);
    }
  else if (multi_key)
    {
      acc_table_t **ap = &acc_table;
      int *np = &acc_num;

      mods |= M_Multi;
      while (acc[0] && acc[1])
	{
	  a = find_or_create_acc (mods, acc[0], 0, ap, np);
	  ap = & (a->u.c.chain);
	  np = & (a->u.c.n_chain);
	  acc ++;
	}
      a = find_or_create_acc (mods & ~M_Multi, acc[0], 0, ap, np);
    }
  else
    {
      key = XStringToKeysym (acc);
      if (key == NoSymbol && !key_char)
	{
	  printf ("no symbol for %s\n", acc);
	  return;
	}
      a = find_or_create_acc (mods, 0, key, &acc_table, &acc_num);
    }

  a->u.a.node = node;
}

#if 0
static void
dump_multi (int ix, int ind, acc_table_t *a, int n)
{
  int i = ix;
  while (n--)
    {
      if (a->mods & M_Multi)
	{
	  printf("%*cacc[%d] mods %x char %c multi %p/%d\n",
		 ind, ' ',
		 i, a->mods, a->key_char,
		 a->u.c.chain, a->u.c.n_chain);
	  dump_multi(0, ind+4, a->u.c.chain, a->u.c.n_chain);
	}
      else
	{
	  printf("%*cacc[%d] mods %x char %c key %d node `%s'\n",
		 ind, ' ',
		 i, a->mods, a->key_char,
		 a->u.a.key, a->u.a.node->v[0].value);
	}
      a++;
      i++;
    }
}
#else
#define dump_multi(x,a,b,c)
#endif

static acc_table_t *cur_table = 0;
static int cur_ntable = 0;

/*!
 * \brief .
 *
 * We sort these such that the ones with explicit modifiers come before
 * the ones with implicit modifiers.
 * That way, a Shift<Key>Code gets chosen before a <Key>Code.
 */
static int
acc_sort_rev (const void *va, const void *vb)
{
  acc_table_t *a = (acc_table_t *) va;
  acc_table_t *b = (acc_table_t *) vb;
  if (a->key_char != b->key_char)
    return a->key_char - b->key_char;
  if (!(a->mods & M_Multi))
    if (a->u.a.key != b->u.a.key)
      return a->u.a.key - b->u.a.key;
  return b->mods - a->mods;
}

int
lesstif_key_event (XKeyEvent * e)
{
  char buf[10], buf2[10];
  KeySym sym, sym2;
  int slen, slen2;
  int mods = 0;
  int i, vi;
  static int sorted = 0;
  acc_table_t *my_table = 0;

  if (!sorted)
    {
      sorted = 1;
      qsort (acc_table, acc_num, sizeof (acc_table_t), acc_sort_rev);
    }

  if (e->state & ShiftMask)
    mods |= M_Shift;
  if (e->state & ControlMask)
    mods |= M_Ctrl;
  if (e->state & Mod1Mask)
    mods |= M_Alt;

  e->state &= ~(ControlMask | Mod1Mask);
  slen = XLookupString (e, buf, sizeof (buf), &sym, NULL);

  if (e->state & ShiftMask)
    {
      e->state &= ~ShiftMask;
      slen2 = XLookupString (e, buf2, sizeof (buf2), &sym2, NULL);
    }
  else
    slen2 = slen;

  /* Ignore these.  */
  switch (sym)
    {
    case XK_Shift_L:
    case XK_Shift_R:
    case XK_Control_L:
    case XK_Control_R:
    case XK_Caps_Lock:
    case XK_Shift_Lock:
    case XK_Meta_L:
    case XK_Meta_R:
    case XK_Alt_L:
    case XK_Alt_R:
    case XK_Super_L:
    case XK_Super_R:
    case XK_Hyper_L:
    case XK_Hyper_R:
    case XK_ISO_Level3_Shift:
      return 1;
    }

  if (cur_table == 0)
    {
      cur_table  = acc_table;
      cur_ntable = acc_num;
    }

  //printf("\nmods %x key %d str `%s' in %p/%d\n", mods, (int)sym, buf, cur_table, cur_ntable);

#define KM(m) ((m) & ~M_Multi)
  for (i = 0; i < cur_ntable; i++)
    {
      dump_multi (i, 0, cur_table+i, 1);
      if (KM(cur_table[i].mods) == mods)
	{
	  if (sym == acc_table[i].u.a.key)
	    break;
	}
      if (KM(cur_table[i].mods) == (mods & ~M_Shift))
	{
	  if (slen == 1 && buf[0] == cur_table[i].key_char)
	    break;
	  if (sym == cur_table[i].u.a.key)
	    break;
	}
      if (mods & M_Shift && KM(cur_table[i].mods) == mods)
	{
	  if (slen2 == 1 && buf2[0] == cur_table[i].key_char)
	    break;
	  if (sym2 == acc_table[i].u.a.key)
	    break;
	}
    }

  if (i == cur_ntable)
    {
      if (cur_table == acc_table)
	lesstif_log ("Key \"%s\" not tied to an action\n", buf);
      else
	lesstif_log ("Key \"%s\" not tied to a multi-key action\n", buf);
      cur_table = 0;
      return 0;
    }
  if (cur_table[i].mods & M_Multi)
    {
      cur_ntable = cur_table[i].u.c.n_chain;
      cur_table = cur_table[i].u.c.chain;
      dump_multi (0, 0, cur_table, cur_ntable);
      return 1;
    }

  if (e->window == XtWindow (work_area))
    {
      have_xy = 1;
      action_x = e->x;
      action_y = e->y;
    }
  else
    have_xy = 0;

  /* Parsing actions may not return until more user interaction
     happens, so remember which table we're scanning.  */
  my_table = cur_table;
  for (vi = 1; vi < my_table[i].u.a.node->c; vi++)
    if (resource_type (my_table[i].u.a.node->v[vi]) == 10)
      if (hid_parse_actions
	  (my_table[i].u.a.node->v[vi].value))
	break;
  cur_table = 0;
  return 1;
}

static void
add_resource_to_menu (Widget menu, Resource * node, XtCallbackProc callback)
{
  int i, j;
  char *v;
  Widget sub, btn;
  Resource *r;

  for (i = 0; i < node->c; i++)
    switch (resource_type (node->v[i]))
      {
      case 101:		/* named subnode */
	n = 0;
	stdarg (XmNtearOffModel, XmTEAR_OFF_ENABLED);
	sub = XmCreatePulldownMenu (menu, node->v[i].name, args, n);
	XtSetValues (sub, args, n);
	n = 0;
	stdarg (XmNsubMenuId, sub);
	btn = XmCreateCascadeButton (menu, node->v[i].name, args, n);
	XtManageChild (btn);
	add_resource_to_menu (sub, node->v[i].subres, callback);
	break;

      case 1:			/* unnamed subres */
	n = 0;
#if 0
	if ((v = resource_value (node->v[i].subres, "fg")))
	  {
	    do_color (v, XmNforeground);
	  }
	if ((v = resource_value (node->v[i].subres, "bg")))
	  {
	    do_color (v, XmNbackground);
	  }
	if ((v = resource_value (node->v[i].subres, "font")))
	  {
	    XFontStruct *fs = XLoadQueryFont (display, v);
	    if (fs)
	      {
		XmFontList fl =
		  XmFontListCreate (fs, XmSTRING_DEFAULT_CHARSET);
		stdarg (XmNfontList, fl);
	      }
	  }
#endif
	if ((v = resource_value (node->v[i].subres, "m")))
	  {
	    stdarg (XmNmnemonic, v);
	  }
	if ((r = resource_subres (node->v[i].subres, "a")))
	  {
	    XmString as = XmStringCreatePCB (r->v[0].value);
	    stdarg (XmNacceleratorText, as);
	    //stdarg(XmNaccelerator, r->v[1].value);
	    note_accelerator (r->v[1].value, node->v[i].subres);
	  }
	v = "button";
	for (j = 0; j < node->v[i].subres->c; j++)
	  if (resource_type (node->v[i].subres->v[j]) == 10)
	    {
	      v = node->v[i].subres->v[j].value;
	      break;
	    }
	stdarg (XmNlabelString, XmStringCreatePCB (v));
	if (node->v[i].subres->flags & FLAG_S)
	  {
	    int nn = n;
	    stdarg (XmNtearOffModel, XmTEAR_OFF_ENABLED);
	    sub = XmCreatePulldownMenu (menu, v, args + nn, n - nn);
	    n = nn;
	    stdarg (XmNsubMenuId, sub);
	    btn = XmCreateCascadeButton (menu, "menubutton", args, n);
	    XtManageChild (btn);
	    add_resource_to_menu (sub, node->v[i].subres, callback);
	  }
	else
	  {
	    Resource *radio = resource_subres (node->v[i].subres, "radio");
	    char *checked = resource_value (node->v[i].subres, "checked");
	    char *label = resource_value (node->v[i].subres, "sensitive");
	    if (radio)
	      {
		ToggleItem *ti = (ToggleItem *) malloc (sizeof (ToggleItem));
		ti->next = toggle_items;
		ti->group = radio->v[0].value;
		ti->item = radio->v[1].value;
		ti->callback = callback;
		ti->node = node->v[i].subres;
		toggle_items = ti;

		if (resource_value (node->v[i].subres, "set"))
		  {
		    stdarg (XmNset, True);
		  }
		stdarg (XmNindicatorType, XmONE_OF_MANY);
		btn = XmCreateToggleButton (menu, "menubutton", args, n);
		ti->w = btn;
		XtAddCallback (btn, XmNvalueChangedCallback,
			       (XtCallbackProc) radio_callback,
			       (XtPointer) ti);
	      }
	    else if (checked)
	      {
		if (strchr (checked, ','))
		  stdarg (XmNindicatorType, XmONE_OF_MANY);
		else
		  stdarg (XmNindicatorType, XmN_OF_MANY);
		btn = XmCreateToggleButton (menu, "menubutton", args, n);
		XtAddCallback (btn, XmNvalueChangedCallback,
			       callback, (XtPointer) node->v[i].subres);
	      }
	    else if (label && strcmp (label, "false") == 0)
	      {
		stdarg (XmNalignment, XmALIGNMENT_BEGINNING);
		btn = XmCreateLabel (menu, "menulabel", args, n);
	      }
	    else
	      {
		btn = XmCreatePushButton (menu, "menubutton", args, n);
		XtAddCallback (btn, XmNactivateCallback,
			       callback, (XtPointer) node->v[i].subres);
	      }

	    for (j = 0; j < node->v[i].subres->c; j++)
	      switch (resource_type (node->v[i].subres->v[j]))
		{
		case 110:	/* named value = X resource */
		  {
		    char *n = node->v[i].subres->v[j].name;
		    if (strcmp (n, "fg") == 0)
		      n = "foreground";
		    if (strcmp (n, "bg") == 0)
		      n = "background";
		    if (strcmp (n, "m") == 0
			|| strcmp (n, "a") == 0
			|| strcmp (n, "sensitive") == 0)
		      break;
		    if (strcmp (n, "checked") == 0)
		      {
			note_widget_flag (btn, XmNset,
					  node->v[i].subres->v[j].value);
			break;
		      }
		    if (strcmp (n, "active") == 0)
		      {
			note_widget_flag (btn, XmNsensitive,
					  node->v[i].subres->v[j].value);
			break;
		      }
		    XtVaSetValues (btn, XtVaTypedArg,
				   n,
				   XtRString,
				   node->v[i].subres->v[j].value,
				   strlen (node->v[i].subres->v[j].value) + 1,
				   NULL);
		  }
		  break;
		}

	    XtManageChild (btn);
	  }
	break;

      case 10:			/* unnamed value */
	n = 0;
	if (node->v[i].value[0] == '@')
	  {
	    if (strcmp (node->v[i].value, "@layerview") == 0)
	      insert_layerview_buttons (menu);
	    if (strcmp (node->v[i].value, "@layerpick") == 0)
	      insert_layerpick_buttons (menu);
	    if (strcmp (node->v[i].value, "@routestyles") == 0)
	      lesstif_insert_style_buttons (menu);
	  }
	else if (strcmp (node->v[i].value, "-") == 0)
	  {
	    btn = XmCreateSeparator (menu, "sep", args, n);
	    XtManageChild (btn);
	  }
	else if (i > 0)
	  {
	    btn = XmCreatePushButton (menu, node->v[i].value, args, n);
	    XtManageChild (btn);
	  }
	break;
      }
}

extern char *lesstif_pcbmenu_path;

Widget
lesstif_menu (Widget parent, char *name, Arg * margs, int mn)
{
  Widget mb = XmCreateMenuBar (parent, name, margs, mn);
  char *filename;
  Resource *r = 0, *bir;
  char *home_pcbmenu, *home;
  int screen;
  Resource *mr;

  display = XtDisplay (mb);
  screen = DefaultScreen (display);
  cmap = DefaultColormap (display, screen);

  /* homedir is set by the core */
  home = homedir;
  home_pcbmenu = NULL;
  if (home == NULL)
    {
      Message ("Warning:  could not determine home directory (from HOME)\n");
    }
  else 
    {
      home_pcbmenu = Concat (home, PCB_DIR_SEPARATOR_S, ".pcb", 
         PCB_DIR_SEPARATOR_S, "pcb-menu.res", NULL);
    }

  if (access ("pcb-menu.res", R_OK) == 0)
    filename = "pcb-menu.res";
  else if (home_pcbmenu != NULL && (access (home_pcbmenu, R_OK) == 0))
    filename = home_pcbmenu;
  else if (access (lesstif_pcbmenu_path, R_OK) == 0)
    filename = lesstif_pcbmenu_path;
  else
    filename = 0;

  bir = resource_parse (0, pcb_menu_default);
  if (!bir)
    {
      fprintf (stderr, "Error: internal menu resource didn't parse\n");
      exit(1);
    }

  if (filename)
    r = resource_parse (filename, 0);

  if (!r)
    r = bir;

  if (home_pcbmenu != NULL)
    {
      free (home_pcbmenu);
    }

  mr = resource_subres (r, "MainMenu");
  if (!mr)
    mr = resource_subres (bir, "MainMenu");
  if (mr)
    add_resource_to_menu (mb, mr, (XtCallbackProc) callback);

  mr = resource_subres (r, "Mouse");
  if (!mr)
    mr = resource_subres (bir, "Mouse");
  if (mr)
    load_mouse_resource (mr);


  if (do_dump_keys)
    DumpKeys2 ();

  return mb;
}

void lesstif_uninit_menu (void)
{
/*! \todo XtDestroyWidget (...); */
}
