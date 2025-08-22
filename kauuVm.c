#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// --- グローバル変数 ---
static Display *dpy;
static volatile sig_atomic_t g_child_is_dead = 0;
static Atom wm_protocols;
static Atom wm_delete_window;

// --- シグナルハンドラ --- (変更なし)
void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
    g_child_is_dead = 1;
}

// ★変更点: 起動したプロセスのPIDを返すようにする
pid_t launch_chromium() {
    pid_t pid = fork();
    if (pid == 0) { // 子プロセス
        char *const args[] = {"/usr/bin/chromium", "--no-first-run", "--no-sandbox", "--start-maximized", NULL};
        execvp(args[0], args);
        perror("execvp");
        exit(1);
    } else if (pid < 0) { // fork失敗
        perror("fork");
        exit(1);
    }
    // 親プロセスは子のPIDを返す
    return pid;
}

int main(void) {
    // ★追加: 起動したブラウザのPIDを保存する変数
    pid_t browser_pid = -1;

    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    signal(SIGCHLD, sigchld_handler);
    
    wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
    wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

    int screen = DefaultScreen(dpy);
    Window root_win = RootWindow(dpy, screen);
    unsigned int width = DisplayWidth(dpy, screen);
    unsigned int height = DisplayHeight(dpy, screen);

    XSelectInput(dpy, root_win, SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(dpy, False);

    // --- メインループ ---
    while (1) {
        g_child_is_dead = 0;
        // ★変更点: 戻り値としてPIDを受け取る
        browser_pid = launch_chromium();
        printf("Launched Chromium with PID: %d\n", browser_pid);

        while (!g_child_is_dead) {
            XEvent ev;
            XNextEvent(dpy, &ev);

            switch (ev.type) {
                case MapRequest: {
                    // (この部分は変更なし)
                    Window win = ev.xmaprequest.window;
                    Window transient_for;
                    if (XGetTransientForHint(dpy, win, &transient_for)) {
                        XMapWindow(dpy, win);
                    } else {
                        XSetWMProtocols(dpy, win, &wm_delete_window, 1);
                        XMoveResizeWindow(dpy, win, 0, 0, width, height);
                        XMapWindow(dpy, win);
                    }
                    break;
                }
                case ClientMessage: {
                    if (ev.xclient.message_type == wm_protocols && (Atom)ev.xclient.data.l[0] == wm_delete_window) {
                        // ★★★★★ 最も重要な変更点 ★★★★★
                        // XKillClientではなく、保存しておいたPIDに直接シグナルを送る
                        if (browser_pid != -1) {
                            printf("WM_DELETE_WINDOW received, sending SIGTERM to PID %d\n", browser_pid);
                            kill(browser_pid, SIGTERM);
                        }
                    }
                    break;
                }
                case ConfigureRequest: {
                    // (この部分は変更なし)
                    XConfigureRequestEvent *req = &ev.xconfigurerequest;
                    XWindowChanges wc;
                    wc.x = req->x;
                    wc.y = req->y;
                    wc.width = req->width;
                    wc.height = req->height;
                    wc.border_width = req->border_width;
                    wc.sibling = req->above;
                    wc.stack_mode = req->detail;
                    XConfigureWindow(dpy, req->window, req->value_mask, &wc);
                    break;
                }
            } // switch
        } // inner while
        printf("SIGCHLD received, browser process terminated. Relaunching...\n");
    } // outer while

    XCloseDisplay(dpy);
    return 0;
}