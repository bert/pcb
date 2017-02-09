/*!
 * \file src/common/hidinit.c
 *
 * \brief General initialization routines for HIDs.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
 *
 * Copyright (C) 1998,1999,2000,2001 harry eaton
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Contact addresses for paper mail and Email:
 * harry eaton, 6697 Buttonhole Ct, Columbia, MD 21044 USA
 * haceaton@aplcomm.jhuapl.edu
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(WIN32) && defined(HAVE_WINDOWS_H)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "global.h"
#include "hid.h"
#include "hidnogui.h"
#include "../hidint.h"

/* for dlopen() and friends on windows */
#include "compat.h"

#include "error.h"
#include "global.h"
#include "misc.h"
#include "pcb-printf.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define HID_DEF(x) extern void hid_ ## x ## _init(void);
#include "hid/common/hidlist.h"
#undef HID_DEF

HID **hid_list = 0;
int hid_num_hids = 0;

HID *gui = NULL;
HID *exporter = NULL;

int pixel_slop = 1;

/*!
 * \brief Search a directory for plugins and load them if found
 *
 * The specified path is searched for shared library objects and dynamic link
 * libraries. If any are found, check their symbols for a "hid_x_init" (where
 * x is the basename of the library file) or a pcb_plugin_init, and call it as 
 * a function if found.
 * 
 * \todo the MacOS .dylib should be added to this list
 *
 */
static void
hid_load_dir (char *dirname)
{
  DIR *dir;
  struct dirent *de;

  dir = opendir (dirname);
  if (!dir)
    {
      free (dirname);
      return;
    }
  while ((de = readdir (dir)) != NULL)
    {
      void *sym;
      void (*symv)();
      void *so;
      char *basename, *path, *symname;
      struct stat st;

      basename = strdup (de->d_name);
      if (strlen (basename) > 3
	  && strcasecmp (basename+strlen(basename)-3, ".so") == 0)
	basename[strlen(basename)-3] = 0;
      else if (strlen (basename) > 4
	       && strcasecmp (basename+strlen(basename)-4, ".dll") == 0)
	basename[strlen(basename)-4] = 0;
      path = Concat (dirname, PCB_DIR_SEPARATOR_S, de->d_name, NULL);

      if (stat (path, &st) == 0
	  && (
/* mingw and win32 do not support S_IXGRP or S_IXOTH */
#if defined(S_IXGRP)
	  (st.st_mode & S_IXGRP) ||
#endif
#if defined(S_IXOTH)
	  (st.st_mode & S_IXOTH) ||
#endif
	  (st.st_mode & S_IXUSR) )
	  && S_ISREG (st.st_mode))
	{
	  if ((so = dlopen (path, RTLD_NOW | RTLD_GLOBAL)) == NULL)
	    {
	      fprintf(stderr, "dl_error: %s\n", dlerror ());
	    }
	  else
	    {
	      symname = Concat ("hid_", basename, "_init", NULL);
	      if ((sym = dlsym (so, symname)) != NULL)
		{
		  symv = (void (*)()) sym;
		  symv();
		}
	      else if ((sym = dlsym (so, "pcb_plugin_init")) != NULL)
		{
		  symv = (void (*)()) sym;
		  symv();
		}
	      free (symname);
	    }
	}
      free (basename);
      free (path);
    }
  free (dirname);
  closedir (dir);
}

/*!
 * \brief Initialize the available hids and load plugins
 *
 * The file hid/common/hidlist.h contains a list of HID_DEF statements, compiled
 * by the build system. The HID_DEF macro is redefined here to call the init
 * function for each of those hids.
 */
void
hid_init ()
{
  /* Setup a "nogui" default HID */
  gui = hid_nogui_get_hid ();

  /* Call all of the hid initialization functions */
#define HID_DEF(x) hid_ ## x ## _init();
#include "hid/common/hidlist.h"
#undef HID_DEF

  /* Search the exec_prefix for plugins and load them */
  hid_load_dir (Concat (exec_prefix, PCB_DIR_SEPARATOR_S, "lib",
	PCB_DIR_SEPARATOR_S, "pcb",
	PCB_DIR_SEPARATOR_S, "plugins",
	PCB_DIR_SEPARATOR_S, HOST, NULL));
  hid_load_dir (Concat (exec_prefix, PCB_DIR_SEPARATOR_S, "lib",
	PCB_DIR_SEPARATOR_S, "pcb",
	PCB_DIR_SEPARATOR_S, "plugins", NULL));

  /* Search homedir for plugins and load them. homedir is set by the core
   * immediately on startup */
  if (homedir != NULL)
    {
      hid_load_dir (Concat (homedir, PCB_DIR_SEPARATOR_S, ".pcb",
	PCB_DIR_SEPARATOR_S, "plugins",
	PCB_DIR_SEPARATOR_S, HOST, NULL));
      hid_load_dir (Concat (homedir, PCB_DIR_SEPARATOR_S, ".pcb",
	PCB_DIR_SEPARATOR_S, "plugins", NULL));
    }
  hid_load_dir (Concat ("plugins", PCB_DIR_SEPARATOR_S, HOST, NULL));
  hid_load_dir (Concat ("plugins", NULL));
}

