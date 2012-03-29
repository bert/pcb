/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>

#include <time.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "misc.h"
#include "error.h"
#include "buffer.h"
#include "mirror.h"
#include "create.h"

#include "hid.h"
#include "../hidint.h"
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"
#include "hid/common/hidinit.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#include "vrml.h"

static fplist_struct *fplist;
static int nfplist, mfplist;
static fplist_struct *maplist;
static int nmaplist, mmaplist;
static fplist_struct *templist;
static globalvar_struct *globalvars;
static int nglobalvars, mglobalvars;

/* model types for vrml_export_model */
#define VRML_MODEL_STANDARD	0	/* primary model */
#define VRML_MODEL_OVERLAY	1

static int
vrml_load_footprints (char *libpath, char *toppath)
{
  char olddir[MAXPATHLEN + 1];	/* The directory we start out in (cwd) */
  char subdir[MAXPATHLEN + 1];	/* The directory holding footprints to load */
  DIR *subdirobj;		/* Interable object holding all subdir entries */
  struct dirent *subdirentry;	/* Individual subdir entry */
  struct stat buffer;		/* Buffer used in stat */
  size_t l;
  size_t len;

  memset (subdir, 0, sizeof subdir);
  memset (olddir, 0, sizeof olddir);
  if (GetWorkingDirectory (olddir) == NULL)
    return 0;

  if (strcmp (libpath, "(local)") == 0)
    strcpy (subdir, ".");
  else
    strcpy (subdir, libpath);

  if (chdir (subdir))
    return 0;

  if (GetWorkingDirectory (subdir) == NULL)
    return 0;

  if ((subdirobj = opendir (subdir)) == NULL)
    return 0;

  while ((subdirentry = readdir (subdirobj)) != NULL)
    {
      l = strlen (subdirentry->d_name);
      if (!stat (subdirentry->d_name, &buffer) && S_ISREG (buffer.st_mode)
	  && subdirentry->d_name[0] != '.'
	  && NSTRCMP (subdirentry->d_name, "CVS") != 0
	  && NSTRCMP (subdirentry->d_name, "Makefile") != 0
	  && NSTRCMP (subdirentry->d_name, "Makefile.am") != 0
	  && NSTRCMP (subdirentry->d_name, "Makefile.in") != 0
	  && (l < 4 || NSTRCMP (subdirentry->d_name + (l - 4), ".png") != 0)
	  && (l < 5 || NSTRCMP (subdirentry->d_name + (l - 5), ".html") != 0))
	{
	  if (l > 4
	      && NSTRCMP (subdirentry->d_name + (l - 4), VRML_MAP_EXT) == 0)
	    {
	      if (nmaplist == mmaplist)
		{
		  if ((templist =
		       (fplist_struct *) realloc (maplist,
						  sizeof (fplist_struct) *
						  (mmaplist + 100))) == NULL)
		    return 0;
		  maplist = templist;
		  mmaplist += 100;
		}
	      len =
		strlen (subdir) + strlen (PCB_DIR_SEPARATOR_S) +
		strlen (subdirentry->d_name) + 1;
	      maplist[nmaplist].path = (char *) calloc (1, len);
	      strcpy (maplist[nmaplist].path, subdir);
	      strcat (maplist[nmaplist].path, PCB_DIR_SEPARATOR_S);
	      maplist[nmaplist].name =
		maplist[nmaplist].path + strlen (maplist[nmaplist].path);
	      strcat (maplist[nmaplist].path, subdirentry->d_name);
	      nmaplist++;
	    }
	  else
	    {
	      if (nfplist == mfplist)
		{
		  if ((templist =
		       (fplist_struct *) realloc (fplist,
						  sizeof (fplist_struct) *
						  (mfplist + 100))) == NULL)
		    return 0;
		  fplist = templist;
		  mfplist += 100;
		}
	      len =
		strlen (subdir) + strlen (PCB_DIR_SEPARATOR_S) +
		strlen (subdirentry->d_name) + 1;
	      fplist[nfplist].path = (char *) calloc (1, len);
	      strcpy (fplist[nfplist].path, subdir);
	      strcat (fplist[nfplist].path, PCB_DIR_SEPARATOR_S);
	      fplist[nfplist].name =
		fplist[nfplist].path + strlen (fplist[nfplist].path);
	      strcat (fplist[nfplist].path, subdirentry->d_name);
	      nfplist++;
	    }
	}
    }

  closedir (subdirobj);

  chdir (olddir);

  return 1;
}

