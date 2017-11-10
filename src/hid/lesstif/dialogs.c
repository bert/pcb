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
#include "pcb-printf.h"

#include "hid.h"
#include "../hidint.h"
#include "lesstif.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented GUI function %s\n", __FUNCTION__), abort()

static Arg args[30];
static int n;
#define stdarg(t,v) XtSetArg(args[n], t, v); n++

static int ok;

#define TOP_SIDE_NAME "(top)"
#define BOTTOM_SIDE_NAME "(bottom)"

/* ------------------------------------------------------------ */

static void
dialog_callback (Widget w, void *v, void *cbs)
{
  ok = (int) (size_t) v;
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
  xms_save, xms_fp;

static void
setup_fsb_dialog ()
{
  if (fsb)
    return;

  xms_pcb = XmStringCreatePCB ("*.pcb");
  xms_fp = XmStringCreatePCB ("*.fp");
  xms_net = XmStringCreatePCB ("*.net");
  xms_vend = XmStringCreatePCB ("*.vend");
  xms_all = XmStringCreatePCB ("*");
  xms_load = XmStringCreatePCB ("Load From");
  xms_loadv = XmStringCreatePCB ("Load Vendor");
  xms_save = XmStringCreatePCB ("Save As");

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
Load (int argc, char **argv, Coord x, Coord y)
{
  const char *function;
  char *name;
  XmString xmname, pattern;

  if (argc > 1)
    return hid_actionv ("LoadFrom", argc, argv);

  function = argc ? argv[0] : "Layout";

  setup_fsb_dialog ();

  if (strcasecmp (function, "Netlist") == 0)
    pattern = xms_net;
  else if (strcasecmp (function, "ElementToBuffer") == 0)
    pattern = xms_fp;
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
LoadVendor (int argc, char **argv, Coord x, Coord y)
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
"Save(AllConnections|AllUnusedPins|ElementConnections)\n"
"Save(PasteBuffer)";

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
Save (int argc, char **argv, Coord x, Coord y)
{
  const char *function;
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
  gchar *buf, *scan;
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

      XtManageChild (log_form);
    }
  if (pending_newline)
    {
      XmTextInsert (log_text, log_size++, "\n");
      pending_newline = 0;
    }
  buf = pcb_vprintf (fmt, ap);
  scan = &buf[strlen (buf) - 1];
  while (scan >= buf && *scan == '\n')
    {
      pending_newline++;
      *scan-- = 0;
    }
  XmTextInsert (log_text, log_size, buf);
  log_size += strlen (buf);

  scan = strrchr(buf, '\n');
  if (scan)
    scan++;
  else
    scan = buf;
  XmTextSetCursorPosition (log_text, log_size - strlen(scan));
  g_free (buf);
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
  xs = XmStringCreatePCB (cancelmsg);

  if (okmsg)
    {
      stdarg (XmNcancelLabelString, xs);
      xs = XmStringCreatePCB (okmsg);
      XtManageChild (confirm_cancel);
    }
  else
    XtUnmanageChild (confirm_cancel);

  stdarg (XmNokLabelString, xs);

  xs = XmStringCreatePCB (msg);
  stdarg (XmNmessageString, xs);
  XtSetValues (confirm_dialog, args, n);

  wait_for_dialog (confirm_dialog);

  n = 0;
  stdarg (XmNdefaultPosition, False);
  XtSetValues (confirm_dialog, args, n);

  return ok;
}

static int
ConfirmAction (int argc, char **argv, Coord x, Coord y)
{
  int rv = lesstif_confirm_dialog (argc > 0 ? argv[0] : 0,
				   argc > 1 ? argv[1] : 0,
				   argc > 2 ? argv[2] : 0,
				   0);
  return rv;
}

/* ------------------------------------------------------------ */

int
lesstif_close_confirm_dialog ()
{
  return lesstif_confirm_dialog ("OK to lose data ?", NULL);
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
/* FIXME -- make this a proper file select dialog box */
char *
lesstif_fileselect (const char *title, const char *descr,
		    char *default_file, char *default_ext,
		    const char *history_tag, int flags)
{

  return lesstif_prompt_for (title, default_file);
}

/* ------------------------------------------------------------ */

static Widget prompt_dialog = 0;
static Widget prompt_label, prompt_text;

char *
lesstif_prompt_for (const char *msg, const char *default_string)
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
  xs = XmStringCreatePCB ((char *)msg);
  stdarg (XmNlabelString, xs);
  XtSetValues (prompt_label, args, n);
  XmTextSetString (prompt_text, (char *)default_string);
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
PromptFor (int argc, char **argv, Coord x, Coord y)
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
			  int n_attrs, HID_Attr_Val * results,
			  const char * title, const char * descr)
{
  Widget dialog, topform, lform, form;
  Widget *wl;
  int i, rv;
  static XmString empty = 0;
  int actual_nattrs = 0;
  int attrcount = 0;

  if (!empty)
    empty = XmStringCreatePCB (" ");

  for (i = 0; i < n_attrs; i++)
    {
      if (attrs[i].help_text != ATTR_UNDOCUMENTED)
	actual_nattrs ++;
      results[i] = attrs[i].default_val;
      if (results[i].str_value)
	results[i].str_value = strdup (results[i].str_value);
    }

  wl = (Widget *) malloc (n_attrs * sizeof (Widget));

  topform = create_form_ok_dialog ((char *)title, 1);
  dialog = XtParent (topform);

  n = 0;
  stdarg (XmNfractionBase, n_attrs);
  XtSetValues (topform, args, n);

  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_FORM);
  stdarg (XmNbottomAttachment, XmATTACH_FORM);
  stdarg (XmNleftAttachment, XmATTACH_FORM);
  stdarg (XmNfractionBase, actual_nattrs);
  lform = XmCreateForm (topform, "attributes", args, n);
  XtManageChild (lform);

  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_FORM);
  stdarg (XmNbottomAttachment, XmATTACH_FORM);
  stdarg (XmNleftAttachment, XmATTACH_WIDGET);
  stdarg (XmNleftWidget, lform);
  stdarg (XmNrightAttachment, XmATTACH_FORM);
  stdarg (XmNfractionBase, actual_nattrs);
  form = XmCreateForm (topform, "attributes", args, n);
  XtManageChild (form);

  attrcount = -1;
  for (i = 0; i < n_attrs; i++)
    {
      Widget w;

      if (attrs[i].help_text == ATTR_UNDOCUMENTED)
	continue;
      attrcount ++;

      n = 0;
      stdarg (XmNleftAttachment, XmATTACH_FORM);
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      stdarg (XmNtopAttachment, XmATTACH_POSITION);
      stdarg (XmNtopPosition, attrcount);
      stdarg (XmNbottomAttachment, XmATTACH_POSITION);
      stdarg (XmNbottomPosition, attrcount + 1);
      stdarg (XmNalignment, XmALIGNMENT_END);
      w = XmCreateLabel (lform, attrs[i].name, args, n);
      XtManageChild (w);
    }

  attrcount = -1;
  for (i = 0; i < n_attrs; i++)
    {
      static char buf[30];
      n = 0;

      if (attrs[i].help_text == ATTR_UNDOCUMENTED)
	continue;
      attrcount ++;

      stdarg (XmNleftAttachment, XmATTACH_FORM);
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      stdarg (XmNtopAttachment, XmATTACH_POSITION);
      stdarg (XmNtopPosition, attrcount);
      stdarg (XmNbottomAttachment, XmATTACH_POSITION);
      stdarg (XmNbottomPosition, attrcount + 1);
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
	case HID_Coord:
	  stdarg (XmNcolumns, 13);
	  stdarg (XmNresizeWidth, True);
	  pcb_snprintf (buf, sizeof (buf), "%$mS", results[i].coord_value);
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
	case HID_Enum:
	  {
	    static XmString empty = 0;
	    Widget submenu, default_button=0;
	    int sn = n;

	    if (empty == 0)
	      empty = XmStringCreatePCB ("");

	    submenu = XmCreatePulldownMenu (form, attrs[i].name, args+sn, n-sn);

	    n = sn;
	    stdarg (XmNlabelString, empty);
	    stdarg (XmNsubMenuId, submenu);
	    wl[i] = XmCreateOptionMenu (form, attrs[i].name, args, n);

	    for (sn=0; attrs[i].enumerations[sn]; sn++)
	      {
		Widget btn;
		XmString label;
		n = 0;
		label = XmStringCreatePCB ((char *)attrs[i].enumerations[sn]);
		stdarg (XmNuserData, & attrs[i].enumerations[sn]);
		stdarg (XmNlabelString, label);
		btn = XmCreatePushButton (submenu, "menubutton", args, n);
		XtManageChild (btn);
		XmStringFree (label);
		if (sn == attrs[i].default_val.int_value)
		  default_button = btn;
	      }
	    if (default_button)
	      {
		n = 0;
		stdarg (XmNmenuHistory, default_button);
		XtSetValues (wl[i], args, n);
	      }
	  }
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

      if (attrs[i].help_text == ATTR_UNDOCUMENTED)
	continue;

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
	case HID_Coord:
	  cp = XmTextGetString (wl[i]);
	  results[i].coord_value = GetValue (cp, NULL, NULL);
	  break;
	case HID_Real:
	  cp = XmTextGetString (wl[i]);
	  sscanf (cp, "%lg", &results[i].real_value);
	  break;
	case HID_Enum:
	  {
	    const char **uptr;
	    Widget btn;

	    n = 0;
	    stdarg (XmNmenuHistory, &btn);
	    XtGetValues (wl[i], args, n);
	    n = 0;
	    stdarg (XmNuserData, &uptr);
	    XtGetValues (btn, args, n);
	    results[i].int_value = uptr - attrs[i].enumerations;
	  }
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
DoWindows (int argc, char **argv, Coord x, Coord y)
{
  const char *a = argc == 1 ? argv[0] : "";
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
About (int argc, char **argv, Coord x, Coord y)
{
  static Widget about = 0;
  if (!about)
    {
      Cardinal n = 0;
      XmString xs = XmStringCreatePCB (GetInfoString ());
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
Print (int argc, char **argv, Coord x, Coord y)
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
  if (lesstif_attribute_dialog (opts, n, vals, "Print", ""))
    {
      free (vals);
      return 1;
    }
  printer->do_export (vals);
  free (vals);
  return 0;
}

static HID_Attribute
printer_calibrate_attrs[] = {
  {"Enter Values here:", "",
   HID_Label, 0, 0, {0, 0, 0}, 0, 0},
  {"x-calibration", "X scale for calibrating your printer",
   HID_Real, 0.5, 25, {0, 0, 1.00}, 0, 0},
  {"y-calibration", "Y scale for calibrating your printer",
   HID_Real, 0.5, 25, {0, 0, 1.00}, 0, 0}
};
static HID_Attr_Val printer_calibrate_values[3];

static const char printcalibrate_syntax[] =
"PrintCalibrate()";

static const char printcalibrate_help[] =
"Calibrate the printer.";

/* %start-doc actions PrintCalibrate

This will print a calibration page, which you would measure and type
the measurements in, so that future printouts will be more precise.

%end-doc */

static int
PrintCalibrate (int argc, char **argv, Coord x, Coord y)
{
  HID *printer = hid_find_printer ();
  printer->calibrate (0.0, 0.0);
  if (gui->attribute_dialog (printer_calibrate_attrs, 3,
			     printer_calibrate_values,
			     "Printer Calibration Values", "Enter calibration values for your printer"))
    return 1;
  printer->calibrate (printer_calibrate_values[1].real_value,
		      printer_calibrate_values[2].real_value);
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
Export (int argc, char **argv, Coord x, Coord y)
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
			     (XtPointer) ((size_t) i + 1));
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
  if (lesstif_attribute_dialog (opts, n, vals, "Export", NULL))
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
sz_str2val (Widget w, bool pcbu)
{
  char *buf = XmTextGetString (w);
  if (!pcbu)
    return strtol (buf, NULL, 0);
  return GetValueEx (buf, NULL, NULL, NULL, Settings.grid_unit->suffix);
}

static void
sz_val2str (Widget w, Coord u, int pcbu)
{
  static char buf[40];
  if (pcbu)
    pcb_snprintf (buf, sizeof (buf), "%m+%.2mS", Settings.grid_unit->allow, u);
  else
    pcb_snprintf (buf, sizeof (buf), "%#mS %%", u);
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

  crosshair_update_range();
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

  ls = g_strdup_printf (_("Units are %s."), Settings.grid_unit->in_suffix);
  n = 0;
  stdarg (XmNlabelString, XmStringCreatePCB (ls));
  XtSetValues (sz_units, args, n);
  g_free (ls);
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
  stdarg (XmNlabelString, XmStringCreatePCB (label));
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
AdjustSizes (int argc, char **argv, Coord x, Coord y)
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

static int lg_setcol[MAX_ALL_LAYER];
static int lg_width, lg_height;
static int lg_r[MAX_ALL_LAYER + 1];
static int lg_c[MAX_ALL_LAYER + 1];
static int lg_label_width, lg_fa, lg_fd;
static GC lg_gc = 0;

#if 0
static Widget lglabels[MAX_ALL_LAYER];
static Widget lgbuttons[MAX_ALL_LAYER][MAX_GROUP];
#endif

typedef struct {
  XFontStruct *font;
  Pixel fg, bg, sel;
} LgResource;

static LgResource lgr;

static XtResource lg_resources[] = {
  { "font", "Font", XtRFontStruct, sizeof(XFontStruct*), XtOffset(LgResource*, font), XtRString, (void *)"fixed" },
  { "foreground", "Foreground", XtRPixel, sizeof(Pixel), XtOffset(LgResource*, fg), XtRString, (void *)"black" },
  { "selectColor", "Foreground", XtRPixel, sizeof(Pixel), XtOffset(LgResource*, sel), XtRString, (void *)"blue" },
  { "background", "Background", XtRPixel, sizeof(Pixel), XtOffset(LgResource*, bg), XtRString, (void *)"white" }
};

#if 0
static void
lgbutton_cb (Widget w, int ij, void *cbs)
{
  int layer, group, k;

  layer = ij / max_group;
  group = ij % max_group;
  group = MoveLayerToGroup (layer, group);
  for (k = 0; k < max_group; k++)
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
  for (i = 0; i < max_group; i++)
    XDrawLine(display, win, lg_gc, lg_c[i], 0, lg_c[i], lg_height);
  for (i = 1; i < max_copper_layer + SILK_LAYER; i++)
    XDrawLine(display, win, lg_gc, lg_label_width, lg_r[i], lg_width, lg_r[i]);
  for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
    {
      int dir;
      XCharStruct size;
      int swidth;
      const char *name;

      if (i == bottom_silk_layer)
	name = BOTTOM_SIDE_NAME;
      else if (i == top_silk_layer)
	name = TOP_SIDE_NAME;
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
  for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
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
  layer = cbs->event->xbutton.y * (max_copper_layer + SILK_LAYER) /
          lg_height;
  group = (cbs->event->xbutton.x - lg_label_width) * max_group / (lg_width - lg_label_width);
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

  for (i=0; i<=max_group; i++)
    lg_c[i] = lg_label_width + (lg_width - lg_label_width) * i / max_group;
  for (i=0; i<=max_copper_layer + SILK_LAYER; i++)
    lg_r[i] = lg_height * i / (max_copper_layer + SILK_LAYER);
  lgbutton_expose (w, 0, 0);
}

void
lesstif_update_layer_groups ()
{
  int sets[MAX_ALL_LAYER][MAX_GROUP];
  int i, j, n;
  LayerGroupType *l = &(PCB->LayerGroups);

  if (!layer_groups_form)
    return;

  memset (sets, 0, sizeof (sets));

  for (i = 0; i < max_group; i++)
    for (j = 0; j < l->Number[i]; j++)
      {
	sets[l->Entries[i][j]][i] = 1;
	lg_setcol[l->Entries[i][j]] = i;
      }

  lg_label_width = 0;
  for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
    {
      int dir;
      XCharStruct size;
      int swidth;
      const char *name;

      if (i == bottom_silk_layer)
	name = BOTTOM_SIDE_NAME;
      else if (i == top_silk_layer)
	name = TOP_SIDE_NAME;
      else
	name = PCB->Data->Layer[i].Name;
      XTextExtents (lgr.font, name, strlen(name), &dir, &lg_fa, &lg_fd, &size);
      swidth = size.rbearing - size.lbearing;
      if (lg_label_width < swidth)
	lg_label_width = swidth;
    }
  lg_label_width += 4;

  n = 0;
  stdarg(XmNwidth, lg_label_width + (lg_fa+lg_fd) * max_group);
  stdarg(XmNheight, (lg_fa+lg_fd) * (max_copper_layer + SILK_LAYER));
  XtSetValues(lg_buttonform, args, n);
  lgbutton_expose (lg_buttonform, 0, 0);

#if 0
  for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
    {
      char *name = "unknown";
      n = 0;
      if (i < max_copper_layer)
	name = PCB->Data->Layer[i].Name;
      else if (i == bottom_silk_layer)
	name = BOTTOM_SIDE_NAME;
      else if (i == top_silk_layer)
	name = TOP_SIDE_NAME;
      stdarg (XmNlabelString, XmStringCreatePCB (name));
      XtSetValues (lglabels[i], args, n);
      for (j = 0; j < max_group; j++)
	{
	  if (sets[i][j] != XmToggleButtonGetState (lgbuttons[i][j]))
	    {
	      XmToggleButtonSetState (lgbuttons[i][j], sets[i][j], 0);
	    }
	}
    }
  XtUnmanageChild(lg_buttonform);
  for (i = 0; i < MAX_ALL_LAYER; i++)
    for (j = 0; j < MAX_GROUP; j++)
      {
	if (i < max_copper_layer + SILK_LAYER && j < max_group)
	  {
	    XtManageChild(lgbuttons[i][j]);
	    n = 0;
	    stdarg (XmNleftPosition, j * (max_copper_layer + SILK_LAYER));
	    stdarg (XmNrightPosition, (j + 1) * (max_copper_layer + SILK_LAYER));
	    stdarg (XmNtopPosition, i * max_group);
	    stdarg (XmNbottomPosition, (i + 1) * max_group);
	    XtSetValues(lgbuttons[i][j], args, n);
	  }
	else
	  XtUnmanageChild(lgbuttons[i][j]);
      }
  n = 0;
  stdarg (XmNfractionBase, max_copper_layer + SILK_LAYER);
  XtSetValues (layer_groups_form, args, n);
  n = 0;
  stdarg (XmNfractionBase, max_group * (max_copper_layer + SILK_LAYER));
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
EditLayerGroups (int argc, char **argv, Coord x, Coord y)
{
  if (!layer_groups_form)
    {

      n = 0;
      stdarg (XmNfractionBase, max_copper_layer + SILK_LAYER);
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
      stdarg (XmNfractionBase, max_group * (MAX_ALL_LAYER));
      lg_buttonform = XmCreateForm (layer_groups_form, "lgbutton", args, n);

      for (i = 0; i < MAX_ALL_LAYER; i++)
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

	  for (j = 0; j < MAX_GROUP; j++)
	    {
	      n = 0;
	      stdarg (XmNleftAttachment, XmATTACH_POSITION);
	      stdarg (XmNleftPosition, j * (MAX_LAYER));
	      stdarg (XmNrightAttachment, XmATTACH_POSITION);
	      stdarg (XmNrightPosition, (j + 1) * (MAX_LAYER));
	      stdarg (XmNtopAttachment, XmATTACH_POSITION);
	      stdarg (XmNtopPosition, i * MAX_LAYER);
	      stdarg (XmNbottomAttachment, XmATTACH_POSITION);
	      stdarg (XmNbottomPosition, (i + 1) * MAX_LAYER);
	      stdarg (XmNlabelString, XmStringCreatePCB (" "));
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
			     (XtPointer) (i * max_group + j));
	    }
	}
#endif
    }
  lesstif_update_layer_groups ();
  XtManageChild (layer_groups_form);
  return 1;
}

/* ------------------------------------------------------------ */

typedef struct {
  Widget del;
  Widget w_name;
  Widget w_value;
} AttrRow;

static AttrRow *attr_row = 0;
static int attr_num_rows = 0;
static int attr_max_rows = 0;
static Widget attr_dialog = NULL, f_top;
static AttributeListType *attributes_list;

static void attributes_delete_callback (Widget w, void *v, void *cbs);

static void
fiddle_with_bb_layout ()
{
  int i;
  int max_height = 0;
  int max_del_width = 0;
  int max_name_width = 0;
  int max_value_width = 0;
  short ncolumns = 20;
  short vcolumns = 20;

  for (i=0; i<attr_num_rows; i++)
    {
      String v;

      n = 0;
      stdarg (XmNvalue, &v);
      XtGetValues (attr_row[i].w_name, args, n);
      if (ncolumns < strlen (v))
	ncolumns = strlen (v);

      n = 0;
      stdarg (XmNvalue, &v);
      XtGetValues (attr_row[i].w_value, args, n);
      if (vcolumns < strlen (v))
	vcolumns = strlen (v);
    }

  for (i=0; i<attr_num_rows; i++)
    {
      n = 0;
      stdarg (XmNcolumns, ncolumns);
      XtSetValues (attr_row[i].w_name, args, n);

      n = 0;
      stdarg (XmNcolumns, vcolumns);
      XtSetValues (attr_row[i].w_value, args, n);
    }

  for (i=0; i<attr_num_rows; i++)
    {
      Dimension w, h;
      n = 0;
      stdarg (XmNwidth, &w);
      stdarg (XmNheight, &h);

      XtGetValues (attr_row[i].del, args, n);
      if (max_height < h)
	max_height = h;
      if (max_del_width < w)
	max_del_width = w;

      XtGetValues (attr_row[i].w_name, args, n);
      if (max_height < h)
	max_height = h;
      if (max_name_width < w)
	max_name_width = w;

      XtGetValues (attr_row[i].w_value, args, n);
      if (max_height < h)
	max_height = h;
      if (max_value_width < w)
	max_value_width = w;
    }

  for (i=0; i<attr_num_rows; i++)
    {
      n = 0;
      stdarg (XmNx, 0);
      stdarg (XmNy, i * max_height);
      stdarg (XmNwidth, max_del_width);
      stdarg (XmNheight, max_height);
      XtSetValues (attr_row[i].del, args, n);

      n = 0;
      stdarg (XmNx, max_del_width);
      stdarg (XmNy, i * max_height);
      stdarg (XmNwidth, max_name_width);
      stdarg (XmNheight, max_height);
      XtSetValues (attr_row[i].w_name, args, n);

      n = 0;
      stdarg (XmNx, max_del_width + max_name_width);
      stdarg (XmNy, i * max_height);
      stdarg (XmNwidth, max_value_width);
      stdarg (XmNheight, max_height);
      XtSetValues (attr_row[i].w_value, args, n);
    }

  n = 0;
  stdarg (XmNwidth, max_del_width + max_name_width + max_value_width + 1);
  stdarg (XmNheight, max_height * attr_num_rows + 1);
  XtSetValues (f_top, args, n);
}

static void
lesstif_attributes_need_rows (int new_max)
{
  if (attr_max_rows < new_max)
    {
      if (attr_row)
	attr_row = (AttrRow *) realloc (attr_row, new_max * sizeof(AttrRow));
      else
	attr_row = (AttrRow *) malloc (new_max * sizeof(AttrRow));
    }

  while (attr_max_rows < new_max)
    {
      n = 0;
      attr_row[attr_max_rows].del = XmCreatePushButton (f_top, "del", args, n);
      XtManageChild (attr_row[attr_max_rows].del);
      XtAddCallback (attr_row[attr_max_rows].del, XmNactivateCallback,
		     (XtCallbackProc) attributes_delete_callback,
		     (XtPointer) (size_t) attr_max_rows);

      n = 0;
      stdarg (XmNresizeWidth, True);
      attr_row[attr_max_rows].w_name = XmCreateTextField (f_top, "name", args, n);
      XtManageChild (attr_row[attr_max_rows].w_name);
      XtAddCallback (attr_row[attr_max_rows].w_name, XmNvalueChangedCallback,
		     (XtCallbackProc) fiddle_with_bb_layout, NULL);

      n = 0;
      stdarg (XmNresizeWidth, True);
      attr_row[attr_max_rows].w_value = XmCreateTextField (f_top, "value", args, n);
      XtManageChild (attr_row[attr_max_rows].w_value);
      XtAddCallback (attr_row[attr_max_rows].w_value, XmNvalueChangedCallback,
		     (XtCallbackProc) fiddle_with_bb_layout, NULL);

      attr_max_rows ++;
    }

  /* Manage any previously unused rows we now need to show.  */
  while (attr_num_rows < new_max)
    {
      XtManageChild (attr_row[attr_num_rows].del);
      XtManageChild (attr_row[attr_num_rows].w_name);
      XtManageChild (attr_row[attr_num_rows].w_value);
      attr_num_rows ++;
    }
}

static void
lesstif_attributes_revert ()
{
  int i;

  lesstif_attributes_need_rows (attributes_list->Number);

  /* Unmanage any previously used rows we don't need.  */
  while (attr_num_rows > attributes_list->Number)
    {
      attr_num_rows --;
      XtUnmanageChild (attr_row[attr_num_rows].del);
      XtUnmanageChild (attr_row[attr_num_rows].w_name);
      XtUnmanageChild (attr_row[attr_num_rows].w_value);
    }

  /* Fill in values */
  for (i=0; i<attributes_list->Number; i++)
    {
      XmTextFieldSetString (attr_row[i].w_name, attributes_list->List[i].name);
      XmTextFieldSetString (attr_row[i].w_value, attributes_list->List[i].value);
    }

  fiddle_with_bb_layout ();
}

static void
attributes_new_callback (Widget w, void *v, void *cbs)
{
  lesstif_attributes_need_rows (attr_num_rows + 1); /* also bumps attr_num_rows */
  XmTextFieldSetString (attr_row[attr_num_rows-1].w_name, "");
  XmTextFieldSetString (attr_row[attr_num_rows-1].w_value, "");

  fiddle_with_bb_layout ();
}

static void
attributes_delete_callback (Widget w, void *v, void *cbs)
{
  int i, n;
  Widget wn, wv;

  n = (int) (size_t) v;

  wn = attr_row[n].w_name;
  wv = attr_row[n].w_value;

  for (i=n; i<attr_num_rows-1; i++)
    {
      attr_row[i].w_name = attr_row[i+1].w_name;
      attr_row[i].w_value = attr_row[i+1].w_value;
    }
  attr_row[attr_num_rows-1].w_name = wn;
  attr_row[attr_num_rows-1].w_value = wv;
  attr_num_rows --;

  XtUnmanageChild (wn);
  XtUnmanageChild (wv);

  fiddle_with_bb_layout ();
}

static void
attributes_revert_callback (Widget w, void *v, void *cbs)
{
  lesstif_attributes_revert ();
}

void
lesstif_attributes_dialog (char *owner, AttributeListType *attrs_list)
{
  Widget bform, sw, b_ok, b_cancel, b_revert, b_new;
  Widget sep;

  if (attr_dialog == NULL)
    {
      n = 0;
      stdarg (XmNautoUnmanage, False);
      stdarg (XmNtitle, owner);
      stdarg (XmNwidth, 400);
      stdarg (XmNheight, 300);
      attr_dialog = XmCreateFormDialog (mainwind, "attributes", args, n);

      n = 0;
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      stdarg (XmNbottomAttachment, XmATTACH_FORM);
      stdarg (XmNorientation, XmHORIZONTAL);
      stdarg (XmNentryAlignment, XmALIGNMENT_CENTER);
      stdarg (XmNpacking, XmPACK_COLUMN);
      bform = XmCreateRowColumn (attr_dialog, "attributes", args, n);
      XtManageChild (bform);

      n = 0;
      b_ok = XmCreatePushButton (bform, "OK", args, n);
      XtManageChild (b_ok);
      XtAddCallback (b_ok, XmNactivateCallback,
		     (XtCallbackProc) dialog_callback,
		     (XtPointer) 0);

      n = 0;
      b_new = XmCreatePushButton (bform, "New", args, n);
      XtManageChild (b_new);
      XtAddCallback (b_new, XmNactivateCallback,
		     (XtCallbackProc) attributes_new_callback,
		     NULL);

      n = 0;
      b_revert = XmCreatePushButton (bform, "Revert", args, n);
      XtManageChild (b_revert);
      XtAddCallback (b_revert, XmNactivateCallback,
		     (XtCallbackProc) attributes_revert_callback,
		     NULL);

      n = 0;
      b_cancel = XmCreatePushButton (bform, "Cancel", args, n);
      XtManageChild (b_cancel);
      XtAddCallback (b_cancel, XmNactivateCallback,
		     (XtCallbackProc) dialog_callback,
		     (XtPointer) 1);

      n = 0;
      stdarg (XmNleftAttachment, XmATTACH_FORM);
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      stdarg (XmNbottomAttachment, XmATTACH_WIDGET);
      stdarg (XmNbottomWidget, bform);
      sep = XmCreateSeparator (attr_dialog, "attributes", args, n);
      XtManageChild (sep);

      n = 0;
      stdarg (XmNtopAttachment, XmATTACH_FORM);
      stdarg (XmNleftAttachment, XmATTACH_FORM);
      stdarg (XmNrightAttachment, XmATTACH_FORM);
      stdarg (XmNbottomAttachment, XmATTACH_WIDGET);
      stdarg (XmNbottomWidget, sep);
      stdarg (XmNscrollingPolicy, XmAUTOMATIC);
      sw = XmCreateScrolledWindow (attr_dialog, "attributes", args, n);
      XtManageChild (sw);

      n = 0;
      stdarg (XmNmarginHeight, 0);
      stdarg (XmNmarginWidth, 0);
      f_top = XmCreateBulletinBoard (sw, "f_top", args, n);
      XtManageChild (f_top);
    }
  else
    {
      n = 0;
      stdarg (XmNtitle, owner);
      XtSetValues (XtParent (attr_dialog), args, n);
    }

  attributes_list = attrs_list;
  lesstif_attributes_revert ();

  fiddle_with_bb_layout ();

  if (wait_for_dialog (attr_dialog) == 0)
    {
      int i;
      /* Copy the values back */
      for (i=0; i<attributes_list->Number; i++)
	{
	  if (attributes_list->List[i].name)
	    free (attributes_list->List[i].name);
	  if (attributes_list->List[i].value)
	    free (attributes_list->List[i].value);
	}
      if (attributes_list->Max < attr_num_rows)
	{
	  int sz = attr_num_rows * sizeof (AttributeType);
	  if (attributes_list->List == NULL)
	    attributes_list->List = (AttributeType *) malloc (sz);
	  else
	    attributes_list->List = (AttributeType *) realloc (attributes_list->List, sz);
	  attributes_list->Max = attr_num_rows;
	}
      for (i=0; i<attr_num_rows; i++)
	{
	  attributes_list->List[i].name = strdup (XmTextFieldGetString (attr_row[i].w_name));
	  attributes_list->List[i].value = strdup (XmTextFieldGetString (attr_row[i].w_value));
	  attributes_list->Number = attr_num_rows;
	}
    }

  return;
}

/* ------------------------------------------------------------ */

static const char importgui_syntax[] =
"ImportGUI()";

static const char importgui_help[] =
"Lets the user choose the schematics to import from";

/* %start-doc actions ImportGUI

Displays a dialog that lets the user select the schematic(s) to import
from, then saves that information in the layout's attributes for
future imports.

%end-doc */

static int
ImportGUI (int argc, char **argv, Coord x, Coord y)
{
  static int I_am_recursing = 0;
  static XmString xms_sch = 0, xms_import = 0;
  int rv;
  XmString xmname;
  char *name, *bname;
  char *original_dir, *target_dir, *last_slash;

  if (I_am_recursing)
    return 1;

  if (xms_sch == 0)
    xms_sch = XmStringCreatePCB ("*.sch");
  if (xms_import == 0)
    xms_import = XmStringCreatePCB ("Import from");

  setup_fsb_dialog ();

  n = 0;
  stdarg (XmNtitle, "Import From");
  XtSetValues (XtParent (fsb), args, n);

  n = 0;
  stdarg (XmNpattern, xms_sch);
  stdarg (XmNmustMatch, True);
  stdarg (XmNselectionLabelString, xms_import);
  XtSetValues (fsb, args, n);

  n = 0;
  stdarg (XmNdirectory, &xmname);
  XtGetValues (fsb, args, n);
  XmStringGetLtoR (xmname, XmFONTLIST_DEFAULT_TAG, &original_dir);

  if (!wait_for_dialog (fsb))
    return 1;

  n = 0;
  stdarg (XmNdirectory, &xmname);
  XtGetValues (fsb, args, n);
  XmStringGetLtoR (xmname, XmFONTLIST_DEFAULT_TAG, &target_dir);

  n = 0;
  stdarg (XmNdirSpec, &xmname);
  XtGetValues (fsb, args, n);

  XmStringGetLtoR (xmname, XmFONTLIST_DEFAULT_TAG, &name);

  /* If the user didn't change directories, use just the base name.
     This is the common case and means we don't have to get clever
     about converting absolute paths into relative paths.  */
  bname = name;
  if (strcmp (original_dir, target_dir) == 0)
    {
      last_slash = strrchr (name, '/');
      if (last_slash)
	bname = last_slash + 1;
    }

  AttributePut (PCB, "import::src0", bname);

  XtFree (name);


  I_am_recursing = 1;
  rv = hid_action ("Import");
  I_am_recursing = 0;

  return rv;
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
  {"PrintCalibrate", 0, PrintCalibrate,
   printcalibrate_help, printcalibrate_syntax},
  {"Export", 0, Export,
   export_help, export_syntax},
  {"AdjustSizes", 0, AdjustSizes,
   adjustsizes_help, adjustsizes_syntax},
  {"EditLayerGroups", 0, EditLayerGroups,
   editlayergroups_help, editlayergroups_syntax},
  {"ImportGUI", 0, ImportGUI,
   importgui_help, importgui_syntax},
};

REGISTER_ACTIONS (lesstif_dialog_action_list)
