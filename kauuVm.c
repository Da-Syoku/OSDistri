#include <stdio.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#include <unistd.h>




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
	Window root;
	Window browser_win = None;
	XEvent eve;
	Display* dis;

	dis = XOpenDisplay( NULL );
	if (dis = NULL) {
		fprintf(stderr, "Cannot open display\n");
		exit(1);
	}
	root = DefaultRootWindow(dis);
	XSelectInput(dis, root, SubstructureRedirectMask | SubstructureNotifyMask);
	open_bro();

	while (1) {
		XNextEvent(dis, &eve);//waiting while next event
		if (eve.type == MapRequest){

			XMapRequestEvent *map_req = &eve.xmaprequest;
			if (browser_win == None){
				browser_win = map_req->window;
				int screenw = DisplayWidth(dis, DefaultScreen(dis));
				int screenh = DisplayHeight(dis, DefaultScreen(dis));
				XMoveResizeWindow(dis, browser_win, 0, 0, screenw, screenh);
				XMapWindow(dis, browser_win);
			}
		} else if (eve.type == DestroyNotify){
			XDestroyWindowEvent *des_ev = &eve.xdestroywindow;
			if (des_ev->window == browser_win){
				browser_win = None;
				open_bro();
			}
		}
	}
	XCloseDisplay(dis);
	return 0;
}