static int
vrml_parse_mapfile (char *line, char **refdes_result, char **var_result,
		    char **value_result, int *condition)
{
  char *s, *rr = 0, *rv = 0;

  if (refdes_result)
    *refdes_result = NULL;
  if (var_result)
    *var_result = NULL;
  if (value_result)
    *value_result = NULL;

  s = line + strlen (line);

  while (s > line && (isspace (*s) || *s == 0))
    *s-- = 0;

  s = line;
  while (*s && isblank (*s))
    s++;

  if (*s == 0 || *s == '#')
    return 0;
  if (*s == '[')
    {
      rr = s;
      while (*s && (!isblank (*s)))
	s++;
      *s = 0;
      if (strcmp (rr, "[common]") == 0)
	*condition = 1;
      else if (strcmp (rr, "[vrml]") == 0)
	*condition = 1;
      else
	*condition = 0;
      return 0;
    }
  if (refdes_result)
    {
      rr = s;
      while (*s && (!isblank (*s) && *s != ':' && *s != '='))
	s++;
      if (*s)
	*s++ = 0;
      if (!strlen (rr))
	return 0;
    }
  while (*s && (isblank (*s) || *s == ':' || *s == '='))
    s++;
  rv = s;
  while (*s && (!isblank (*s) && *s != ':' && *s != '='))
    s++;
  if (*s)
    *s++ = 0;
  if (!strlen (rv))
    return 0;
  while (*s && (isblank (*s) || *s == ':' || *s == '='))
    s++;

  if (refdes_result)
    *refdes_result = strdup (rr);
  if (var_result)
    *var_result = strdup (rv);
  if (value_result)
    *value_result = strdup (s);
  return 1;
}



static int
vrml_parse_option (char *line, char **option_result, char **arg_result)
{
  char *s, *ss, option[64], arg[512];
  int argc = 1;

  if (option_result)
    *option_result = NULL;
  if (arg_result)
    *arg_result = NULL;

  s = line;
  while (*s == ' ' || *s == '\t')
    ++s;
  if (!*s || *s == '\n' || *s == '#' || *s == '[')
    return 0;
  if ((ss = strchr (s, '\n')) != NULL)
    *ss = '\0';
  arg[0] = '\0';
  sscanf (s, "%63s %511[^\n]", option, arg);

  s = option;			/* Strip trailing ':' or '=' */
  while (*s && *s != ':' && *s != '=')
    ++s;
  *s = '\0';

  s = arg;			/* Strip leading ':', '=', and whitespace */
  while (*s == ' ' || *s == '\t' || *s == ':' || *s == '=' || *s == '"')
    ++s;
  if ((ss = strchr (s, '"')) != NULL)
    *ss = '\0';

  if (option_result)
    *option_result = strdup (option);
  if (arg_result && *s)
    {
      *arg_result = strdup (s);
      ++argc;
    }
  return argc;
}


