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
#include "mymem.h"

#include <X11/cursorfont.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>

static Display *display=0;
static int screen;
static Colormap cmap;

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

static void
invoke_action (Widget w, char *rstr)
{
  static char **list = 0;
  static int max = 0;
  int num = 0;
  char *str = 0;
  char *sp, *aname;
  int maybe_empty = 0;

  if (str)
    free(str);

  sp = str = MyStrdup(rstr, __FUNCTION__);
  while (*sp && (*sp == ' ' || *sp == '\t'))
    sp ++;
  aname = sp;
  while (*sp && *sp != '(')
    sp++;
  if (! *sp)
    {
      XtCallActionProc(w, aname, 0, 0, 0);
      return;
    }
  *sp++ = 0;
  while (1) {
    if (*sp == ')' && !maybe_empty)
      {
	int i;
	*sp = 0;
	XtCallActionProc(w, aname, 0, list, num);
	break;
      }
    else if (*sp == 0 && !maybe_empty)
      break;
    else
      {
	maybe_empty = 0;
	if (num <= max)
	  {
	    max += 10;
	    if (list)
	      list = (char **)realloc(list, max * sizeof(char *));
	    else
	      list = (char **)malloc(max * sizeof(char *));
	  }
	list[num++] = sp;
	while (*sp && *sp != ',' && *sp != ')')
	  sp++;
	if (*sp == ',')
	  {
	    maybe_empty = 1;
	    *sp++ = 0;
	  }
      }
  }
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
					  check_icon_bits, check_icon_width,
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

Widget
MenuCreateFromResource(Widget menu, Resource *res, Widget top, Widget left, int chain)
{
  int i, j;
  char *v, *val;
  Widget sub, btn;
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
	      char menu_name[20];
	      int nn = n;
	      sprintf(menu_name, "menu%d", menu_id++);
	      sub = XtCreateManagedWidget(menu_name, simpleMenuWidgetClass,
					  menu, args+nn, n-nn);
	      n = nn;
	      arg(XtNmenuName, strdup(menu_name));
	      btn = XtCreateManagedWidget(v, menuButtonWidgetClass,
					  menu, args, n);
	      MenuCreateFromResource(sub, res->v[i].subres, 0, 0, 0);
	      XtAddCallback(sub, XtNpopupCallback,
			    (XtCallbackProc)MenuPopupCallback, (XtPointer)res->v[i].subres);
	    }
	  else
	    {
	      btn = XtCreateManagedWidget(v, smeBSBObjectClass,
					  menu, args, n);
	      res->v[i].subres->user_ptr = btn;
	      XtAddCallback(btn, XtNcallback,
			    (XtCallbackProc)callback, (XtPointer)res->v[i].subres);
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
