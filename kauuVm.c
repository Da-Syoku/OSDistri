#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
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
// ★★★ 追加: フォーカス管理用のAtom ★★★
static Atom wm_take_focus;

static Atom net_wm_window_type;
static Atom net_wm_window_type_dialog;
static Atom net_wm_window_type_menu;
static Atom net_wm_window_type_dropdown_menu;
static Atom net_wm_window_type_popup_menu;
static Atom net_wm_window_type_toolbar;
static Atom net_wm_window_type_splash;


// --- シグナルハンドラ --- (変更なし)
void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
    g_child_is_dead = 1;
}

// --- ブラウザを起動する関数 --- (変更なし)
pid_t launch_browser() {
    pid_t pid = fork();
    if (pid == 0) {
        char *const args[] = {"/usr/bin/firefox", "--new-instance", NULL};
        execvp(args[0], args);
        perror("execvp failed");
        exit(1);
    } else if (pid < 0) {
        perror("fork failed");
        exit(1);
    }
    return pid;
}

// --- is_popup ヘルパー関数 --- (変更なし)
int is_popup(Window win) {
    Window transient_for;
    if (XGetTransientForHint(dpy, win, &transient_for)) {
        return 1;
    }

    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    Atom *data = NULL;
    int status = XGetWindowProperty(dpy, win, net_wm_window_type, 0L, sizeof(Atom), False, XA_ATOM,
                                  &actual_type, &actual_format, &nitems, &bytes_after, (unsigned char **)&data);

    if (status == Success && data) {
        for (unsigned long i = 0; i < nitems; i++) {
            if (data[i] == net_wm_window_type_dialog ||
                data[i] == net_wm_window_type_menu ||
                data[i] == net_wm_window_type_dropdown_menu ||
                data[i] == net_wm_window_type_popup_menu ||
                data[i] == net_wm_window_type_toolbar ||
                data[i] == net_wm_window_type_splash)
            {
                XFree(data);
                return 1;
            }
        }
        XFree(data);
    }
    
    return 0;
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
    // ★★★ 追加: WM_TAKE_FOCUS の Atom を初期化 ★★★
    wm_take_focus = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
    
    net_wm_window_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    net_wm_window_type_dialog = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    // (以下、他のAtom初期化は変更なし)
    net_wm_window_type_menu = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_MENU", False);
    net_wm_window_type_dropdown_menu = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", False);
    net_wm_window_type_popup_menu = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_POPUP_MENU", False);
    net_wm_window_type_toolbar = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);
    net_wm_window_type_splash = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_SPLASH", False);

    int screen = DefaultScreen(dpy);
    Window root_win = RootWindow(dpy, screen);
    unsigned int width = DisplayWidth(dpy, screen);
    unsigned int height = DisplayHeight(dpy, screen);

    XSelectInput(dpy, root_win, SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(dpy, False);

    while (1) {
        // ... (ループ開始部分は変更なし)
        g_child_is_dead = 0;
        browser_pid = launch_browser();
        browser_win = None; 
        printf("Launched Browser with PID: %d\n", browser_pid);
        
        int should_restart = 0;

        while (!g_child_is_dead) {
            XEvent ev;
            XNextEvent(dpy, &ev);

            switch (ev.type) {
                case MapRequest: {
                    Window win = ev.xmaprequest.window;
                    
                    if (is_popup(win)) {
                        XMapWindow(dpy, win);
                    } else {
                        browser_win = win;
                        printf("Main browser window ID: %lu\n", browser_win);
                        
                        // ★★★ 変更: WM_TAKE_FOCUS をプロトコルに追加 ★★★
                        Atom protocols[] = {wm_delete_window, wm_take_focus};
                        XSetWMProtocols(dpy, win, protocols, 2);

                        XMoveResizeWindow(dpy, win, 0, 0, width, height);
                        XMapWindow(dpy, win);

                        // ★★★ 追加: メインウィンドウに即座にフォーカスを当てる ★★★
                        XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
                    }
                    break;
                }
                case ClientMessage: {
                    // ★★★ 変更: WM_TAKE_FOCUS の処理を追加 ★★★
                    if (ev.xclient.message_type == wm_protocols) {
                        if ((Atom)ev.xclient.data.l[0] == wm_delete_window) {
                            if (browser_pid != -1) {
                                kill(browser_pid, SIGTERM);
                            }
                        } 
                        else if ((Atom)ev.xclient.data.l[0] == wm_take_focus) {
                            printf("WM_TAKE_FOCUS received for window %lu\n", ev.xclient.window);
                            // アプリケーションからの要求に応じてフォーカスを設定
                            XSetInputFocus(dpy, ev.xclient.window, RevertToPointerRoot, ev.xclient.data.l[1]);
                        }
                    }
                    break;
                }
                // ... (ConfigureRequest, UnmapNotify/DestroyNotify は変更なし) ...
                case ConfigureRequest: {
                    XConfigureRequestEvent *req = &ev.xconfigurerequest;
                    XWindowChanges wc;
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
                        should_restart = 1;
                    }
                    break;
                }
            } // switch
            
            if (should_restart) {
                break;
            }
        } // inner while

        printf("Browser process terminated. Relaunching...\n");
        
        if (browser_pid != -1) {
           waitpid(browser_pid, NULL, 0);
        }
    } // outer while

    XCloseDisplay(dpy);
    return 0;
}