static char *
vrml_libpaths ()
{
  char *libpaths;
  char *p;
  char *gtkpaths = NULL, *opt;
  char configpath[MAXPATHLEN + 1];
  char line[2048];
  FILE *fc;

  strcpy (configpath, homedir);
  strcat (configpath, PCB_DIR_SEPARATOR_S);
  strcat (configpath, ".pcb");
  strcat (configpath, PCB_DIR_SEPARATOR_S);
  strcat (configpath, "preferences");
  fc = fopen (configpath, "r");
  if (fc)
    {
      while (fgets (line, sizeof (line), fc))
	{
	  if (vrml_parse_option (line, &opt, &gtkpaths)
	      && !strcmp (opt, "library-newlib"))
	    {
	      if (opt)
		free (opt);
	      break;
	    }
	  if (opt)
	    free (opt);
	  if (gtkpaths)
	    free (gtkpaths);
	}
      fclose (fc);
    }


  if (!gtkpaths)
    {
      return strdup (Settings.LibraryTree);
    }
  else
    {
      libpaths =
	(char *) calloc (1,
			 strlen (Settings.LibraryTree) + strlen (gtkpaths) +
			 strlen (PCB_PATH_DELIMETER) + 1);
      if (!libpaths)
	{
	  free (gtkpaths);
	  return NULL;
	}
      strcpy (libpaths, Settings.LibraryTree);
      for (p = strtok (gtkpaths, PCB_PATH_DELIMETER); p && *p;
	   p = strtok (NULL, PCB_PATH_DELIMETER))
	{
	  if (!strstr (Settings.LibraryTree, p))
	    {
	      strcat (libpaths, PCB_PATH_DELIMETER);
	      strcat (libpaths, p);
	    }
	}
      free (gtkpaths);
      return libpaths;
    }
}

static void
vrml_load_globalmap (char *filename)
{
  FILE *fm;
  char line[2048];
  char *ref, *opt, *val;
  int condition = 1;

  fm = fopen (filename, "r");
  if (fm)
    {
      while (fgets (line, sizeof (line), fm))
	{
	  if (vrml_parse_mapfile (line, &ref, &opt, &val, &condition))
	    {
	      if (!condition)
		continue;
	      if (nglobalvars == mglobalvars)
		{
		  globalvar_struct *templist;
		  if ((templist =
		       (globalvar_struct *) realloc (globalvars,
						     sizeof (globalvar_struct)
						     * (mglobalvars +
							30))) != NULL)
		    {
		      globalvars = templist;
		      mglobalvars += 30;
		    }
		}
	      globalvars[nglobalvars].refdes = ref;
	      globalvars[nglobalvars].name = opt;
	      globalvars[nglobalvars].value = val;
	      nglobalvars++;
	    }
	}
      fclose (fm);
    }
}

static void
vrml_init_footprints (void)
{
  char toppath[MAXPATHLEN + 1];	/* String holding abs path to top level library dir */
  char working[MAXPATHLEN + 1];	/* String holding abs path to working dir */
  char *libpaths;		/* String holding list of library paths to search */
  char *p;			/* Helper string used in iteration */
  DIR *dirobj;			/* Iterable directory object */
  struct dirent *direntry = NULL;	/* Object holding individual directory entries */
  struct stat buffer;		/* buffer used in stat */


  fplist = NULL;
  nfplist = mfplist = 0;
  maplist = NULL;
  nmaplist = mmaplist = 0;
  globalvars = 0;
  nglobalvars = mglobalvars = 0;


  memset (toppath, 0, sizeof toppath);
  memset (working, 0, sizeof working);

  if (GetWorkingDirectory (working) == NULL)
    return;

  if (PCB && PCB->Filename)
    {
      int bl;
      strcpy (toppath, PCB->Filename);
      bl = strlen (toppath);
      if (bl > 4 && strcmp (toppath + bl - 4, ".pcb") == 0)
	toppath[bl - 4] = 0;

      strcat (toppath, VRML_MAP_EXT);
      vrml_load_globalmap (toppath);
    }

  libpaths = vrml_libpaths ();

  if (!libpaths)
    return;

  for (p = strtok (libpaths, PCB_PATH_DELIMETER); p && *p;
       p = strtok (NULL, PCB_PATH_DELIMETER))
    {
      strncpy (toppath, p, sizeof (toppath) - 1);

      if (chdir (working))
	{
	  free (libpaths);
	  return;
	}

      if (chdir (toppath))
	continue;

      if (GetWorkingDirectory (toppath) == NULL)
	continue;

      vrml_load_footprints ("(local)", toppath);


      if ((dirobj = opendir (toppath)) == NULL)
	continue;

      while ((direntry = readdir (dirobj)) != NULL)
	{
	  if (!stat (direntry->d_name, &buffer)
	      && S_ISDIR (buffer.st_mode)
	      && direntry->d_name[0] != '.'
	      && NSTRCMP (direntry->d_name, "CVS") != 0)
	    {
	      vrml_load_footprints (direntry->d_name, toppath);
	    }
	}
      closedir (dirobj);
    }

  chdir (working);
  if (libpaths)
    free (libpaths);

}

