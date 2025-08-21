#include <X11/Xlib.h>
#include <X11/Xutil.h> // XGetTransientForHint のために追加
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// --- グローバル変数 ---
static Display *dpy;
// 子プロセスが終了したことを通知するためのフラグ
// volatile: コンパイラの最適化で意図せず変更されないようにする
// sig_atomic_t: シグナルハンドラ内で安全にアクセスできる整数型
static volatile sig_atomic_t g_child_is_dead = 0;
static Atom wm_protocols;
static Atom wm_delete_window;

// --- シグナルハンドラ ---
// 子プロセスが終了した(SIGCHLD)ときに呼ばれる関数
void sigchld_handler(int sig) {
    // ゾンビプロセスが残らないように待つ
    while (waitpid(-1, NULL, WNOHANG) > 0);
    g_child_is_dead = 1;
}

// --- Chromiumを起動する関数 ---
void launch_chromium() {
    // fork & exec の部分は変更なし
    pid_t pid = fork();
    if (pid == 0) {
        // 子プロセス
        char *const args[] = {"/usr/bin/chromium", "--no-sandbox", NULL};
        execvp(args[0], args);
        perror("execvp");
        exit(1);
    } else if (pid < 0) {
        perror("fork");
        exit(1);
    }
    // 親プロセスは子のpidを返さず、シグナルを待つ
}

int main(void) {
    // 1. Xサーバーに接続
    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    // 2. シグナルハンドラを設定
    signal(SIGCHLD, sigchld_handler);

    //詳細データの入手
    wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
    wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

    // 3. スクリーンとルートウィンドウの情報を取得
    int screen = DefaultScreen(dpy);
    Window root_win = RootWindow(dpy, screen);
    unsigned int width = DisplayWidth(dpy, screen);
    unsigned int height = DisplayHeight(dpy, screen);

    // 4. ルートウィンドウのイベントを監視するよう設定
    XSelectInput(dpy, root_win, SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(dpy, False);

    // 5. メインループ: Chromiumが終了するたびに再起動
    while (1) {
        g_child_is_dead = 0;
        launch_chromium();

        // 6. イベントループ: 子プロセスが死ぬまでイベントを処理
        //    CPUを消費しない効率的なループ
        while (!g_child_is_dead) {
            XEvent ev;
            // イベントが来るまでここでブロック(待機)する
            XNextEvent(dpy, &ev);

            switch (ev.type){

                // 7. 新しいウィンドウが表示されようとした時 (MapRequest)
                case MapRequest: {
                    Window win = ev.xmaprequest.window;
                    Window transient_for;
                    XGetTransientForHint(dpy, win, &transient_for);
                    // 8. ★重要★ 一時的なウィンドウ(ポップアップ等)でないかチェック
                   // XGetTransientForHintがTrueを返した場合、それはダイアログやツールチップ
                    if (XGetTransientForHint(dpy, win, &transient_for)) {
                        // 一時的なウィンドウはWMの管理対象外とし、そのまま表示
                     XMapWindow(dpy, win);
                     } else {
                    // メインウィンドウと判断し、最大化して表示
                    XSetWMProtocols(dpy, win, &wm_delete_window, 1);
                    XMoveResizeWindow(dpy, win, 0, 0, width, height);
                    XMapWindow(dpy, win);
                    printf("Main window mapped and maximized.\n");
                    }
                break; // イベント処理を抜ける
                }

                //configureRequestイベントの処理
                 case ClientMessage: {
                if (ev.xclient.message_type == wm_protocols && (Atom)ev.xclient.data.l[0] == wm_delete_window) {
                    XKillClient(dpy, ev.xclient.window);
                    printf("Window closed by client message.\n");
                }
                break;
                }
                 case ConfigureRequest: {
                XConfigureRequestEvent *req = &ev.xconfigurerequest;
                XWindowChange wc;
                wc.x = req->x;
                    wc.y = req->y;
                    wc.width = req->width;
                    wc.height = req->height;
                    wc.border_width = req->border_width;
                    wc.sibling = req->above;
                    wc.stack_mode = req->detail;
                    
                    XConfigureWindow(dpy, req->window, req->value_mask, &wc);
                    printf("ConfigureRequest handled for window.\n");
                    break;
                }
            }
        }
        printf("Chromium process terminated. Relaunching...\n");
    }

    XCloseDisplay(dpy);
    return 0;
}