/*
 * PCB, an interactive printed circuit board editor
 * D-Bus IPC logic
 * Copyright (C) 2006 University of Cambridge
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef PCB_DBUS_H
#define PCB_DBUS_H

/* Carry out all actions to setup the D-Bus and register appropriate callbacks */
void pcb_dbus_setup ();

/* Carry out all actions to finalise the D-Bus connection */
void pcb_dbus_finish ();


#endif /* !PCB_DBUS_H */
