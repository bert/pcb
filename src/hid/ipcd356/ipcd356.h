/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *
 *  IPC-D-356 Netlist export
 *  Copyright (C) 2012, 2013 Jerome Marchand (Jerome.Marchand@gmail.com)
 *  Copyright (C) 2012, 2013 PCB Contributors (see ChangeLog for details)
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 *
 */


typedef struct
{
  char NName[11];   /* Per IPC Spec: Alias is 'NNAME' + 5 Chars, (+Termination) */
  char * NetName;
} IPCD356_Alias;

typedef struct
{
  int AliasN;           /* number of entries */
  IPCD356_Alias *Alias;
} IPCD356_AliasList;

void IPCD356_WriteNet (FILE *, char *);
void IPCD356_WriteHeader (FILE *);
void IPCD356_End (FILE *);
int IPCD356_Netlist (void);
int IPCD356_WriteAliases (FILE *, IPCD356_AliasList *);
void ResetVisitPinsViasAndPads (void);
void CheckNetName (char *, IPCD356_AliasList *);
IPCD356_AliasList * CreateAliasList (void);
IPCD356_AliasList * AddAliasToList (IPCD356_AliasList * aliaslist);
int IPCD356_SanityCheck(void);

/* EOF */