static void
vrml_release_footprints (void)
{
  int i;

  for (i = 0; i < nfplist; i++)
    {
      if (fplist[i].path)
	free (fplist[i].path);
    }
  for (i = 0; i < nmaplist; i++)
    {
      if (maplist[i].path)
	free (maplist[i].path);
    }
  for (i = 0; i < nglobalvars; i++)
    {
      if (globalvars[i].refdes)
	free (globalvars[i].refdes);
      if (globalvars[i].name)
	free (globalvars[i].name);
      if (globalvars[i].value)
	free (globalvars[i].value);
    }

  if (fplist)
    free (fplist);
  if (maplist)
    free (maplist);
  if (globalvars)
    free (globalvars);

}

static char *
vrml_locate_footprint (char *name)
{
  int i;

  for (i = 0; i < nfplist; i++)
    {

      if (!strcmp (fplist[i].name, name))
	{

	  return fplist[i].path;
	}
    }

  return NULL;
}

static char *
vrml_locate_map (char *name)
{
  int i;

  for (i = 0; i < nmaplist; i++)
    {

      if (!strcmp (maplist[i].name, name))
	{
	  return maplist[i].path;
	}
    }
  return NULL;
}

static void
vrml_init_map (map3d_struct * map)
{
  memset (map, 0, sizeof (map3d_struct));
}

static void
vrml_load_map (map3d_struct * map, char *filename)
{
  FILE *fm;
  char line[2048];
  char *opt, *val;
  int condition = 1;

  fm = fopen (filename, "r");
  if (fm)
    {
      while (fgets (line, sizeof (line), fm))
	{
	  if (vrml_parse_mapfile (line, NULL, &opt, &val, &condition))
	    {
	      if (!condition)
		continue;
	      if (map->nvars == map->mvars)
		{
		  mapvar_struct *templist;
		  if ((templist =
		       (mapvar_struct *) realloc (map->variables,
						  sizeof (mapvar_struct)
						  * (map->mvars +
						     10))) != NULL)
		    {
		      map->variables = templist;
		      map->mvars += 10;
		    }
		}
	      map->variables[map->nvars].name = opt;
	      map->variables[map->nvars].value = val;
	      map->nvars++;
	    }
	}
      fclose (fm);
    }
}

static int is_popen;

static char *
vrml_process_value (char *varvalue, char *value, int postprocess)
{
  static char temp_value[512];
  int l;
  char *cmd, *invalue;
  FILE *f;

  if (!postprocess)
    return varvalue;
  strcpy (temp_value, "");
  invalue = (value) ? value : "";
  if (varvalue[0] == '@')
    {
      if (!strcmp (varvalue + 1, "value"))
	{
	  return value;
	}
      else
	{
	  l =
	    strlen (pcblibdir) + 1 + strlen (VRMLBASE) + 1 +
	    strlen (VRMLSCRIPTS) + 1 + strlen (varvalue) + 1 +
	    strlen (invalue) + 1;
	  if ((cmd = (char *) malloc (l * sizeof (char))) != NULL)
	    {
	      sprintf (cmd, "%s%s%s%s%s%s%s %s", pcblibdir,
		       PCB_DIR_SEPARATOR_S, VRMLBASE, PCB_DIR_SEPARATOR_S,
		       VRMLSCRIPTS, PCB_DIR_SEPARATOR_S, varvalue + 1,
		       invalue);
	      f = popen (cmd, "r");
	      if (f)
		{
		  if (!fgets (temp_value, sizeof (temp_value), f))
		    strcpy (temp_value, "");
		  pclose (f);
		}
	      else
		{
		  strcpy (temp_value, "");
		}
	      free (cmd);
	      return temp_value;
	    }
	}
    }
  return varvalue;
}


