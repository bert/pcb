#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "global.h"
#include "data.h"
#include "error.h"

#include "hid.h"
#include "../hidint.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static HID_Action **all_actions = 0;
static int all_actions_sorted = 0;
static int n_actions = 0;

HID_Action *current_action = NULL;

static const char *
check_action_name (const char *s)
{
  while (*s)
    if (isspace ((int) *s++) || *s == '(')
      return (s-1);
  return NULL;
}

void
hid_register_actions (HID_Action * a, int n)
{
  int i, count = 0;
  
  all_actions = (HID_Action **)realloc (all_actions,
                         (n_actions + n) * sizeof (HID_Action*));
  for (i = 0; i < n; i++)
    {
      if (check_action_name (a[i].name))
        {
          Message (_("ERROR! Invalid action name, "
                     "action \"%s\" not registered.\n"), a[i].name);
          continue;
        }
      all_actions[n_actions + count++] = a + i;
    }
  n_actions += count;
  all_actions_sorted = 0;
}

void
hid_register_action (HID_Action * a)
{
  hid_register_actions (a, 1);
}

static int
action_sort_compar (const void *va, const void *vb)
{
  HID_Action *a = *(HID_Action **) va;
  HID_Action *b = *(HID_Action **) vb;
  return strcmp (a->name, b->name);
}

static void
sort_actions ()
{
  qsort (all_actions, n_actions, sizeof (HID_Action*), action_sort_compar);
  all_actions_sorted = 1;
}

static int
action_search_compar (const void *va, const void *vb)
{
  char *name = (char*)va;
  HID_Action *action = *(HID_Action**)vb;
  return strcmp (name, action->name);
}

HID_Action *
hid_find_action (const char *name)
{
  HID_Action **action;
  int i;
  
  if (name == NULL)
    return 0;

  if (!all_actions_sorted)
    sort_actions ();

  action = (HID_Action **)bsearch (name, all_actions, n_actions, sizeof (HID_Action*),
                    action_search_compar);
  if (action)
    return *action;

  for (i = 0; i < n_actions; i++)
    if (strcasecmp (all_actions[i]->name, name) == 0)
      return all_actions[i];

  printf ("unknown action `%s'\n", name);
  return 0;
}

void
print_actions ()
{
  int i;

  if (!all_actions_sorted)
    sort_actions ();
  
  fprintf (stderr, "Registered Actions:\n");
  for (i = 0; i < n_actions; i++)
    {
      if (all_actions[i]->description)
	fprintf (stderr, "  %s - %s\n", all_actions[i]->name,
		 all_actions[i]->description);
      else
	fprintf (stderr, "  %s\n", all_actions[i]->name);
      if (all_actions[i]->syntax)
	{
	  const char *bb, *eb;
	  bb = eb = all_actions[i]->syntax;
	  while (1)
	    {
	      for (eb = bb; *eb && *eb != '\n'; eb++)
		;
	      fwrite ("    ", 4, 1, stderr);
	      fwrite (bb, eb - bb, 1, stderr);
	      fputc ('\n', stderr);
	      if (*eb == 0)
		break;
	      bb = eb + 1;
	    }
	}
    }
}

static void
dump_string (char prefix, const char *str)
{
  int eol = 1;
  while (*str)
    {
      if (eol)
	{
	  putchar (prefix);
	  eol = 0;
	}
      putchar (*str);
      if (*str == '\n')
	eol = 1;
      str ++;
    }
  if (!eol)
    putchar ('\n');
}

void
dump_actions ()
{
  int i;

  if (!all_actions_sorted)
    sort_actions ();

  for (i = 0; i < n_actions; i++)
    {
      const char *desc = all_actions[i]->description;
      const char *synt = all_actions[i]->syntax;

      desc = desc ? desc : "";
      synt = synt ? synt : "";

      printf ("A%s\n", all_actions[i]->name);
      dump_string ('D', desc);
      dump_string ('S', synt);
    }
}

int
hid_action (const char *name)
{
  return hid_actionv (name, 0, 0);
}

int
hid_actionl (const char *name, ...)
{
  char *argv[20];
  int argc = 0;
  va_list ap;
  char *arg;

  va_start (ap, name);
  while ((arg = va_arg (ap, char *)) != 0)
      argv[argc++] = arg;
  va_end (ap);
  return hid_actionv (name, argc, argv);
}

int
hid_actionv (const char *name, int argc, char **argv)
{
  Coord x = 0, y = 0;
  int i, ret;
  HID_Action *a, *old_action;

  if (!name)
    return 1;

  a = hid_find_action (name);
  if (!a)
    {
      int i;
      Message ("no action %s(", name);
      for (i = 0; i < argc; i++)
        Message ("%s%s", i ? ", " : "", argv[i]);
      Message (")\n");
      return 1;
    }

  if (a->need_coord_msg)
    gui->get_coords (_(a->need_coord_msg), &x, &y);

  if (Settings.verbose)
    {
      printf ("Action: \033[34m%s(", name);
      for (i = 0; i < argc; i++)
	printf ("%s%s", i ? "," : "", argv[i]);
      printf (")\033[0m\n");
    }
  
  old_action     = current_action;
  current_action = a;
  ret = current_action->trigger_cb (argc, argv, x, y);
  current_action = old_action;
  
  return ret;
}

