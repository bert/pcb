/*!
 * \file src/hid/scadcomp.c
 *
 * \brief OpenSCAD export HID - Footprint and map management.
 *
 * \author Milan Prochac <milan@prochac.sk>
 *
 * <hr>
 *
 * PCB, interactive printed circuit board design
 *
 * This code is based on the GERBER and VRML export HID
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
 */

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

#include "scad.h"

static fplist_struct *fplist;
static int nfplist, mfplist;
static fplist_struct *maplist;
static int nmaplist, mmaplist;
static fplist_struct *templist;
static projectvar_struct *projectvars;
static int nprojectvars, mprojectvars;

/* model types for scad_export_model */
#define SCAD_MODEL_STANDARD	0 /*!< primary model */
#define SCAD_MODEL_OVERLAY	1

/*!
 * \briefLoads database of footprints and the corresponding map files.
 */
static int
scad_load_footprints (char *libpath, char *toppath)
{
  char olddir[MAXPATHLEN + 1]; /*!< The directory we start out in (cwd). */
  char subdir[MAXPATHLEN + 1]; /*!< The directory holding footprints to load. */
  DIR *subdirobj; /*!< Interable object holding all subdir entries. */
  struct dirent *subdirentry; /*!< Individual subdir entry. */
  struct stat buffer; /*!< Buffer used in stat. */
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
          && NSTRCMP (subdirentry->d_name, "Makefile.in") != 0 && (l < 4 || NSTRCMP (subdirentry->d_name + (l - 4), ".png") != 0) && (l < 5 || NSTRCMP (subdirentry->d_name + (l - 5), ".html") != 0))
        {
          if (l > 4 && NSTRCMP (subdirentry->d_name + (l - 4), SCAD_MAP_EXT) == 0)
            {
              if (nmaplist == mmaplist)
                {
                  if ((templist = (fplist_struct *) realloc (maplist, sizeof (fplist_struct) * (mmaplist + 100))) == NULL)
                    return 0;
                  maplist = templist;
                  mmaplist += 100;
                }
              len = strlen (subdir) + strlen (PCB_DIR_SEPARATOR_S) + strlen (subdirentry->d_name) + 1;
              maplist[nmaplist].path = (char *) calloc (1, len);
              strcpy (maplist[nmaplist].path, subdir);
              strcat (maplist[nmaplist].path, PCB_DIR_SEPARATOR_S);
              maplist[nmaplist].name = maplist[nmaplist].path + strlen (maplist[nmaplist].path);
              strcat (maplist[nmaplist].path, subdirentry->d_name);
              nmaplist++;
            }
          else
            {
              if (nfplist == mfplist)
                {
                  if ((templist = (fplist_struct *) realloc (fplist, sizeof (fplist_struct) * (mfplist + 100))) == NULL)
                    return 0;
                  fplist = templist;
                  mfplist += 100;
                }
              len = strlen (subdir) + strlen (PCB_DIR_SEPARATOR_S) + strlen (subdirentry->d_name) + 1;
              fplist[nfplist].path = (char *) calloc (1, len);
              strcpy (fplist[nfplist].path, subdir);
              strcat (fplist[nfplist].path, PCB_DIR_SEPARATOR_S);
              fplist[nfplist].name = fplist[nfplist].path + strlen (fplist[nfplist].path);
              strcat (fplist[nfplist].path, subdirentry->d_name);
              nfplist++;
            }
        }
    }

  closedir (subdirobj);

  chdir (olddir);

  return 1;
}

/*!
 * \brief Parses single line of map file (propject map or component map).
 */
static int
scad_parse_mapfile_line (char *line, char **refdes_result, char **var_result, char **value_result, int *condition)
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
      else if (strcmp (rr, "[scad]") == 0)
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


/*!
 * \brief Parses option in PCB  preferences file.
 */
static int
scad_parse_option (char *line, char **option_result, char **arg_result)
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

  s = option; /* Strip trailing ':' or '=' */
  while (*s && *s != ':' && *s != '=')
    ++s;
  *s = '\0';

  s = arg; /* Strip leading ':', '=', and whitespace */
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


