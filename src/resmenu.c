/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <X11/Intrinsic.h>

#include "global.h"

#include "resource.h"
#include "action.h"
#include "check_icon.data"
#include "error.h"
#include "mymem.h"

#include <X11/cursorfont.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID("$Id$");

static Arg args[100];
static int n;
#define arg(a,b) XtSetArg(args[n], a, b); n++

/* These are in actionlist.c */
extern struct {
  char *name;
  int (*func)(int);
  int parm;
} FlagFuncList[];
extern int FlagFuncListSize;

/* ************************************************************ */

#define AT_activate 0
#define AT_create 'c'
#define AT_popup 'p'

extern struct {
  char *name;
  int type;
} ActionTypeList[];

static int
action_type (char *str)
{
  int i;
  for (i=0; ActionTypeList[i].name; i++)
    {
      int len = strlen(ActionTypeList[i].name);
      if (strncmp (str, ActionTypeList[i].name, len) == 0
	  && str[len] == '(')
	{
	  /*	  printf("dj: action_type for `%s' is %d\n", str, ActionTypeList[i].type);*/
	  return ActionTypeList[i].type;
	}
    }
  /*printf("dj: action_type for `%s' defaults to AT_activate\n", str);*/
  return AT_activate;
}

static char *
invoke_action (Widget w, char *rstr)
{
  static char **list = 0;
  static int max = 0;
  int num = 0;
  static char *str = NULL;
  char *sp, *aname;
  int maybe_empty = 0;

  if (str != NULL)
    {
      free (str);
      str = NULL;
    }

  sp = str = MyStrdup(rstr, "invoke_action");

  /* eat leading spaces and tabs */
  while (*sp && (*sp == ' ' || *sp == '\t'))
    sp ++;

  aname = sp;

  /* search for the leading ( */
  while (*sp && *sp != '(')
    sp++;

  /*
   * we didn't find a leading ( so invoke the action
   * with no parameters or event.
   */
  if (! *sp)
    {
      XtCallActionProc(w, aname, 0, 0, 0);
      return 0;
    }

  /* 
   * we found a leading ( so see if we have parameters to pass to the
   * action 
   */
  *sp++ = 0;
  while (1) {
    /* 
     * maybe_empty == 0 means that the last char examined was not a
     * "," 
     */
    if (*sp == ')' && !maybe_empty)
      {
	*sp = 0;
	XtCallActionProc(w, aname, 0, list, num);
	return sp+1;
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
	      list = (char **)realloc(list, max * sizeof(char *));
	    else
	      list = (char **)malloc(max * sizeof(char *));
	  }
	list[num++] = sp;

	/* search for a "," or a ")" */
	while (*sp && *sp != ',' && *sp != ')')
	  sp++;
	if (*sp == ',')
	  {
	    maybe_empty = 1;
	    *sp++ = 0;
	  }
      }
  }

  return 0;
}

static void
invoke_multiple_actions (Widget w, char *string)
{
  char *sp = string;;
  while (sp)
    {
      while (*sp == ' ' || *sp == '\t')
	*sp ++;
      if (! *sp)
	return;
      sp = invoke_action (w, sp);
    }
}

/* ************************************************************ */

/* ACTION(ExecuteAction,ActionExecuteAction) */

void
ActionExecuteAction(Widget W, XEvent *Event, String *Params, Cardinal *num)
{
  int i;
  for (i=0; i<*num; i++)
    invoke_multiple_actions (W, Params[i]);
}

/* ACTION(ExecuteFile,ActionExecuteFile) */

void
ActionExecuteFile(Widget W, XEvent *Event, String *Params, Cardinal *num)
{
  FILE *fp;
  char *fname;
  char line[256];
  int n=0;
  char *sp;

  if (*num != 1)
  {
	Message("Usage:  ExecuteFile(filename)");
	return ;
  }

  fname = Params[0];

  if ( (fp = fopen(fname, "r")) == NULL )
    {
      fprintf(stderr, "Could not open actions file \"%s\".\n", fname);
      return ;
    }

  while ( fgets(line, sizeof(line), fp) != NULL )
    {
      n++;
      sp = line;
      
      /* eat the trailing newline */
      while (*sp && *sp != '\r' && *sp != '\n')
	sp ++;
      *sp = '\0';
  
      /* eat leading spaces and tabs */
      sp = line;
      while (*sp && (*sp == ' ' || *sp == '\t'))
	sp ++;
  
      /* 
       * if we have anything left and its not a comment line
       * then execute it
       */

      if (*sp && *sp != '#') 
	{
	  Message("%s : line %-3d : \"%s\"\n", fname, n, sp);
	  /* printf ("%s : line %-3d : \"%s\"\n", fname, n, sp); */
	  invoke_multiple_actions(W, sp);
	}
    }
  
  fclose(fp);
}

