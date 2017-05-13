/* -*- mode: C; c-file-style: "gnu" -*- */

/*!
 * \file src/dbus-pcbmain.c
 *
 * \brief PCB HID main loop integration.
 *
 * Adapted from dbus-gmain.c from dbus-glib bindings:
 *
 * Copyright (C) 2002, 2003 CodeFactory AB
 *
 * Copyright (C) 2005 Red Hat, Inc.
 *
 * Licensed under the Academic Free License version 2.1
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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <stdio.h>

#include "global.h"
#include "dbus-pcbmain.h"

typedef struct _IOWatchHandler IOWatchHandler;
typedef struct _TimeoutHandler TimeoutHandler;

struct _IOWatchHandler
{
  DBusWatch *dbus_watch;
  hidval pcb_watch;
};

struct _TimeoutHandler
{
  DBusTimeout *dbus_timeout;
  hidval pcb_timer;
  int interval;
};


static void
block_hook_cb (hidval data)
{
  DBusConnection *connection = (DBusConnection *) data.ptr;
  if (dbus_connection_get_dispatch_status (connection) !=
      DBUS_DISPATCH_DATA_REMAINS)
    return;

  // TODO: IS THIS NEEDED?
  // dbus_connection_ref (connection);

  /* Only dispatch once - we don't want to starve other mainloop users */
  dbus_connection_dispatch (connection);

  // dbus_connection_unref (connection);
  return;
}

static void
io_watch_handler_dbus_freed (void *data)
{
  IOWatchHandler *handler;
  handler = (IOWatchHandler *)data;

  // Remove the watch registered with the HID
  gui->unwatch_file (handler->pcb_watch);
  free (handler);
}


void
io_watch_handler_cb (hidval pcb_watch,
		     int fd, unsigned int condition, hidval data)
{
  IOWatchHandler *handler;
  unsigned int dbus_condition = 0;

  handler = (IOWatchHandler *) data.ptr;

  // TODO: IS THIS NEEDED?
  //if (connection)
  //  dbus_connection_ref (connection);

  if (condition & PCB_WATCH_READABLE)
    dbus_condition |= DBUS_WATCH_READABLE;
  if (condition & PCB_WATCH_WRITABLE)
    dbus_condition |= DBUS_WATCH_WRITABLE;
  if (condition & PCB_WATCH_ERROR)
    dbus_condition |= DBUS_WATCH_ERROR;
  if (condition & PCB_WATCH_HANGUP)
    dbus_condition |= DBUS_WATCH_HANGUP;

  /* We don't touch the handler after this, because DBus
   * may have disabled the watch and thus killed the handler
   */
  dbus_watch_handle (handler->dbus_watch, dbus_condition);
  handler = NULL;

  //if (connection)
  //  dbus_connection_unref (connection);

  return;
}


static void
timeout_handler_dbus_freed (void *data)
{
  TimeoutHandler *handler;
  handler = (TimeoutHandler *)data;

  // Remove the timeout registered with the HID
  gui->stop_timer (handler->pcb_timer);
  free (handler);
}


void
timeout_handler_cb (hidval data)
{
  TimeoutHandler *handler;
  handler = (TimeoutHandler *)data.ptr;

  // Re-add the timeout, as PCB will remove the current one
  // Do this before calling to dbus, incase DBus removes the timeout.
  // We can't touch handler after libdbus has been run for this reason.
  handler->pcb_timer =
    gui->add_timer (timeout_handler_cb, handler->interval, data);

  dbus_timeout_handle (handler->dbus_timeout);
}


static dbus_bool_t
watch_add (DBusWatch * dbus_watch, void *data)
{
  IOWatchHandler *handler;
  int fd;
  unsigned int pcb_condition;
  unsigned int dbus_flags;
  hidval temp;

  // We won't create a watch until it becomes enabled.
  if (!dbus_watch_get_enabled (dbus_watch))
    return TRUE;

  dbus_flags = dbus_watch_get_flags (dbus_watch);

  pcb_condition = PCB_WATCH_ERROR | PCB_WATCH_HANGUP;
  if (dbus_flags & DBUS_WATCH_READABLE)
    pcb_condition |= PCB_WATCH_READABLE;
  if (dbus_flags & DBUS_WATCH_WRITABLE)
    pcb_condition |= PCB_WATCH_READABLE;

#if HAVE_DBUS_WATCH_GET_UNIX_FD
  fd = dbus_watch_get_unix_fd (dbus_watch);
#else
  fd = dbus_watch_get_fd (dbus_watch);
#endif

  handler = (IOWatchHandler *)malloc (sizeof (IOWatchHandler));
  temp.ptr = (void *)handler;
  handler->dbus_watch = dbus_watch;
  handler->pcb_watch =
    gui->watch_file (fd, pcb_condition, io_watch_handler_cb, temp);

  dbus_watch_set_data (dbus_watch, handler, io_watch_handler_dbus_freed);
  return TRUE;
}