static char *
vrml_find_variable (map3d_struct * map, char *refdes, char *value, char *name,
		    int postprocess)
{
  int i;

  if (refdes)
    {
      for (i = 0; i < nglobalvars; i++)
	{
	  if (!strcmp (globalvars[i].name, name)
	      && !strcmp (globalvars[i].refdes, refdes))
	    {
	      return vrml_process_value (globalvars[i].value, value,
					 postprocess);
	    }
	}
    }
  if (map)
    {
      for (i = 0; i < map->nvars; i++)
	{
	  if (!strcmp (map->variables[i].name, name))
	    {
	      return vrml_process_value (map->variables[i].value, value,
					 postprocess);
	    }
	}
    }
  return NULL;
}


static void
vrml_close_model (FILE * f)
{
  if (f)
    {
      if (is_popen)
	pclose (f);
      else
	fclose (f);

    }
}

static FILE *
vrml_open_model (char *model, char *first_line, int size)
{
  int l;
  FILE *f = NULL;
  char *cmd;

  if (model[0] == '@')
    {
      l =
	strlen (pcblibdir) + 1 + strlen (VRMLBASE) + 1 +
	strlen (VRMLSCRIPTS) + 1 + strlen (model);
      if ((cmd = (char *) malloc (l * sizeof (char))) != NULL)
	{
	  sprintf (cmd, "%s%s%s%s%s%s%s", pcblibdir,
		   PCB_DIR_SEPARATOR_S, VRMLBASE, PCB_DIR_SEPARATOR_S,
		   VRMLSCRIPTS, PCB_DIR_SEPARATOR_S, model + 1);
	  f = popen (cmd, "r");
	}

      is_popen = 1;
    }
  else
    {
      l =
	strlen (pcblibdir) + 1 + strlen (VRMLBASE) + 1 +
	strlen (VRMLMODELS) + 1 + strlen (model) + 1;
      if ((cmd = (char *) malloc (l * sizeof (char))) != NULL)
	{
	  sprintf (cmd, "%s%s%s%s%s%s%s", pcblibdir,
		   PCB_DIR_SEPARATOR_S, VRMLBASE, PCB_DIR_SEPARATOR_S,
		   VRMLMODELS, PCB_DIR_SEPARATOR_S, model);
	  f = fopen (cmd, "r");
	  is_popen = 0;
	}
    }
  if (cmd)
    free (cmd);

  if (f && fgets (first_line, size, f))
    {
      return f;
    }
  else
    {
      if (f)
	vrml_close_model (f);
      return NULL;
    }

  return NULL;

}

