// simple_wm_minimal.c

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Chromiumを起動するだけのシンプルな関数
void launch_chromium() {
    pid_t pid = fork();
    if (pid == 0) { // 子プロセス
        char *const args[] = {"/usr/bin/chromium", "--no-first-run", "--no-sandbox", "--start-maximized", NULL};
        execvp(args[0], args);
        // execvpが戻ってきたらエラー
        perror("execvp");
        exit(1);
    } else if (pid < 0) { // fork失敗
        perror("fork");
        exit(1);
    }
    // 親プロセスは何もせず次に進む
}

int main(void) {
    Display *dpy;

    // 1. Xサーバーに接続する
    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    // 2. 基本的な情報を取得する
    int screen = DefaultScreen(dpy);
    Window root_win = RootWindow(dpy, screen);
    unsigned int width = DisplayWidth(dpy, screen);
    unsigned int height = DisplayHeight(dpy, screen);

    // 3. ウィンドウマネージャーとして動作するために、ルートウィンドウのイベントを監視する
    //    SubstructureRedirectMask: 新しいウィンドウの表示要求などをWMが横取りするために必要
    XSelectInput(dpy, root_win, SubstructureRedirectMask);
    XSync(dpy, False);

    // 4. Chromiumを一度だけ起動する
    launch_chromium();

    // 5. イベントループ (無限ループ)
    while (1) {
        XEvent ev;
        // イベントが来るまで待つ
        XNextEvent(dpy, &ev);

        // 6. もし新しいウィンドウの表示要求(MapRequest)が来たら
        if (ev.type == MapRequest) {
            Window win = ev.xmaprequest.window;
            Window transient_for;

            // 7. それがポップアップやメニュー(transient window)か確認する
            if (XGetTransientForHint(dpy, win, &transient_for)) {
                // ポップアップやメニューなら、そのまま表示するだけ
                XMapWindow(dpy, win);
            } else {
                // メインウィンドウなら、画面いっぱいにリサイズして表示する
                XMoveResizeWindow(dpy, win, 0, 0, width, height);
                XMapWindow(dpy, win);
            }
        }
    } // イベントループの終わり

    // (この部分は通常実行されない)
    XCloseDisplay(dpy);
    return 0;
}