void
hid_uninit (void)
{

}

/*!
 * \brief Add an HID to the list of HIDs.
 */
void
hid_register_hid (HID * hid)
{
  int i;
  int sz = (hid_num_hids + 2) * sizeof (HID *);

  if (hid->struct_size != sizeof (HID))
    {
      fprintf (stderr, "Warning: hid \"%s\" has an incompatible ABI.\n",
	       hid->name);
      return;
    }

  for (i=0; i<hid_num_hids; i++)
    if (hid == hid_list[i])
      return;

  hid_num_hids++;
  if (hid_list)
    hid_list = (HID **) realloc (hid_list, sz);
  else
    hid_list = (HID **) malloc (sz);

  hid_list[hid_num_hids - 1] = hid;
  hid_list[hid_num_hids] = 0;
}


HID *
hid_find_gui ()
{
  int i;

  for (i = 0; i < hid_num_hids; i++)
    if (!hid_list[i]->printer && !hid_list[i]->exporter)
      return hid_list[i];

  fprintf (stderr, "Error: No GUI available.\n");
  exit (1);
}

HID *
hid_find_printer ()
{
  int i;

  for (i = 0; i < hid_num_hids; i++)
    if (hid_list[i]->printer)
      return hid_list[i];

  return 0;
}

HID *
hid_find_exporter (const char *which)
{
  int i;

  for (i = 0; i < hid_num_hids; i++)
    if (hid_list[i]->exporter && strcmp (which, hid_list[i]->name) == 0)
      return hid_list[i];

  fprintf (stderr, "Invalid exporter %s, available ones:", which);
  for (i = 0; i < hid_num_hids; i++)
    if (hid_list[i]->exporter)
      fprintf (stderr, " %s", hid_list[i]->name);
  fprintf (stderr, "\n");

  return 0;
}

HID **
hid_enumerate ()
{
  return hid_list;
}

HID_AttrNode *hid_attr_nodes = 0;

