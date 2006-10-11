/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "xincludes.h"

#include "compat.h"
#include "global.h"
#include "data.h"
#include "crosshair.h"
#include "misc.h"

#include "hid.h"
#include "../hidint.h"
#include "lesstif.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented GUI function %s\n", __FUNCTION__), abort()

static Arg args[30];
static int n;
#define stdarg(t,v) XtSetArg(args[n], t, v); n++

static int ok;

#define COMPONENT_SIDE_NAME "(top)"
#define SOLDER_SIDE_NAME "(bottom)"

/* ------------------------------------------------------------ */

static void
dialog_callback (Widget w, void *v, void *cbs)
{
  ok = (int) v;
}

static int
wait_for_dialog (Widget w)
{
  ok = -1;
  XtManageChild (w);
  while (ok == -1 && XtIsManaged (w))
    {
      XEvent e;
      XtAppNextEvent (app_context, &e);
      XtDispatchEvent (&e);
    }
  XtUnmanageChild (w);
  return ok;
}

/* ------------------------------------------------------------ */

static Widget fsb = 0;
static XmString xms_pcb, xms_net, xms_vend, xms_all, xms_load, xms_loadv,
  xms_save;

static void
setup_fsb_dialog ()
{
  if (fsb)
    return;

  xms_pcb = XmStringCreateLocalized ("*.pcb");
  xms_net = XmStringCreateLocalized ("*.net");
  xms_vend = XmStringCreateLocalized ("*.vend");
  xms_all = XmStringCreateLocalized ("*");
  xms_load = XmStringCreateLocalized ("Load From");
  xms_loadv = XmStringCreateLocalized ("Load Vendor");
  xms_save = XmStringCreateLocalized ("Save As");

  n = 0;
  fsb = XmCreateFileSelectionDialog (mainwind, "file", args, n);

  XtAddCallback (fsb, XmNokCallback, (XtCallbackProc) dialog_callback,
		 (XtPointer) 1);
  XtAddCallback (fsb, XmNcancelCallback, (XtCallbackProc) dialog_callback,
		 (XtPointer) 0);
}

static const char load_syntax[] =
"Load()\n"
"Load(Layout|LayoutToBuffer|ElementToBuffer|Netlist|Revert)";

static const char load_help[] =
"Load layout data from a user-selected file.";

/* %start-doc actions Load

This action is a GUI front-end to the core's @code{LoadFrom} action
(@pxref{LoadFrom Action}).  If you happen to pass a filename, like
@code{LoadFrom}, then @code{LoadFrom} is called directly.  Else, the
user is prompted for a filename to load, and then @code{LoadFrom} is
called with that filename.

%end-doc */

static int
Load (int argc, char **argv, int x, int y)
{
  char *function;
  char *name;
  XmString xmname, pattern;

  if (argc > 1)
    return hid_actionv ("LoadFrom", argc, argv);

  function = argc ? argv[0] : "Layout";

  setup_fsb_dialog ();

  if (strcasecmp (function, "Netlist") == 0)
    pattern = xms_net;
  else
    pattern = xms_pcb;

  n = 0;
  stdarg (XmNtitle, "Load From");
  XtSetValues (XtParent (fsb), args, n);

  n = 0;
  stdarg (XmNpattern, pattern);
  stdarg (XmNmustMatch, True);
  stdarg (XmNselectionLabelString, xms_load);
  XtSetValues (fsb, args, n);

  if (!wait_for_dialog (fsb))
    return 1;

  n = 0;
  stdarg (XmNdirSpec, &xmname);
  XtGetValues (fsb, args, n);

  XmStringGetLtoR (xmname, XmFONTLIST_DEFAULT_TAG, &name);

  hid_actionl ("LoadFrom", function, name, NULL);

  XtFree (name);

  return 0;
}

static const char loadvendor_syntax[] =
"LoadVendor()";

static const char loadvendor_help[] =
"Loads a user-selected vendor resource file.";

/* %start-doc actions LoadVendor

The user is prompted for a file to load, and then
@code{LoadVendorFrom} is called (@pxref{LoadVendorFrom Action}) to
load that vendor file.

%end-doc */

static int
LoadVendor (int argc, char **argv, int x, int y)
{
  char *name;
  XmString xmname, pattern;

  if (argc > 0)
    return hid_actionv ("LoadVendorFrom", argc, argv);

  setup_fsb_dialog ();

  pattern = xms_vend;

  n = 0;
  stdarg (XmNtitle, "Load Vendor");
  XtSetValues (XtParent (fsb), args, n);

  n = 0;
  stdarg (XmNpattern, pattern);
  stdarg (XmNmustMatch, True);
  stdarg (XmNselectionLabelString, xms_loadv);
  XtSetValues (fsb, args, n);

  if (!wait_for_dialog (fsb))
    return 1;

  n = 0;
  stdarg (XmNdirSpec, &xmname);
  XtGetValues (fsb, args, n);

  XmStringGetLtoR (xmname, XmFONTLIST_DEFAULT_TAG, &name);

  hid_actionl ("LoadVendorFrom", name, NULL);

  XtFree (name);

  return 0;
}

static const char save_syntax[] =
"Save()\n"
"Save(Layout|LayoutAs)\n"
"Save(AllConnections|AllUnusedPins|ElementConnections)";

static const char save_help[] =
"Save layout data to a user-selected file.";

/* %start-doc actions Save

This action is a GUI front-end to the core's @code{SaveTo} action
(@pxref{SaveTo Action}).  If you happen to pass a filename, like
@code{SaveTo}, then @code{SaveTo} is called directly.  Else, the
user is prompted for a filename to save, and then @code{SaveTo} is
called with that filename.

%end-doc */

