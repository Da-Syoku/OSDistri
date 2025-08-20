#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>

void open_bro() {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        // --no-sandboxはテスト環境で役立つことが多い
        execlp("chromium", "chromium", "--user-data-dir=/tmp/mywm-chromium-profile", "--no-sandbox", NULL);
        perror("execlp failed");
        exit(1);
    }
}

int main(void) {
    Display *dis;
    Window root; // ルートウィンドウを格納する変数を追加
    XEvent eve;

    dis = XOpenDisplay(NULL);
    if (dis == NULL) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }
    
    // デスクトップ全体（ルートウィンドウ）を取得
    root = DefaultRootWindow(dis);

    // --- ★ステップ1：ここが最重要★ ---
    // Xサーバーに「新しいウィンドウの表示リクエストは私に送って」と宣言する
    // SubstructureRedirectMask がそのための魔法の言葉
    XSelectInput(dis, root, SubstructureRedirectMask);
    
    printf("ウィンドウマネージャーとして待機します。ブラウザを起動中...\n");

    open_bro();

    // --- ★ステップ2：イベントループを修正★ ---
    while (1) {
        // 次のイベントが来るまでここで待つ
        XNextEvent(dis, &eve);

        // もしイベントが「ウィンドウ表示リクエスト(MapRequest)」なら
        if (eve.type == MapRequest) {
            // イベント情報から、新しいウィンドウのIDを取り出す
            Window new_win = eve.xmaprequest.window;

            // ★目標達成！★
            // これでウィンドウIDが手に入った！ターミナルに表示してみる
            printf("新しいウィンドウを捕捉しました！ ウィンドウID: %lu\n", new_win);

            // とりあえず、ウィンドウの表示を許可する
            XMapWindow(dis, new_win);

            // これで、変数 new_win を使って、このウィンドウを自由に操作できる！
            // break; // とりあえず1つ捕まえたらループを抜けても良い
        }
    }

    printf("プログラムを終了します。\n");
    XCloseDisplay(dis);
    return 0;
}