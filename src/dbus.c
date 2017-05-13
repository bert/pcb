/*!
 * \file src/dbus.c
 *
 * \brief D-Bus IPC logic
 *
 * D-Bus code originally derrived from example-service.c in the
 * dbus-glib bindings.
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

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <string.h>

#include "dbus.h"
#include "dbus-pcbmain.h"
#include "dbus-introspect.h"
#include "global.h"
#include "data.h"

/* For lrealpath */
#include "lrealpath.h"


#define PCB_DBUS_CANONICAL_NAME    "org.seul.geda.pcb"
#define PCB_DBUS_OBJECT_PATH       "/org/seul/geda/pcb"
#define PCB_DBUS_INTERFACE         "org.seul.geda.pcb"
#define PCB_DBUS_ACTIONS_INTERFACE "org.seul.geda.pcb.actions"

static DBusConnection *pcb_dbus_conn;


static DBusHandlerResult
handle_get_filename (DBusConnection * connection, DBusMessage * message,
		     void *data)
{
  DBusMessage *reply;
  DBusMessageIter iter;
  DBusHandlerResult result;
  char *filename;

  result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  // TODO: Should check the message signature matches what we expect?

  reply = dbus_message_new_method_return (message);
  if (reply == NULL)
    {
      fprintf (stderr, "pcb_dbus: Couldn't create reply message\n");
      return result;
    }
  dbus_message_iter_init_append (reply, &iter);

  if (PCB->Filename)
    filename = lrealpath (PCB->Filename);
  else
    filename = NULL;

  if (filename == NULL)
    {
#ifdef DEBUG
      fprintf (stderr,
	       "pcb_dbus: DEBUG: Couldn't get working filename, assuming none\n");
#endif
      filename = strdup ("");
      if (filename == NULL)
	{
	  fprintf (stderr,
		   "pcb_dbus: Couldn't strdup( \"\" ) for the filename\n");
	  goto out;
	}
    }
  if (!dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &filename))
    {
      fprintf (stderr,
	       "pcb_dbus: Couldn't append return filename string to message reply, Out Of Memory!\n");
      free (filename);
      goto out;
    }
  free (filename);
  if (!dbus_connection_send (connection, reply, NULL))
    {
      fprintf (stderr, "pcb_dbus: Couldn't send message, Out Of Memory!\n");
      goto out;
    }
  result = DBUS_HANDLER_RESULT_HANDLED;

out:
  dbus_message_unref (reply);
  return result;
}


static DBusHandlerResult
handle_exec_action (DBusConnection * connection, DBusMessage * message,
		    void *data)
{
  DBusMessage *reply;
  DBusMessageIter iter;
  DBusHandlerResult result;
  DBusError err;
  dbus_uint32_t retval;
  char *action_name;
  char **argv;
  int argc;
#ifdef DEBUG
  int i;
#endif

  result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  // TODO: Should check the message signature matches what we expect?

  // initialise the error struct
  dbus_error_init (&err);

  /* DON'T FREE action_name, as it belongs to DBUS,
   * DO    FREE argv, using dbus_free_string_array()
   */
  argv = NULL;
  if (!dbus_message_get_args (message,
			      &err,
			      DBUS_TYPE_STRING, &action_name,
			      DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &argv, &argc,
			      DBUS_TYPE_INVALID))
    {
      fprintf (stderr, "Failed to read method arguments\n");
      if (argv)
	dbus_free_string_array (argv);
      return result;
    }

#ifdef DEBUG
  fprintf (stderr, "pcb_dbus: DEBUG: Executing action: %s(", action_name);
  if (argc > 0)
    fprintf (stderr, " \"%s\"", argv[0]);
  for (i = 1; i < argc; i++)
    fprintf (stderr, ", \"%s\"", argv[i]);
  fprintf (stderr, " )\n");
#endif

  // TODO: Proper return value from actions
  hid_actionv (action_name, argc, argv);
  retval = 0;

  dbus_free_string_array (argv);

  reply = dbus_message_new_method_return (message);
  if (reply == NULL)
    {
      fprintf (stderr, "pcb_dbus: Couldn't create reply message\n");
      return result;
    }
  dbus_message_iter_init_append (reply, &iter);
  if (!dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &retval))
    {
      fprintf (stderr, "pcb_dbus: Couldn't sent message, Out Of Memory!\n");
      goto out;
    }

  if (!dbus_connection_send (connection, reply, NULL))
    {
      fprintf (stderr, "pcb_dbus: Couldn't send message, Out Of Memory!\n");
      goto out;
    }

  result = DBUS_HANDLER_RESULT_HANDLED;
out:
  dbus_message_unref (reply);
  return result;
}


static DBusHandlerResult
handle_introspect (DBusConnection * connection, DBusMessage * message,
		   void *data)
{
  DBusMessage *reply;
  DBusMessageIter iter;
  DBusHandlerResult result;

  result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  reply = dbus_message_new_method_return (message);
  if (reply == NULL)
    {
      fprintf (stderr, "pcb_dbus: Couldn't create reply message\n");
      return result;
    }
  dbus_message_iter_init_append (reply, &iter);
  if (!dbus_message_iter_append_basic
      (&iter, DBUS_TYPE_STRING, &pcb_dbus_introspect_xml))
    {
      fprintf (stderr,
	       "pcb_dbus: Couldn't add introspect XML to message return\n");
      goto out;
    }
  if (!dbus_connection_send (pcb_dbus_conn, reply, NULL))
    {
      fprintf (stderr,
	       "pcb_dbus: Couldn't queue reply message for sending\n");
      goto out;
    }
  result = DBUS_HANDLER_RESULT_HANDLED;
out:
  dbus_message_unref (reply);
  return result;
}

static void
unregister_dbus_handler (DBusConnection * connection, void *data)
{
}