static int
vrml_expand_output (char *line, map3d_struct * map, char *refdes, char *value,
		    int cnd, int counter)
{
  char *s_left, *s_block, *s_var, *s_end;
  char *s_right;

  s_left = line;
  while (isblank (*s_left))
    s_left++;

  s_block = strstr (s_left, "#}}");
  if (s_block && s_block == s_left)
    {
      return 1;
    }
  s_block = strstr (s_left, "#{{");
  if (s_block && s_block == s_left)
    {
      char *s_varend;
      s_var = s_block + 3;
      while (isblank (*s_var))
	s_var++;
      s_varend = s_var;
      while (*s_varend && !isspace (*s_varend))
	s_varend++;
      *s_varend = 0;
      if (strlen (s_var))
	{
	  return (vrml_find_variable (map, refdes, value, s_var, 1)) ? 1 : 0;
	}

    }

  if (!cnd)
    return 0;

  s_block = strstr (line, "#@@");
  if (s_block)
    {
      int subst_ok = 0;

      /* Conditional variable substitution */

      s_right = s_block + 3;
      while (isspace (*s_right))
	s_right++;
      s_var = strchr (s_right, '@');
      while (s_var)
	{
	  if (*(s_var + 1) == '@' && *(s_var + 2) == '@')
	    {
	      /* instance ID, skip it */
	      s_var = strchr (s_var + 3, '@');
	    }
	  else
	    {
	      /* Standard variable */
	      subst_ok = 1;
	      s_end = strchr (s_var + 1, '@');
	      if (s_end)
		{
		  *s_end = 0;
		  if (!vrml_find_variable (map, refdes, value, s_var + 1, 1))
		    {
		      /* variable not found, cannot substitute */
		      subst_ok = 0;
		      break;
		    }
		  *s_end = '@';
		  s_var = strchr (s_end + 1, '@');
		}
	      else
		{
		  /* no more veriables */
		  break;
		}
	    }
	}
      if (subst_ok)
	{
	  /* copy right part over left part */
	  while (*s_right)
	    {
	      *s_left++ = *s_right++;
	    }
	  *s_left = 0;
	}
      else
	{
	  /* strip right part */
	  *s_block = 0;
	  while (s_block >= line)
	    {
	      s_block--;
	      if (!isspace (*s_block))
		break;
	      *s_block = 0;
	    }
	}
    }
  /* Standard variable substitution */

  s_right = line;
  s_var = strchr (line, '@');
  while (s_var)
    {
      char *s_value;
      if (*(s_var + 1) == '@' && *(s_var + 2) == '@')
	{
	  *s_var = 0;
	  fprintf (vrml_output, "%s%d", s_right, counter);
	  s_right = s_var + 3;
	}
      else
	{
	  s_end = strchr (s_var + 1, '@');
	  if (s_end)
	    {
	      *s_var = 0;
	      *s_end = 0;
	      s_value = vrml_find_variable (map, refdes, value, s_var + 1, 1);
	      if (s_value)
		{
		  fprintf (vrml_output, "%s%s", s_right, s_value);
		}
	      else
		{
		  fputs (s_right, vrml_output);
		}

	      s_right = s_end + 1;
	    }
	  else
	    {
	      break;
	    }
	}
      s_var = strchr (s_right, '@');
    }

  fputs (s_right, vrml_output);

  return 1;

}

static void
vrml_export_model (int which, FILE * fe, char *line, int size,
		   map3d_struct * map, int x, int y, float angle,
		   int onsolder, char *refdes, char *value, int counter)
{
  int is_tr;
  int cnd = 1;
  char *mr, *mt, *ms;

  fprintf (vrml_output, "Transform {\n");
  fprintf (vrml_output, "scale %d %d %d\n",
	   VRML_COMPONENT_SCALE, VRML_COMPONENT_SCALE, VRML_COMPONENT_SCALE);
  if (angle != 0.)
    fprintf (vrml_output, "rotation 0 0 1 %f\n", angle);
  if (x != 0 || y != 0)
    fprintf (vrml_output, "translation %d %d %d \n", x, -y,
	     ((onsolder) ? -1 : 1) * (BOARD_THICKNESS / 2 +
				      OUTER_COPPER_THICKNESS));
  fprintf (vrml_output, "children [\n");
  if (onsolder)
    {
      fprintf (vrml_output, "Transform {\n");
      fprintf (vrml_output,
	       "rotation 1 0 0 3.1415926535897932384626433832795\n");
      fprintf (vrml_output, "children [\n");
    }

  if (which == VRML_MODEL_OVERLAY)
    {
      mr = vrml_find_variable (NULL, refdes, value, "overlay_rotate", 1);
      if (!mr)
	mr = vrml_find_variable (map, refdes, value, "overlay_rotate", 1);
      ms = vrml_find_variable (NULL, refdes, value, "overlay_scale", 1);
      if (!ms)
	ms = vrml_find_variable (map, refdes, value, "overlay_scale", 1);
      mt = vrml_find_variable (NULL, refdes, value, "overlay_translate", 1);
      if (!mt)
	mt = vrml_find_variable (map, refdes, value, "overlay_translate", 1);
    }
  else
    {
      mr = vrml_find_variable (NULL, refdes, value, "rotate", 1);
      if (!mr)
	mr = vrml_find_variable (map, refdes, value, "rotate", 1);
      ms = vrml_find_variable (NULL, refdes, value, "scale", 1);
      if (!ms)
	ms = vrml_find_variable (map, refdes, value, "scale", 1);
      mt = vrml_find_variable (NULL, refdes, value, "translate", 1);
      if (!mt)
	mt = vrml_find_variable (map, refdes, value, "translate", 1);
    }
  is_tr = (mt || mr || ms);
  if (is_tr)
    {
      fprintf (vrml_output, "Transform {\n");
      if (mr)
	fprintf (vrml_output, "rotation %s\n", mr);
      if (ms)
	fprintf (vrml_output, "scale %s\n", ms);
      if (mt)
	fprintf (vrml_output, "translation %s\n", mt);
      fprintf (vrml_output, "children [\n");
    }
  /* Flush first line of text, already read in buffer */
//      cnd = vrml_expand_output (line, map, refdes, value, cnd, counter);
  cnd = vrml_expand_output (line, map, refdes, value, cnd, counter);
  while (fgets (line, size, fe))
    {
      cnd = vrml_expand_output (line, map, refdes, value, cnd, counter);
    }
  if (is_tr)
    {
      fprintf (vrml_output, "]}\n");
    }
  if (onsolder)
    {
      fprintf (vrml_output, "]}\n");
    }
  fprintf (vrml_output, "]}\n");
  vrml_close_model (fe);
}