static int
Save (int argc, char **argv, int x, int y)
{
  char *function;
  char *name;
  XmString xmname, pattern;

  if (argc > 1)
    hid_actionv ("SaveTo", argc, argv);

  function = argc ? argv[0] : "Layout";
  
  if (strcasecmp (function, "Layout") == 0)
    if (PCB->Filename)
      return hid_actionl ("SaveTo", "Layout", PCB->Filename, NULL);

  setup_fsb_dialog ();

  pattern = xms_pcb;

  XtManageChild (fsb);

  n = 0;
  stdarg (XmNtitle, "Save As");
  XtSetValues (XtParent (fsb), args, n);

  n = 0;
  stdarg (XmNpattern, pattern);
  stdarg (XmNmustMatch, False);
  stdarg (XmNselectionLabelString, xms_save);
  XtSetValues (fsb, args, n);

  if (!wait_for_dialog (fsb))
    return 1;

  n = 0;
  stdarg (XmNdirSpec, &xmname);
  XtGetValues (fsb, args, n);

  XmStringGetLtoR (xmname, XmFONTLIST_DEFAULT_TAG, &name);

  if (strcasecmp (function, "PasteBuffer") == 0)
    hid_actionl ("PasteBuffer", "Save", name, NULL);
  else
    {
      /* 
       * if we got this far and the function is Layout, then
       * we really needed it to be a LayoutAs.  Otherwise 
       * ActionSaveTo() will ignore the new file name we
       * just obtained.
       */
      if (strcasecmp (function, "Layout") == 0)
	hid_actionl ("SaveTo", "LayoutAs", name, NULL);
      else
	hid_actionl ("SaveTo", function, name, NULL);
    }
  XtFree (name);

  return 0;
}

/* ------------------------------------------------------------ */

static Widget log_form, log_text;
static char *msg_buffer = 0;
static int msg_buffer_size = 0;
static int log_size = 0;
static int pending_newline = 0;

static void
log_clear (Widget w, void *up, void *cbp)
{
  XmTextSetString (log_text, "");
  log_size = 0;
  pending_newline = 0;
}

static void
log_dismiss (Widget w, void *up, void *cbp)
{
  XtUnmanageChild (log_form);
}

void
lesstif_logv (const char *fmt, va_list ap)
{
  int i;
  char *bp;
  if (!mainwind)
    {
      vprintf (fmt, ap);
      return;
    }
  if (!log_form)
    {
      Widget clear_button, dismiss_button;

      n = 0;
      stdarg (XmNautoUnmanage, False);
      stdarg (XmNwidth, 600);
      stdarg (XmNheight, 200);
      stdarg (XmNtitle, "PCB Log");
      log_form = XmCreateFormDialog (mainwind, "log", args, n);

      n = 0;
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      stdarg (XmNbottomAttachment, XmATTACH_FORM);
      clear_button = XmCreatePushButton (log_form, "clear", args, n);
      XtManageChild (clear_button);
      XtAddCallback (clear_button, XmNactivateCallback,
		     (XtCallbackProc) log_clear, 0);

      n = 0;
      stdarg (XmNrightAttachment, XmATTACH_WIDGET);
      stdarg (XmNrightWidget, clear_button);
      stdarg (XmNbottomAttachment, XmATTACH_FORM);
      dismiss_button = XmCreatePushButton (log_form, "dismiss", args, n);
      XtManageChild (dismiss_button);
      XtAddCallback (dismiss_button, XmNactivateCallback,
		     (XtCallbackProc) log_dismiss, 0);

      n = 0;
      stdarg (XmNeditable, False);
      stdarg (XmNeditMode, XmMULTI_LINE_EDIT);
      stdarg (XmNcursorPositionVisible, True);
      stdarg (XmNtopAttachment, XmATTACH_FORM);
      stdarg (XmNleftAttachment, XmATTACH_FORM);
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      stdarg (XmNbottomAttachment, XmATTACH_WIDGET);
      stdarg (XmNbottomWidget, clear_button);
      log_text = XmCreateScrolledText (log_form, "text", args, n);
      XtManageChild (log_text);

      msg_buffer = (char *) malloc (1002);
      msg_buffer_size = 1002;
      XtManageChild (log_form);
    }
  bp = msg_buffer;
  if (pending_newline)
    {
      *bp++ = '\n';
      pending_newline = 0;
    }
#ifdef HAVE_VSNPRINTF
  i = vsnprintf (bp, msg_buffer_size, fmt, ap);
  if (i >= msg_buffer_size)
    {
      msg_buffer_size = i + 100;
      msg_buffer = (char *) realloc (msg_buffer, msg_buffer_size + 2);
      vsnprintf (bp, msg_buffer_size, fmt, ap);
    }
#else
  vsprintf (bp, fmt, ap);
#endif
  bp = msg_buffer + strlen (msg_buffer) - 1;
  while (bp >= msg_buffer && bp[0] == '\n')
    {
      pending_newline++;
      *bp-- = 0;
    }
  XmTextInsert (log_text, log_size, msg_buffer);
  log_size += strlen (msg_buffer);
  XmTextSetCursorPosition (log_text, log_size);
}

void
lesstif_log (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  lesstif_logv (fmt, ap);
  va_end (ap);
}

/* ------------------------------------------------------------ */

static Widget confirm_dialog = 0;
static Widget confirm_cancel, confirm_ok, confirm_label;

int
lesstif_confirm_dialog (char *msg, ...)
{
  char *cancelmsg, *okmsg;
  va_list ap;
  XmString xs;

  if (mainwind == 0)
    return 1;

  if (confirm_dialog == 0)
    {
      n = 0;
      stdarg (XmNdefaultButtonType, XmDIALOG_OK_BUTTON);
      stdarg (XmNtitle, "Confirm");
      confirm_dialog = XmCreateQuestionDialog (mainwind, "confirm", args, n);
      XtAddCallback (confirm_dialog, XmNcancelCallback,
		     (XtCallbackProc) dialog_callback, (XtPointer) 0);
      XtAddCallback (confirm_dialog, XmNokCallback,
		     (XtCallbackProc) dialog_callback, (XtPointer) 1);

      confirm_cancel =
	XmMessageBoxGetChild (confirm_dialog, XmDIALOG_CANCEL_BUTTON);
      confirm_ok = XmMessageBoxGetChild (confirm_dialog, XmDIALOG_OK_BUTTON);
      confirm_label =
	XmMessageBoxGetChild (confirm_dialog, XmDIALOG_MESSAGE_LABEL);
      XtUnmanageChild (XmMessageBoxGetChild
		       (confirm_dialog, XmDIALOG_HELP_BUTTON));
    }

  va_start (ap, msg);
  cancelmsg = va_arg (ap, char *);
  okmsg = va_arg (ap, char *);
  va_end (ap);

  if (!cancelmsg)
    {
      cancelmsg = "Cancel";
      okmsg = "Ok";
    }

  n = 0;
  xs = XmStringCreateLocalized (cancelmsg);

  if (okmsg)
    {
      stdarg (XmNcancelLabelString, xs);
      xs = XmStringCreateLocalized (okmsg);
      XtManageChild (confirm_cancel);
    }
  else
    XtUnmanageChild (confirm_cancel);

  stdarg (XmNokLabelString, xs);

  xs = XmStringCreateLocalized (msg);
  stdarg (XmNmessageString, xs);
  XtSetValues (confirm_dialog, args, n);

  wait_for_dialog (confirm_dialog);

  n = 0;
  stdarg (XmNdefaultPosition, False);
  XtSetValues (confirm_dialog, args, n);

  return ok;
}