static DBusHandlerResult
handle_dbus_message (DBusConnection * connection, DBusMessage * message,
		     void *data)
{
  int msg_type;
  msg_type = dbus_message_get_type (message);

  switch (msg_type)
    {
    case DBUS_MESSAGE_TYPE_METHOD_CALL:
      {
	const char *method_name;
	const char *interface_name;

	method_name = dbus_message_get_member (message);
	if (method_name == NULL)
	  {
	    fprintf (stderr, "pcb_dbus: Method had no name specified\n");
	    break;
	  }

	interface_name = dbus_message_get_interface (message);
	if (interface_name == NULL)
	  {
	    fprintf (stderr, "pcb_dbus: Method had no interface specified\n");
	    break;
	  }

	if (strcmp (interface_name, PCB_DBUS_INTERFACE) == 0)
	  {
	    if (strcmp (method_name, "GetFilename") == 0)
	      {
		return handle_get_filename (connection, message, data);
	      }
	    fprintf (stderr, "pcb_dbus: Interface '%s' has no method '%s'\n",
		     interface_name, method_name);
	    break;
	  }
	else if (strcmp (interface_name, PCB_DBUS_ACTIONS_INTERFACE) == 0)
	  {
	    if (strcmp (method_name, "ExecAction") == 0)
	      {
		return handle_exec_action (connection, message, data);
	      }
	    fprintf (stderr, "pcb_dbus: Interface '%s' has no method '%s'\n",
		     interface_name, method_name);
	    break;
	  }
	else if (strcmp (interface_name, DBUS_INTERFACE_INTROSPECTABLE) == 0)
	  {
	    if (strcmp (method_name, "Introspect") == 0)
	      {
		return handle_introspect (connection, message, data);
	      }
	    fprintf (stderr, "pcb_dbus: Interface '%s' has no method '%s'\n",
		     interface_name, method_name);
	    break;
	  }
	else
	  {
	    fprintf (stderr, "pcb_dbus: Interface '%s' was not recognised\n",
		     interface_name);
	    break;
	  }
      }
      break;

    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
      fprintf (stderr, "pcb_dbus: DBUG: Method return message\n");
      // WON'T ACTUALLY BE ANY UNLESS WE MAKE AN ASYNCRONOUS CALL?
      break;

    case DBUS_MESSAGE_TYPE_ERROR:
      fprintf (stderr, "pcb_dbus: DEBUG: Error message\n");
      // HOPE NOT!
      break;

    case DBUS_MESSAGE_TYPE_SIGNAL:
      fprintf (stderr, "pcb_dbus: DEBUG: Signal message\n");
      // NONE AT PRESENT
      break;

    default:
      fprintf (stderr,
	       "pcb_dbus: DEBUG: Message type wasn't one we know about!\n");
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


/*!
 * \brief Carry out all actions to setup the D-Bus and register
 * appropriate callbacks.
 */
void
pcb_dbus_setup (void)
{
  DBusError err;
  int ret;
  const DBusObjectPathVTable object_vtable = {
    unregister_dbus_handler,
    handle_dbus_message,
    NULL, NULL, NULL, NULL
  };

  // Initialise the error variable
  dbus_error_init (&err);

  // Connect to the bus
  pcb_dbus_conn = dbus_bus_get_private (DBUS_BUS_SESSION, &err);
  if (dbus_error_is_set (&err))
    {
      fprintf (stderr, "pcb_dbus: DBus connection Error (%s)\n", err.message);
      dbus_error_free (&err);
    }
  if (pcb_dbus_conn == NULL)
    return;

  // Request the canonical name for PCB on the bus
  ret = dbus_bus_request_name (pcb_dbus_conn, PCB_DBUS_CANONICAL_NAME,
			       DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
  if (dbus_error_is_set (&err))
    {
      fprintf (stderr, "pcb_dbus: DBus name error (%s)\n", err.message);
      dbus_error_free (&err);
    }
  if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER
      && ret != DBUS_REQUEST_NAME_REPLY_IN_QUEUE)
    {
      fprintf (stderr,
	       "pcb_dbus: Couldn't gain ownership or queued ownership of the canonical DBus name\n");
      return;
    }

  if (!dbus_connection_register_object_path (pcb_dbus_conn, PCB_DBUS_OBJECT_PATH, &object_vtable, NULL	// void * user_data
      ))
    {
      fprintf (stderr, "pcb_dbus: Couldn't register DBUS handler for %s\n",
	       PCB_DBUS_OBJECT_PATH);
      return;
    }

  // Setup intergration with the pcb mainloop
  pcb_dbus_connection_setup_with_mainloop (pcb_dbus_conn);

//  dbus_error_free(&err);
  return;
}



/*!
 * \brief Carry out all actions to finalise the D-Bus connection.
 */
void
pcb_dbus_finish (void)
{
  DBusError err;

  // Initialise the error variable
  dbus_error_init (&err);

  // TODO: Could emit a "goodbye" signal here?

  dbus_connection_flush (pcb_dbus_conn);

  dbus_connection_unregister_object_path (pcb_dbus_conn,
					  PCB_DBUS_OBJECT_PATH);

  dbus_bus_release_name (pcb_dbus_conn, PCB_DBUS_CANONICAL_NAME, &err);

  dbus_error_free (&err);

  pcb_dbus_connection_finish_with_mainloop (pcb_dbus_conn);

  dbus_connection_close (pcb_dbus_conn);
  dbus_connection_unref (pcb_dbus_conn);

  // Call DBus shutdown. This doesn't work with shared connections,
  // only private ones (like we took out earlier).
  // If any future module / plugin to PCB wants to use DBus too,
  // we must remove this call. DBus will get shut-down when the app exits.
  dbus_shutdown ();
}
