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

// --- Chromiumを起動する関数 --- (変更なし)
pid_t launch_chromium() {
    pid_t pid = fork();
    if (pid == 0) {
        char *const args[] = {"/usr/bin/chromium", "--no-first-run", "--no-sandbox", "--start-maximized", NULL};
        execvp(args[0], args);
        perror("execvp");
        exit(1);
    } else if (pid < 0) {
        perror("fork");
        exit(1);
    }
    return pid;
}

int main(void) {
    pid_t browser_pid = -1;
    // ★追加: 管理対象のメインウィンドウのIDを保存する変数
    Window browser_win = None;

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

    while (1) {
        g_child_is_dead = 0;
        browser_pid = launch_chromium();
        // ★リセット: 新しいセッションではウィンドウIDは未定
        browser_win = None; 
        printf("Launched Chromium with PID: %d\n", browser_pid);

        while (!g_child_is_dead) {
            XEvent ev;
            XNextEvent(dpy, &ev);

            switch (ev.type) {
                case MapRequest: {
                    Window win = ev.xmaprequest.window;
                    Window transient_for;
                    if (XGetTransientForHint(dpy, win, &transient_for)) {
                        XMapWindow(dpy, win);
                    } else {
                        // ★追加: メインウィンドウのIDを保存
                        browser_win = win;
                        printf("Main browser window ID: %lu\n", browser_win);
                        
                        XSetWMProtocols(dpy, win, &wm_delete_window, 1);
                        XMoveResizeWindow(dpy, win, 0, 0, width, height);
                        XMapWindow(dpy, win);
                    }
                    break;
                }
                case ClientMessage: {
                    if (ev.xclient.message_type == wm_protocols && (Atom)ev.xclient.data.l[0] == wm_delete_window) {
                        if (browser_pid != -1) {
                            kill(browser_pid, SIGTERM);
                        }
                    }
                    break;
                }
                case ConfigureRequest: {
                    XConfigureRequestEvent *req = &ev.xconfigurerequest;
                    XWindowChanges wc;
                    wc.x = req->x; wc.y = req->y; wc.width = req.width; wc.height = req->height;
                    wc.border_width = req->border_width; wc.sibling = req->above; wc.stack_mode = req->detail;
                    XConfigureWindow(dpy, req->window, req->value_mask, &wc);
                    break;
                }
                // ★★★★★ 新規追加: ウィンドウが非表示/破棄されたイベントの処理 ★★★★★
                case UnmapNotify:
                case DestroyNotify: {
                    Window win;
                    if (ev.type == UnmapNotify) win = ev.xunmap.window;
                    else win = ev.xdestroywindow.window;

                    // イベントが発生したのが、管理対象のメインウィンドウなら
                    if (win == browser_win) {
                        printf("Main window unmapped or destroyed. Killing process %d to restart.\n", browser_pid);
                        // プロセスが生き残っている可能性があるので、明示的にkillする
                        if (browser_pid != -1) {
                            kill(browser_pid, SIGKILL); // SIGTERMより強力なSIGKILLで確実に終了させる
                        }
                        // browser_winをリセット
                        browser_win = None;
                    }
                    break;
                }
            } // switch
        } // inner while
        printf("SIGCHLD received, browser process terminated. Relaunching...\n");
    } // outer while

    XCloseDisplay(dpy);
    return 0;
}