/* ************************************************************ */

typedef struct RMFlagHash {
  struct RMFlagHash *next;
  char *flag;
  int value;
  int (*func)(int parm);
  int funcparm;
  int serialno;
} RMFlagHash;

static RMFlagHash *flag_hash[256];
static int serialno = 0;

static int
hash_key(char *flag)
{
  int k = 0;
  while (*flag)
    {
      k *= 13;
      k += (unsigned char)*flag;
      flag++;
    }
  return k % (sizeof(flag_hash)/sizeof(flag_hash[0]));
}

static void
hash_set(char *flag, int value)
{
  int k = hash_key(flag);
  RMFlagHash *fh;
  for (fh=flag_hash[k]; fh && strcmp(fh->flag, flag); fh=fh->next);
  if (!fh)
    {
      fh = (RMFlagHash *)malloc(sizeof(RMFlagHash));
      fh->next = flag_hash[k];
      flag_hash[k] = fh;
      fh->flag = flag;
      fh->serialno = -1;
    }
  fh->value = value;
}

static int
hash_get(char *flag)
{
  static int initted = 0;
  int k;
  RMFlagHash *fh;

  if (!initted)
    {
      int i;
      initted = 1;
      for (i=0; i<FlagFuncListSize; i++)
	{
	  k = hash_key(FlagFuncList[i].name);
	  fh = (RMFlagHash *)malloc(sizeof(RMFlagHash));
	  fh->next = flag_hash[k];
	  flag_hash[k] = fh;
	  fh->flag = FlagFuncList[i].name;
	  fh->func = FlagFuncList[i].func;
	  fh->funcparm = FlagFuncList[i].parm;
	  fh->serialno = -1;
	}
    }

  k = hash_key(flag);
  for (fh=flag_hash[k]; fh && strcmp(fh->flag, flag); fh=fh->next);
  if (!fh)
    return 0;
  if (fh->func && fh->serialno != serialno)
    {
      fh->value = fh->func(fh->funcparm);
      fh->serialno = serialno;
    }
  return fh->value;
}

void
MenuSetFlag(char *flag, int value)
{
  hash_set(flag, value);
}

static int
xboolean_true(Cardinal argc, String *argv)
{
  int x;
  if (argc < 1)
    return 1;
  x = hash_get(argv[0]);
  if (argc < 2)
    return x ? 1 : 0;
  if (strcasecmp (argv[1], "true") == 0)
    return x ? 1 : 0;
  if (strcasecmp (argv[1], "false") == 0)
    return x ? 0 : 1;
  return x == strtol(argv[1], 0, 0) ? 1 : 0;
}
static int
boolean_true(Cardinal argc, String *argv)
{
  int rv;
  /*printf("boolean_true(%s,%s) = ", argc>0?argv[0]:"(null)", argc>1?argv[1]:"(null)");*/
  rv = xboolean_true(argc, argv);
  /*printf("%d\n", rv);*/
  return rv;
}

void
ActionCheckWhen (Widget w, XEvent * e, String * argv, Cardinal * argc)
{
  static Pixmap check_pixmap = BadAlloc;
  int v = boolean_true(*argc, argv);

  if (check_pixmap == BadAlloc)
    check_pixmap = XCreateBitmapFromData (XtDisplay(XtParent(w)),
					  RootWindowOfScreen (XtScreen (XtParent(w))),
					  (char *) check_icon_bits, check_icon_width,
					  check_icon_height);

  if (v)
    XtVaSetValues (w, XtNleftBitmap, check_pixmap, NULL);
  else
    XtVaSetValues (w, XtNleftBitmap, None, NULL);
}

/* ACTION(CheckWhen,ActionCheckWhen) POPUP */

void
ActionActiveWhen (Widget w, XEvent * e, String * argv, Cardinal * argc)
{
  int v = boolean_true(*argc, argv);
  n = 0;
  arg(XtNsensitive, v);
  XtSetValues(w, args, n);
}

/* ACTION(ActiveWhen,ActionActiveWhen) POPUP */

static void
MenuPopupCallback(Widget w, Resource *res, void *pbcs)
{
  int i, j;

  /*printf("popup(%s) has %d subs\n", XtName(w), res->c);*/
  serialno ++;

  for (i=0; i<res->c; i++)
    {
      if (res->v[i].subres && res->v[i].subres->user_ptr)
	{
	  Widget sub = (Widget)res->v[i].subres->user_ptr;

	  for (j=1; j<res->v[i].subres->c; j++)
	    {
	      switch (resource_type(res->v[i].subres->v[j]))
		{
		case 10: /* unnamed value = callback, check for special ones */
		  if (action_type(res->v[i].subres->v[j].value) == AT_popup)
		    invoke_action (sub, res->v[i].subres->v[j].value);
		  break;
		}
	    }
	}
    }
}