static void
vrml_export_element (map3d_struct * map, int x, int y, float angle,
		     int onsolder, char *refdes, char *value, int counter)
{
  FILE *fe = NULL;
  char line[2048];
  char *model;

  model = vrml_find_variable (NULL, refdes, value, "model", 0);
  if (!model)
    model = vrml_find_variable (map, refdes, value, "model", 0);
  if (model)
    {
      fe = vrml_open_model (model, line, sizeof (line));
      if (!fe)
	{
	  model = vrml_find_variable (map, refdes, value, "alt_model", 0);
	  if (model)
	    fe = vrml_open_model (model, line, sizeof (line));	/* otherwise keep the error value */
	}
      if (fe)			/* primary or alternate */
	vrml_export_model (VRML_MODEL_STANDARD, fe, line, sizeof (line), map,
			   x, y, angle, onsolder, refdes, value, counter);
    }

  model = vrml_find_variable (NULL, refdes, value, "overlay", 0);
  if (!model)
    model = vrml_find_variable (map, refdes, value, "overlay", 0);
  if (model)
    {
      fe = vrml_open_model (model, line, sizeof (line));
      if (!fe)
	{
	  model = vrml_find_variable (map, refdes, value, "alt_overlay", 0);
	  if (model)
	    fe = vrml_open_model (model, line, sizeof (line));	/* otherwise keep the error value */
	}
      if (fe)			/* primary or alternate */
	vrml_export_model (VRML_MODEL_OVERLAY, fe, line, sizeof (line), map,
			   x, y, angle, onsolder, refdes, value, counter);
    }
}

static void
vrml_free_map (map3d_struct * map)
{
  int i;

  if (map->variables)
    {
      for (i = 0; i < map->nvars; i++)
	{
	  if (map->variables[i].name)
	    free (map->variables[i].name);
	  if (map->variables[i].value)
	    free (map->variables[i].value);
	}
      free (map->variables);
    }
}

