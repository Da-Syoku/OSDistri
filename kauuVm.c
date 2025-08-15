#include <stdio.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#include <unistd.h>

Display dis;

void open_bro() {

	pid_t pid = fork();

	if (pid == -1) { //If pid is 0, pid is child prosess
		perror("cant get proccese id!");
		exit(1);
	} else if (pid ==0) {
		execlp("chromium", "chromium", "--no-sandbox", NULL); //execlp is function to  replace prosses for new one

		perror("cant open browser!");
		exit(1);
	}
}

int main(void) {
	
	XEvent eve;

	dis = XOpenDisplay( NULL );
	if (dis == NULL){
		fprintf(stderr, "cannot open display\n");
	}

	XSelectInput(dis, SubstructureRedirectMask | SubstructureNotifyMask);

	open_bro();

	while (1) {
		XNextEvent(dis, &eve);
		if (eve.type == MapRequest) {
            // 新しいウィンドウが表示されようとしている
            XMapRequestEvent *map_req = &eve.xmaprequest;

            // まだブラウザを管理していなければ、これを管理対象にする
            if (dis == None) {
                dis = map_req->window;

                // 画面サイズを取得
                int screen_width = DisplayWidth(dis, DefaultScreen(dis));
                int screen_height = DisplayHeight(dis, DefaultScreen(dis));
                
                // ウィンドウを画面全体にリサイズして移動させる
                XMoveResizeWindow(dis, 0, 0, screen_width, screen_height);
                
                // ウィンドウの表示を許可する
                XMapWindow(dis);
            }
        } else if (eve.type == DestroyNotify) {
            // ウィンドウが破棄された
            XDestroyWindowEvent *destroy_ev = &eve.xdestroywindow;

            // 破棄されたのが管理していたブラウザのウィンドウなら
            if (destroy_ev->window == dis) {
                dis = None; // 管理対象がいなくなったのでリセット
                open_bro();         // 再度ブラウザを起動
            }
        }
	}
	XCloseDisplay(dis);
	return 0;
}
