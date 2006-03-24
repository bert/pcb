/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <X11/Intrinsic.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/ToggleB.h>
#include <Xm/CascadeB.h>
#include <Xm/MenuShell.h>
#include <Xm/Separator.h>

#include "global.h"
#include "data.h"
#include "misc.h"

#include "hid.h"
#include "../hidint.h"
#include "lesstif.h"
#include "resource.h"
#include "mymem.h"

#include "pcb-menu.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

#ifndef R_OK
/* Common value for systems that don't define it.  */
#define R_OK 4
#endif

Display *display;
static Colormap cmap;

static Arg args[30];
static int n;
#define stdarg(t,v) XtSetArg(args[n], t, v), n++

static void note_accelerator (char *acc, Resource * node);

static int
GetXY (int argc, char **argv, int x, int y)
{
  return 0;
}

static int
CheckWhen (int argc, char **argv, int x, int y)
{
  return 0;
}

static int
Debug (int argc, char **argv, int x, int y)
{
  int i;
  printf ("Debug:");
  for (i = 0; i < argc; i++)
    printf (" [%d] `%s'", i, argv[i]);
  printf (" x,y %d,%d\n", x, y);
  return 0;
}

static int
Return (int argc, char **argv, int x, int y)
{
  return atoi (argv[0]);
}

static int do_dump_keys = 0;
static int
DumpKeys (int argc, char **argv, int x, int y)
{
  do_dump_keys = 1;
  return 0;
}

/*-----------------------------------------------------------------------------*/

#define LB_SILK	(MAX_LAYER+0)
#define LB_RATS	(MAX_LAYER+1)
#define LB_NUMPICK (LB_RATS+1)
/* more */
#define LB_PINS	(MAX_LAYER+2)
#define LB_VIAS	(MAX_LAYER+3)
#define LB_BACK	(MAX_LAYER+4)
#define LB_MASK	(MAX_LAYER+5)
#define LB_NUM  (MAX_LAYER+6)

typedef struct
{
  Widget w[LB_NUM];
  int is_pick;
} LayerButtons;

static LayerButtons *layer_button_list = 0;
static int num_layer_buttons = 0;
static int fg_colors[LB_NUM];
static int bg_color;
static int got_colors = 0;

extern Widget lesstif_m_layer;

