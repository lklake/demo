/*
 *
 *     add-server.c: server program, receives message,
 *                   adds numbers in message and
 *                   gives back result to client
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>
#include <X11/XKBlib.h>
#include <signal.h>

const char *const INTERFACE_NAME = "in.softprayog.dbus_example";
const char *const SERVER_BUS_NAME = "in.softprayog.add_server";
const char *const OBJECT_PATH_NAME = "/in/softprayog/adder";
const char *const METHOD_NAME = "add_numbers";

DBusError dbus_error;
static Bool running = True;

void print_dbus_error(char *str);
void sig_handler(int signum) {
    printf("\nsignum:【%d】received,stopping\n", signum);
    running = False;
}
int main(int argc, char **argv) {
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    DBusConnection *conn;
    int ret;

    dbus_error_init(&dbus_error);

    conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);

    if (dbus_error_is_set(&dbus_error))
        print_dbus_error("dbus_bus_get");

    if (!conn){
        exit(-1);
    }
    // Get a well known name
    ret = dbus_bus_request_name(conn, SERVER_BUS_NAME, DBUS_NAME_FLAG_DO_NOT_QUEUE, &dbus_error);

    if (dbus_error_is_set(&dbus_error))
        print_dbus_error("dbus_bus_get");

    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        fprintf(stderr, "Dbus: not primary owner, ret = %d\n", ret);
        exit(-1);
    }
    Display *dpy = XOpenDisplay(NULL);

    // Handle request from clients
    while (running) {
        // Block for msg from client
        if (!dbus_connection_read_write_dispatch(conn, -1)) {
            fprintf(stderr, "Not connected now.\n");
            exit(-1);
        }
        DBusMessage *message;
        if ((message = dbus_connection_pop_message(conn)) == NULL) {
            fprintf(stderr, "Did not get message\n");
            continue;
        }

        if (dbus_message_is_method_call(message, INTERFACE_NAME, METHOD_NAME)) {
            u_int64_t key_code;
            u_int64_t state;
            dbus_bool_t pressed;

            if (dbus_message_get_args(message, &dbus_error,
                                      DBUS_TYPE_UINT64, &key_code,
                                      DBUS_TYPE_UINT64, &state,
                                      DBUS_TYPE_BOOLEAN, &pressed,
                                      DBUS_TYPE_INVALID)) {
                printf("key %s %s\n",
                       XKeysymToString(XkbKeycodeToKeysym(dpy, key_code, 0, state & ShiftMask ? 1 : 0)),
                       pressed ? "pressed" : "released");
                // send reply
                DBusMessage *reply;
                char *answer = "ok";
                if ((reply = dbus_message_new_method_return(message)) == NULL) {
                    fprintf(stderr, "Error in dbus_message_new_method_return\n");
                    exit(-1);
                }

                DBusMessageIter iter;
                dbus_message_iter_init_append(reply, &iter);
                if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &answer)) {
                    fprintf(stderr, "Error in dbus_message_iter_append_basic\n");
                    exit(-1);
                }

                if (!dbus_connection_send(conn, reply, NULL)) {
                    fprintf(stderr, "Error in dbus_connection_send\n");
                    exit(-1);
                }

                dbus_connection_flush(conn);

                dbus_message_unref(reply);
            } else {
                DBusMessage *dbus_error_msg;
                char error_msg[] = "Error in input";
                if ((dbus_error_msg = dbus_message_new_error(message, DBUS_ERROR_FAILED, error_msg)) == NULL) {
                    fprintf(stderr, "Error in dbus_message_new_error\n");
                    exit(-1);
                }

                if (!dbus_connection_send(conn, dbus_error_msg, NULL)) {
                    fprintf(stderr, "Error in dbus_connection_send\n");
                    exit(-1);
                }

                dbus_connection_flush(conn);

                dbus_message_unref(dbus_error_msg);
            }
        }
    }
    return 0;
}

void print_dbus_error(char *str) {
    fprintf(stderr, "%s: %s\n", str, dbus_error.message);
    dbus_error_free(&dbus_error);
}