/*!
 * \brief Prepares list of paths, where component library is located.
 */
static char *
scad_libpaths ()
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
          if (scad_parse_option (line, &opt, &gtkpaths) && !strcmp (opt, "library-newlib"))
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
      libpaths = (char *) calloc (1, strlen (Settings.LibraryTree) + strlen (gtkpaths) + strlen (PCB_PATH_DELIMETER) + 1);
      if (!libpaths)
        {
          free (gtkpaths);
          return NULL;
        }
      strcpy (libpaths, Settings.LibraryTree);
      for (p = strtok (gtkpaths, PCB_PATH_DELIMETER); p && *p; p = strtok (NULL, PCB_PATH_DELIMETER))
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

/*!
 * \brief Loads project map file.
 */
static void
scad_load_projectmap (char *filename)
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
          if (scad_parse_mapfile_line (line, &ref, &opt, &val, &condition))
            {
              if (!condition)
                continue;
              if (nprojectvars == mprojectvars)
                {
                  projectvar_struct *templist;
                  if ((templist = (projectvar_struct *) realloc (projectvars, sizeof (projectvar_struct) * (mprojectvars + 30))) != NULL)
                    {
                      projectvars = templist;
                      mprojectvars += 30;
                    }
                }
              projectvars[nprojectvars].refdes = ref;
              projectvars[nprojectvars].name = opt;
              projectvars[nprojectvars].value = val;
              nprojectvars++;
            }
        }
      fclose (fm);
    }
}

static void
scad_init_footprints (void)
{
  char toppath[MAXPATHLEN + 1]; /* String holding abs path to top level library dir */
  char working[MAXPATHLEN + 1]; /* String holding abs path to working dir */
  char *libpaths; /* String holding list of library paths to search */
  char *p; /* Helper string used in iteration */
  DIR *dirobj; /* Iterable directory object */
  struct dirent *direntry = NULL; /* Object holding individual directory entries */
  struct stat buffer; /* buffer used in stat */


  fplist = NULL;
  nfplist = mfplist = 0;
  maplist = NULL;
  nmaplist = mmaplist = 0;
  projectvars = 0;
  nprojectvars = mprojectvars = 0;


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

      strcat (toppath, SCAD_MAP_EXT);
      scad_load_projectmap (toppath);
    }

  libpaths = scad_libpaths ();

  if (!libpaths)
    return;

  for (p = strtok (libpaths, PCB_PATH_DELIMETER); p && *p; p = strtok (NULL, PCB_PATH_DELIMETER))
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

      scad_load_footprints ("(local)", toppath);


      if ((dirobj = opendir (toppath)) == NULL)
        continue;

      while ((direntry = readdir (dirobj)) != NULL)
        {
          if (!stat (direntry->d_name, &buffer) && S_ISDIR (buffer.st_mode) && direntry->d_name[0] != '.' && NSTRCMP (direntry->d_name, "CVS") != 0)
            {
              scad_load_footprints (direntry->d_name, toppath);
            }
        }
      closedir (dirobj);
    }

  chdir (working);
  if (libpaths)
    free (libpaths);

}

static void
scad_release_footprints (void)
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
  for (i = 0; i < nprojectvars; i++)
    {
      if (projectvars[i].refdes)
        free (projectvars[i].refdes);
      if (projectvars[i].name)
        free (projectvars[i].name);
      if (projectvars[i].value)
        free (projectvars[i].value);
    }

  if (fplist)
    free (fplist);
  if (maplist)
    free (maplist);
  if (projectvars)
    free (projectvars);

}

static char *
scad_locate_footprint (char *name)
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
scad_locate_map (char *name)
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
scad_init_map (map3d_struct * map)
{
  memset (map, 0, sizeof (map3d_struct));
}