static int
LayersChanged (int argc, char **argv, int x, int y)
{
  int l, i, set;
  char *name;
  int current_layer;

  if (!layer_button_list)
    return 0;

  if (!got_colors)
    {
      got_colors = 1;
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
	  if (i < MAX_LAYER)
	    {
	      XmString s = XmStringCreateLocalized (PCB->Data->Layer[i].Name);
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
      stdarg (XmNlabelString, XmStringCreateLocalized (name));
      XtSetValues (lesstif_m_layer, args, n);
    }

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
  if (layer < MAX_LAYER)
    {
      int i;
      int group = GetLayerGroupNumberByNumber (layer);
      for (i = 0; i < PCB->LayerGroups.Number[group]; i++)
	{
	  l = PCB->LayerGroups.Entries[group][i];
	  if (l != layer && l < MAX_LAYER)
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
  if (layer < MAX_LAYER)
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
  stdarg (XmNlabelString, XmStringCreateLocalized (name));
  XtSetValues (lesstif_m_layer, args, n);
  lesstif_invalidate_all ();
}

static int
SelectLayer (int argc, char **argv, int x, int y)
{
  int newl;
  if (argc == 0)
    return 1;
  if (strcasecmp (argv[0], "silk") == 0)
    newl = LB_SILK;
  else if (strcasecmp (argv[0], "rats") == 0)
    newl = LB_RATS;
  else
    newl = atoi (argv[0]) - 1;
  layerpick_button_callback (0, newl, 0);
  return 0;
}

static int
ToggleView (int argc, char **argv, int x, int y)
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
      for (i = 0; i < MAX_LAYER + 2; i++)
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
      char *name = "Label";
      switch (i)
	{
	case LB_SILK:
	  name = "Silk";
	  break;
	case LB_RATS:
	  name = "Rat Lines";
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
      if (i < MAX_LAYER)
	{
	  char buf[20], av[30];
	  Resource *ar;
	  XmString as;
	  sprintf (buf, "Ctrl-%d", i + 1);
	  as = XmStringCreateLocalized (buf);
	  stdarg (XmNacceleratorText, as);
	  ar = resource_create (0);
	  sprintf (av, "ToggleView(%d)", i + 1);
	  resource_add_val (ar, 0, strdup (av), 0);
	  resource_add_val (ar, 0, strdup (av), 0);
	  ar->flags |= FLAG_V;
	  sprintf (av, "Ctrl<Key>%d", i + 1);
	  note_accelerator (av, ar);
	  stdarg (XmNmnemonic, i + '1');
	}
      Widget btn = XmCreateToggleButton (menu, name, args, n);
      XtManageChild (btn);
      XtAddCallback (btn, XmNvalueChangedCallback,
		     (XtCallbackProc) layer_button_callback, (XtPointer) i);
      lb->w[i] = btn;
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
      char *name = "Label";
      switch (i)
	{
	case LB_SILK:
	  name = "Silk";
	  break;
	case LB_RATS:
	  name = "Rat Lines";
	  break;
	}
      n = 0;
      if (i < MAX_LAYER)
	{
	  char buf[20], av[30];
	  Resource *ar;
	  XmString as;
	  sprintf (buf, "%d", i + 1);
	  as = XmStringCreateLocalized (buf);
	  stdarg (XmNacceleratorText, as);
	  ar = resource_create (0);
	  switch (i)
	    {
	    case LB_SILK:
	      strcpy (av, "SelectLayer(Silk)");
	      break;
	    case LB_RATS:
	      strcpy (av, "SelectLayer(Rats)");
	      break;
	    default:
	      sprintf (av, "SelectLayer(%d)", i + 1);
	      break;
	    }
	  resource_add_val (ar, 0, strdup (av), 0);
	  resource_add_val (ar, 0, strdup (av), 0);
	  ar->flags |= FLAG_V;
	  sprintf (av, "<Key>%d", i + 1);
	  note_accelerator (av, ar);
	  stdarg (XmNmnemonic, i + '1');
	}
      stdarg (XmNindicatorType, XmONE_OF_MANY);
      Widget btn = XmCreateToggleButton (menu, name, args, n);
      XtManageChild (btn);
      XtAddCallback (btn, XmNvalueChangedCallback,
		     (XtCallbackProc) layerpick_button_callback,
		     (XtPointer) i);
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
      wflags =
	MyRealloc (wflags, max_wflags * sizeof (WidgetFlagType),
		   __FUNCTION__);
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
  {"DumpKeys", 0, 0, DumpKeys},
  {"Debug", 0, 0, Debug},
  {"DebugXY", 1, "Click X,Y for Debug", Debug},
  {"GetXY", 1, "", GetXY},
  {"CheckWhen", 0, 0, CheckWhen},
  {"Return", 0, 0, Return},
  {"LayersChanged", 0, 0, LayersChanged},
  {"ToggleView", 0, 0, ToggleView},
  {"SelectLayer", 0, 0, SelectLayer}
};

REGISTER_ACTIONS (lesstif_menu_action_list);

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
  if (!need_xy)
    return 0;
  if (w != work_area)
    return 1;
  have_xy = 1;
  action_x = e->xbutton.x;
  action_y = e->xbutton.y;
  return 0;
}

void
lesstif_get_xy (const char *message)
{
  XmString ls = XmStringCreateLocalized ((char *)message);

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
lesstif_get_coords (const char *msg, int *px, int *py)
{
  if (!have_xy)
    lesstif_get_xy (msg);
  lesstif_coords_to_pcb (action_x, action_y, px, py);
}

int
lesstif_call_action (const char *aname, int argc, char **argv)
{
  int px, py;
  HID_Action *a = hid_find_action (aname);
  if (!a)
    {
      int i;
      printf ("no action %s(", aname);
      for (i = 0; i < argc; i++)
	printf ("%s%s", i ? ", " : "", argv[i]);
      printf (")\n");
      return 1;
    }

  if (a->needs_coords && !have_xy)
    {
      const char *msg;
      if (strcmp (aname, "GetXY") == 0)
	msg = argv[0];
      else
	msg = a->need_coord_msg;
      lesstif_get_xy (msg);
    }
  lesstif_coords_to_pcb (action_x, action_y, &px, &py);
  return a->trigger_cb (argc, argv, px, py);
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
      //printf("have xy from %s: %d %d\n", XtName(aw), action_x, action_y);
    }

  lesstif_need_idle_proc ();
  for (vi = 1; vi < node->c; vi++)
    if (resource_type (node->v[vi]) == 10)
      if (hid_parse_actions (node->v[vi].value, lesstif_call_action))
	return;
}

#define M_Shift 1
#define M_Ctrl 2
#define M_Alt 4

typedef struct
{
  char mods;
  char key_char;
  KeySym key;
  Resource *node;
} acc_table_t;

static acc_table_t *acc_table;
static int acc_max = 0, acc_num = 0;

static int
acc_sort (const void *va, const void *vb)
{
  acc_table_t *a = (acc_table_t *) va;
  acc_table_t *b = (acc_table_t *) vb;
  if (a->key_char != b->key_char)
    return a->key_char - b->key_char;
  if (a->key != b->key)
    return a->key - b->key;
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
      ch[0] = toupper (acc_table[i].key_char);
      printf ("%16s%s\t", mod,
	      acc_table[i].key_char ? ch : XKeysymToString (acc_table[i].
							    key));

      for (vi = 1; vi < acc_table[i].node->c; vi++)
	if (resource_type (acc_table[i].node->v[vi]) == 10)
	  {
	    printf ("%s%s", tabs, acc_table[i].node->v[vi].value);
	    tabs = "\n\t\t\t  ";
	  }

      printf ("\n");
    }
  exit (0);
}

static void
note_accelerator (char *acc, Resource * node)
{
  acc_table_t *a;
  if (acc_num == acc_max)
    {
      acc_max += 20;
      if (acc_table)
	acc_table =
	  (acc_table_t *) realloc (acc_table, acc_max * sizeof (acc_table_t));
      else
	acc_table = (acc_table_t *) malloc (acc_max * sizeof (acc_table_t));
    }
  a = acc_table + acc_num;
  acc_num++;
  /*printf("acc `%s' res `%s'\n", acc, node->v[0].value); */

  memset (a, 0, sizeof (*a));

  while (isalpha ((int) acc[0]))
    {
      if (strncmp (acc, "Shift", 5) == 0)
	{
	  a->mods |= M_Shift;
	  acc += 5;
	}
      else if (strncmp (acc, "Ctrl", 4) == 0)
	{
	  a->mods |= M_Ctrl;
	  acc += 4;
	}
      else if (strncmp (acc, "Alt", 3) == 0)
	{
	  a->mods |= M_Alt;
	  acc += 3;
	}
      else
	{
	  printf ("Must be Shift/Ctrl/Alt: %s\n", acc);
	  acc_num--;
	  return;
	}
      while (*acc == ' ')
	acc++;
    }
  if (strncmp (acc, "<Key>", 5))
    {
      printf ("not <Key>\n");
      acc_num--;
      return;
    }
  acc += 5;
  if (acc[0] && acc[1] == 0)
    a->key_char = acc[0];
  a->key = XStringToKeysym (acc);
  if (a->key == NoSymbol && !a->key_char)
    {
      printf ("no symbol for %s\n", acc);
      acc_num--;
      return;
    }
  a->node = node;
  //printf("acc[%d] mods %x char %c key %d node `%s'\n",
  //acc_num-1, a->mods, a->key_char, (int)(a->key), a->node->v[0].value);
}

int
lesstif_key_event (XKeyEvent * e)
{
  char buf[10], buf2[10];
  KeySym sym, sym2;
  int slen, slen2;
  int mods = 0;
  int i, vi;

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

  //printf("\nmods %x key %d str `%s'\n", mods, (int)sym, buf);

  for (i = 0; i < acc_num; i++)
    {
      //printf("acc[%d] mods %x char %c key %d node `%s'\n",
      //i, acc_table[i].mods, acc_table[i].key_char, acc_table[i].key, acc_table[i].node->v[0].value);
      if (acc_table[i].mods == (mods & ~M_Shift))
	{
	  //printf("mods match 1\n");
	  if (slen == 1 && buf[0] == acc_table[i].key_char)
	    break;
	  if (sym == acc_table[i].key)
	    break;
	}
      if (mods & M_Shift && acc_table[i].mods == mods)
	{
	  //printf("mods match 2\n");
	  if (slen2 == 1 && buf2[0] == acc_table[i].key_char)
	    break;
	  if (sym2 == acc_table[i].key)
	    break;
	}
    }
  if (i == acc_num)
    return 0;
  if (e->window == XtWindow (work_area))
    {
      have_xy = 1;
      action_x = e->x;
      action_y = e->y;
    }
  else
    have_xy = 0;

  for (vi = 1; vi < acc_table[i].node->c; vi++)
    if (resource_type (acc_table[i].node->v[vi]) == 10)
      if (hid_parse_actions
	  (acc_table[i].node->v[vi].value, lesstif_call_action))
	break;
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
	    XmString as = XmStringCreateLocalized (r->v[0].value);
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
	stdarg (XmNlabelString, XmStringCreateLocalized (v));
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

static char *pcbmenu_path = "pcb-menu.res";

static HID_Attribute pcbmenu_attr[] = {
  {"pcb-menu", "Location of pcb-menu.res file",
   HID_String, 0, 0, {0, PCBLIBDIR "/pcb-menu.res", 0}, 0, &pcbmenu_path}
};

REGISTER_ATTRIBUTES(pcbmenu_attr)

Widget
lesstif_menu (Widget parent, char *name, Arg * margs, int mn)
{
  Widget mb = XmCreateMenuBar (parent, name, margs, mn);
  char *filename;
  Resource *r;
  char *home_pcbmenu;

  display = XtDisplay (mb);
  int screen = DefaultScreen (display);
  cmap = DefaultColormap (display, screen);

  home_pcbmenu = Concat (getenv ("HOME"), "/.pcb/pcb-menu.res", NULL);

  if (access ("pcb-menu.res", R_OK) == 0)
    filename = "pcb-menu.res";
  else if (access (home_pcbmenu, R_OK) == 0)
    filename = home_pcbmenu;
  else if (access (pcbmenu_path, R_OK) == 0)
    filename = pcbmenu_path;
  else
    filename = 0;

  if (filename)
    r = resource_parse (filename, 0);
  else
    r = resource_parse (0, pcb_menu_default);

  if (r)
    {
      Resource *mr = resource_subres (r, "MainMenu");

      if (mr)
	add_resource_to_menu (mb, mr, (XtCallbackProc) callback);
    }

  if (do_dump_keys)
    DumpKeys2 ();

  return mb;
}
