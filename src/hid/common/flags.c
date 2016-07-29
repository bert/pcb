#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "data.h"
#include "misc.h"

#include "hid.h"
#include "../hidint.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

typedef struct HID_FlagNode
{
  struct HID_FlagNode *next;
  HID_Flag *flags;
  int n;
} HID_FlagNode;

HID_FlagNode *hid_flag_nodes = 0;
static int n_flags = 0;
static HID_Flag *all_flags = 0;

void
hid_register_flags (HID_Flag * a, int n)
{
  HID_FlagNode *ha;

  /* printf("%d flag%s registered\n", n, n==1 ? "" : "s"); */
  ha = (HID_FlagNode *) malloc (sizeof (HID_FlagNode));
  ha->next = hid_flag_nodes;
  hid_flag_nodes = ha;
  ha->flags = a;
  ha->n = n;
  n_flags += n;
  if (all_flags)
    {
      free (all_flags);
      all_flags = 0;
    }
}

static int
flag_sort (const void *va, const void *vb)
{
  HID_Flag *a = (HID_Flag *) va;
  HID_Flag *b = (HID_Flag *) vb;
  return strcmp (a->name, b->name);
}

HID_Flag *
hid_find_flag (const char *name)
{
  HID_FlagNode *hf;
  int i, n, lower, upper;

  if (all_flags == 0)
    {
      n = 0;
      all_flags = (HID_Flag *)malloc (n_flags * sizeof (HID_Flag));
      for (hf = hid_flag_nodes; hf; hf = hf->next)
	for (i = 0; i < hf->n; i++)
	  all_flags[n++] = hf->flags[i];
      qsort (all_flags, n_flags, sizeof (HID_Flag), flag_sort);
    }

  lower = -1;
  upper = n_flags + 1;
  /*printf("search flag %s\n", name); */
  while (lower < upper - 1)
    {
      i = (lower + upper) / 2;
      n = strcmp (all_flags[i].name, name);
      /*printf("try [%d].%s, cmp %d\n", i, all_flags[i].name, n); */
      if (n == 0)
	return all_flags + i;
      if (n > 0)
	upper = i;
      else
	lower = i;
    }
  printf ("unknown flag `%s'\n", name);
  return 0;
}

int
hid_get_flag (const char *name)
{
  static char *buf = 0;
  static int nbuf = 0;
  const char *cp;
  HID_Flag *f;

  cp = strchr (name, ',');
  if (cp)
    {
      int wv;

      if (nbuf < (cp - name + 1))
	{
	  nbuf = cp - name + 10;
	  buf = (char *)realloc (buf, nbuf);
	}
      memcpy (buf, name, cp - name);
      buf[cp - name] = 0;
      /* A number without units is just a number.  */
      wv = GetValueEx (cp + 1, NULL, NULL, NULL, NULL);
      f = hid_find_flag (buf);
      if (!f)
	return 0;
      return f->function (f->parm) == wv;
    }

  f = hid_find_flag (name);
  if (!f)
    return 0;
  return f->function (f->parm);
}


void
hid_save_and_show_layer_ons (int *save_array)
{
  int i;
  for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
    {
      save_array[i] = PCB->Data->Layer[i].On;
      PCB->Data->Layer[i].On = 1;
    }
}

void
hid_restore_layer_ons (int *save_array)
{
  int i;
  for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
    PCB->Data->Layer[i].On = save_array[i];
}

const char *
layer_type_to_file_name (int idx, int style)
{
  int group;
  int nlayers;
  const char *single_name;

  switch (idx)
    {
    case SL (SILK, TOP):
      return "topsilk";
    case SL (SILK, BOTTOM):
      return "bottomsilk";
    case SL (MASK, TOP):
      return "topmask";
    case SL (MASK, BOTTOM):
      return "bottommask";
    case SL (PDRILL, 0):
      return "plated-drill";
    case SL (UDRILL, 0):
      return "unplated-drill";
    case SL (PASTE, TOP):
      return "toppaste";
    case SL (PASTE, BOTTOM):
      return "bottompaste";
    case SL (INVISIBLE, 0):
      return "invisible";
    case SL (FAB, 0):
      return "fab";
    case SL (ASSY, TOP):
      return "topassembly";
    case SL (ASSY, BOTTOM):
      return "bottomassembly";
    default:
      group = GetLayerGroupNumberByNumber(idx);
      nlayers = PCB->LayerGroups.Number[group];
      single_name = PCB->Data->Layer[idx].Name;
      if (group == GetLayerGroupNumberBySide(TOP_SIDE))
	{
	  if (style == FNS_first
	      || (style == FNS_single
		  && nlayers == 2))
	    return single_name;
	  return "top";
	}
      else if (group == GetLayerGroupNumberBySide(BOTTOM_SIDE))
	{
	  if (style == FNS_first
	      || (style == FNS_single
		  && nlayers == 2))
	    return single_name;
	  return "bottom";
	}
      else if (nlayers == 1
	       && (strcmp (PCB->Data->Layer[idx].Name, "route") == 0 ||
		   strcmp (PCB->Data->Layer[idx].Name, "outline") == 0))
	{
	  return "outline";
	}
      else
	{
	  static char buf[20];
	  if (style == FNS_first
	      || (style == FNS_single
		  && nlayers == 1))
	    return single_name;
	  sprintf (buf, "group%d", group);
	  return buf;
	}
      break;
    }
}

const char *
layer_type_to_file_name_ex (int idx, int style, const char *layer_name)
{
  if (idx == SL (PDRILL, 0)
      || idx == SL (UDRILL, 0))
    return layer_name;
  else
    return layer_type_to_file_name (idx,  style);
}
