#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hid.h"
#include "../hidint.h"

#define HID_DEF(x) extern void hid_ ## x ## _init(void);
#include "hid/common/hidlist.h"
#undef HID_DEF

HID **hid_list = 0;
int hid_num_hids = 0;

extern HID hid_nogui;

HID *gui = &hid_nogui;

int pixel_slop = 1;

void
hid_init()
{
  gui = &hid_nogui;
#define HID_DEF(x) hid_ ## x ## _init();
#include "hid/common/hidlist.h"
#undef HID_DEF
}

void
hid_register_hid (HID *hid)
{
  int sz = (hid_num_hids + 2) * sizeof (HID *);

  hid_num_hids ++;
  if (hid_list)
    hid_list = (HID **) realloc (hid_list, sz);
  else
    hid_list = (HID **) malloc (sz);

  hid_list[hid_num_hids - 1] = hid;
  hid_list[hid_num_hids] = 0;
}

static void (*gui_start)(int *, char ***) = 0;
static HID *default_gui = 0;

void
hid_register_gui(HID *Pgui, void (*func)(int *, char ***))
{
  if (gui_start)
    return;

  default_gui = Pgui;
  gui_start = func;
}


HID *
hid_find_gui ()
{
  int i;

  for (i=0; i<hid_num_hids; i++)
    if (! hid_list[i]->printer && ! hid_list[i]->exporter)
      return hid_list[i];

  fprintf(stderr, "Error: No GUI available.\n");
  exit(1);
}

HID *
hid_find_printer ()
{
  int i;

  for (i=0; i<hid_num_hids; i++)
    if (hid_list[i]->printer)
      return hid_list[i];

  return 0;
}

HID *
hid_find_exporter (const char *which)
{
  int i;

  for (i=0; i<hid_num_hids; i++)
    if (hid_list[i]->exporter
	&& strcmp (which, hid_list[i]->name) == 0)
      return hid_list[i];

  fprintf(stderr, "Invalid exporter %s, available ones:", which);
  for (i=0; i<hid_num_hids; i++)
    if (hid_list[i]->exporter)
      fprintf(stderr, " %s", hid_list[i]->name);
  fprintf(stderr, "\n");

  return 0;
}

HID **
hid_enumerate ()
{
  return hid_list;
}

HID_AttrNode *hid_attr_nodes = 0;

void
hid_register_attributes(HID_Attribute *a, int n)
{
  HID_AttrNode *ha;

  /* printf("%d attributes registered\n", n); */
  ha = (HID_AttrNode *)malloc (sizeof(HID_AttrNode));
  ha->next = hid_attr_nodes;
  hid_attr_nodes = ha;
  ha->attributes = a;
  ha->n = n;
}