static int
hid_parse_actionstring (const char *rstr, char require_parens)
{
  char **list = NULL;
  int max = 0;
  int num;
  char *str = NULL;
  const char *sp;
  char *cp, *aname, *cp2;
  int maybe_empty = 0;
  char in_quotes = 0;
  char parens = 0;
  int retcode = 0;

  /*fprintf(stderr, "invoke: `%s'\n", rstr);*/

  sp = rstr;
  str = (char *)malloc(strlen(rstr)+1);

another:
  num = 0;
  cp = str;

  /* eat leading spaces and tabs */
  while (*sp && isspace ((int) *sp))
    sp++;
  
  if (!*sp)
    {
      retcode = 0;
      goto cleanup;
    }
  
  aname = cp;

  /* copy the action name, assumes name does not have a space or '('
   * in its name */
  while (*sp && !isspace ((int) *sp) && *sp != '(')
    *cp++ = *sp++;
  *cp++ = 0;

  /* skip whitespace */
  while (*sp && isspace ((int) *sp))
    sp++;

  /*
   * we only have an action name, so invoke the action
   * with no parameters or event.
   */
  if (!*sp)
    {
      retcode = hid_actionv (aname, 0, 0);
      goto cleanup;
    }

  /* are we using parenthesis? */
  if (*sp == '(')
    {
      parens = 1;
      sp++;
    }
  else if (require_parens)
    {
      Message (_("Syntax error: %s\n"), rstr);
      Message (_("    expected: Action(arg1, arg2)"));
      retcode = 1;
      goto cleanup;
    }
  
  /* get the parameters to pass to the action */
  while (1)
    {
      /* 
       * maybe_empty == 0 means that the last char examined was not a
       * ","
       */
      if (!maybe_empty && ((parens && *sp == ')') || (!parens && !*sp)))
	{
          retcode = hid_actionv (aname, num, list);
          if (retcode)
            goto cleanup;

          /* strip any white space or ';' following the action */
          if (parens)
            sp++;
          while (*sp && (isspace ((int) *sp) || *sp == ';'))
            sp++;
	  goto another;
	}
      else if (*sp == 0 && !maybe_empty)
	break;
      else
        {
	  maybe_empty = 0;
	  in_quotes = 0;
	  /* 
	   * if we have more parameters than memory in our array of
	   * pointers, then either allocate some or grow the array
	   */
	  if (num >= max)
	    {
	      max += 10;
	      if (list)
		list = (char **) realloc (list, max * sizeof (char *));
	      else
		list = (char **) malloc (max * sizeof (char *));
	    }
	  /* Strip leading whitespace.  */
	  while (*sp && isspace ((int) *sp))
	    sp++;
	  list[num++] = cp;
	  
	  /* search for the end of the argument, we want to keep going
           * if we are in quotes or the char is not a delimiter
           */
	  while (*sp && (in_quotes || ((*sp != ',')
                                       && (!parens || *sp != ')')
                                       && (parens || !isspace ((int) *sp)))))
	    {
	      /*
	       * single quotes give literal value inside, including '\'. 
	       * you can't have a single inside single quotes.
	       * doubles quotes gives literal value inside, but allows escape.
	       */
	      if ((*sp == '"' || *sp == '\'') && (!in_quotes || *sp == in_quotes))
	        {
	          in_quotes = in_quotes ? 0 : *sp;
	          sp++;
	          continue;
	        }
	      /* unless within single quotes, \<char> will just be <char> */
	      else if (*sp == '\\' && in_quotes != '\'')
	        sp++;
	      *cp++ = *sp++;
	    }
	  cp2 = cp - 1;
	  *cp++ = 0;
	  if (*sp == ',' || (!parens && isspace ((int) *sp)))
	    {
	      maybe_empty = 1;
	      sp++;
	    }
	  /* Strip trailing whitespace.  */
	  for (; isspace ((int) *cp2) && cp2 >= list[num - 1]; cp2--)
	    *cp2 = 0;
	}
    }
  
 cleanup:
  
  if (list != NULL)
    free(list);
  
  if (str != NULL)
    free (str);
  
  return retcode;
}

int hid_parse_command (const char *str_)
{
  return hid_parse_actionstring (str_, FALSE);
}

int hid_parse_actions (const char *str_)
{
  return hid_parse_actionstring (str_, TRUE);
}

/* trick for the doc extractor */
#define static

/* %start-doc actions 00macros

@macro hidaction

This is one of a number of actions which are part of the HID
interface.  The core functions use these actions to tell the current
GUI when to change the presented information in response to changes
that the GUI may not know about.  The user normally does not invoke
these directly.

@end macro

%end-doc */


static const char pcbchanged_syntax[] =
  "PCBChanged([revert])";
static const char pcbchanged_help[] =
  "Tells the GUI that the whole PCB has changed. The optional \"revert\""
  "parameter can be used as a hint to the GUI that the same design is being"
  "reloaded, and that it might keep some viewport settings";

/* %start-doc actions PCBChanged

@hidaction

%end-doc */

static const char routestyleschanged_syntax[] = 
  "RouteStylesChanged()";
static const char routestyleschanged_help[] = 
  "Tells the GUI that the routing styles have changed.";

/* %start-doc actions RouteStylesChanged

@hidaction

%end-doc */

static const char netlistchanged_syntax[] = 
  "NetlistChanged()";
static const char netlistchanged_help[] = 
  "Tells the GUI that the netlist has changed.";

/* %start-doc actions NetlistChanged

@hidaction

%end-doc */

static const char layerschanged_syntax[] = 
  "LayersChanged()";
static const char layerschanged_help[] = 
  "Tells the GUI that the layers have changed.";

/* %start-doc actions LayersChanged

This includes layer names, colors, stacking order, visibility, etc.

@hidaction

%end-doc */

static const char librarychanged_syntax[] = 
  "LibraryChanged()";
static const char librarychanged_help[] = 
  "Tells the GUI that the libraries have changed.";

/* %start-doc actions LibraryChanged

@hidaction

%end-doc */