void
hid_register_attributes (HID_Attribute * a, int n)
{
  HID_AttrNode *ha;

  /* printf("%d attributes registered\n", n); */
  ha = (HID_AttrNode *) malloc (sizeof (HID_AttrNode));
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

  (*argc)--;
  (*argv)++;

  for (ha = hid_attr_nodes; ha; ha = ha->next)
    for (i = 0; i < ha->n; i++)
      {
	HID_Attribute *a = ha->attributes + i;
	switch (a->type)
	  {
	  case HID_Label:
	    break;
	  case HID_Integer:
	    if (a->value)
	      *(int *) a->value = a->default_val.int_value;
	    break;
	  case HID_Coord:
	    if (a->value)
	      *(Coord *) a->value = a->default_val.coord_value;
	    break;
	  case HID_Boolean:
	    if (a->value)
	      *(char *) a->value = a->default_val.int_value;
	    break;
	  case HID_Real:
	    if (a->value)
	      *(double *) a->value = a->default_val.real_value;
	    break;
	  case HID_String:
	    if (a->value)
	      *(const char **) a->value = a->default_val.str_value;
	    break;
	  case HID_Enum:
	    if (a->value)
	      *(int *) a->value = a->default_val.int_value;
	    break;
	  case HID_Mixed:
	    if (a->value) {
              *(HID_Attr_Val *) a->value = a->default_val;
	  case HID_Unit:
	    if (a->value)
	      *(int *) a->value = a->default_val.int_value;
	    break;
           }
           break;
	  default:
	    abort ();
	  }
      }

  while (*argc && (*argv)[0][0] == '-' && (*argv)[0][1] == '-')
    {
      int bool_val;
      int arg_ofs;

      bool_val = 1;
      arg_ofs = 2;
    try_no_arg:
      for (ha = hid_attr_nodes; ha; ha = ha->next)
	for (i = 0; i < ha->n; i++)
	  if (strcmp ((*argv)[0] + arg_ofs, ha->attributes[i].name) == 0)
	    {
	      HID_Attribute *a = ha->attributes + i;
	      char *ep;
              const Unit *unit;
	      switch (ha->attributes[i].type)
		{
		case HID_Label:
		  break;
		case HID_Integer:
		  if (a->value)
		    *(int *) a->value = strtol ((*argv)[1], 0, 0);
		  else
		    a->default_val.int_value = strtol ((*argv)[1], 0, 0);
		  (*argc)--;
		  (*argv)++;
		  break;
		case HID_Coord:
		  if (a->value)
		    *(Coord *) a->value = GetValue ((*argv)[1], NULL, NULL);
		  else
		    a->default_val.coord_value = GetValue ((*argv)[1], NULL, NULL);
		  (*argc)--;
		  (*argv)++;
		  break;
		case HID_Real:
		  if (a->value)
		    *(double *) a->value = strtod ((*argv)[1], 0);
		  else
		    a->default_val.real_value = strtod ((*argv)[1], 0);
		  (*argc)--;
		  (*argv)++;
		  break;
		case HID_String:
		  if (a->value)
		    *(char **) a->value = (*argv)[1];
		  else
		    a->default_val.str_value = (*argv)[1];
		  (*argc)--;
		  (*argv)++;
		  break;
		case HID_Boolean:
		  if (a->value)
		    *(char *) a->value = bool_val;
		  else
		    a->default_val.int_value = bool_val;
		  break;
		case HID_Mixed:
		  a->default_val.real_value = strtod ((*argv)[1], &ep);
		  goto do_enum;
		case HID_Enum:
		  ep = (*argv)[1];
		do_enum:
		  ok = 0;
		  for (e = 0; a->enumerations[e]; e++)
		    if (strcmp (a->enumerations[e], ep) == 0)
		      {
			ok = 1;
			a->default_val.int_value = e;
			a->default_val.str_value = ep;
			break;
		      }
		  if (!ok)
		    {
		      fprintf (stderr,
			       "ERROR:  \"%s\" is an unknown value for the --%s option\n",
			       (*argv)[1], a->name);
		      exit (1);
		    }
		  (*argc)--;
		  (*argv)++;
		  break;
		case HID_Path:
		  abort ();
		  a->default_val.str_value = (*argv)[1];
		  (*argc)--;
		  (*argv)++;
		  break;
		case HID_Unit:
                  unit = get_unit_struct ((*argv)[1]);
                  if (unit == NULL)
		    {
		      fprintf (stderr,
			       "ERROR:  unit \"%s\" is unknown to pcb (option --%s)\n",
			       (*argv)[1], a->name);
		      exit (1);
		    }
		  a->default_val.int_value = unit->index;
		  a->default_val.str_value = unit->suffix;
		  (*argc)--;
		  (*argv)++;
		  break;
		}
	      (*argc)--;
	      (*argv)++;
	      ha = 0;
	      goto got_match;
	    }
      if (bool_val == 1 && strncmp ((*argv)[0], "--no-", 5) == 0)
	{
	  bool_val = 0;
	  arg_ofs = 5;
	  goto try_no_arg;
	}
      fprintf (stderr, "unrecognized option: %s\n", (*argv)[0]);
      exit (1);
    got_match:;
    }

  (*argc)++;
  (*argv)--;
}

static int
attr_hash (HID_Attribute *a)
{
  unsigned char *cp = (unsigned char *)a;
  int i, rv=0;
  for (i=0; i<(int)((char *)&(a->hash) - (char *)a); i++)
    rv = (rv * 13) ^ (rv >> 16) ^ cp[i];
  return rv;
}

