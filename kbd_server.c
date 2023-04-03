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

const char *const INTERFACE_NAME = "test.localhost.xdnd_interface";
const char *const SERVER_BUS_NAME = "test.localhost.xdnd_server";
const char *const OBJECT_PATH_NAME = "/test/localhost/xdnd_object";
const char *const METHOD_NAME = "xdnd";

DBusError dbus_error;
static Bool running = True;

// Atom definitions
static Atom XdndAware, XA_ATOM, XdndEnter, XdndPosition, XdndActionCopy, XdndLeave, XdndStatus, XdndDrop,
        XdndSelection, XDND_DATA, XdndTypeList, XdndFinished, WM_PROTOCOLS, WM_DELETE_WINDOW, typesWeAccept[6];


void print_dbus_error(char *str);
void sig_handler(int signum) {
    printf("\nsignum:【%d】received,stopping\n", signum);
    running = False;
}


// This is sent by the source to the target to say it can call XConvertSelection
static void sendXdndDrop(Display *disp, Window source, Window target)
{
        // Declare message struct and populate its values
        XEvent message;
        memset(&message, 0, sizeof(message));
        message.xclient.type = ClientMessage;
        message.xclient.display = disp;
        message.xclient.window = target;
        message.xclient.message_type = XdndDrop;
        message.xclient.format = 32;
        message.xclient.data.l[0] = source;
        //message.xclient.data.l[1] reserved

        // Send it to target window
        if (XSendEvent(disp, target, False, 0, &message) == 0){
            fprintf(stderr, "XSendEvent Send it to target window failed\n");
            exit(-1);
        }

}

// This somewhat naively calculates what window we are over by drilling down
// to its children and so on using recursion
static Window getWindowPointerIsOver(Display *disp, Window startingWindow,
                                     int p_rootX, int p_rootY, int originX, int originY)
{
    // Window we are returning
    Window returnWindow = None;

    // Get stacked list of children in stacked order
    Window rootReturn, parentReturn, childReturn, *childList;
    unsigned int numOfChildren;
    if (XQueryTree(disp, startingWindow, &rootReturn, &parentReturn,
                   &childList, &numOfChildren) != 0) {
        // Search through children
        for (int i = numOfChildren - 1; i >= 0; --i) {
            // Get window attributes
            XWindowAttributes childAttrs;
            XGetWindowAttributes(disp, childList[i], &childAttrs);

            // Check if cursor is in this window
            if (p_rootX >= originX + childAttrs.x &&
                p_rootX < originX + childAttrs.x + childAttrs.width &&
                p_rootY >= originY + childAttrs.y &&
                p_rootY < originY + childAttrs.y + childAttrs.height) {
                returnWindow = getWindowPointerIsOver(disp, childList[i],
                                                      p_rootX, p_rootY, originX + childAttrs.x, originY + childAttrs.y);
                break;
            }
        }
        XFree(childList);
    }

    // We are are bottom of recursion stack, set correct window to be returned up through each level
    if (returnWindow == None)
        returnWindow = startingWindow;

    return returnWindow;
}
// This is sent by the source to the target to say the data is ready
static void sendSelectionNotify(Display *disp, XSelectionRequestEvent *selectionRequest, const char *pathStr)
{
        // Allocate buffer (two bytes at end for CR/NL and another for null byte)
        size_t sizeOfPropertyData = strlen("file://") + strlen(pathStr) + 3;
        char *propertyData = malloc(sizeOfPropertyData);
        if (!propertyData){
            fprintf(stderr, "failed malloc propertyData\n");
            exit(-1);
        }

        // Copy data to buffer
        strcpy(propertyData, "file://");
        strcat(propertyData, pathStr);
        propertyData[sizeOfPropertyData-3] = 0xD;
        propertyData[sizeOfPropertyData-2] = 0xA;
        propertyData[sizeOfPropertyData-1] = '\0';
        printf("propertyData目标 :%s",propertyData);
        // Set property on target window - do not copy end null byte
        XChangeProperty(disp, selectionRequest->requestor, selectionRequest->property,
                        typesWeAccept[0], 8, PropModeReplace, (unsigned char *)propertyData, sizeOfPropertyData-1);

        // Free property buffer
        free(propertyData);

        // Declare message struct and populate its values
        XEvent message;
        memset(&message, 0, sizeof(message));
        message.xselection.type = SelectionNotify;
        message.xselection.display = disp;
        message.xselection.requestor = selectionRequest->requestor;
        message.xselection.selection = selectionRequest->selection;
        message.xselection.target = selectionRequest->target;
        message.xselection.property = selectionRequest->property;
        message.xselection.time = selectionRequest->time;

        // Send it to target window
        if (XSendEvent(disp, selectionRequest->requestor, False, 0, &message) == 0){
            fprintf(stderr, "failed XSendEvent propertyData\n");
            exit(-1);
        }

}
// This sends the XdndEnter message which initiates the XDND protocol exchange
static void sendXdndEnter(Display *disp, int xdndVersion, Window source, Window target)
{
        // Declare message struct and populate its values
        XEvent message;
        memset(&message, 0, sizeof(message));
        message.xclient.type = ClientMessage;
        message.xclient.display = disp;
        message.xclient.window = target;
        message.xclient.message_type = XdndEnter;
        message.xclient.format = 32;
        message.xclient.data.l[0] = source;
        message.xclient.data.l[1] = xdndVersion << 24;
        message.xclient.data.l[2] = typesWeAccept[0];
        message.xclient.data.l[3] = None;
        message.xclient.data.l[4] = None;

        // Send it to target window
        if (XSendEvent(disp, target, False, 0, &message) == 0){
            fprintf(stderr, "failed sendXdndEnter XSendEvent \n");
            exit(-1);
        }

}

