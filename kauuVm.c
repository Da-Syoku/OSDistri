#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>

// グローバルでDisplayへのポインタと管理対象のウィンドウIDを持っておくと便利
Display *dis;
Window browser_win = None; // 最初は管理しているウィンドウはないのでNoneで初期化

// ブラウザを起動する関数はそのまま
void open_bro() {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        execlp("chromium", "chromium", "--no-sandbox", NULL); // --no-sandboxは環境によって必要
        // execlpが失敗した場合のみ、以下のコードが実行される
        perror("execlp failed");
        exit(1);
    }
}

int main(void) {
    Window root;
    XEvent eve;

    // Xサーバーに接続
    dis = XOpenDisplay(NULL);
    if (dis == NULL) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    // ルートウィンドウ（デスクトップ全体）を取得
    root = DefaultRootWindow(dis);

    // --- ここが重要(1) ---
    // ウィンドウマネージャーとして動作するために、Xサーバーに通知を要求する
    // SubstructureRedirectMask: 新しいウィンドウの表示リクエスト(MapRequest)を横取りする
    // SubstructureNotifyMask: 子ウィンドウの状態変化（破棄など）を通知してもらう
    XSelectInput(dis, root, SubstructureRedirectMask | SubstructureNotifyMask);

    // 最初にブラウザを起動
    open_bro();

    // イベントループ
    while (1) {
        XNextEvent(dis, &eve); // 次のイベントが来るまで待機

        // --- ここが重要(2) ---
        // イベントの種類によって処理を分岐
        if (eve.type == MapRequest) {
            // 新しいウィンドウが表示されようとしている
            XMapRequestEvent *map_req = &eve.xmaprequest;

            // まだブラウザを管理していなければ、これを管理対象にする
            if (browser_win == None) {
                browser_win = map_req->window;

                // 画面サイズを取得
                int screen_width = DisplayWidth(dis, DefaultScreen(dis));
                int screen_height = DisplayHeight(dis, DefaultScreen(dis));
                
                // ウィンドウを画面全体にリサイズして移動させる
                XMoveResizeWindow(dis, browser_win, 0, 0, screen_width, screen_height);
                
                // ウィンドウの表示を許可する
                XMapWindow(dis, browser_win);
            }
        } else if (eve.type == DestroyNotify) {
            // ウィンドウが破棄された
            XDestroyWindowEvent *destroy_ev = &eve.xdestroywindow;

            // 破棄されたのが管理していたブラウザのウィンドウなら
            if (destroy_ev->window == browser_win) {
                browser_win = None; // 管理対象がいなくなったのでリセット
                open_bro();         // 再度ブラウザを起動
            }
        }
    }

    XCloseDisplay(dis);
    return 0;
}