static int
ConfirmAction (int argc, char **argv, int x, int y)
{
  int rv = lesstif_confirm_dialog (argc > 0 ? argv[0] : 0,
				   argc > 1 ? argv[1] : 0,
				   argc > 2 ? argv[2] : 0,
				   0);
  return rv;
}

/* ------------------------------------------------------------ */

static Widget report = 0, report_form;

void
lesstif_report_dialog (char *title, char *msg)
{
  if (!report)
    {
      if (mainwind == 0)
	return;

      n = 0;
      stdarg (XmNautoUnmanage, False);
      stdarg (XmNwidth, 600);
      stdarg (XmNheight, 200);
      stdarg (XmNtitle, title);
      report_form = XmCreateFormDialog (mainwind, "report", args, n);

      n = 0;
      stdarg (XmNeditable, False);
      stdarg (XmNeditMode, XmMULTI_LINE_EDIT);
      stdarg (XmNcursorPositionVisible, False);
      stdarg (XmNtopAttachment, XmATTACH_FORM);
      stdarg (XmNleftAttachment, XmATTACH_FORM);
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      stdarg (XmNbottomAttachment, XmATTACH_FORM);
      report = XmCreateScrolledText (report_form, "text", args, n);
      XtManageChild (report);
    }
  n = 0;
  stdarg (XmNtitle, title);
  XtSetValues (report_form, args, n);
  XmTextSetString (report, msg);

  XtManageChild (report_form);
}

/* ------------------------------------------------------------ */

static Widget prompt_dialog = 0;
static Widget prompt_label, prompt_text;

char *
lesstif_prompt_for (char *msg, char *default_string)
{
  char *rv;
  XmString xs;
  if (prompt_dialog == 0)
    {
      n = 0;
      stdarg (XmNautoUnmanage, False);
      stdarg (XmNtitle, "PCB Prompt");
      prompt_dialog = XmCreateFormDialog (mainwind, "prompt", args, n);

      n = 0;
      stdarg (XmNtopAttachment, XmATTACH_FORM);
      stdarg (XmNleftAttachment, XmATTACH_FORM);
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      stdarg (XmNalignment, XmALIGNMENT_BEGINNING);
      prompt_label = XmCreateLabel (prompt_dialog, "label", args, n);
      XtManageChild (prompt_label);

      n = 0;
      stdarg (XmNtopAttachment, XmATTACH_WIDGET);
      stdarg (XmNtopWidget, prompt_label);
      stdarg (XmNbottomAttachment, XmATTACH_WIDGET);
      stdarg (XmNleftAttachment, XmATTACH_FORM);
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      stdarg (XmNeditable, True);
      prompt_text = XmCreateText (prompt_dialog, "text", args, n);
      XtManageChild (prompt_text);
      XtAddCallback (prompt_text, XmNactivateCallback,
		     (XtCallbackProc) dialog_callback, (XtPointer) 1);
    }
  if (!default_string)
    default_string = "";
  if (!msg)
    msg = "Enter text:";
  n = 0;
  xs = XmStringCreateLocalized (msg);
  stdarg (XmNlabelString, xs);
  XtSetValues (prompt_label, args, n);
  XmTextSetString (prompt_text, default_string);
  XmTextSetCursorPosition (prompt_text, strlen (default_string));
  wait_for_dialog (prompt_dialog);
  rv = XmTextGetString (prompt_text);
  return rv;
}

static const char promptfor_syntax[] =
"PromptFor([message[,default]])";

static const char promptfor_help[] =
"Prompt for a response.";

/* %start-doc actions PromptFor

This is mostly for testing the lesstif HID interface.  The parameters
are passed to the @code{prompt_for()} HID function, causing the user
to be prompted for a response.  The respose is simply printed to the
user's stdout.

%end-doc */

static int
PromptFor (int argc, char **argv, int x, int y)
{
  char *rv = lesstif_prompt_for (argc > 0 ? argv[0] : 0,
				 argc > 1 ? argv[1] : 0);
  printf ("rv = `%s'\n", rv);
  return 0;
}

/* ------------------------------------------------------------ */

static Widget
create_form_ok_dialog (char *name, int ok)
{
  Widget dialog, topform;
  n = 0;
  dialog = XmCreateQuestionDialog (mainwind, name, args, n);

  XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_SYMBOL_LABEL));
  XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_MESSAGE_LABEL));
  XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
  XtAddCallback (dialog, XmNcancelCallback, (XtCallbackProc) dialog_callback,
		 (XtPointer) 0);
  if (ok)
    XtAddCallback (dialog, XmNokCallback, (XtCallbackProc) dialog_callback,
		   (XtPointer) 1);
  else
    XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_OK_BUTTON));

  n = 0;
  topform = XmCreateForm (dialog, "attributes", args, n);
  XtManageChild (topform);
  return topform;
}