static float
vrml_element_angle (ElementType * el, ElementType * fp)
{
  int pxe = 0, pye = 0, pxf = 0, pyf = 0;
  int i, mx;
  GList *gel;
  GList *gfp;

  mx = min (el->PinN, fp->PinN);

  gel = el->Pin;
  gfp = fp->Pin;
  for (i = 0; i < mx; i++)
    {
      pxe = ((PinType *) gel->data)->X - el->MarkX;
      pye = ((PinType *) gel->data)->Y - el->MarkY;
      pxf = ((PinType *) gfp->data)->X - fp->MarkX;
      pyf = ((PinType *) gfp->data)->Y - fp->MarkY;
      if ((pxe || pye) && (pxf || pyf))
	break;
      gel = g_list_next (gel);
      gfp = g_list_next (gfp);
    }
  if (i == mx)
    {
      mx = min (el->PadN, fp->PadN);
      gel = el->Pad;
      gfp = fp->Pad;
      for (i = 0; i < mx; i++)
	{
	  pxe =
	    (((PadType *) gel->data)->Point1.X +
	     ((PadType *) gel->data)->Point2.X) / 2 - el->MarkX;
	  pye =
	    (((PadType *) gel->data)->Point1.Y +
	     ((PadType *) gel->data)->Point2.Y) / 2 - el->MarkY;
	  pxf =
	    (((PadType *) gfp->data)->Point1.X +
	     ((PadType *) gfp->data)->Point2.X) / 2 - fp->MarkX;
	  pyf =
	    (((PadType *) gfp->data)->Point1.Y +
	     ((PadType *) gfp->data)->Point2.Y) / 2 - fp->MarkY;
	  if ((pxe || pye) && (pxf || pyf))
	    break;
	  gel = g_list_next (gel);
	  gfp = g_list_next (gfp);
	}
    }
  if (i == mx)
    {
      return 0.;
    }
  else
    {
      return atan2 (-(float) pye, (float) pxe) - atan2 (-(float) pyf,
							(float) pxf);
    }

}

void
vrml_process_components (void)
{
  int cx, cy;
  int counter = 0;
  BufferType bf;
  char *with_fp, *fpe, *mpf, *mpe;
  float angle;
  map3d_struct map = { 0, 0, 0 };

  vrml_init_footprints ();

  bf.Data = CreateNewBuffer ();

  ELEMENT_LOOP (PCB->Data);
  {
// element
    cx = element->MarkX;
    cy = element->MarkY;
    if (element->PinN == 0 && element->PadN == 0)
      {
	continue;
      }

    with_fp = NULL;
    fpe = vrml_locate_footprint (element->Name[0].TextString);
    if (!fpe)
      {
	with_fp = Concat (element->Name[0].TextString, ".fp", NULL);
	fpe = vrml_locate_footprint (with_fp);
      }
    if (fpe)
      {
	if (LoadElementToBuffer (&bf, fpe, 1) && bf.Data->ElementN)
	  {
	    if ((TEST_FLAG (ONSOLDERFLAG, (element))
		 && !Settings.ShowSolderSide)
		|| (!TEST_FLAG (ONSOLDERFLAG, (element))
		    && Settings.ShowSolderSide))
	      {
		MirrorElementCoordinates (bf.Data, element, 0);
		angle =
		  -vrml_element_angle (element,
				       (ElementType *) (bf.Data->
							Element->data));
		MirrorElementCoordinates (bf.Data, element, 0);
	      }
	    else
	      {
		angle =
		  vrml_element_angle (element,
				      (ElementType *) (bf.Data->
						       Element->data));
	      }
	    mpf = Concat (element->Name[0].TextString, VRML_MAP_EXT, NULL);
	    mpe = vrml_locate_map (mpf);
	    if (!mpe)
	      {
		free (mpf);
		mpf =
		  Concat (element->Name[0].TextString, VRML_FPMAP_EXT, NULL);
		mpe = vrml_locate_map (mpf);
	      }
	    vrml_init_map (&map);
	    if (mpe)
	      vrml_load_map (&map, mpe);

	    vrml_export_element (&map, cx, cy, angle,
				 TEST_FLAG (ONSOLDERFLAG, (element)) ? 1 : 0,
				 element->Name[1].TextString,
				 element->Name[2].TextString, counter++);

	    vrml_free_map (&map);
	    free (mpf);
	    ClearBuffer (&bf);
	  }
	else
	  {
	    ClearBuffer (&bf);
	  }
      }
  }
  if (with_fp)
    free (with_fp);
  END_LOOP;

  vrml_release_footprints ();
  ClearBuffer (&bf);

}
