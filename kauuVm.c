#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

int main(void) {
    Display *dis;
    XEvent eve;

    dis = XOpenDisplay(NULL);
    if (dis == NULL) {
        // ログファイルに書き出されるように、fprintf(stderr, ...) を使う
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }
    
    Window root = DefaultRootWindow(dis);

    // ウィンドウマネージャーとして登録する
    XSelectInput(dis, root, SubstructureRedirectMask);
    
    // 標準エラー出力にメッセージを出す（ログファイルで確認するため）
    fprintf(stderr, "BareWM started. Waiting for windows...\n");

    // イベントループ
    while (1) {
        XNextEvent(dis, &eve);

        if (eve.type == MapRequest) {
            fprintf(stderr, "Caught a new window! ID: %lu\n", eve.xmaprequest.window);
            
            // ウィンドウの表示を許可するだけ
            XMapWindow(dis, eve.xmaprequest.window);
        }
    }

    XCloseDisplay(dis); // この行には到達しないが、作法として記述
    return 0;
}