int
lesstif_attribute_dialog (HID_Attribute * attrs,
			  int n_attrs, HID_Attr_Val * results)
{
  Widget dialog, topform, lform, form;
  Widget *wl;
  int i, rv;
  static XmString empty = 0;

  if (!empty)
    empty = XmStringCreateLocalized (" ");

  for (i = 0; i < n_attrs; i++)
    {
      results[i] = attrs[i].default_val;
      if (results[i].str_value)
	results[i].str_value = strdup (results[i].str_value);
    }

  wl = (Widget *) malloc (n_attrs * sizeof (Widget));

  topform = create_form_ok_dialog ("attributes", 1);
  dialog = XtParent (topform);

  n = 0;
  stdarg (XmNfractionBase, n_attrs);
  XtSetValues (topform, args, n);

  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_FORM);
  stdarg (XmNbottomAttachment, XmATTACH_FORM);
  stdarg (XmNleftAttachment, XmATTACH_FORM);
  stdarg (XmNfractionBase, n_attrs);
  lform = XmCreateForm (topform, "attributes", args, n);
  XtManageChild (lform);

  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_FORM);
  stdarg (XmNbottomAttachment, XmATTACH_FORM);
  stdarg (XmNleftAttachment, XmATTACH_WIDGET);
  stdarg (XmNleftWidget, lform);
  stdarg (XmNrightAttachment, XmATTACH_FORM);
  stdarg (XmNfractionBase, n_attrs);
  form = XmCreateForm (topform, "attributes", args, n);
  XtManageChild (form);

  for (i = 0; i < n_attrs; i++)
    {
      Widget w;

      n = 0;
      stdarg (XmNleftAttachment, XmATTACH_FORM);
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      stdarg (XmNtopAttachment, XmATTACH_POSITION);
      stdarg (XmNtopPosition, i);
      stdarg (XmNbottomAttachment, XmATTACH_POSITION);
      stdarg (XmNbottomPosition, i + 1);
      stdarg (XmNalignment, XmALIGNMENT_END);
      w = XmCreateLabel (lform, attrs[i].name, args, n);
      XtManageChild (w);
    }

  for (i = 0; i < n_attrs; i++)
    {
      static char buf[30];
      n = 0;
      stdarg (XmNleftAttachment, XmATTACH_FORM);
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      stdarg (XmNtopAttachment, XmATTACH_POSITION);
      stdarg (XmNtopPosition, i);
      stdarg (XmNbottomAttachment, XmATTACH_POSITION);
      stdarg (XmNbottomPosition, i + 1);
      stdarg (XmNalignment, XmALIGNMENT_END);

      switch (attrs[i].type)
	{
	case HID_Label:
	  stdarg (XmNlabelString, empty);
	  wl[i] = XmCreateLabel (form, attrs[i].name, args, n);
	  break;
	case HID_Boolean:
	  stdarg (XmNlabelString, empty);
	  stdarg (XmNset, results[i].int_value);
	  wl[i] = XmCreateToggleButton (form, attrs[i].name, args, n);
	  break;
	case HID_String:
	  stdarg (XmNcolumns, 40);
	  stdarg (XmNresizeWidth, True);
	  stdarg (XmNvalue, results[i].str_value);
	  wl[i] = XmCreateTextField (form, attrs[i].name, args, n);
	  break;
	case HID_Integer:
	  stdarg (XmNcolumns, 13);
	  stdarg (XmNresizeWidth, True);
	  sprintf (buf, "%d", results[i].int_value);
	  stdarg (XmNvalue, buf);
	  wl[i] = XmCreateTextField (form, attrs[i].name, args, n);
	  break;
	case HID_Real:
	  stdarg (XmNcolumns, 16);
	  stdarg (XmNresizeWidth, True);
	  sprintf (buf, "%g", results[i].real_value);
	  stdarg (XmNvalue, buf);
	  wl[i] = XmCreateTextField (form, attrs[i].name, args, n);
	  break;
	default:
	  wl[i] = XmCreateLabel (form, "UNIMPLEMENTED", args, n);
	  break;
	}

      XtManageChild (wl[i]);
    }

  rv = wait_for_dialog (dialog);

  for (i = 0; i < n_attrs; i++)
    {
      char *cp;
      switch (attrs[i].type)
	{
	case HID_Boolean:
	  results[i].int_value = XmToggleButtonGetState (wl[i]);
	  break;
	case HID_String:
	  results[i].str_value = XmTextGetString (wl[i]);
	  break;
	case HID_Integer:
	  cp = XmTextGetString (wl[i]);
	  sscanf (cp, "%d", &results[i].int_value);
	  break;
	case HID_Real:
	  cp = XmTextGetString (wl[i]);
	  sscanf (cp, "%lg", &results[i].real_value);
	  break;
	default:
	  break;
	}
    }

  free (wl);
  XtDestroyWidget (dialog);

  return rv ? 0 : 1;
}

/* ------------------------------------------------------------ */

static const char dowindows_syntax[] =
"DoWindows(1|2|3|4)\n"
"DoWindows(Layout|Library|Log|Netlist)";

static const char dowindows_help[] =
"Open various GUI windows.";

/* %start-doc actions DoWindows

@table @code

@item 1
@itemx Layout
Open the layout window.  Since the layout window is always shown
anyway, this has no effect.

@item 2
@itemx Library
Open the library window.

@item 3
@itemx Log
Open the log window.

@item 4
@itemx Netlist
Open the netlist window.

@end table

%end-doc */

static int
DoWindows (int argc, char **argv, int x, int y)
{
  char *a = argc == 1 ? argv[0] : "";
  if (strcmp (a, "1") == 0 || strcasecmp (a, "Layout") == 0)
    {
    }
  else if (strcmp (a, "2") == 0 || strcasecmp (a, "Library") == 0)
    {
      lesstif_show_library ();
    }
  else if (strcmp (a, "3") == 0 || strcasecmp (a, "Log") == 0)
    {
      if (log_form == 0)
	lesstif_log ("");
      XtManageChild (log_form);
    }
  else if (strcmp (a, "4") == 0 || strcasecmp (a, "Netlist") == 0)
    {
      lesstif_show_netlist ();
    }
  else
    {
      lesstif_log ("Usage: DoWindows(1|2|3|4|Layout|Library|Log|Netlist)");
      return 1;
    }
  return 0;
}

/* ------------------------------------------------------------ */
static const char about_syntax[] =
"About()";

static const char about_help[] =
"Tell the user about this version of PCB.";

/* %start-doc actions About

This just pops up a dialog telling the user which version of
@code{pcb} they're running.

%end-doc */


static int
About (int argc, char **argv, int x, int y)
{
  static Widget about = 0;
  if (!about)
    {
      Cardinal n = 0;
      XmString xs = XmStringCreateLocalized (GetInfoString ());
      stdarg (XmNmessageString, xs);
      stdarg (XmNtitle, "About PCB");
      about = XmCreateInformationDialog (mainwind, "about", args, n);
      XtUnmanageChild (XmMessageBoxGetChild (about, XmDIALOG_CANCEL_BUTTON));
      XtUnmanageChild (XmMessageBoxGetChild (about, XmDIALOG_HELP_BUTTON));
    }
  wait_for_dialog (about);
  return 0;
}

