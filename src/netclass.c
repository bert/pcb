#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//#include <stdio.h>
#include <string.h>
//#include <assert.h>

#include "data.h"
#include "rats.h"
#include "find.h"
#include "undo.h"
#include "pcb-printf.h"
#include "netclass.h"

static int num_netclass = 10;

static char *netclass_names[10] =
  {
    NULL,
    "PE",
    "DC+",
    "BAT+",
    "PHASE1",
    "PHASE2",
    "PHASE3",
    "PHASE4",
    "PHASE5",
    "PHASE6"
  };

static Coord clearances[10][10] =
  {
    {MM_TO_COORD (0.2), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0)},
    {MM_TO_COORD (4.0), MM_TO_COORD (0.2), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0)},
    {MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (0.2), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0)},
    {MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (0.2), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0)},
    {MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (0.2), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0)},
    {MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (0.2), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0)},
    {MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (0.2), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0)},
    {MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (0.2), MM_TO_COORD (4.0), MM_TO_COORD (4.0)},
    {MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (0.2), MM_TO_COORD (4.0)},
    {MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (4.0), MM_TO_COORD (0.2)}
  };


static int
netclass_index (char *netclass)
{
  int i;

  for (i = 0; i < num_netclass; i++)
    if (netclass == NULL)
      {
        if (netclass_names[i] == NULL)
          return i;
      }
    else if (netclass_names[i] != NULL && strcmp (netclass_names[i], netclass) == 0)
      {
        return i;
      }

  g_return_val_if_fail (netclass != NULL, 0);
  g_warning ("Netclass '%s' not found, returning 0th (default) class", netclass);

  return 0;
}

static Coord
clearance_between_netclasses (int a, int b)
{
  g_return_val_if_fail (a >= 0 && a < num_netclass, 0);
  g_return_val_if_fail (b >= 0 && b < num_netclass, 0);

  return clearances[a][b];
}

Coord
get_min_clearance_for_netclass (char *netclass)
{
  int netclass_i = netclass_index (netclass);
  int i;
  Coord min_clearance = MAX_COORD;

  for (i = 0; i < num_netclass; i++)
    min_clearance = MIN (min_clearance, clearances[netclass_i][i]);

#if 0
  pcb_printf ("Looking up minimum clearance for netclass %s%s%s, found '%mn'\n",
              netclass != NULL ? "'" : "",
              netclass != NULL ? netclass : "(NULL)",
              netclass != NULL ? "'" : "",
              min_clearance);
#endif

  return min_clearance;
}

Coord
get_max_clearance_for_netclass (char *netclass)
{
  int netclass_i = netclass_index (netclass);
  int i;
  Coord max_clearance = 0;

  for (i = 0; i < num_netclass; i++)
    max_clearance = MAX (max_clearance, clearances[netclass_i][i]);

#if 0
  pcb_printf ("Looking up maximum clearance for netclass %s%s%s, found '%mn'\n",
              netclass != NULL ? "'" : "",
              netclass != NULL ? netclass : "(NULL)",
              netclass != NULL ? "'" : "",
              max_clearance);
#endif

  return max_clearance;
}

Coord
get_clearance_between_netclasses (char *netclass_a, char *netclass_b)
{
  int a = netclass_index (netclass_a);
  int b = netclass_index (netclass_b);
  Coord clearance = clearance_between_netclasses (a, b);

#if 0
  pcb_printf ("Looking up clearance between netclass %s%s%s and %s%s%s, found '%mn'\n",
              netclass_a != NULL ? "'" : "",
              netclass_a != NULL ? netclass_a : "(NULL)",
              netclass_a != NULL ? "'" : "",
              netclass_b != NULL ? "'" : "",
              netclass_b != NULL ? netclass_b : "(NULL)",
              netclass_b != NULL ? "'" : "",
              clearance);
#endif

  return clearance;
}

char *
get_netclass_for_netname (char *netname)
{
  char *menu_netname;

//  g_return_val_if_fail (netname != NULL, NULL);
  if (netname == NULL)
    return NULL;

  /* Find netlist entry */
  MENU_LOOP (&PCB->NetlistLib);
  {
    if (!menu->Name)
    continue;

    ENTRY_LOOP (menu);
    {
      if (!entry->ListEntry)
        continue;

      menu_netname = g_strdup (menu->Name);
      /* For some reason, the netname has spaces in front of it, strip them */
      g_strstrip (menu_netname);
      if (strcmp (menu_netname, netname) == 0) {
        g_free (menu_netname);
        return menu->Netclass;
      }
      g_free (menu_netname);
    }
    END_LOOP;
  }
  END_LOOP;

  return NULL;
}