// This sends the XdndPosition messages, which update the target on the state of the cursor
// and selected action
static void sendXdndPosition(Display *disp, Window source, Window target, int time, int p_rootX, int p_rootY)
{
        // Declare message struct and populate its values
        XEvent message;
        memset(&message, 0, sizeof(message));
        message.xclient.type = ClientMessage;
        message.xclient.display = disp;
        message.xclient.window = target;
        message.xclient.message_type = XdndPosition;
        message.xclient.format = 32;
        message.xclient.data.l[0] = source;
        //message.xclient.data.l[1] reserved
        message.xclient.data.l[2] = p_rootX << 16 | p_rootY;
        message.xclient.data.l[3] = time;
        message.xclient.data.l[4] = XdndActionCopy;

        // Send it to target window
        if (XSendEvent(disp, target, False, 0, &message) == 0){
            fprintf(stderr, "failed sendXdndPosition XSendEvent \n");
            exit(-1);
        }

}


// This is sent by the source when the exchange is abandoned
static void sendXdndLeave(Display *disp, Window source, Window target)
{
        // Declare message struct and populate its values
        XEvent message;
        memset(&message, 0, sizeof(message));
        message.xclient.type = ClientMessage;
        message.xclient.display = disp;
        message.xclient.window = target;
        message.xclient.message_type = XdndLeave;
        message.xclient.format = 32;
        message.xclient.data.l[0] = source;
        // Rest of array members reserved so not set

        // Send it to target window
        if (XSendEvent(disp, target, False, 0, &message) == 0){
            fprintf(stderr, "failed sendXdndLeave XSendEvent \n");
            exit(-1);
        }

}