/* ------------------------------------------------------------ */

static const char print_syntax[] =
"Print()";

static const char print_help[] =
"Print the layout.";

/* %start-doc actions Print

This will find the default printing HID, prompt the user for its
options, and print the layout.

%end-doc */

static int
Print (int argc, char **argv, int x, int y)
{
  HID_Attribute *opts;
  HID *printer;
  HID_Attr_Val *vals;
  int n;

  printer = hid_find_printer ();
  if (!printer)
    {
      lesstif_confirm_dialog ("No printer?", "Oh well", 0);
      return 1;
    }
  opts = printer->get_export_options (&n);
  vals = (HID_Attr_Val *) calloc (n, sizeof (HID_Attr_Val));
  if (lesstif_attribute_dialog (opts, n, vals))
    {
      free (vals);
      return 1;
    }
  printer->do_export (vals);
  free (vals);
  return 0;
}

static const char export_syntax[] =
"Export()";

static const char export_help[] =
"Export the layout.";

/* %start-doc actions Export

Prompts the user for an exporter to use.  Then, prompts the user for
that exporter's options, and exports the layout.

%end-doc */

static int
Export (int argc, char **argv, int x, int y)
{
  static Widget selector = 0;
  HID_Attribute *opts;
  HID *printer, **hids;
  HID_Attr_Val *vals;
  int n, i;
  Widget prev = 0;
  Widget w;

  hids = hid_enumerate ();

  if (!selector)
    {
      n = 0;
      stdarg (XmNtitle, "Export HIDs");
      selector = create_form_ok_dialog ("export", 0);
      for (i = 0; hids[i]; i++)
	{
	  if (hids[i]->exporter)
	    {
	      n = 0;
	      if (prev)
		{
		  stdarg (XmNtopAttachment, XmATTACH_WIDGET);
		  stdarg (XmNtopWidget, prev);
		}
	      else
		{
		  stdarg (XmNtopAttachment, XmATTACH_FORM);
		}
	      stdarg (XmNrightAttachment, XmATTACH_FORM);
	      stdarg (XmNleftAttachment, XmATTACH_FORM);
	      w =
		XmCreatePushButton (selector, (char *) hids[i]->name, args,
				    n);
	      XtManageChild (w);
	      XtAddCallback (w, XmNactivateCallback,
			     (XtCallbackProc) dialog_callback,
			     (XtPointer) (i + 1));
	      prev = w;
	    }
	}
      selector = XtParent (selector);
    }

  i = wait_for_dialog (selector);

  if (i <= 0)
    return 1;
  printer = hids[i - 1];

  exporter = printer;

  opts = printer->get_export_options (&n);
  vals = (HID_Attr_Val *) calloc (n, sizeof (HID_Attr_Val));
  if (lesstif_attribute_dialog (opts, n, vals))
    {
      free (vals);
      return 1;
    }
  printer->do_export (vals);
  free (vals);
  exporter = NULL;
  return 0;
}

/* ------------------------------------------------------------ */

static Widget sizes_dialog = 0;
static Widget sz_pcb_w, sz_pcb_h, sz_bloat, sz_shrink, sz_drc_wid, sz_drc_slk,
  sz_drc_drill, sz_drc_ring;
static Widget sz_text;
static Widget sz_set, sz_reset, sz_units;

static int
sz_str2val (Widget w, int pcbu)
{
  double d;
  char *buf;
  buf = XmTextGetString (w);
  if (!pcbu)
    return atoi (buf);
  sscanf (buf, "%lf", &d);
  if (Settings.grid_units_mm)
    return MM_TO_PCB (d);
  else
    return MIL_TO_PCB (d);
}

static void
sz_val2str (Widget w, int u, int pcbu)
{
  double d;
  static char buf[40];
  if (pcbu)
    {
      if (Settings.grid_units_mm)
	d = PCB_TO_MM (u);
      else
	d = PCB_TO_MIL (u);
      sprintf (buf, "%.2f", d + 0.002);
    }
  else
    sprintf (buf, "%d %%", u);
  XmTextSetString (w, buf);
}

static void
sizes_set ()
{
  PCB->MaxWidth = sz_str2val (sz_pcb_w, 1);
  PCB->MaxHeight = sz_str2val (sz_pcb_h, 1);
  PCB->Bloat = sz_str2val (sz_bloat, 1);
  PCB->Shrink = sz_str2val (sz_shrink, 1);
  PCB->minWid = sz_str2val (sz_drc_wid, 1);
  PCB->minSlk = sz_str2val (sz_drc_slk, 1);
  PCB->minDrill = sz_str2val (sz_drc_drill, 1);
  PCB->minRing = sz_str2val (sz_drc_ring, 1);
  Settings.TextScale = sz_str2val (sz_text, 0);

  Settings.Bloat = PCB->Bloat;
  Settings.Shrink = PCB->Shrink;
  Settings.minWid = PCB->minWid;
  Settings.minSlk = PCB->minSlk;
  Settings.minDrill = PCB->minDrill;
  Settings.minRing = PCB->minRing;

  SetCrosshairRange (0, 0, PCB->MaxWidth, PCB->MaxHeight);
  lesstif_pan_fixup ();
}

void
lesstif_sizes_reset ()
{
  char *ls;
  if (!sizes_dialog)
    return;
  sz_val2str (sz_pcb_w, PCB->MaxWidth, 1);
  sz_val2str (sz_pcb_h, PCB->MaxHeight, 1);
  sz_val2str (sz_bloat, PCB->Bloat, 1);
  sz_val2str (sz_shrink, PCB->Shrink, 1);
  sz_val2str (sz_drc_wid, PCB->minWid, 1);
  sz_val2str (sz_drc_slk, PCB->minSlk, 1);
  sz_val2str (sz_drc_drill, PCB->minDrill, 1);
  sz_val2str (sz_drc_ring, PCB->minRing, 1);
  sz_val2str (sz_text, Settings.TextScale, 0);

  if (Settings.grid_units_mm)
    ls = "Units are MMs";
  else
    ls = "Units are Mils";
  n = 0;
  stdarg (XmNlabelString, XmStringCreateLocalized (ls));
  XtSetValues (sz_units, args, n);
}