/* ************************************************************ */

static void
callback(Widget w, Resource *res, void *pbcs)
{
  int vi;
  char *cmd = resource_value(res, "cmd");
  if (cmd)
    {
      char *fmt = "%s < /dev/null &";
      char *tmp = (char *)malloc(strlen(fmt)+strlen(cmd)+2);
      sprintf(tmp, fmt, cmd);
      system(cmd);
    }
  for (vi=1; vi<res->c; vi++)
    if (resource_type(res->v[vi]) == 10)
      if (action_type(res->v[vi].value) == AT_activate)
	invoke_action (w, res->v[vi].value);
}

void
MenuSetRight(Widget menu, const char *string)
{
  Pixmap rb;
  XFontStruct *font;
  int height, width;
  int dir, ascent, descent;
  XCharStruct overall;
  static GC gc = 0;
  Display *display = XtDisplay (XtParent (menu));
  Window window = XDefaultRootWindow (display);

#define SPACE_BETWEEN 10
#define SPACE_RIGHT 5

  n = 0;
  arg(XtNfont, &font);
  XtGetValues(menu, args, n);

  XTextExtents (font, string, strlen(string), &dir, &ascent, &descent, &overall);
  height = ascent + descent;
  width = overall.lbearing + overall.rbearing + SPACE_BETWEEN;

  rb = XCreatePixmap(display, window, width, height, 1);

  if (gc == 0)
    gc = XCreateGC (display, rb, 0, 0);

  XSetFont (display, gc, font->fid);
  XSetForeground (display, gc, 0);
  XFillRectangle (display, rb, gc, 0, 0, width, height);
  XSetForeground (display, gc, 1);
  XDrawString (display, rb, gc, overall.lbearing + SPACE_BETWEEN, ascent, string, strlen(string));

  n = 0;
  arg(XtNrightBitmap, rb);
  arg(XtNrightMargin, width + 8);
  XtSetValues(menu, args, n);
}

static char *
append (char *str, char *add)
{
  int len = strlen (add);
  int sl;
  if (str)
    {
      sl = strlen(str);
      str = (char *)realloc (str, sl + len + 1);
    }
  else
    {
      sl = 0;
      str = (char *)malloc (len + 1);
    }
  memcpy (str + sl, add, len+1);
  return str;
}

static char *accel = 0;

static int
accel_cmp (const void *va, const void *vb)
{
  char *a = *(char **)va;
  char *b = *(char **)vb;

  /* This is odd, but we want #override first and Mod<Key> before
     <Key>.  */
  if (*a == '#' || *b == '#')
    return strcmp (a, b);
  return strcmp (b, a);
}

void
MenuSetAccelerators (Widget w)
{
  int na, i;
  char **alist, *cp;
  char *tmpa;

  XtAccelerators a;
  if (!accel)
    return;

  tmpa = MyStrdup (accel, "MenuSetAccelerators");
  for (na=1, cp = accel; *cp; cp ++)
    if (*cp == '\n')
      na ++;
  alist = (char **) malloc (na * sizeof (char *));
  alist[0] = tmpa;
  for (na=1, cp = tmpa; *cp; cp ++)
    if (*cp == '\n')
      {
	*cp = 0;
	alist[na] = cp+1;
	na++;
      }
  qsort (alist, na, sizeof(alist[0]), accel_cmp);
  accel[0] = 0;
  for (i=0; i<na; i++)
    {
      if (i)
	strcat(accel, "\n");
      strcat(accel, alist[i]);
    }
  free(tmpa);
  free(alist);

  a = XtParseAcceleratorTable(accel);
  n = 0;
  arg (XtNaccelerators, a);
  XtSetValues(w, args, n);
  XtInstallAccelerators (w, w);
}

