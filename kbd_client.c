//
// Created by happy on 3/10/23.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kbd_client.h"
#include "demo_error.h"

//const char *const CLIENT_BUS_NAME = "in.softprayog.add_client";
const char *const SERVER_OBJECT_PATH_NAME = "/test/localhost/xdnd_object";
//const char *const CLIENT_OBJECT_PATH_NAME = "/in/softprayog/add_client";

const char *const INTERFACE_NAME = "test.localhost.xdnd_interface";
const char *const SERVER_BUS_NAME = "test.localhost.xdnd_server";
const char *const OBJECT_PATH_NAME = "/test/localhost/xdnd_object";
const char *const METHOD_NAME = "xdnd";


DBusError dbus_error;

void print_dbus_error(char *str) {
    LOG_ERR ("%s: %s", str, dbus_error.message);
    dbus_error_free(&dbus_error);
}

DBusConnection *get_connection() {
    DBusConnection *conn;
    dbus_error_init(&dbus_error);

    conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);

    if (dbus_error_is_set(&dbus_error))
        print_dbus_error("dbus_bus_get");

    return conn;
}

int
send_receive_data(DBusConnection *conn, const u_int64_t key_code, u_int64_t state, dbus_bool_t pressed, char **output) {
    DBusMessage *request;

    if ((request = dbus_message_new_method_call(SERVER_BUS_NAME, SERVER_OBJECT_PATH_NAME,
                                                INTERFACE_NAME, METHOD_NAME)) == NULL) {
        LOG_ERR ("Error in dbus_message_new_method_call");
        return -1;
    }

    DBusMessageIter iter;
    dbus_message_iter_init_append(request, &iter);
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT64, &key_code)) {
        LOG_ERR ("Error in dbus_message_iter_append_basic");
        return -1;
    }
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT64, &state)) {
        LOG_ERR ("Error in dbus_message_iter_append_basic");
        return -1;
    }
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_BOOLEAN, &pressed)) {
        LOG_ERR ("Error in dbus_message_iter_append_basic");
        return -1;
    }
    DBusPendingCall *pending_return;
    if (!dbus_connection_send_with_reply(conn, request, &pending_return, -1)) {
        LOG_ERR ("Error in dbus_connection_send_with_reply");
        return -1;
    }

    if (pending_return == NULL) {
        LOG_ERR ("pending return is NULL");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(request);

    dbus_pending_call_block(pending_return);

    DBusMessage *reply;
    if ((reply = dbus_pending_call_steal_reply(pending_return)) == NULL) {
        LOG_ERR ("Error in dbus_pending_call_steal_reply");
        return -1;
    }

    dbus_pending_call_unref(pending_return);

    char *tmp;
    if (FALSE == dbus_message_get_args(reply, &dbus_error, DBUS_TYPE_STRING, &tmp, DBUS_TYPE_INVALID)) {
        LOG_ERR ("Did not get arguments in reply");
        return -1;
    }
    if (dbus_error_is_set(&dbus_error)) {
        LOG_ERR ("an error occurred: %s", dbus_error.message);
        dbus_message_unref(request);
        dbus_error_free(&dbus_error);
        return -1;
    }
    *output = strdup(tmp);
    if (DBUS_MESSAGE_TYPE_ERROR == dbus_message_get_type(reply)) {
        dbus_message_unref(reply);
        return -1;
    }
    dbus_message_unref(reply);
    return 0;
}

int
send_path(DBusConnection *conn, char* req, char **output) {
    DBusMessage *request;

    if ((request = dbus_message_new_method_call(SERVER_BUS_NAME, SERVER_OBJECT_PATH_NAME,
                                                INTERFACE_NAME, METHOD_NAME)) == NULL) {
        LOG_ERR ("Error in dbus_message_new_method_call");
        return -1;
    }
    DBusMessageIter iter;
    dbus_message_iter_init_append(request, &iter);
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &req)) {
        LOG_ERR ("Error in dbus_message_iter_append_basic for send path");
        return -1;
    }
//    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT64, &key_code)) {
//        LOG_ERR ("Error in dbus_message_iter_append_basic");
//        return -1;
//    }
//    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT64, &state)) {
//        LOG_ERR ("Error in dbus_message_iter_append_basic");
//        return -1;
//    }
//    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_BOOLEAN, &pressed)) {
//        LOG_ERR ("Error in dbus_message_iter_append_basic");
//        return -1;
//    }
    DBusPendingCall *pending_return;
    if (!dbus_connection_send_with_reply(conn, request, &pending_return, -1)) {
        LOG_ERR ("Error in dbus_connection_send_with_reply");
        return -1;
    }

    if (pending_return == NULL) {
        LOG_ERR ("pending return is NULL");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(request);

    dbus_pending_call_block(pending_return);

    DBusMessage *reply;
    if ((reply = dbus_pending_call_steal_reply(pending_return)) == NULL) {
        LOG_ERR ("Error in dbus_pending_call_steal_reply");
        return -1;
    }

    dbus_pending_call_unref(pending_return);

    char *tmp;
    if (FALSE == dbus_message_get_args(reply, &dbus_error, DBUS_TYPE_STRING, &tmp, DBUS_TYPE_INVALID)) {
        LOG_ERR ("Did not get arguments in reply");
        return -1;
    }
    if (dbus_error_is_set(&dbus_error)) {
        LOG_ERR ("an error occurred: %s", dbus_error.message);
        dbus_message_unref(request);
        dbus_error_free(&dbus_error);
        return -1;
    }
    *output = strdup(tmp);
    if (DBUS_MESSAGE_TYPE_ERROR == dbus_message_get_type(reply)) {
        dbus_message_unref(reply);
        return -1;
    }
    dbus_message_unref(reply);
    return 0;
}