static Widget
size_field (Widget parent, char *label, int posn)
{
  Widget w, l;
  n = 0;
  stdarg (XmNrightAttachment, XmATTACH_FORM);
  stdarg (XmNtopAttachment, XmATTACH_POSITION);
  stdarg (XmNtopPosition, posn);
  stdarg (XmNbottomAttachment, XmATTACH_POSITION);
  stdarg (XmNbottomPosition, posn + 1);
  stdarg (XmNcolumns, 10);
  w = XmCreateTextField (parent, "field", args, n);
  XtManageChild (w);

  n = 0;
  stdarg (XmNleftAttachment, XmATTACH_FORM);
  stdarg (XmNrightAttachment, XmATTACH_WIDGET);
  stdarg (XmNrightWidget, w);
  stdarg (XmNtopAttachment, XmATTACH_POSITION);
  stdarg (XmNtopPosition, posn);
  stdarg (XmNbottomAttachment, XmATTACH_POSITION);
  stdarg (XmNbottomPosition, posn + 1);
  stdarg (XmNlabelString, XmStringCreateLocalized (label));
  stdarg (XmNalignment, XmALIGNMENT_END);
  l = XmCreateLabel (parent, "label", args, n);
  XtManageChild (l);

  return w;
}

static const char adjustsizes_syntax[] =
"AdjustSizes()";

static const char adjustsizes_help[] =
"Let the user change the board size, DRC parameters, etc";

/* %start-doc actions AdjustSizes

Displays a dialog box that lets the user change the board
size, DRC parameters, and text scale.

The units are determined by the default display units.

%end-doc */

static int
AdjustSizes (int argc, char **argv, int x, int y)
{
  if (!sizes_dialog)
    {
      Widget inf, sep;

      n = 0;
      stdarg (XmNmarginWidth, 3);
      stdarg (XmNmarginHeight, 3);
      stdarg (XmNhorizontalSpacing, 3);
      stdarg (XmNverticalSpacing, 3);
      stdarg (XmNautoUnmanage, False);
      stdarg (XmNtitle, "Board Sizes");
      sizes_dialog = XmCreateFormDialog (mainwind, "sizes", args, n);

      n = 0;
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      stdarg (XmNbottomAttachment, XmATTACH_FORM);
      sz_reset = XmCreatePushButton (sizes_dialog, "Reset", args, n);
      XtManageChild (sz_reset);
      XtAddCallback (sz_reset, XmNactivateCallback,
		     (XtCallbackProc) lesstif_sizes_reset, 0);

      n = 0;
      stdarg (XmNrightAttachment, XmATTACH_WIDGET);
      stdarg (XmNrightWidget, sz_reset);
      stdarg (XmNbottomAttachment, XmATTACH_FORM);
      sz_set = XmCreatePushButton (sizes_dialog, "Set", args, n);
      XtManageChild (sz_set);
      XtAddCallback (sz_set, XmNactivateCallback, (XtCallbackProc) sizes_set,
		     0);

      n = 0;
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      stdarg (XmNleftAttachment, XmATTACH_FORM);
      stdarg (XmNbottomAttachment, XmATTACH_WIDGET);
      stdarg (XmNbottomWidget, sz_reset);
      sep = XmCreateSeparator (sizes_dialog, "sep", args, n);
      XtManageChild (sep);

      n = 0;
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      stdarg (XmNleftAttachment, XmATTACH_FORM);
      stdarg (XmNbottomAttachment, XmATTACH_WIDGET);
      stdarg (XmNbottomWidget, sep);
      sz_units = XmCreateLabel (sizes_dialog, "units", args, n);
      XtManageChild (sz_units);

      n = 0;
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      stdarg (XmNleftAttachment, XmATTACH_FORM);
      stdarg (XmNtopAttachment, XmATTACH_FORM);
      stdarg (XmNbottomAttachment, XmATTACH_WIDGET);
      stdarg (XmNbottomWidget, sz_units);
      stdarg (XmNfractionBase, 9);
      inf = XmCreateForm (sizes_dialog, "sizes", args, n);
      XtManageChild (inf);

      sz_pcb_w = size_field (inf, "PCB Width", 0);
      sz_pcb_h = size_field (inf, "PCB Height", 1);
      sz_bloat = size_field (inf, "Bloat", 2);
      sz_shrink = size_field (inf, "Shrink", 3);
      sz_drc_wid = size_field (inf, "DRC Min Wid", 4);
      sz_drc_slk = size_field (inf, "DRC Min Silk", 5);
      sz_drc_drill = size_field (inf, "DRC Min Drill", 6);
      sz_drc_ring = size_field (inf, "DRC Min Annular Ring", 7);
      sz_text = size_field (inf, "Text Scale", 8);
    }
  lesstif_sizes_reset ();
  XtManageChild (sizes_dialog);
  return 0;
}

/* ------------------------------------------------------------ */

static Widget layer_groups_form = 0;
static Widget lg_buttonform = 0;

static int lg_setcol[MAX_LAYER+2];
static int lg_width, lg_height;
static int lg_r[MAX_LAYER+3];
static int lg_c[MAX_LAYER+1];
static int lg_label_width, lg_fa, lg_fd;
static GC lg_gc = 0;

#if 0
static Widget lglabels[MAX_LAYER + 2];
static Widget lgbuttons[MAX_LAYER + 2][MAX_LAYER];
#endif

typedef struct {
  XFontStruct *font;
  Pixel fg, bg, sel;
} LgResource;

static LgResource lgr;

static XtResource lg_resources[] = {
  { "font", "Font", XtRFontStruct, sizeof(XFontStruct*), XtOffset(LgResource*, font), XtRString, "fixed" },
  { "foreground", "Foreground", XtRPixel, sizeof(Pixel), XtOffset(LgResource*, fg), XtRString, "black" },
  { "selectColor", "Foreground", XtRPixel, sizeof(Pixel), XtOffset(LgResource*, sel), XtRString, "blue" },
  { "background", "Background", XtRPixel, sizeof(Pixel), XtOffset(LgResource*, bg), XtRString, "white" }
};

