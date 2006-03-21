#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "data.h"
#include "misc.h"

#include "hid.h"
#include "../hidint.h"

typedef struct HID_FlagNode {
  struct HID_FlagNode *next;
  HID_Flag *flags;
  int n;
} HID_FlagNode;

HID_FlagNode *hid_flag_nodes = 0;
static int n_flags = 0;
static HID_Flag *all_flags = 0;

void
hid_register_flags(HID_Flag *a, int n)
{
  HID_FlagNode *ha;

  /* printf("%d flag%s registered\n", n, n==1 ? "" : "s"); */
  ha = (HID_FlagNode *) malloc (sizeof(HID_FlagNode));
  ha->next = hid_flag_nodes;
  hid_flag_nodes = ha;
  ha->flags = a;
  ha->n = n;
  n_flags += n;
  if (all_flags)
    {
      free(all_flags);
      all_flags = 0;
    }
}

static int
flag_sort(const void *va, const void *vb)
{
  HID_Flag *a = (HID_Flag *)va;
  HID_Flag *b = (HID_Flag *)vb;
  return strcmp (a->name, b->name);
}

HID_Flag *
hid_find_flag(const char *name)
{
  HID_FlagNode *hf;
  int i, n, lower, upper;

  if (all_flags == 0)
    {
      n = 0;
      all_flags = malloc(n_flags * sizeof(HID_Flag));
      for (hf=hid_flag_nodes; hf; hf=hf->next)
	for (i=0; i<hf->n; i++)
	  all_flags[n++] = hf->flags[i];
      qsort(all_flags, n_flags, sizeof(HID_Flag), flag_sort);
    }

  lower = -1;
  upper = n_flags+1;
  /*printf("search flag %s\n", name);*/
  while (lower < upper-1)
    {
      i = (lower+upper)/2;
      n = strcmp (all_flags[i].name, name);
      /*printf("try [%d].%s, cmp %d\n", i, all_flags[i].name, n);*/
      if (n == 0)
	return all_flags+i;
      if (n > 0)
	upper = i;
      else
	lower = i;
    }
  printf("unknown flag `%s'\n", name);
  return 0;
}

int
hid_get_flag(const char *name)
{
  static char *buf = 0;
  static int nbuf = 0;
  const char *cp;
  int v;
  HID_Flag *f;
  
  cp = strchr(name, ',');
  if (cp)
    {
      int wv;

      if (nbuf < (cp - name + 1))
	{
	  nbuf = cp - name + 10;
	  buf = MyRealloc(buf, nbuf, "hid_get_flag");
	}
      memcpy (buf, name, cp-name);
      buf[cp-name] = 0;
      wv = strtol(cp+1, 0, 0);
      f = hid_find_flag(buf);
      if (!f)
	return 0;
      return f->function(f->parm) == wv;
    }

  f = hid_find_flag(name);
  if (!f)
    return 0;
  return f->function(f->parm);
}


void
hid_save_and_show_layer_ons (int *save_array)
{
  int i;
  for (i=0; i<MAX_LAYER+2; i++)
    {
      save_array[i] = PCB->Data->Layer[i].On;
      PCB->Data->Layer[i].On = 1;
    }
}

void
hid_restore_layer_ons (int *save_array)
{
  int i;
  for (i=0; i<MAX_LAYER+2; i++)
    PCB->Data->Layer[i].On = save_array[i];
}
