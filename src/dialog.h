/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 *  RCS: $Id$
 */

/* prototypes for dialog routines
 */

#ifndef	__DIALOG_INCLUDED__
#define	__DIALOG_INCLUDED__

#include "global.h"

/* ---------------------------------------------------------------------------
 * default return codes for buttons
 */
#define	CANCEL_BUTTON	0
#define	OK_BUTTON		1
#define	ALL_BUTTON		2

/* ---------------------------------------------------------------------------
 * some useful types
 */
typedef struct						/* a dialogs buttons */
{
	char			*Name,			/* the widgets name */
					*Label;			/* the buttons text */
	XtCallbackProc	Callback;		/* the buttons select-handler */
	XtPointer		ClientData;		/* data passed to the handler */
	Widget			W;				/* the button widget itself */
} DialogButtonType, *DialogButtonTypePtr;

 
long int	DialogEventLoop(long int *);
Widget		AddButtons(Widget, Widget, DialogButtonTypePtr, size_t);
Widget		CreateDialogBox(char *, DialogButtonTypePtr, size_t, char *);
void		StartDialog(Widget);
void		EndDialog(Widget);
void		MessageDialog(char *);
void		MessagePrompt(char *);
void		FinishInputDialog(Boolean);
char        *GetUserInput(char *, char *);
void		AboutDialog(void);
Boolean		GetOKForDataLoss(void);
Boolean		ConfirmDialog(char *);
int			ConfirmReplaceFileDialog(char *, Boolean);

#endif