#if 0
static void
lgbutton_cb (Widget w, int ij, void *cbs)
{
  int layer, group, k;

  layer = ij / max_layer;
  group = ij % max_layer;
  group = MoveLayerToGroup (layer, group);
  for (k = 0; k < max_layer; k++)
    {
      if (k == group)
	XmToggleButtonSetState (lgbuttons[layer][k], 1, 0);
      else
	XmToggleButtonSetState (lgbuttons[layer][k], 0, 0);
    }
}
#endif

static void
lgbutton_expose (Widget w, XtPointer u, XmDrawingAreaCallbackStruct *cbs)
{
  int i;
  Window win = XtWindow(w);

  if (cbs && cbs->event->xexpose.count)
    return;
  if (lg_gc == 0 && !cbs)
    return;
  if (lg_gc == 0 && cbs)
    {
      lg_gc = XCreateGC (display, win, 0, 0);
      XSetFont (display, lg_gc, lgr.font->fid);
    }

  XSetForeground (display, lg_gc, lgr.bg);
  XFillRectangle (display, win, lg_gc, 0, 0, lg_width, lg_height);
  XSetForeground (display, lg_gc, lgr.fg);
  for (i=0; i<max_layer; i++)
    XDrawLine(display, win, lg_gc, lg_c[i], 0, lg_c[i], lg_height);
  for (i=1; i<max_layer+2; i++)
    XDrawLine(display, win, lg_gc, lg_label_width, lg_r[i], lg_width, lg_r[i]);
  for (i=0; i<max_layer+2; i++)
    {
      int dir;
      XCharStruct size;
      int swidth;
      const char *name;

      if (i == max_layer)
	name = SOLDER_SIDE_NAME;
      else if (i == max_layer + 1)
	name = COMPONENT_SIDE_NAME;
      else
	name = PCB->Data->Layer[i].Name;
      XTextExtents (lgr.font, name, strlen(name), &dir, &lg_fa, &lg_fd, &size);
      swidth = size.rbearing - size.lbearing;
      XDrawString(display, win, lg_gc,
		  (lg_label_width - swidth)/2 - size.lbearing,
		  (lg_r[i] + lg_r[i+1] + lg_fd + lg_fa)/2 - 1,
		  name, strlen(name));
    }
  XSetForeground (display, lg_gc, lgr.sel);
  for (i=0; i<max_layer+2; i++)
    {
      int c = lg_setcol[i];
      int x1 = lg_c[c] + 2;
      int x2 = lg_c[c+1] - 2;
      int y1 = lg_r[i] + 2;
      int y2 = lg_r[i+1] - 2;
      XFillRectangle (display, win, lg_gc, x1, y1, x2-x1+1, y2-y1+1);
    }
}

static void
lgbutton_input (Widget w, XtPointer u, XmDrawingAreaCallbackStruct *cbs)
{
  int layer, group;
  if (cbs->event->type != ButtonPress)
    return;
  layer = cbs->event->xbutton.y * (max_layer+2) / lg_height;
  group = (cbs->event->xbutton.x - lg_label_width) * max_layer / (lg_width - lg_label_width);
  group = MoveLayerToGroup (layer, group);
  lg_setcol[layer] = group;
  lgbutton_expose (w, 0, 0);
  gui->invalidate_all ();
}

static void
lgbutton_resize (Widget w, XtPointer u, XmDrawingAreaCallbackStruct *cbs)
{
  int i;
  Dimension width, height;
  n = 0;
  stdarg(XmNwidth, &width);
  stdarg(XmNheight, &height);
  XtGetValues(w, args, n);
  lg_width = width;
  lg_height = height;

  for (i=0; i<=max_layer; i++)
    lg_c[i] = lg_label_width + (lg_width - lg_label_width) * i / max_layer;
  for (i=0; i<=max_layer+2; i++)
    lg_r[i] = lg_height * i / (max_layer+2);
  lgbutton_expose (w, 0, 0);
}

void
lesstif_update_layer_groups ()
{
  int sets[MAX_LAYER + 2][MAX_LAYER];
  int i, j, n;
  LayerGroupType *l = &(PCB->LayerGroups);

  if (!layer_groups_form)
    return;

  memset (sets, 0, sizeof (sets));

  for (i = 0; i < max_layer; i++)
    for (j = 0; j < l->Number[i]; j++)
      {
	sets[l->Entries[i][j]][i] = 1;
	lg_setcol[l->Entries[i][j]] = i;
      }

  lg_label_width = 0;
  for (i=0; i<max_layer+2; i++)
    {
      int dir;
      XCharStruct size;
      int swidth;
      const char *name;

      if (i == max_layer)
	name = SOLDER_SIDE_NAME;
      else if (i == max_layer + 1)
	name = COMPONENT_SIDE_NAME;
      else
	name = PCB->Data->Layer[i].Name;
      XTextExtents (lgr.font, name, strlen(name), &dir, &lg_fa, &lg_fd, &size);
      swidth = size.rbearing - size.lbearing;
      if (lg_label_width < swidth)
	lg_label_width = swidth;
    }
  lg_label_width += 4;

  n = 0;
  stdarg(XmNwidth, lg_label_width + (lg_fa+lg_fd) * max_layer);
  stdarg(XmNheight, (lg_fa+lg_fd) * (max_layer + 2));
  XtSetValues(lg_buttonform, args, n);
  lgbutton_expose (lg_buttonform, 0, 0);

#if 0
  for (i = 0; i < max_layer + 2; i++)
    {
      char *name = "unknown";
      n = 0;
      if (i < max_layer)
	name = PCB->Data->Layer[i].Name;
      else if (i == max_layer)
	name = SOLDER_SIDE_NAME;
      else if (i == max_layer + 1)
	name = COMPONENT_SIDE_NAME;
      stdarg (XmNlabelString, XmStringCreateLocalized (name));
      XtSetValues (lglabels[i], args, n);
      for (j = 0; j < max_layer; j++)
	{
	  if (sets[i][j] != XmToggleButtonGetState (lgbuttons[i][j]))
	    {
	      XmToggleButtonSetState (lgbuttons[i][j], sets[i][j], 0);
	    }
	}
    }
  XtUnmanageChild(lg_buttonform);
  for (i = 0; i < MAX_LAYER + 2; i++)
    for (j = 0; j < MAX_LAYER; j++)
      {
	if (i < max_layer + 2 && j < max_layer)
	  {
	    XtManageChild(lgbuttons[i][j]);
	    n = 0;
	    stdarg (XmNleftPosition, j * (max_layer + 2));
	    stdarg (XmNrightPosition, (j + 1) * (max_layer + 2));
	    stdarg (XmNtopPosition, i * max_layer);
	    stdarg (XmNbottomPosition, (i + 1) * max_layer);
	    XtSetValues(lgbuttons[i][j], args, n);
	  }
	else
	  XtUnmanageChild(lgbuttons[i][j]);
      }
  n = 0;
  stdarg (XmNfractionBase, max_layer + 2);
  XtSetValues (layer_groups_form, args, n);
  n = 0;
  stdarg (XmNfractionBase, max_layer * (max_layer + 2));
  XtSetValues (lg_buttonform, args, n);
  XtManageChild(lg_buttonform);
#endif
}

