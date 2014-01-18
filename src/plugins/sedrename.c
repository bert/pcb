/*!
 * \file sedrename.c
 *
 * \brief Sed rename plug-in for PCB.
 *
 * \author Copyright (C) 2008 Peter Clifton <pcjc2@cam.ac.uk>
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Compile like this:
 *
 * gcc -Wall -I$HOME/pcbsrc/pcb.clean/src -I$HOME/pcbsrc/pcb.clean -O2 -shared sedrename.c -o sedrename.so
 *
 * The resulting sedrename.so goes in $HOME/.pcb/plugins/sedrename.so.
 */

#include <stdio.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "hid.h"
#include "misc.h"
#include "create.h"
#include "error.h"
#include "change.h"
#include "rtree.h"
#include "undo.h"

static int
sedrename (int argc, char **argv, Coord x, Coord y)
{
  FILE *fp;
  int result = STATUS_OK;
  char *sed_prog = "sed ";
  char *sed_postfix = " > sedrename.tmp";
  char *sed_arguments;
  char *sed_cmd;

  static char *last_argument = NULL;

  if (last_argument == NULL)
    last_argument = strdup ("s///");

#if 0
  if (argc != 1)
  {
    Message("Usage: sedrename sed_expression");
    return 1;
  }

  sed_arguments = argv[0];
#else
  if (argc != 0)
  {
    Message("Usage: sedrename");
    return 1;
  }

  sed_arguments = gui->prompt_for ("sedrename", last_argument);
  if (sed_arguments == NULL)
  {
    Message("Got null for sed arguments");
    return 1;
  }
  free (last_argument);
  last_argument = sed_arguments;
#endif

  sed_cmd = (char *) malloc (strlen (sed_prog) + strlen (sed_arguments) + strlen (sed_postfix) + 1);
  memcpy (sed_cmd, sed_prog, strlen (sed_prog) + 1);
  strcpy (sed_cmd + strlen (sed_prog), sed_arguments);
  strcpy (sed_cmd + strlen (sed_prog) + strlen (sed_arguments), sed_postfix);

  printf ("Calling sed: '%s'\n", sed_cmd);

  if ((fp = popen (sed_cmd, "w")) == NULL)
  {
    PopenErrorMessage (sed_cmd);
    free (sed_cmd);
    return STATUS_ERROR;
  }

  free (sed_cmd);

  SET_FLAG (NAMEONPCBFLAG, PCB);

  ELEMENT_LOOP (PCB->Data);
  {

    if (!TEST_FLAG (SELECTEDFLAG, element))
      continue;

    /* BADNESS.. IF SED DOESN'T MATCH OUTPUT LINE FOR LINE, WE STALL */
    fprintf (fp, "%s\n", element->Name[1].TextString);
  }
  END_LOOP;

  if (pclose (fp))
   return STATUS_ERROR;

  if ((fp = fopen ("sedrename.tmp", "rb")) == NULL)
  {
    Message("Cannot open sedrename.tmp for reading");
    return STATUS_ERROR;
  }

  ELEMENT_LOOP (PCB->Data);
  {
    char *new_ref;

    if (!TEST_FLAG (SELECTEDFLAG, element))
      continue;

    if (fscanf (fp, "%as", &new_ref) != EOF)
    {
      printf ("Send '%s', got '%s'\n", element->Name[1].TextString, new_ref);

      AddObjectToChangeNameUndoList (ELEMENT_TYPE, NULL, NULL,
                                     element,
                                     NAMEONPCB_NAME (element));

      ChangeObjectName (ELEMENT_TYPE, element, NULL, NULL, new_ref);
    }
  }
  END_LOOP;

  gui->invalidate_all ();

  IncrementUndoSerialNumber ();

  return (fclose (fp) ? STATUS_ERROR : result);
}

static HID_Action sedrename_action_list[] = {
  {"sedrename", NULL, sedrename,
   NULL, NULL}
};

REGISTER_ACTIONS (sedrename_action_list)

void
pcb_plugin_init()
{
  register_sedrename_action_list();
}