void
hid_parse_command_line (int *argc, char ***argv)
{
  HID_AttrNode *ha;
  int i, e, ok;

  (*argc) --;
  (*argv) ++;

  for (ha=hid_attr_nodes; ha; ha=ha->next)
    for (i=0; i<ha->n; i++)
      {
	HID_Attribute *a = ha->attributes+i;
	switch (a->type)
	  {
	  case HID_Label:
	    break;
	  case HID_Integer:
	    if (a->value)
	      *(int *)a->value = a->default_val.int_value;
	    break;
	  case HID_Boolean:
	    if (a->value)
	      *(char *)a->value = a->default_val.int_value;
	    break;
	  case HID_Real:
	    if (a->value)
	      *(double *)a->value = a->default_val.real_value;
	    break;
	  case HID_String:
	    if (a->value)
	      *(char **)a->value = a->default_val.str_value;
	    break;
	  case HID_Enum:
	    if (a->value)
	      *(int *)a->value = a->default_val.int_value;
	    break;
	  default:
	    abort();
	  }
      }

  while (*argc && (*argv)[0][0] == '-' && (*argv)[0][1] == '-')
    {
      for (ha=hid_attr_nodes; ha; ha=ha->next)
	for (i=0; i<ha->n; i++)
	  if (strcmp ((*argv)[0]+2, ha->attributes[i].name) == 0)
	    {
	      HID_Attribute *a = ha->attributes+i;
	      char *ep;
	      switch (ha->attributes[i].type)
		{
		case HID_Label:
		  break;
		case HID_Integer:
		  if (a->value)
		    *(int *)a->value = strtol((*argv)[1], 0, 0);
		  else
		    a->default_val.int_value = strtol((*argv)[1], 0, 0);
		  (*argc)--;
		  (*argv)++;
		  break;
		case HID_Real:
		  if (a->value)
		    *(double *)a->value = strtod((*argv)[1], 0);
		  else
		    a->default_val.real_value = strtod((*argv)[1], 0);
		  (*argc)--;
		  (*argv)++;
		  break;
		case HID_String:
		  if (a->value)
		    *(char **)a->value = (*argv)[1];
		  else
		    a->default_val.str_value = (*argv)[1];
		  (*argc)--;
		  (*argv)++;
		  break;
		case HID_Boolean:
		  if (a->value)
		    *(char *)a->value = 1;
		  else
		    a->default_val.int_value = 1;
		  break;
		case HID_Mixed:
		  abort();
		  a->default_val.real_value = strtod((*argv)[1], &ep);
		  goto do_enum;
		case HID_Enum:
		  ep = (*argv)[1];
		do_enum:
		  ok = 0;
		  for (e=0; a->enumerations[e]; e++)
		    if (strcmp (a->enumerations[e], (*argv)[1]) == 0)
		      {
			ok = 1;
			a->default_val.int_value = e;
			a->default_val.str_value = ep;
			break;
		      }
		  if (! ok )
		    {
		      fprintf(stderr, "ERROR:  \"%s\" is an unknown value for the --%s option\n",
			      (*argv)[1], a->name);
		      exit (1);
		    }
		  (*argc)--;
		  (*argv)++;
		  break;
		case HID_Path:
		  abort();
		  a->default_val.str_value = (*argv)[1];
		  (*argc)--;
		  (*argv)++;
		  break;
		}
	      (*argc)--;
	      (*argv)++;
	      ha = 0;
	      goto got_match;
	    }
      fprintf(stderr, "unrecognized option: %s\n", (*argv)[0]);
      exit(1);
    got_match: ;
    }

  (*argc) ++;
  (*argv) --;
}

#define HASH_SIZE 32

typedef struct ecache {
  struct ecache *next;
  const char *name;
  hidval val;
} ecache;

typedef struct ccache {
  ecache *colors[HASH_SIZE];
  ecache *lru;
} ccache;

static void
copy_color (int set, hidval *cval, hidval *aval)
{
  if (set)
    memcpy (cval, aval, sizeof(hidval));
  else
    memcpy (aval, cval, sizeof(hidval));
}

int
hid_cache_color (int set,
		 const char *name,
		 hidval *val,
		 void **vcache)
{
  int hash;
  const char *cp;
  ccache *cache;
  ecache *e;

  cache = (ccache *)*vcache;
  if (cache == 0)
    cache = *vcache = (void *) calloc (sizeof(ccache), 1);

  if (cache->lru && strcmp (cache->lru->name, name) == 0)
    {
      copy_color(set, &(cache->lru->val), val);
      return 1;
    }

  for (cp=name, hash=0; *cp; cp++)
    hash += (*cp) & 0xff;
  hash  %= HASH_SIZE;

  for (e=cache->colors[hash]; e; e=e->next)
    if (strcmp (e->name, name) == 0)
      {
	copy_color(set, &(e->val), val);
	return 1;
      }
  if (!set)
    return 0;

  e = (ecache *) malloc (sizeof (ecache));
  e->next = cache->colors[hash];
  cache->colors[hash] = e;
  e->name = strdup (name);
  memcpy (&(e->val), val, sizeof(hidval));

  return 1;
}