static void
scad_load_map (map3d_struct * map, char *filename)
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
          if (scad_parse_mapfile_line (line, NULL, &opt, &val, &condition))
            {
              if (!condition)
                continue;
              if (map->nvars == map->mvars)
                {
                  mapvar_struct *templist;
                  if ((templist = (mapvar_struct *) realloc (map->variables, sizeof (mapvar_struct) * (map->mvars + 10))) != NULL)
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

/*
 * Export of the components
 */

static int is_popen;

static char *
scad_process_value (char *varvalue, char *value, int postprocess)
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
          l = strlen (pcblibdir) + 1 + strlen (SCADBASE) + 1 + strlen (SCADSCRIPTS) + 1 + strlen (varvalue) + 1 + strlen (invalue) + 1;
          if ((cmd = (char *) malloc (l * sizeof (char))) != NULL)
            {
              sprintf (cmd, "%s%s%s%s%s%s%s %s", pcblibdir, PCB_DIR_SEPARATOR_S, SCADBASE, PCB_DIR_SEPARATOR_S, SCADSCRIPTS, PCB_DIR_SEPARATOR_S, varvalue + 1, invalue);
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
scad_find_variable (map3d_struct * map, char *refdes, char *value, char *name, int postprocess)
{
  int i;

  if (refdes)
    {
      for (i = 0; i < nprojectvars; i++)
        {
          if (!strcmp (projectvars[i].name, name) && !strcmp (projectvars[i].refdes, refdes))
            {
              return scad_process_value (projectvars[i].value, value, postprocess);
            }
        }
    }
  if (map)
    {
      for (i = 0; i < map->nvars; i++)
        {
          if (!strcmp (map->variables[i].name, name))
            {
              return scad_process_value (map->variables[i].value, value, postprocess);
            }
        }
    }
  return NULL;
}


static void
scad_close_model (FILE * f)
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
scad_open_model (char *model, char *first_line, int size)
{
  int l;
  FILE *f = NULL;
  char *cmd;

  if (model[0] == '@')
    {
      l = strlen (pcblibdir) + 1 + strlen (SCADBASE) + 1 + strlen (SCADSCRIPTS) + 1 + strlen (model);
      if ((cmd = (char *) malloc (l * sizeof (char))) != NULL)
        {
          sprintf (cmd, "%s%s%s%s%s%s%s", pcblibdir, PCB_DIR_SEPARATOR_S, SCADBASE, PCB_DIR_SEPARATOR_S, SCADSCRIPTS, PCB_DIR_SEPARATOR_S, model + 1);
          f = popen (cmd, "r");
        }

      is_popen = 1;
    }
  else
    {
      l = strlen (pcblibdir) + 1 + strlen (SCADBASE) + 1 + strlen (SCADMODELS) + 1 + strlen (model) + 1;
      if ((cmd = (char *) malloc (l * sizeof (char))) != NULL)
        {
          sprintf (cmd, "%s%s%s%s%s%s%s", pcblibdir, PCB_DIR_SEPARATOR_S, SCADBASE, PCB_DIR_SEPARATOR_S, SCADMODELS, PCB_DIR_SEPARATOR_S, model);
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
        scad_close_model (f);
      return NULL;
    }

  return NULL;

}

/*!
 * \brief Expands all variables in the model description.
 *
 * - it processes single line.
 * - keeps status of conditional blocks.
 * - it's kind of black magic.
 */
static int
scad_expand_output (char *line, map3d_struct * map, char *refdes, char *value, int cnd, int counter)
{
  char *s_left, *s_block, *s_var, *s_end;
  char *s_right;

  s_left = line;
  while (isblank (*s_left))
    s_left++;

  s_block = strstr (s_left, "//}}");
  if (s_block && s_block == s_left)
    {
      return 1;
    }
  s_block = strstr (s_left, "//{{");
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
          return (scad_find_variable (map, refdes, value, s_var, 1)) ? 1 : 0;
        }

    }

  if (!cnd)
    return 0;

  s_block = strstr (line, "//@@");
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
                  if (!scad_find_variable (map, refdes, value, s_var + 1, 1))
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
          fprintf (scad_output, "%s%d", s_right, counter);
          s_right = s_var + 3;
        }
      else
        {
          s_end = strchr (s_var + 1, '@');
          if (s_end)
            {
              *s_var = 0;
              *s_end = 0;
              s_value = scad_find_variable (map, refdes, value, s_var + 1, 1);
              if (s_value)
                {
                  fprintf (scad_output, "%s%s", s_right, s_value);
                }
              else
                {
                  fputs (s_right, scad_output);
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

  fputs (s_right, scad_output);

  return 1;

}

/*!
 * \brief Export of the single model.
 *
 * - adjusts the model position, rotation, scale.
 * - processes the model line-by-line and perfoprms variable expansion.
 */
static void
scad_export_model (int model_type, FILE * f, char *line, int size, map3d_struct * map, int x, int y, float angle, int onsolder, char *refdes, char *value, int counter)
{
  int condition = 1; /* Variable, holding the status of conditional output */
  char *model_rotation, *model_translate, *model_scale;


  model_rotation = scad_find_variable (NULL, refdes, value, (model_type == SCAD_MODEL_OVERLAY) ? "overlay_rotate" : "rotate", 1);
  if (!model_rotation)
    model_rotation = scad_find_variable (map, refdes, value, (model_type == SCAD_MODEL_OVERLAY) ? "overlay_rotate" : "rotate", 1);
  model_scale = scad_find_variable (NULL, refdes, value, (model_type == SCAD_MODEL_OVERLAY) ? "overlay_scale" : "scale", 1);
  if (!model_scale)
    model_scale = scad_find_variable (map, refdes, value, (model_type == SCAD_MODEL_OVERLAY) ? "overlay_scale" : "scale", 1);
  model_translate = scad_find_variable (NULL, refdes, value, (model_type == SCAD_MODEL_OVERLAY) ? "overlay_translate" : "translate", 1);
  if (!model_translate)
    model_translate = scad_find_variable (map, refdes, value, (model_type == SCAD_MODEL_OVERLAY) ? "overlay_translate" : "translate", 1);

  /*  transformed = model_translate != NULL || model_rotation != NULL || model_scale != NULL || x != 0 || y != 0 || angle != 0.; */


  if (model_translate)
    fprintf (scad_output, "translate (%s) ", model_translate);
  fprintf (scad_output, "translate ([%f, %f, %f]) ", scad_scale_coord ((float) x), -scad_scale_coord ((float) y), ((onsolder) ? -1. : 1.) * (BOARD_THICKNESS / 2. + OUTER_COPPER_THICKNESS));

  /* rotate order: angle onsolder map */
  if (angle != 0.)
    fprintf (scad_output, "rotate ([0, 0, %f]) ", angle);
  if (onsolder)
    fprintf (scad_output, "rotate([180.,0,0]) ");
  if (model_rotation)
    fprintf (scad_output, "rotate (%s) ", model_rotation);

  if (model_scale)
    fprintf (scad_output, "scale (%s) ", model_scale);

  fprintf (scad_output, "{\n");

  /* Flush first line of text, already read in buffer */

  condition = scad_expand_output (line, map, refdes, value, condition, counter);
  while (fgets (line, size, f))
    {
      condition = scad_expand_output (line, map, refdes, value, condition, counter);
    }

  fprintf (scad_output, "}\n");
}

/*!
 * \brief Export of the single element.
 *
 * - identifies the model for the component - primary and overlay.
 * - exports both models.
 */
static void
scad_export_element (map3d_struct * map, int x, int y, float angle, int onsolder, char *desc, char *refdes, char *value, int counter)
{
  FILE *f = NULL;
  char line[2048];
  char *model_name;
  char *default_model_name;

  /* get model name from project map file */
  model_name = scad_find_variable (NULL, refdes, value, "model", 0);
  /* if not defined, get model name from comp[onent map file (if exists) */
  if (!model_name && map)
    model_name = scad_find_variable (map, refdes, value, "model", 0);
  if (model_name)
    {
      /* if model is defined, try to open it */
      f = scad_open_model (model_name, line, sizeof (line));
      if (!f)
        {
          /* if failed, try alternate model */
          model_name = scad_find_variable (map, refdes, value, "alt_model", 0);
          if (model_name)
            f = scad_open_model (model_name, line, sizeof (line));
        }
      /* if either of models exists, process it */
      if (f)
        {
          scad_export_model (SCAD_MODEL_STANDARD, f, line, sizeof (line), map, x, y, angle, onsolder, refdes, value, counter);
          scad_close_model (f);
        }
    }
  else
    {
      /* no model variable found, try default model */
      default_model_name = Concat (desc, SCAD_EXT, NULL);
      if (default_model_name)
        {
          f = scad_open_model (default_model_name, line, sizeof (line));
          free (default_model_name);
          if (f)
            {
              scad_export_model (SCAD_MODEL_STANDARD, f, line, sizeof (line), map, x, y, angle, onsolder, refdes, value, counter);
              scad_close_model (f);
            }
        }
      goto done;
    }

  model_name = scad_find_variable (NULL, refdes, value, "overlay", 0);
  if (!model_name && map)
    model_name = scad_find_variable (map, refdes, value, "overlay", 0);
  if (model_name)
    {
      f = scad_open_model (model_name, line, sizeof (line));
      if (!f)
        {
          model_name = scad_find_variable (map, refdes, value, "alt_overlay", 0);
          if (model_name)
            f = scad_open_model (model_name, line, sizeof (line));
        }
      if (f)
        {
          scad_export_model (SCAD_MODEL_OVERLAY, f, line, sizeof (line), map, x, y, angle, onsolder, refdes, value, counter);
          scad_close_model (f);
        }
    }

done:
  return;
}

static void
scad_free_map (map3d_struct * map)
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

/*!
 * \brief Component angle calculation.
 *
 * - calculates angle between first pin or pad and center of component
 *   for footprint and element placed on the board.
 * - returns difference of these angles as rotation of the component.
 *
 * \warning For components with single pin or single square pad we are
 * not able to detect the angle.
 */
static float
scad_element_angle (ElementType * element, ElementType * footprint)
{
  Coord element_dx = 0, element_dy = 0, footprint_dx = 0, footprint_dy = 0;
  int i, pin_count;
  GList *element_pin;
  GList *footprint_pin;

  pin_count = min (element->PinN, footprint->PinN);

  element_pin = element->Pin;
  footprint_pin = footprint->Pin;
  for (i = 0; i < pin_count; i++)
    {
      element_dx = ((PinType *) element_pin->data)->X - element->MarkX;
      element_dy = ((PinType *) element_pin->data)->Y - element->MarkY;
      footprint_dx = ((PinType *) footprint_pin->data)->X - footprint->MarkX;
      footprint_dy = ((PinType *) footprint_pin->data)->Y - footprint->MarkY;
      if ((element_dx != 0 || element_dy != 0) && (footprint_dx != 0 || footprint_dy != 0))
        break;
      element_pin = g_list_next (element_pin);
      footprint_pin = g_list_next (footprint_pin);
    }
  if (i == pin_count)
    {
      pin_count = min (element->PadN, footprint->PadN);
      element_pin = element->Pad;
      footprint_pin = footprint->Pad;
      for (i = 0; i < pin_count; i++)
        {
          element_dx = (((PadType *) element_pin->data)->Point1.X + ((PadType *) element_pin->data)->Point2.X) / 2 - element->MarkX;
          element_dy = (((PadType *) element_pin->data)->Point1.Y + ((PadType *) element_pin->data)->Point2.Y) / 2 - element->MarkY;
          footprint_dx = (((PadType *) footprint_pin->data)->Point1.X + ((PadType *) footprint_pin->data)->Point2.X) / 2 - footprint->MarkX;
          footprint_dy = (((PadType *) footprint_pin->data)->Point1.Y + ((PadType *) footprint_pin->data)->Point2.Y) / 2 - footprint->MarkY;
          if ((element_dx != 0 || element_dy != 0) && (footprint_dx != 0 || footprint_dy != 0))
            break;
          element_pin = g_list_next (element_pin);
          footprint_pin = g_list_next (footprint_pin);
        }
    }

  if (i == pin_count)
    {
      /* No appropriate pin found */
      return 0.;
    }
  else
    {
      /* Calculate the angle, in degrees */
      return (atan2 (-(float) element_dy, (float) element_dx) - atan2 (-(float) footprint_dy, (float) footprint_dx)) / M_PI * 180.;
    }

}

/*!
 * \brief Alternative component angle calculation.
 *
 * - angle is identified by location of pin/pad No. 1.
 */
static float
xyToAngle (Coord x, Coord y)
{
  float theta;

  if ((x > 0) && (y >= 0))
    theta = 180.0;
  else if ((x <= 0) && (y > 0))
    theta = 90.0;
  else if ((x < 0) && (y <= 0))
    theta = 0.0;
  else if ((x >= 0) && (y < 0))
    theta = 270.0;
  else
    {
      theta = 0.0;
      Message ("xyToAngle(): unable to figure out angle of element\n"
               "     because the pin is at the centroid of the part.\n"
               "     This is a BUG!!!\n"
               "     Setting to %g degrees\n", theta);
    }

  return theta;
}

static float
scad_xy_element_angle (ElementType * element)
{
  float theta = 0.0,  pin1angle = 0;
  int64_t sumx=0, sumy=0;
  Coord pin1x = 0, pin1y = 0;
  Coord pin2x = 0, pin2y = 0;
  Coord x, y;
  int found_pin1;
  int found_pin2;
  int pin_cnt=0;

    PIN_LOOP (element);
    {
      sumx += pin->X;
      sumy += pin->Y;
      pin_cnt++;

      if (NSTRCMP (pin->Number, "1") == 0)
        {
          pin1x = pin->X;
          pin1y = pin->Y;
          pin1angle = 0.; /* pins have no notion of angle */
          found_pin1 = 1;
        }
      else if (NSTRCMP (pin->Number, "2") == 0)
        {
          pin2x = pin->X;
          pin2y = pin->Y;
          found_pin2 = 1;
        }
    }
    END_LOOP;

    PAD_LOOP (element);
    {
      sumx += (pad->Point1.X + pad->Point2.X) / 2;
      sumy += (pad->Point1.Y + pad->Point2.Y) / 2;
      pin_cnt++;

      if (NSTRCMP (pad->Number, "1") == 0)
        {
          pin1x = (pad->Point1.X + pad->Point2.X) / 2;
          pin1y = (pad->Point1.Y + pad->Point2.Y) / 2;
          /*
           * NOTE:  We swap the Y points because in PCB, the Y-axis
           * is inverted.  Increasing Y moves down.  We want to deal
           * in the usual increasing Y moves up coordinates though.
           */
          pin1angle = atan2 (pad->Point1.Y - pad->Point2.Y,
                             pad->Point2.X - pad->Point1.X);
          found_pin1 = 1;
        }
      else if (NSTRCMP (pad->Number, "2") == 0)
        {
          pin2x = (pad->Point1.X + pad->Point2.X) / 2;
          pin2y = (pad->Point1.Y + pad->Point2.Y) / 2;
          found_pin2 = 1;
        }

    }
    END_LOOP;

    if (pin_cnt > 0)
      {
        x = sumx /  pin_cnt;
        y = sumy /  pin_cnt;

        if (found_pin1)
          {
            /* recenter pin #1 onto the axis which cross at the part
             * centroid. */
            pin1x -= x;
            pin1y -= y;
            pin1y = -pin1y;

            /* if only 1 pin, use pin 1's angle */
            if (pin_cnt == 1)
              theta = pin1angle;
            else
              {
                /* if pin #1 is at (0,0) use pin #2 for rotation */
                if ((pin1x == 0) && (pin1y == 0))
                  {
                    if (found_pin2)
                      theta = xyToAngle (pin2x, pin2y);
                    else
                      {
                        Message
                          ("ExportSCAD: unable to figure out angle of element\n"
                           "     %s because pin #1 is at the centroid of the part.\n"
                           "     and I could not find pin #2's location\n"
                           "     Setting to %g degrees\n",
                           UNKNOWN (NAMEONPCB_NAME (element)), theta);
                      }
                  }
                else
                  theta = xyToAngle (pin1x, pin1y);
              }
          }
        /* we did not find pin #1 */
        else
          {
            theta = 0.0;
            Message
              ("ExportSCAD: unable to figure out angle because I could\n"
               "     not find pin #1 of element %s\n"
               "     Setting to %g degrees\n",
               UNKNOWN (NAMEONPCB_NAME (element)), theta);
          }
      }

  return theta;
}

/*!
 * \brief Main function for components export.
 *
 * - initialize footprint and element database.
 * - loops through all components on the board and for each component:
 *   -- loads the footprint into temporary buffer and calculates angle.
 *   -- export the element.
 */
void
scad_process_components (void)
{
  int counter = 0;
  BufferType element_buffer;
  char *name_with_fpext, *fp_path, *map_filename, *map_path;
  float angle;
  map3d_struct map = { 0, 0, 0 };

  scad_init_footprints ();

  element_buffer.Data = CreateNewBuffer ();

  fprintf (scad_output, "module all_components() {\n");


  ELEMENT_LOOP (PCB->Data);
  {

    /* default angle, when all  angle calculations fail */
    angle = 0;

    /* no description - element cannot be identified */
    if (EMPTY_STRING_P (DESCRIPTION_NAME (element)))
      {
        continue;
      }


    if (element->PinN != 0 || element->PadN != 0)
      {
        /* element has at least one pin, we can try ti find angle */
        /* we need to find footprint */
        name_with_fpext = NULL;
        fp_path = scad_locate_footprint (DESCRIPTION_NAME (element));
        if (!fp_path)
          {
            name_with_fpext = Concat (DESCRIPTION_NAME (element), ".fp", NULL);
            if (name_with_fpext)
              {
                fp_path = scad_locate_footprint (name_with_fpext);
                free (name_with_fpext);
              }
          }
        if (fp_path)
          {
            /* footprint found, we can load it and calculate angle between first pins/pads */
            if (LoadElementToBuffer (&element_buffer, fp_path, 1) && element_buffer.Data->ElementN)
              {
                if ((TEST_FLAG (ONSOLDERFLAG, (element)) && !Settings.ShowBottomSide) || (!TEST_FLAG (ONSOLDERFLAG, (element)) && Settings.ShowBottomSide))
                  {
                    MirrorElementCoordinates (element_buffer.Data, element, 0);
                    angle = -scad_element_angle (element, (ElementType *) (element_buffer.Data->Element->data));
                    MirrorElementCoordinates (element_buffer.Data, element, 0);
                  }
                else
                  {
                    angle = scad_element_angle (element, (ElementType *) (element_buffer.Data->Element->data));
                  }
                ClearBuffer (&element_buffer);
              }
            else
              {
                /* footprint cannot be loaded, use alternate method */
                angle = scad_xy_element_angle (element);
              }
          }
        else
          {
            /* footprint not found, use alternate method */
            angle = scad_xy_element_angle (element);
          }
      }

    map_filename = Concat (DESCRIPTION_NAME (element), SCAD_MAP_EXT, NULL);
    map_path = scad_locate_map (map_filename);
    if (!map_path)
      {
        free (map_filename);
        map_filename = Concat (DESCRIPTION_NAME (element), SCAD_FPMAP_EXT, NULL);
        map_path = scad_locate_map (map_filename);
      }
    scad_init_map (&map);
    if (map_path)
      scad_load_map (&map, map_path);

    scad_export_element (&map, element->MarkX, element->MarkY, angle, TEST_FLAG (ONSOLDERFLAG, (element)) ? 1 : 0, DESCRIPTION_NAME (element), NAMEONPCB_NAME (element), VALUE_NAME (element), counter++);

    scad_free_map (&map);
    if (map_filename)
      free (map_filename);
    ClearBuffer (&element_buffer);
  }

  END_LOOP;

  fprintf (scad_output, "}\n\n");

  scad_release_footprints ();
  ClearBuffer (&element_buffer);

}
