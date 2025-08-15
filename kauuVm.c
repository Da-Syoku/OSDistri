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
		execlp("chromium", "chromium", NULL); //execlp is function to  replace prosses for new one

		perror("cant open browser!");
		exit(1);
	}
}

int main(void) {
	Display *dis;
	XEvent eve;

	dis = XOpenDisplay( NULL );

	open_bro();

	while (1) {
		XNextEvent(dis, &eve);
	}
	XCloseDisplay(dis);
	return 0;
}
