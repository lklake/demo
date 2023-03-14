#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <string.h>
#include <X11/XKBlib.h>
#include "LinkedList.h"

#include <signal.h>
#include "kbd_client.h"
#include "demo_error.h"

#define TO_TYPE(ptr, type) (*((type *)(ptr)))


static Bool isGrabbing = False;
static Display *dpy = None;
static Bool running = True;

XKeyEvent createKeyEvent(Display *display, Window win, Window winRoot,
                         Bool press, int keycode, int modifiers) {
    XKeyEvent event;

    event.display = display;
    event.window = win;
    event.root = winRoot;
    event.subwindow = None;
    event.time = CurrentTime;
    event.x = 1;
    event.y = 1;
    event.x_root = 1;
    event.y_root = 1;
    event.same_screen = True;
    event.keycode = XKeysymToKeycode(display, keycode);
    event.state = modifiers;

    if (press)
        event.type = KeyPress;
    else
        event.type = KeyRelease;
    return event;
}
static Bool getClassHint(Window wd, char **res_name, char **res_class) {
    XClassHint xch;
    if (res_name == NULL && res_class == NULL) {
        return False;
    }
    xch.res_name = NULL;
    xch.res_class = NULL;
    if (XGetClassHint(dpy, wd, &xch)) {
        if (res_name && xch.res_name) {
            *res_name = strdup(xch.res_name);
        }
        if (res_class && xch.res_class) {
            *res_class = strdup(xch.res_class);
        }
        XFree(xch.res_name);
        XFree(xch.res_class);
        return True;
    } else {
        return False;
    }
}

/**
 *
 * @param wd 待检查的目标窗口
 * @param tag 用于匹配窗口的标签
 * @return 如果目标窗口与标签匹配，返回True，否则返回False
 * 通过替换该函数的实现，可以选择不同的匹配方法。
 */
static Bool isTargetWindow(Window wd, const char *tag) {
    char *res_class;
    Bool result = False;
    if (getClassHint(wd, NULL, &res_class) > 0) {
        if (strcmp(res_class, tag) == 0) {
            result = True;
        }
        XFree(res_class);
    }
    return result;
}

/**
 *
 * @param currentWd 搜索的根节点
 * @param tag 用于匹配目标窗口的标签
 * @param result 用于保存结果的链表
 */
static void searchWindows(Window currentWd, const char *tag, LinkedList *result) {
    Window root, parent, *children;
    unsigned childrenCount;
    if (isTargetWindow(currentWd, tag)) {
        result->insert(result, 0, &currentWd, sizeof(currentWd));
    }
    if (0 != XQueryTree(dpy, currentWd, &root, &parent, &children, &childrenCount)) {
        for (int i = 0; i < childrenCount; ++i) {
            searchWindows(children[i], tag, result);
        }
        XFree(children);
    }
}


//搜索win为根的子树，查找类别名出现在nameList中的窗口，FocusChangeMask
static void selectInput(LinkedList *windowList, long mask) {
    for (int i = 0; i < windowList->size(windowList); i++) {
        Window w = TO_TYPE(windowList->at(windowList, i), Window);
        XSelectInput(dpy, w, mask);
    }
}

static Window getCurrentFocus() {
    Window wd;
    int revert_to;
    XGetInputFocus(dpy, &wd, &revert_to);
    if (wd == None) {
        LOG_WARN("no focus window");
    }
    return wd;
}

static int startGrabbing(Window wd) {
    if (!isGrabbing) {
        if (GrabSuccess != XGrabKeyboard(dpy, wd, False, GrabModeAsync, GrabModeAsync, CurrentTime)) {
            LOG_ERR("Error: grabbing keyboard failed");
            return -1;
        }
        isGrabbing = 1;
        LOG_MSG("start grabbing");
        //清除由于 grab 产生的Focus out事件。
        XSync(dpy, True);
    }
    return 0;
}

static void stopGrabbing() {
    if (isGrabbing) {
        LOG_MSG("stop grabbing");
        XUngrabKeyboard(dpy, CurrentTime);
        //清除由于 ungrab 产生的Focus in事件，防止Focus in事件对应的窗口已经关闭。
        XSync(dpy, True);
        isGrabbing = 0;
    }
}

//当前输入焦点窗口若与tagList匹配，则捕获输入焦点
static void tryGrabFocusedWindow(LinkedList *tagList) {
    Window currentFocus = getCurrentFocus();
    for (int i = 0; i < tagList->size(tagList); i++) {
        if (isTargetWindow(currentFocus, tagList->at(tagList, i))) {
            startGrabbing(XDefaultRootWindow(dpy));
            break;
        }
    }
}