static const char editlayergroups_syntax[] =
"EditLayerGroups()";

static const char editlayergroups_help[] =
"Let the user change the layer groupings";

/* %start-doc actions EditLayerGroups

Displays a dialog that lets the user view and change the layer
groupings.  Each layer (row) can be a member of any one layer group
(column).  Note the special layers @code{solder} and @code{component}
allow you to specify which groups represent the top and bottom of the
board.

See @ref{ChangeName Action}.

%end-doc */

static int
EditLayerGroups (int argc, char **argv, int x, int y)
{
  if (!layer_groups_form)
    {

      n = 0;
      stdarg (XmNfractionBase, max_layer + 2);
      stdarg (XmNtitle, "Layer Groups");
      layer_groups_form = XmCreateFormDialog (mainwind, "layers", args, n);

      n = 0;
      stdarg (XmNtopAttachment, XmATTACH_FORM);
      stdarg (XmNbottomAttachment, XmATTACH_FORM);
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      stdarg (XmNleftAttachment, XmATTACH_FORM);
      lg_buttonform = XmCreateDrawingArea (layer_groups_form, "layers", args, n);
      XtManageChild (lg_buttonform);

      XtAddCallback (lg_buttonform, XmNexposeCallback,
		     (XtCallbackProc) lgbutton_expose, 0);
      XtAddCallback (lg_buttonform, XmNinputCallback,
		     (XtCallbackProc) lgbutton_input, 0);
      XtAddCallback (lg_buttonform, XmNresizeCallback,
		     (XtCallbackProc) lgbutton_resize, 0);

      XtGetSubresources (layer_groups_form, &lgr,
			 "layergroups", "LayerGroups",
			 lg_resources, XtNumber(lg_resources), 0, 0);
#if 0
      stdarg (XmNfractionBase, max_layer * (MAX_LAYER + 2));
      lg_buttonform = XmCreateForm (layer_groups_form, "lgbutton", args, n);

      for (i = 0; i < MAX_LAYER + 2; i++)
	{
	  n = 0;
	  stdarg (XmNleftAttachment, XmATTACH_FORM);
	  stdarg (XmNtopAttachment, XmATTACH_POSITION);
	  stdarg (XmNtopPosition, i);
	  stdarg (XmNbottomAttachment, XmATTACH_POSITION);
	  stdarg (XmNbottomPosition, i + 1);
	  stdarg (XmNrightAttachment, XmATTACH_WIDGET);
	  stdarg (XmNrightWidget, lg_buttonform);
	  lglabels[i] = XmCreateLabel (layer_groups_form, "layer", args, n);
	  XtManageChild (lglabels[i]);

	  for (j = 0; j < MAX_LAYER; j++)
	    {
	      n = 0;
	      stdarg (XmNleftAttachment, XmATTACH_POSITION);
	      stdarg (XmNleftPosition, j * (MAX_LAYER + 2));
	      stdarg (XmNrightAttachment, XmATTACH_POSITION);
	      stdarg (XmNrightPosition, (j + 1) * (MAX_LAYER + 2));
	      stdarg (XmNtopAttachment, XmATTACH_POSITION);
	      stdarg (XmNtopPosition, i * MAX_LAYER);
	      stdarg (XmNbottomAttachment, XmATTACH_POSITION);
	      stdarg (XmNbottomPosition, (i + 1) * MAX_LAYER);
	      stdarg (XmNlabelString, XmStringCreateLocalized (" "));
	      stdarg (XmNspacing, 0);
	      stdarg (XmNvisibleWhenOff, True);
	      stdarg (XmNfillOnSelect, True);
	      stdarg (XmNshadowThickness, 0);
	      stdarg (XmNmarginWidth, 0);
	      stdarg (XmNmarginHeight, 0);
	      stdarg (XmNhighlightThickness, 0);
	      lgbuttons[i][j] =
		XmCreateToggleButton (lg_buttonform, "label", args, n);
	      XtManageChild (lgbuttons[i][j]);

	      XtAddCallback (lgbuttons[i][j], XmNvalueChangedCallback,
			     (XtCallbackProc) lgbutton_cb,
			     (XtPointer) (i * max_layer + j));
	    }
	}
#endif
    }
  lesstif_update_layer_groups ();
  XtManageChild (layer_groups_form);
  return 1;
}

/* ------------------------------------------------------------ */

HID_Action lesstif_dialog_action_list[] = {
  {"Load", 0, Load,
   load_help, load_syntax},
  {"LoadVendor", 0, LoadVendor,
   loadvendor_help, loadvendor_syntax},
  {"Save", 0, Save,
   save_help, save_syntax},
  {"DoWindows", 0, DoWindows,
   dowindows_help, dowindows_syntax},
  {"PromptFor", 0, PromptFor,
   promptfor_help, promptfor_syntax},
  {"Confirm", 0, ConfirmAction},
  {"About", 0, About,
   about_help, about_syntax},
  {"Print", 0, Print,
   print_help, print_syntax},
  {"Export", 0, Export,
   export_help, export_syntax},
  {"AdjustSizes", 0, AdjustSizes,
   adjustsizes_help, adjustsizes_syntax},
  {"EditLayerGroups", 0, EditLayerGroups,
   editlayergroups_help, editlayergroups_syntax},
};

REGISTER_ACTIONS (lesstif_dialog_action_list)
