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
static Atom net_wm_window_type;
static Atom net_wm_window_type_dialog;
static Atom net_wm_window_type_menu;
static Atom net_wm_window_type_dropdown_menu;
static Atom net_wm_window_type_popup_menu;
static Atom net_wm_window_type_toolbar;
static Atom net_wm_window_type_splash;
static Atom wm_take_focus;
// --- シグナルハンドラ ---
// 子プロセスが終了したときにOSから送られるSIGCHLDシグナルを捕捉します
void sigchld_handler(int sig) {
    // waitpidで終了した子プロセスの情報を回収します（ゾンビプロセス化を防ぐため）
    // WNOHANGを指定することで、終了した子プロセスがいない場合でもブロックしません
    while (waitpid(-1, NULL, WNOHANG) > 0);
    // 子プロセスが終了したことを示すフラグを立てます
    g_child_is_dead = 1;
}

// --- Firefoxを起動する関数 ---
pid_t launch_firefox() {
    pid_t pid = fork();
    if (pid == 0) { // 子プロセス
        // 実行するコマンドと引数を設定します
        char *const args[] = {"/usr/bin/firefox", NULL};
        // execvpでFirefoxを起動します。成功した場合、この先のコードは実行されません。
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

int is_popup(Window win) {
    Window transient_for;
    // まず、従来のWM_TRANSIENT_FORヒントをチェックします
    if (XGetTransientForHint(dpy, win, &transient_for)) {
        return 1;
    }

    // 次に、_NET_WM_WINDOW_TYPEプロパティをチェックします
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    Atom *data = NULL;
    int status = XGetWindowProperty(dpy, win, net_wm_window_type, 0L, sizeof(Atom), False, XA_ATOM, &actual_type, &actual_format, &nitems, &bytes_after, (unsigned char **)&data);

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
                return 1; // ポップアップ系のウィンドウタイプならtrue
            }
        }
        XFree(data);
    }
    
    return 0; // どちらにも当てはまらなければ、メインウィンドウと判断
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
    wm_take_focus = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
    net_wm_window_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    net_wm_window_type_dialog = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
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

    // メインの無限ループ。Chromiumが終了するたびにここから再起動します
    while (1) {
        g_child_is_dead = 0;
        browser_pid = launch_firefox();
        browser_win = None; 
        printf("Launched Firefox with PID: %d\n", browser_pid);


        int should_restart = 0;

        // 子プロセスが生きている間、イベントを処理し続けるループ
        while (!g_child_is_dead) {
            XEvent ev;
            XNextEvent(dpy, &ev);

            switch (ev.type) {
                case MapRequest: {
                    Window win = ev.xmaprequest.window;
                    
                    // ★★★ 改善: 新しい判定関数 is_popup() を使う ★★★
                    if (is_popup(win)) {
                        // ポップアップやメニューはそのまま表示
                        XMapWindow(dpy, win);
                    } else {
                        // メインウィンドウと判断した場合の処理
                        browser_win = win;
                        printf("Main browser window ID: %lu\n", browser_win);
                        Atom protocols[] = {wm_delete_window, wm_take_focus};
                        XSetWMProtocols(dpy, win, protocols, 2);
                        XMoveResizeWindow(dpy, win, 0, 0, width, height);
                        XMapWindow(dpy, win);
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

        // プロセスが完全に終了するのを待つことで、より安定した再起動ができます
        if (browser_pid != -1) {
           waitpid(browser_pid, NULL, 0);
        }
    } // outer while

    XCloseDisplay(dpy);
    return 0;
}