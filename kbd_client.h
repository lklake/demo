//
// Created by happy on 3/10/23.
//

#ifndef DEMO_KBD_CLIENT_H
#define DEMO_KBD_CLIENT_H
#include <dbus/dbus.h>
DBusConnection *get_connection();
int send_receive_data(DBusConnection *conn, u_int64_t key_code,u_int64_t state,dbus_bool_t pressed,char** output);
int send_path(DBusConnection *conn, char* path,dbus_int32_t x,dbus_int32_t y, char **output);
#endif //DEMO_KBD_CLIENT_H
