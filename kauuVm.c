#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

// グローバル変数としてDisplayへのポインタを保持
static Display *dpy;

// Chromiumを起動する関数
void launch_chromium() {
    pid_t pid = fork();
    if (pid == 0) {
        // 子プロセス
        // --no-sandboxは仮想環境などで実行する際に必要になることがあります
        // --start-fullscreen はブラウザ自身に全画面で起動するよう指示しますが、
        // WM側でも制御することで確実性を高めます。
        char *const args[] = {"/usr/bin/chromium", "--no-sandbox", "--start-fullscreen", NULL};
        execvp(args[0], args);
        // execvpが戻ってきたら、それはエラー
        perror("execvp");
        exit(1);
    } else if (pid < 0) {
        // fork失敗
        perror("fork");
        exit(1);
    }
    // 親プロセスは子のpidを待つ
    waitpid(pid, NULL, 0);
}

// X11のエラーハンドラ
int x_error_handler(Display *d, XErrorEvent *e) {
    // エラーをコンソールに出力するが、プログラムは終了させない
    char error_text[1024];
    XGetErrorText(d, e->error_code, error_text, sizeof(error_text));
    fprintf(stderr, "X Error: %s (request code: %d)\n", error_text, e->request_code);
    return 0; // 0を返して続行
}


int main(void) {
    // 1. Xサーバーに接続
    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    // X11のエラーハンドラを設定
    XSetErrorHandler(x_error_handler);

    // 2. スクリーンとルートウィンドウの情報を取得
    int screen = DefaultScreen(dpy);
    Window root_win = RootWindow(dpy, screen);
    unsigned int width = DisplayWidth(dpy, screen);
    unsigned int height = DisplayHeight(dpy, screen);

    // 3. ルートウィンドウのイベントを監視するよう設定
    // SubstructureRedirectMask: 新しいウィンドウの生成(Map)や設定変更(Configure)をWMが横取りするためのマスク
    // SubstructureNotifyMask: ウィンドウの破棄(Destroy)などを検知するためのマスク
    XSelectInput(dpy, root_win, SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(dpy, False); // 設定をサーバーに即時反映

    // 4. メインループ: Chromiumが終了するたびに再起動する
    while (1) {
        // 別スレッドや非同期処理を使わず、シンプルにfork + waitで実装
        // Chromiumが終了するまで launch_chromium() はブロックされる
        pid_t browser_pid = fork();
        if (browser_pid == 0) {
            // 子プロセスがブラウザを起動し、終了を待つ
            launch_chromium();
            exit(0); // ブラウザが終了したら子プロセスも終了
        } else if (browser_pid > 0) {
            // 親プロセス (ウィンドウマネージャ)
            XEvent ev;
            int browser_running = 1;

            // 5. イベントループ: ブラウザが起動している間、Xイベントを処理
            while (browser_running) {
                // シグナルをチェックしてブラウザプロセスが終了したか確認
                int status;
                pid_t result = waitpid(browser_pid, &status, WNOHANG);
                if (result == browser_pid) {
                    // 子プロセスが終了した
                    browser_running = 0;
                    break;
                }

                // Xイベントを処理 (イベントがなければ待機しない)
                while (XPending(dpy) > 0) {
                    XNextEvent(dpy, &ev);

                    // 6. 新しいウィンドウが表示されようとした時 (MapRequest)
                    if (ev.type == MapRequest) {
                        XMapRequestEvent *map_req = &ev.xmaprequest;
                        
                        // 7. ウィンドウを画面全体にリサイズして表示
                        XMoveResizeWindow(dpy, map_req->window, 0, 0, width, height);
                        XMapWindow(dpy, map_req->window);
                        printf("Window mapped and resized to fullscreen.\n");
                    }
                }
                 // CPUを使いすぎないように少し待つ
                usleep(100000); // 0.1秒
            }
            printf("Chromium process terminated. Relaunching...\n");
        } else {
            perror("fork");
            exit(1);
        }
    }

    // この部分は実行されないが、作法として
    XCloseDisplay(dpy);
    return 0;
}