static void
watch_remove (DBusWatch * dbus_watch, void *data)
{
  // Free the associated data. Its destroy callback removes the watch
  dbus_watch_set_data (dbus_watch, NULL, NULL);
}

static void
watch_toggled (DBusWatch * dbus_watch, void *data)
{
  /* Simply add/remove the watch completely */
  if (dbus_watch_get_enabled (dbus_watch))
    watch_add (dbus_watch, data);
  else
    watch_remove (dbus_watch, data);
}


static dbus_bool_t
timeout_add (DBusTimeout * timeout, void *data)
{
  TimeoutHandler *handler;
  hidval temp;

  // We won't create a timeout until it becomes enabled.
  if (!dbus_timeout_get_enabled (timeout))
    return TRUE;

  //FIXME: Need to store the interval, as PCB requires us
  //       to manually re-add the timer each time it expires.
  //       This is non-ideal, and hopefully can be changed?

  handler = (TimeoutHandler *)malloc (sizeof (TimeoutHandler));
  temp.ptr = (void *)handler;
  handler->dbus_timeout = timeout;
  handler->interval = dbus_timeout_get_interval (timeout);
  handler->pcb_timer =
    gui->add_timer (timeout_handler_cb, handler->interval, temp);

  dbus_timeout_set_data (timeout, handler, timeout_handler_dbus_freed);
  return TRUE;
}

static void
timeout_remove (DBusTimeout * timeout, void *data)
{
  // Free the associated data. Its destroy callback removes the timer
  dbus_timeout_set_data (timeout, NULL, NULL);
}

static void
timeout_toggled (DBusTimeout * timeout, void *data)
{
  /* Simply add/remove the timeout completely */
  if (dbus_timeout_get_enabled (timeout))
    timeout_add (timeout, data);
  else
    timeout_remove (timeout, data);
}

void
dispatch_status_changed (DBusConnection * conn, DBusDispatchStatus new_status,
			 void *data)
{
  // TODO: Can use this eventually to add one-shot idle work-functions to dispatch
  //       remaining IO. It could possibly replace the block_hook polling mechanism.
  //       (We could use a one-shot block_book to dispatch the work though.)
  //
  //       *** NO DISPATCHING TO BE DONE INSIDE THIS FUNCTION ***
}

// END INTERNALS


/**
 * Sets the watch and timeout functions of a #DBusConnection
 * to integrate the connection with the GUI HID's main loop.
 *
 * @param connection the connection
 */
void
pcb_dbus_connection_setup_with_mainloop (DBusConnection * connection)
{
  //ConnectionSetup *cs;
  hidval temp;

  /* FIXME we never free the slot, so its refcount just keeps growing,
   * which is kind of broken.
   */
  //dbus_connection_allocate_data_slot (&connection_slot);
  //if (connection_slot < 0)
  //  goto nomem;

#if 0
  cs = connection_setup_new (connection);

  if (!dbus_connection_set_data (connection, connection_slot, cs,
				 (DBusFreeFunction) connection_setup_free))
    goto nomem;
#endif

  if (!dbus_connection_set_watch_functions (connection,
					    watch_add,
					    watch_remove,
					    watch_toggled, NULL, NULL))
//                                            cs, NULL))
    goto nomem;

  if (!dbus_connection_set_timeout_functions (connection,
					      timeout_add,
					      timeout_remove,
					      timeout_toggled, NULL, NULL))
//                                              cs, NULL))
    goto nomem;

  dbus_connection_set_dispatch_status_function (connection,
						dispatch_status_changed,
						NULL, NULL);
//                                                cs, NULL);

  /* Register a new mainloop hook to mop up any unfinished IO. */
  temp.ptr = (void *)connection;
  gui->add_block_hook (block_hook_cb, temp);

  return;
nomem:
  fprintf (stderr,
	   "Not enough memory to set up DBusConnection for use with PCB\n");
}

void
pcb_dbus_connection_finish_with_mainloop (DBusConnection * connection)
{
  //ConnectionSetup *cs;

  //cs = dbus_connection_get_data (connection, connection_slot );

  // Replace the stored data with NULL, thus freeing the old data
  // DBus will call the function connection_setup_free() which we registered earlier
  //dbus_connection_set_data (connection, connection_slot, NULL, NULL );

  //dbus_connection_free_data_slot( &connection_slot );

  if (!dbus_connection_set_watch_functions (connection,
					    NULL, NULL, NULL, NULL, NULL))
    goto nomem;

  if (!dbus_connection_set_timeout_functions (connection,
					      NULL, NULL, NULL, NULL, NULL))
    goto nomem;

  dbus_connection_set_dispatch_status_function (connection, NULL, NULL, NULL);
  return;
nomem:
  fprintf (stderr,
	   "Not enough memory when cleaning up DBusConnection mainloop integration\n");

}
