/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "global.h"
#include "data.h"

#include "hid.h"
#include "../hidint.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

typedef struct HID_ActionNode
{
  struct HID_ActionNode *next;
  HID_Action *actions;
  int n;
} HID_ActionNode;

HID_ActionNode *hid_action_nodes = 0;
static int n_actions = 0;
static HID_Action *all_actions = 0;

void
hid_register_actions (HID_Action * a, int n)
{
  HID_ActionNode *ha;

  /*printf("%d actions registered\n", n); */
  ha = (HID_ActionNode *) malloc (sizeof (HID_ActionNode));
  ha->next = hid_action_nodes;
  hid_action_nodes = ha;
  ha->actions = a;
  ha->n = n;
  n_actions += n;
  if (all_actions)
    {
      free (all_actions);
      all_actions = 0;
    }
}

static int
action_sort (const void *va, const void *vb)
{
  HID_Action *a = (HID_Action *) va;
  HID_Action *b = (HID_Action *) vb;
  return strcmp (a->name, b->name);
}

HID_Action *
hid_find_action (const char *name)
{
  HID_ActionNode *ha;
  int i, n, lower, upper;

  if (all_actions == 0)
    {
      n = 0;
      all_actions = malloc (n_actions * sizeof (HID_Action));
      for (ha = hid_action_nodes; ha; ha = ha->next)
	for (i = 0; i < ha->n; i++)
	  all_actions[n++] = ha->actions[i];
      qsort (all_actions, n_actions, sizeof (HID_Action), action_sort);
    }


  lower = -1;
  upper = n_actions;
  /*printf("search action %s\n", name); */
  while (lower < upper - 1)
    {
      i = (lower + upper) / 2;
      n = strcmp (all_actions[i].name, name);
      /*printf("try [%d].%s, cmp %d\n", i, all_actions[i].name, n); */
      if (n == 0)
	return all_actions + i;
      if (n > 0)
	upper = i;
      else
	lower = i;
    }

  for (i = 0; i < n_actions; i++)
    if (strcasecmp (all_actions[i].name, name) == 0)
      return all_actions + i;

  printf ("unknown action `%s'\n", name);
  return 0;
}

void
print_actions ()
{
  int i;
  /* Forces them to be sorted in all_actions */
  hid_find_action (hid_action_nodes->actions[0].name);
  fprintf (stderr, "Registered Actions:\n");
  for (i = 0; i < n_actions; i++)
    {
      if (all_actions[i].description)
	fprintf (stderr, "  %s - %s\n", all_actions[i].name,
		 all_actions[i].description);
      else
	fprintf (stderr, "  %s\n", all_actions[i].name);
      if (all_actions[i].syntax)
	{
	  const char *bb, *eb;
	  bb = eb = all_actions[i].syntax;
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
  int x = 0, y = 0, i;
  HID_Action *a;

  if (Settings.verbose)
    {
      printf ("Action: \033[34m%s(", name);
      for (i = 0; i < argc; i++)
	printf ("%s%s", i ? "," : "", argv[i]);
      printf (")\033[0m\n");
    }

  a = hid_find_action (name);
  if (!a)
    return 1;
  if (a->needs_coords)
    gui->get_coords (a->need_coord_msg, &x, &y);
  return a->trigger_cb (argc, argv, x, y);
}

int
hid_parse_actions (const char *rstr,
		   int (*function) (const char *, int, char **))
{
  static char **list = 0;
  static int max = 0;
  int num = 0;
  static char *str = NULL;
  char *sp, *aname, *sp2;
  int maybe_empty = 0;

  if (function == NULL)
    function = hid_actionv;

  if (str != NULL)
    {
      free (str);
      str = NULL;
    }

  /*fprintf(stderr, "invoke: `%s'\n", rstr); */

  sp = str = strdup (rstr);

another:
  /* eat leading spaces and tabs */
  while (*sp && isspace ((int) *sp))
    sp++;

  if (!*sp)
    return 0;

  aname = sp;

  /* search for the leading ( */
  while (*sp && *sp != '(')
    sp++;

  /*
   * we didn't find a leading ( so invoke the action
   * with no parameters or event.
   */
  if (!*sp)
    {
      if (function (aname, 0, 0))
	return 1;
      goto another;
    }

  /* 
   * we found a leading ( so see if we have parameters to pass to the
   * action 
   */
  *sp++ = 0;
  while (1)
    {
      /* 
       * maybe_empty == 0 means that the last char examined was not a
       * "," 
       */
      if (*sp == ')' && !maybe_empty)
	{
	  *sp++ = 0;
	  if (function (aname, num, list))
	    return 1;
	  goto another;
	}
      else if (*sp == 0 && !maybe_empty)
	break;
      else
	{
	  maybe_empty = 0;
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
	  list[num++] = sp;

	  /* search for a "," or a ")" */
	  while (*sp && *sp != ',' && *sp != ')')
	    sp++;
	  sp2 = sp - 1;
	  if (*sp == ',')
	    {
	      maybe_empty = 1;
	      *sp++ = 0;
	    }
	  /* Strip trailing whitespace.  */
	  for (; isspace ((int) *sp2) && sp2 >= list[num - 1]; sp2--)
	    *sp2 = 0;
	}
    }

  return 0;
}