static char *
g_strdup_netname_for_pinpad (int type, ElementType *ele, void *pinpad)
{
  char *pinname;
  char *netname = NULL;

  pinname = ConnectionName (type, ele, pinpad);

  if (pinname == NULL)
    return NULL;

  /* Find netlist entry */
  MENU_LOOP (&PCB->NetlistLib);
  {
    if (!menu->Name)
    continue;

    ENTRY_LOOP (menu);
    {
      if (!entry->ListEntry)
        continue;

      if (strcmp (entry->ListEntry, pinname) == 0) {
        netname = g_strdup (menu->Name);
        /* For some reason, the netname has spaces in front of it, strip them */
        g_strstrip (netname);
        break;
      }
    }
    END_LOOP;

    if (netname != NULL)
      break;
  }
  END_LOOP;

  return netname;
}

char *
get_netclass_for_via (PinType *via)
{
  char *netname = NULL;
  char *netclass;

//  netname = g_strdup_netname_for_via (pin->Element, via);
  netclass = get_netclass_for_netname (netname);
  g_free (netname);

  return netclass;
}

char *
get_netclass_for_pin (PinType *pin)
{
  char *netname = NULL;
  char *netclass;

  netname = g_strdup_netname_for_pinpad (PIN_TYPE, pin->Element, pin);
//  printf ("Looking up netname for pin, found '%s'\n", netname);
  netclass = get_netclass_for_netname (netname);
//  printf ("Looking up netclass for netname, found '%s'\n", netclass);
  g_free (netname);

  return netclass;
}

char *
get_netclass_for_pad (PadType *pad)
{
  char *netname = NULL;
  char *netclass;

  netname = g_strdup_netname_for_pinpad (PAD_TYPE, pad->Element, pad);
  netclass = get_netclass_for_netname (netname);
  g_free (netname);

  return netclass;
}

static char *
find_first_flagged_netname (int flag)
{
  char *netname = NULL;

  ELEMENT_LOOP (PCB->Data);
    {
      PIN_LOOP (element);
        {
          if (TEST_FLAG (flag, pin))
            {
              netname = g_strdup_netname_for_pinpad (PIN_TYPE, pin->Element, pin);
              if (netname != NULL)
                break;
            }
        }
      END_LOOP;
      PAD_LOOP (element);
        {
          if (TEST_FLAG (flag, pad))
            {
              netname = g_strdup_netname_for_pinpad (PAD_TYPE, pad->Element, pad);
              if (netname != NULL)
                break;
            }
        }
      END_LOOP;
      if (netname != NULL)
        break;
    }
  END_LOOP;

  return netname;
}

/* NB: THIS CALL IS EXPENSIVE!! */
char *
find_netname_for_object (int type, void *ptr1, void *ptr2, void *ptr3)
{
  char *netname;
  int flag = CONNECTEDFLAG;
  ClearFlagOnAllObjects (false /*AndDraw*/, flag, true /* store_undo */);
  /* Find a pin / pad which is connected, and grab the netname from this */
  LookupConnectionByObject (type, ptr1, ptr2, ptr3, false /*AndDraw*/, flag, true /*AndRats*/, true /*store_undo*/);
  netname = find_first_flagged_netname (flag);
  Undo (true);
  return netname;
}

static char *
find_netname_at_xy (LayerType *layer, Coord x, Coord y)
{
  char *netname;
  int flag = CONNECTEDFLAG;
  ClearFlagOnAllObjects (false /*AndDraw*/, flag, true /*store_undo*/);
  /* Find a pin / pad which is connected, and grab the netname from this */
  LookupConnection (x, y, false /*AndDraw*/, 0 /*Range*/, flag, true /*AndRats*/, true /*store_undo*/);
  netname = find_first_flagged_netname (flag);
  Undo (true);
  return netname;
}

char *
get_netclass_for_object (int type, void *ptr1, void *ptr2, void *ptr3)
{
  char *netname = NULL;

  netname = find_netname_for_object (type, ptr1, ptr2, ptr3);

  return get_netclass_for_netname (netname);
}

char *
get_netclass_for_line (LayerType *layer, LineType *line)
{
  return get_netclass_for_object (LINE_TYPE, layer, line, NULL);
}

char *
get_netclass_for_arc (LayerType *layer, ArcType *arc)
{
  return get_netclass_for_object (ARC_TYPE, layer, arc, NULL);
}

char *
get_netclass_at_xy (LayerType *layer, Coord x, Coord y)
{
  char *netname = NULL;

  netname = find_netname_at_xy (layer, x, y);

  return get_netclass_for_netname (netname);
}