void
hid_save_settings (int locally)
{
  char *fname;
  struct stat st;
  FILE *f;
  HID_AttrNode *ha;
  int i;

  if (locally)
    {
      fname = Concat ("pcb.settings", NULL);
    }
  else
    {
      if (homedir == NULL)
	return;
      fname = Concat (homedir, PCB_DIR_SEPARATOR_S, ".pcb", NULL);

      if (stat (fname, &st))
	if (MKDIR (fname, 0777))
	  {
	    free (fname);
	    return;
	  }
      free (fname);

      fname = Concat (homedir, PCB_DIR_SEPARATOR_S, ".pcb", 
               PCB_DIR_SEPARATOR_S, "settings", NULL);
    }

  f = fopen (fname, "w");
  if (!f)
    {
      Message ("Can't open %s", fname);
      free (fname);
      return;
    }

  for (ha = hid_attr_nodes; ha; ha = ha->next)
    {
      for (i = 0; i < ha->n; i++)
	{
	  const char *str;
	  HID_Attribute *a = ha->attributes + i;

	  if (a->hash == attr_hash (a))
	    fprintf (f, "# ");
	  switch (a->type)
	    {
	    case HID_Label:
	      break;
	    case HID_Integer:
	      fprintf (f, "%s = %d\n",
		       a->name,
		       a->value ? *(int *)a->value : a->default_val.int_value);
	      break;
	    case HID_Coord:
	      pcb_fprintf (f, "%s = %$mS\n",
		           a->name,
		           a->value ? *(Coord *)a->value : a->default_val.coord_value);
	      break;
	    case HID_Boolean:
	      fprintf (f, "%s = %d\n",
		       a->name,
		       a->value ? *(char *)a->value : a->default_val.int_value);
	      break;
	    case HID_Real:
	      fprintf (f, "%s = %f\n",
		       a->name,
		       a->value ? *(double *)a->value : a->default_val.real_value);
	      break;
	    case HID_String:
	    case HID_Path:
	      str = a->value ? *(char **)a->value : a->default_val.str_value;
	      fprintf (f, "%s = %s\n", a->name, str ? str : "");
	      break;
	    case HID_Enum:
	      fprintf (f, "%s = %s\n",
		       a->name,
		       a->enumerations[a->value ? *(int *)a->value : a->default_val.int_value]);
	      break;
	    case HID_Mixed:
             {
               HID_Attr_Val *value =
                 a->value ? (HID_Attr_Val*) a->value : &(a->default_val);
               fprintf (f, "%s = %g%s\n",
                        a->name,
                        value->real_value,
                        a->enumerations[value->int_value]);
             }
	      break;
	    case HID_Unit:
	      fprintf (f, "%s = %s\n",
		       a->name,
		       get_unit_list()[a->value ? *(int *)a->value : a->default_val.int_value].suffix);
	      break;
	    }
	}
      fprintf (f, "\n");
    }
  fclose (f);
  free (fname);
}

static void
hid_set_attribute (char *name, char *value)
{
  const Unit *unit;
  HID_AttrNode *ha;
  int i, e, ok;

  for (ha = hid_attr_nodes; ha; ha = ha->next)
    for (i = 0; i < ha->n; i++)
      if (strcmp (name, ha->attributes[i].name) == 0)
	{
	  HID_Attribute *a = ha->attributes + i;
	  switch (ha->attributes[i].type)
	    {
	    case HID_Label:
	      break;
	    case HID_Integer:
	      a->default_val.int_value = strtol (value, 0, 0);
	      break;
	    case HID_Coord:
	      a->default_val.coord_value = GetValue (value, NULL, NULL);
	      break;
	    case HID_Real:
	      a->default_val.real_value = strtod (value, 0);
	      break;
	    case HID_String:
	      a->default_val.str_value = strdup (value);
	      break;
	    case HID_Boolean:
	      a->default_val.int_value = 1;
	      break;
	    case HID_Mixed:
	      a->default_val.real_value = strtod (value, &value);
	      /* fall through */
	    case HID_Enum:
	      ok = 0;
	      for (e = 0; a->enumerations[e]; e++)
		if (strcmp (a->enumerations[e], value) == 0)
		  {
		    ok = 1;
		    a->default_val.int_value = e;
		    a->default_val.str_value = value;
		    break;
		  }
	      if (!ok)
		{
		  fprintf (stderr,
			   "ERROR:  \"%s\" is an unknown value for the %s option\n",
			   value, a->name);
		  exit (1);
		}
	      break;
	    case HID_Path:
	      a->default_val.str_value = value;
	      break;
	    case HID_Unit:
              unit = get_unit_struct (value);
              if (unit == NULL)
		{
		  fprintf (stderr,
		           "ERROR:  unit \"%s\" is unknown to pcb (option --%s)\n",
			   value, a->name);
		  exit (1);
		}
	      a->default_val.int_value = unit->index;
	      a->default_val.str_value = unit->suffix;
	      break;
	    }
	}
}

