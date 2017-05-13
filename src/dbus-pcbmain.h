/*!
 * \file src/dbus-pacbmain.h
 *
 * \brief D-Bus IPC logic
 *
 * PCB, an interactive printed circuit board editor
 *
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef PCB_DBUS_PCBMAIN_H
#define PCB_DBUS_PCBMAIN_H

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

void pcb_dbus_connection_setup_with_mainloop (DBusConnection * connection);
void pcb_dbus_connection_finish_with_mainloop (DBusConnection * connection);

#endif /* !PCB_DBUS_PCBMAIN_H */