int main(int argc, char **argv) {
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


    // Handle request from clients
    while (running) {
        // Block for msg from client
        printf("进入等待循环\n");
        if (!dbus_connection_read_write_dispatch(conn, -1)) {
            fprintf(stderr, "Not connected now.\n");
            exit(-1);
        }
        printf("获取到了dbus connect\n");
        DBusMessage *message;
        if ((message = dbus_connection_pop_message(conn)) == NULL) {
            fprintf(stderr, "Did not get message\n");
            continue;
        }
        printf("获取到了dbus message\n");
        if (dbus_message_is_method_call(message, INTERFACE_NAME, METHOD_NAME)) {
            u_int64_t key_code;
            u_int64_t state;
            dbus_bool_t pressed;
            // 处理接收文件路径
            const char* path;
            int x , y;
            if (dbus_message_get_args(message, &dbus_error,
                                      DBUS_TYPE_STRING, &path,
                                      DBUS_TYPE_INT32,&x,
                                      DBUS_TYPE_INT32,&y,
                                      DBUS_TYPE_INVALID)) {
                printf("参数解析成功 path：%s\n",path);

                Display *dpy = XOpenDisplay(NULL);
                const char *procStr = "Stuart";
                Window wind;
                XEvent event;
                // Define atoms
                XdndAware = XInternAtom(dpy, "XdndAware", False);
                XA_ATOM = XInternAtom(dpy, "XA_ATOM", False);
                XdndEnter = XInternAtom(dpy, "XdndEnter", False);
                XdndPosition = XInternAtom(dpy, "XdndPosition", False);
                XdndActionCopy = XInternAtom(dpy, "XdndActionCopy", False);
                XdndLeave = XInternAtom(dpy, "XdndLeave", False);
                XdndStatus = XInternAtom(dpy, "XdndStatus", False);
                XdndDrop = XInternAtom(dpy, "XdndDrop", False);
                XdndSelection = XInternAtom(dpy, "XdndSelection", False);
                XDND_DATA = XInternAtom(dpy, "XDND_DATA", False);
                XdndTypeList = XInternAtom(dpy, "XdndTypeList", False);
                XdndFinished = XInternAtom(dpy, "XdndFinished", False);
                WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
                WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

//                // Define type atoms we will accept for file drop
                typesWeAccept[0] = XInternAtom(dpy, "text/uri-list", False);
                typesWeAccept[1] = XInternAtom(dpy, "UTF8_STRING", False);
                typesWeAccept[2] = XInternAtom(dpy, "TEXT", False);
                typesWeAccept[3] = XInternAtom(dpy, "STRING", False);
                typesWeAccept[4] = XInternAtom(dpy, "text/plain;charset=utf-8", False);
                typesWeAccept[5] = XInternAtom(dpy, "text/plain", False);

//                // Get screen dimensions
//                screen = DefaultScreen(dpy);
//                screenWidth = DisplayWidth(dpy, screen);
//                screenHeight = DisplayHeight(dpy, screen);
//                printf("%s: screen width: %d, screen height: %d\n", procStr, screenWidth, screenHeight);

                // Define colours
//                unsigned long red = 0xFF << 16;
//                unsigned long blue = 0xFF;
//                unsigned long white = WhitePixel(dpy, screen);
//                unsigned long green = 0xFF << 8; // Just green component

                wind = XCreateSimpleWindow(dpy, RootWindow(dpy, 0), 0, 0, 100, 100, 1,
                                           0, 0);
                if (wind == 0){
                    fprintf(stderr, "failed XCreateSimpleWindow\n");
                    exit(-1);
                }

                // Set window title
                if (XStoreName(dpy, wind, procStr) == 0){
                    fprintf(stderr, "failed XStoreName\n");
                    exit(-1);
                }
                // Set events we are interested in
//                if (XSelectInput(dpy, wind, PointerMotionMask | KeyPressMask | KeyReleaseMask |
//                                             ButtonPressMask | ButtonReleaseMask | ExposureMask |
//                                             EnterWindowMask | LeaveWindowMask) == 0){
//                    fprintf(stderr, "failed XSelectInput XSendEvent \n");
//                    exit(-1);
//                }

                // Add XdndAware property
                int xdndVersion = 5;
                XChangeProperty(dpy, wind, XdndAware,
                                XA_ATOM, 32, PropModeReplace,
                                (void *)&xdndVersion, 1);

//                // Set WM_PROTOCOLS to add WM_DELETE_WINDOW atom so we can end app gracefully
//                XSetWMProtocols(dpy, wind, &WM_DELETE_WINDOW, 1);

                // Show window by mapping it
//                if (XMapWindow(dpy, wind) == 0){
//                    fprintf(stderr, "failed XMapWindow XSendEvent \n");
//                    exit(-1);
//                }



                // xclient 的逻辑 模拟拖拽 无状态机版本
                // 模拟拖拽：Find window
                Window targetWindow = getWindowPointerIsOver(dpy, DefaultRootWindow(dpy),
                                                             x, y, 0, 0);
                if (targetWindow == None){
                    printf("目标窗口未找到\n");
                    break;
                }




                // Claim ownership of Xdnd selection
                XSetSelectionOwner(dpy, XdndSelection, wind, CurrentTime);

                // Send XdndEnter message
                printf("%s: sending XdndEnter to target window 0x%lx\n",
                       procStr, targetWindow);
                // TODO xdndVersion 先固定5
                sendXdndEnter(dpy, 5, wind, targetWindow);


                // Send XdndPosition message
                printf("%s: sending XdndPosition to target window 0x%lx\n",
                       procStr, targetWindow);
                sendXdndPosition(dpy, wind, targetWindow, CurrentTime,
                                 100, 100);



//                XNextEvent(dpy, &event);
//                while(event.type != ClientMessage || event.xclient.message_type != XdndStatus){
//                    printf("event.type is %d\n",event.type);
//                    XNextEvent(dpy, &event);
//                }
                // Check if target will accept drop
//                if ((event.xclient.data.l[1] & 0x1) != 1) {
//                    // Won't accept, break exchange and wipe state
//                    printf("%s: sending XdndLeave message to target window "
//                           "as it won't accept drop\n", procStr);
//                    sendXdndLeave(dpy, wind, targetWindow);
//                    continue;
//                }


                // 模拟拖拽：Send XdndDrop message
                printf("模拟拖拽: sending XdndDrop to target window：0x%lx\n",targetWindow);
                // TODO 具体drop到的位置发生的事件
                sendXdndDrop(dpy, wind, targetWindow);
                // 模拟拖拽：Add data to the target window
                printf("模拟拖拽: sending SelectionNotify to target window\n");
                int havaSendSelectionNotify = 0;
                int havaXdndFinished = 0;
                while (!havaXdndFinished || !havaSendSelectionNotify ){
                    XNextEvent(dpy, &event);
                    switch (event.type) {
                        case SelectionRequest:
                            printf("模拟拖拽: event have found SelectionRequest event.type is %d\n",event.type);
                            sendSelectionNotify(dpy, &event.xselectionrequest,
                                                path);
                            havaSendSelectionNotify = 1;
                            break;
                        case ClientMessage:
                            // Check for XdndStatus message
                            if (event.xclient.message_type == XdndStatus) {
                                // Check if target will accept drop
                                if ((event.xclient.data.l[1] & 0x1) != 1) {
                                    // Won't accept, break exchange and wipe state
                                    printf("%s: sending XdndLeave message to target window "
                                           "as it won't accept drop\n", procStr);
                                    sendXdndLeave(dpy, wind, targetWindow);
                                    break;
                                }
                            }
                            else if(event.xclient.message_type == XdndFinished) {
                                printf("received XdndFinished\n");
                                havaXdndFinished = 1;
                            }
                            break;
                    }
                }


                XClearWindow(dpy, wind);
                XFlush(dpy);
//                XNextEvent(dpy, &event);
//                    while(event.type != SelectionRequest){
//                        printf("event.type is %d\n",event.type);
//                        XNextEvent(dpy, &event);
//                    }
//                    printf("模拟拖拽: event have found SelectionRequest event.type is %d\n",event.type);
//                    sendSelectionNotify(dpy, &event.xselectionrequest,
//                                        path);

//                XNextEvent(dpy, &event);
//                while(event.type != ClientMessage || event.xclient.message_type != XdndFinished){
//                    printf("event.type is %d\n",event.type);
//                    XNextEvent(dpy, &event);
//                }

                // Xserver 逻辑结束
                XDestroyWindow(dpy,wind);
                XCloseDisplay(dpy);

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
            }
//            else if (dbus_message_get_args(message, &dbus_error,
//                                      DBUS_TYPE_UINT64, &key_code,
//                                      DBUS_TYPE_UINT64, &state,
//                                      DBUS_TYPE_BOOLEAN, &pressed,
//                                      DBUS_TYPE_INVALID)) {
//                printf("key %s %s\n",
//                       XKeysymToString(XkbKeycodeToKeysym(dpy, key_code, 0, state & ShiftMask ? 1 : 0)),
//                       pressed ? "pressed" : "released");
//                // send reply
//                DBusMessage *reply;
//                char *answer = "ok";
//                if ((reply = dbus_message_new_method_return(message)) == NULL) {
//                    fprintf(stderr, "Error in dbus_message_new_method_return\n");
//                    exit(-1);
//                }
//
//                DBusMessageIter iter;
//                dbus_message_iter_init_append(reply, &iter);
//                if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &answer)) {
//                    fprintf(stderr, "Error in dbus_message_iter_append_basic\n");
//                    exit(-1);
//                }
//
//                if (!dbus_connection_send(conn, reply, NULL)) {
//                    fprintf(stderr, "Error in dbus_connection_send\n");
//                    exit(-1);
//                }
//
//                dbus_connection_flush(conn);
//
//                dbus_message_unref(reply);
//            }
            else {
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