static void
hid_load_settings_1 (char *fname)
{
  char line[1024], *namep, *valp, *cp;
  FILE *f;

  f = fopen (fname, "r");
  if (!f)
    {
      free (fname);
      return;
    }

  free (fname);
  while (fgets (line, sizeof(line), f) != NULL)
    {
      for (namep=line; *namep && isspace ((int) *namep); namep++)
	;
      if (*namep == '#')
	continue;
      for (valp=namep; *valp && !isspace((int) *valp); valp++)
	;
      if (! *valp)
	continue;
      *valp++ = 0;
      while (*valp && (isspace ((int) *valp) || *valp == '='))
	valp ++;
      if (! *valp)
	continue;
      cp = valp + strlen(valp) - 1;
      while (cp >= valp && isspace ((int) *cp))
	*cp-- = 0;
      hid_set_attribute (namep, valp);
    }

  fclose (f);
}

void
hid_load_settings ()
{
  HID_AttrNode *ha;
  int i;

  for (ha = hid_attr_nodes; ha; ha = ha->next)
    for (i = 0; i < ha->n; i++)
      ha->attributes[i].hash = attr_hash (ha->attributes+i);

  hid_load_settings_1 (Concat (pcblibdir, PCB_DIR_SEPARATOR_S, "settings", NULL));
  if (homedir != NULL)
    hid_load_settings_1 (Concat (homedir, PCB_DIR_SEPARATOR_S, ".pcb",
               PCB_DIR_SEPARATOR_S, "settings", NULL));
  hid_load_settings_1 (Concat ("pcb.settings", NULL));
}

#define HASH_SIZE 31

typedef struct ecache
{
  struct ecache *next;
  const char *name;
  hidval val;
} ecache;

typedef struct ccache
{
  ecache *colors[HASH_SIZE];
  ecache *lru;
} ccache;

static void
copy_color (int set, hidval * cval, hidval * aval)
{
  if (set)
    memcpy (cval, aval, sizeof (hidval));
  else
    memcpy (aval, cval, sizeof (hidval));
}

int
hid_cache_color (int set, const char *name, hidval * val, void **vcache)
{
  unsigned long hash;
  const char *cp;
  ccache *cache;
  ecache *e;

  cache = (ccache *) * vcache;
  if (cache == 0) {
    cache = (ccache *) calloc (sizeof (ccache), 1);
    *vcache = cache;
  }
  if (cache->lru && strcmp (cache->lru->name, name) == 0)
    {
      copy_color (set, &(cache->lru->val), val);
      return 1;
    }

  /* djb2: this algorithm (k=33) was first reported by dan bernstein many
   * years ago in comp.lang.c. another version of this algorithm (now favored
   * by bernstein) uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic
   * of number 33 (why it works better than many other constants, prime or
   * not) has never been adequately explained.
   */
  hash = 5381;
  for (cp = name, hash = 0; *cp; cp++)
    hash = ((hash << 5) + hash) + (*cp & 0xff); /* hash * 33 + c */
  hash %= HASH_SIZE;

  for (e = cache->colors[hash]; e; e = e->next)
    if (strcmp (e->name, name) == 0)
      {
	copy_color (set, &(e->val), val);
	cache->lru = e;
	return 1;
      }
  if (!set)
    return 0;

  e = (ecache *) malloc (sizeof (ecache));
  e->next = cache->colors[hash];
  cache->colors[hash] = e;
  e->name = strdup (name);
  memcpy (&(e->val), val, sizeof (hidval));
  cache->lru = e;

  return 1;
}

/* otherwise homeless function, refactored out of the five export HIDs */
void
derive_default_filename(const char *pcbfile, HID_Attribute *filename_attrib, const char *suffix, char **memory)
{
	char *buf;
	char *pf;

	if (pcbfile == NULL)
	  pf = strdup ("unknown.pcb");
	else
	  pf = strdup (pcbfile);

	if (!pf || (memory && filename_attrib->default_val.str_value != *memory)) return;

	buf = (char *)malloc (strlen (pf) + strlen(suffix) + 1);
	if (memory) *memory = buf;
	if (buf) {
		size_t bl;
		strcpy (buf, pf);
		bl = strlen(buf);
		if (bl > 4 && strcmp (buf + bl - 4, ".pcb") == 0)
			buf[bl - 4] = 0;
		strcat(buf, suffix);
		if (filename_attrib->default_val.str_value)
			free ((void *) filename_attrib->default_val.str_value);
		filename_attrib->default_val.str_value = buf;
	}

	free (pf);
}