Widget
MenuCreateFromResource(Widget menu, Resource *res, Widget top, Widget left, int chain)
{
  int i, j;
  char *v, *val;
  Widget sub, btn;

  if (accel == 0)
    accel = append (accel, "#override");

  for (i=0; i<res->c; i++)
    {
      switch (resource_type(res->v[i]))
	{
	case 101: /* named subres = submenu */
	  res->v[i].subres->user_ptr = 0;
#if 0
	  n = 0;
	  sub = XmCreatePulldownMenu(menu, "menu", args, n);
	  XtSetValues(sub, args, n);
	  XtManageChild(sub);
	  n = 0;
	  arg(XtNsubMenuId, sub);
	  btn = XmCreateCascadeButton(menu, res->v[i].name, args, n);
	  XtManageChild(btn);
	  MenuCreateFromResource(sub, res->v[i].subres, 0, 0, 0);
#endif
	  break;

	case 1: /* unnamed subres = submenu or button */
	  n = 0;
	  res->v[i].subres->user_ptr = 0;
	  if (chain)
	    {
	      arg(XtNfromHoriz, left);
	      arg(XtNfromVert, top);
	      arg(XtNleft, XtChainLeft);
	      arg(XtNright, XtChainLeft);
	      arg(XtNtop, XtChainTop);
	      arg(XtNbottom, XtChainTop);
	    }
	  arg(XtNleftMargin, check_icon_width + 4);
	  arg(XtNsensitive, True);
	  /* This one must be set at create time.  */
	  val = resource_value(res->v[i].subres, "vertSpace");
	  if (val)
	    {
	      arg(XtNvertSpace, atoi(val));
	    }
	  v = "button";
	  for (j=0; j<res->v[i].subres->c; j++)
	    if (resource_type(res->v[i].subres->v[j]) == 10)
	      {
		v = res->v[i].subres->v[j].value;
		break;
	      }
	  arg(XtNlabel, v);
	  if (res->v[i].subres->flags & FLAG_S)
	    {
	      static int menu_id=1;
	      char menu_name[20], *mn;
	      int nn = n;

	      sprintf(menu_name, "menu%d", menu_id++);
	      mn = strdup(menu_name);
	      sub = XtCreateManagedWidget(mn, simpleMenuWidgetClass,
					  menu, args+nn, n-nn);
	      n = nn;
	      arg(XtNmenuName, mn);
	      btn = XtCreateManagedWidget(v, menuButtonWidgetClass,
					  menu, args, n);
	      MenuCreateFromResource(sub, res->v[i].subres, 0, 0, 0);
	      XtAddCallback(sub, XtNpopupCallback,
			    (XtCallbackProc)MenuPopupCallback, (XtPointer)res->v[i].subres);
	    }
	  else
	    {
	      Resource *r, *me;

	      btn = XtCreateManagedWidget(v, smeBSBObjectClass,
					  menu, args, n);
	      res->v[i].subres->user_ptr = btn;
	      XtAddCallback(btn, XtNcallback,
			    (XtCallbackProc)callback, (XtPointer)res->v[i].subres);

	      r = resource_subres (res->v[i].subres, "a");
	      if (r && r->c >= 1)
		{
		  int vi;
		  MenuSetRight (btn, r->v[0].value);

		  if (r->c >= 2)
		    {
		      me = res->v[i].subres;
		      if (accel)
			accel = append (accel, "\n");
		      accel = append (accel, r->v[1].value);
		      accel = append (accel, ":");
		      for (vi=1; vi<me->c; vi++)
			if (resource_type(me->v[vi]) == 10)
			  if (action_type(me->v[vi].value) == AT_activate
			      && strncmp(me->v[vi].value, "GetXY(", 6) != 0)
			    {
			      accel = append (accel, " ");
			      accel = append (accel, me->v[vi].value);
			    }
		    }
		}
	    }
	  for (j=1; j<res->v[i].subres->c; j++)
	    {
	      char *n;
	      switch (resource_type(res->v[i].subres->v[j]))
		{
		case 110: /* named value = X resource */
		  n = res->v[i].subres->v[j].name;
		  if (strcmp(n, "fg") == 0)
		    n = "foreground";
		  XtVaSetValues(btn, XtVaTypedArg,
				n,
				XtRString,
				res->v[i].subres->v[j].value,
				strlen(res->v[i].subres->v[j].value)+1,
				NULL);
		  break;
		case 101: /* named subres = special hooks */
		  break;
		case 10: /* unnamed value = callback, check for special ones */
		  if (action_type(res->v[i].subres->v[j].value) == AT_create)
		    invoke_action (btn, res->v[i].subres->v[j].value);
		  break;
		}
	    }
	  if (chain)
	    left = btn;
	  break;

	case 10: /* unnamed value = decoration */
	  n = 0;
	  if (strcmp(res->v[i].value, "-") == 0)
	    {
	      btn = XtCreateManagedWidget("sep", smeLineObjectClass,
					  menu, args, n);
	    }
	  else if (i > 0)
	    {
	      btn = XtCreateManagedWidget(res->v[i].value, menuButtonWidgetClass,
					  menu, args, n);
	    }
	  break;
	}
    }

  return left;
}
