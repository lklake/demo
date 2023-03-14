cmake_minimum_required(VERSION 3.25)
project(demo C)

set(CMAKE_C_STANDARD 11)

include_directories(.)
find_package(PkgConfig REQUIRED) # Include functions provided by PkgConfig module.

pkg_check_modules(DBUS REQUIRED dbus-1)
# Use results of find_package() call.
include_directories(${DBUS_INCLUDE_DIRS})

#link_directories(${DBUS_LIBRARY_DIRS})

find_package(X11 REQUIRED)
#link_directories( ${X11_LIBRARIES})

#link_libraries(-lX11 -lXRes)
add_executable(demo
        demo.c
        LinkedList.c
        LinkedList.h  kbd_client.c kbd_client.h demo_error.h)

target_link_libraries(demo ${DBUS_LIBRARIES}  ${X11_LIBRARIES})

add_executable(kbd_server
        kbd_server.c)
target_link_libraries(kbd_server ${DBUS_LIBRARIES}  ${X11_LIBRARIES})