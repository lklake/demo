#include <stdio.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include "kbd_client.h"
#include "demo_error.h"


static Bool running = True;


int app_main() {
    DBusConnection *conn = get_connection();
    if (conn == NULL) {
        LOG_ERR("connect to server failed");
    }
    printf("请输入文件路径,不包含中文：\n");
    char *path = (char*)malloc(1024);
    int x,y;
    while(scanf("%s%d%d",path,&x,&y)!=EOF){
        printf("path:%s x:%d y:%d\n", path,x,y);
        char *req = (char *) malloc(strlen(path) + 1024);
        sprintf(req, "%s %d %d", path, x,y);
        printf("req:%s\n", req);
        // 发送从输入缓冲区接收到的数据
        char *output ;
        if (send_path(conn, req, &output) == 0) {
            LOG_DEBUG("\nserver return:%s\n\n", output);
        } else {
            LOG_ERR("server failed to return:%s", output);
        }
        free(output);
    }
    return 0;
}

void sig_handler(int signum) {
    printf("\nsignum:【%d】received,stopping\n", signum);
    running = False;
}

int main() {
//    signal(SIGTERM, sig_handler);
//    signal(SIGINT, sig_handler);
    return app_main();
}

