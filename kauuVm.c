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

// --- シグナルハンドラ ---
// 子プロセスが終了したときにOSから送られるSIGCHLDシグナルを捕捉します
void sigchld_handler(int sig) {
    // waitpidで終了した子プロセスの情報を回収します（ゾンビプロセス化を防ぐため）
    // WNOHANGを指定することで、終了した子プロセスがいない場合でもブロックしません
    while (waitpid(-1, NULL, WNOHANG) > 0);
    // 子プロセスが終了したことを示すフラグを立てます
    g_child_is_dead = 1;
}

// --- Chromiumを起動する関数 ---
pid_t launch_chromium() {
    pid_t pid = fork();
    if (pid == 0) { // 子プロセス
        // 実行するコマンドと引数を設定します
        char *const args[] = {"/usr/bin/firefox", NULL};
        // execvpでChromiumを起動します。成功した場合、この先のコードは実行されません。
        execvp(args[0], args);
        // execvpが失敗した場合のみ、以下のコードが実行されます
        perror("execvp failed");
        exit(1);
    } else if (pid < 0) { // fork失敗
        perror("fork failed");
        exit(1);
    }
    // 親プロセスは子プロセスのPIDを返します
    return pid;
}

int main(void) {
    pid_t browser_pid = -1;
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

    // メインの無限ループ。Chromiumが終了するたびにここから再起動します
    while (1) {
        g_child_is_dead = 0;
        browser_pid = launch_chromium();
        browser_win = None; 
        printf("Launched Chromium with PID: %d\n", browser_pid);
        
        // ★★★ 改善点①: ループを抜けるためのフラグを導入 ★★★
        int should_restart = 0;

        // 子プロセスが生きている間、イベントを処理し続けるループ
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
                    // ★★★ 修正点: `req.width` を `req->width` などに修正 ★★★
                    wc.x = req->x; wc.y = req->y; wc.width = req->width; wc.height = req->height;
                    wc.border_width = req->border_width; wc.sibling = req->above; wc.stack_mode = req->detail;
                    XConfigureWindow(dpy, req->window, req->value_mask, &wc);
                    break;
                }
                case UnmapNotify:
                case DestroyNotify: {
                    Window win;
                    if (ev.type == UnmapNotify) win = ev.xunmap.window;
                    else win = ev.xdestroywindow.window;

                    if (win == browser_win) {
                        printf("Main window unmapped or destroyed. Killing process %d to restart.\n", browser_pid);
                        if (browser_pid != -1) {
                            kill(browser_pid, SIGKILL);
                        }
                        browser_win = None;
                        // ★★★ 改善点②: フラグを立てて、ループを抜ける準備をする ★★★
                        should_restart = 1;
                    }
                    break;
                }
            } // switch
            
            // ★★★ 改善点③: フラグが立っていたら、イベントループを抜ける ★★★
            if (should_restart) {
                break;
            }
        } // inner while

        printf("Browser process terminated. Relaunching...\n");

        // プロセスが完全に終了するのを待つことで、より安定した再起動ができます
        if (browser_pid != -1) {
           waitpid(browser_pid, NULL, 0);
        }
    } // outer while

    XCloseDisplay(dpy);
    return 0;
}