int app_main() {
    //初始化需要监听的窗口类别名
    LinkedList *tagList = malloc(sizeof(LinkedList));
    LinkedList_initialize(tagList);
    LinkedList *windowList = malloc(sizeof(LinkedList));
    LinkedList_initialize(windowList);

    tagList->insert(tagList, 0, "VirtualBox Machine", sizeof("VirtualBox Machine") + 1);
    tagList->insert(tagList, 0, "VirtualBox Manager", sizeof("VirtualBox Manager") + 1);

    dpy = XOpenDisplay(NULL);
    Window rootWd = XDefaultRootWindow(dpy);

    DBusConnection *conn = get_connection();
    if (conn == NULL) {
        LOG_ERR("connect to server failed");
    }

    // 基于一个事实：当virtualbox打开新的VM界面时，会在root窗口下创建一个窗口，因此可以通过监听Root子窗口的创建事件，得知何时打开了新的VM。
    XSelectInput(dpy, rootWd, SubstructureNotifyMask);
    //程序初始化，搜索已经存在的窗口，监听它们的FocusChange事件。
    for (int i = 0; i < tagList->size(tagList); i++) {
        searchWindows(rootWd, tagList->at(tagList, i), windowList);
    }
    selectInput(windowList, FocusChangeMask);
    windowList->clear(windowList);


    tryGrabFocusedWindow(tagList);

    XEvent ev;
    int x11_fd;
    fd_set in_fds;
    struct timeval tv;
    x11_fd = ConnectionNumber(dpy);

    while (running) {
        // Create a File Description Set containing x11_fd
        FD_ZERO(&in_fds);
        FD_SET(x11_fd, &in_fds);
        tv.tv_usec = 0;
        tv.tv_sec = 1;

        int num_ready_fds = select(x11_fd + 1, &in_fds, NULL, NULL, &tv);

        if (num_ready_fds > 0) {
            // 处理事件
            while (XPending(dpy)) {
                XNextEvent(dpy, &ev);
                switch (ev.type) {
                    case CreateNotify:
                        if (ev.xcreatewindow.parent == rootWd) {
                            for (int i = 0; i < tagList->size(tagList); i++) {
                                searchWindows(rootWd, tagList->at(tagList, i), windowList);
                            }
                            selectInput(windowList, FocusChangeMask);
                            windowList->clear(windowList);
                        }
                        // 刚创建的VM窗口可能已经拥有焦点，但由于没有触发FocusIn，可能没有被捕获，因此尝试捕获。
                        tryGrabFocusedWindow(tagList);
                        break;
                    case FocusIn:
                        LOG_MSG("focus in");
                        if (startGrabbing(ev.xfocus.window) != 0) {
                            LOG_ERR("start grab failed");
                        }
                        break;
                    case FocusOut:
                        LOG_MSG("focus out");
                        stopGrabbing();
                        break;
                    case KeyPress: {
                        LOG_DEBUG("%d KeyPress %s", ev.xkey.keycode,
                                  XKeysymToString(XkbKeycodeToKeysym(dpy, ev.xkey.keycode, 0,
                                                                     ev.xkey.state & ShiftMask ? 1 : 0)));
                        char *output;
                        if (send_receive_data(conn, ev.xkey.keycode, ev.xkey.state, TRUE, &output) == 0) {
                            LOG_DEBUG("\nserver return:%s\n\n", output);
                        } else {
                            LOG_ERR("server failed to return:%s", output);
                        }
                        free(output);
                        break;
                    }
                    case KeyRelease: {
                        LOG_DEBUG("%d KeyRelease %s", ev.xkey.keycode,
                                  XKeysymToString(XkbKeycodeToKeysym(dpy, ev.xkey.keycode, 0,
                                                                     ev.xkey.state & ShiftMask ? 1 : 0)));
                        char *output;
                        if (send_receive_data(conn, ev.xkey.keycode, ev.xkey.state, FALSE, &output) == 0) {
                            LOG_DEBUG("\nserver return:%s\n\n", output);
                        } else {
                            LOG_ERR("server failed to return:%s", output);
                        }
                        free(output);
                        break;
                    }
                }
            }
        }
    }
    XCloseDisplay(dpy);
    tagList->free(tagList);
    windowList->free(windowList);
    return 0;
}

int x11_error_handler(Display *display, XErrorEvent *error) {
//    LOG_ERR("ERROR: X11 error");
    return 1;
}

void sig_handler(int signum) {
    printf("\nsignum:【%d】received,stopping\n", signum);
    running = False;
}

int main() {
    XSetErrorHandler(x11_error_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    return app_main();
}

