#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define WTYPE_NORMAL 0
#define WTYPE_DIALOG 1
#define WTYPE_POPUP  2

// --- グローバル変数 ---
static Display *dpy;
static volatile sig_atomic_t g_child_is_dead = 0;
static Atom wm_protocols;
static Atom wm_delete_window;
static Atom wm_take_focus;
static Atom net_wm_window_type;
static Atom net_wm_window_type_dialog;
static Atom net_wm_window_type_menu;
static Atom net_wm_window_type_dropdown_menu;
static Atom net_wm_window_type_popup_menu;
static Atom net_wm_window_type_toolbar;
static Atom net_wm_window_type_splash;


// --- シグナルハンドラ --
void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
    g_child_is_dead = 1;
}

// --- ブラウザを起動する関数 --
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

// リクエストされたウィンドウの種類を判別？
int get_window_type(Window win) {
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    Atom *data = NULL;
    
    // まず、ウィンドウタイプがDIALOGかチェック
    int status = XGetWindowProperty(dpy, win, net_wm_window_type, 0L, sizeof(Atom), False, XA_ATOM,
                                  &actual_type, &actual_format, &nitems, &bytes_after, (unsigned char **)&data);

    if (status == Success && data) {
        for (unsigned long i = 0; i < nitems; i++) {
            if (data[i] == net_wm_window_type_dialog) {
                XFree(data);
                return WTYPE_DIALOG; // ダイアログタイプ
            }
        }
        XFree(data);
        data = NULL; // dataをリセット
    }

    // 次に、一時的なウィンドウ(ポップアップメニューなど)かチェック
    Window transient_for;
    if (XGetTransientForHint(dpy, win, &transient_for)) {
        return WTYPE_POPUP;
    }

    // その他のポップアップタイプもチェック
    status = XGetWindowProperty(dpy, win, net_wm_window_type, 0L, sizeof(Atom), False, XA_ATOM,
                                  &actual_type, &actual_format, &nitems, &bytes_after, (unsigned char **)&data);

    if (status == Success && data) {
        for (unsigned long i = 0; i < nitems; i++) {
            if (data[i] == net_wm_window_type_menu ||
                data[i] == net_wm_window_type_dropdown_menu ||
                data[i] == net_wm_window_type_popup_menu ||
                data[i] == net_wm_window_type_toolbar ||
                data[i] == net_wm_window_type_splash)
            {
                XFree(data);
                return WTYPE_POPUP;
            }
        }
        XFree(data);
    }
    
    return WTYPE_NORMAL; // 上記のいずれでもなければ、通常のウィンドウ
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
    //  WM_TAKE_FOCUS の Atom を初期化
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

    while (1) {
 
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
                    // ★★★ 改善: 新しいウィンドウタイプ判別に基づき処理を分岐 ★★★
                    int w_type = get_window_type(win);
                    
                    switch (w_type) {
                        case WTYPE_NORMAL: // 通常ウィンドウの場合
                            browser_win = win;
                            printf("Main browser window ID: %lu\n", browser_win);
                            
                            Atom protocols[] = {wm_delete_window, wm_take_focus};
                            XSetWMProtocols(dpy, win, protocols, 2);

                            XMoveResizeWindow(dpy, win, 0, 0, width, height);
                            XMapWindow(dpy, win);
                            XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
                            break;
                        
                        case WTYPE_DIALOG: // ダイアログウィンドウの場合
                            printf("Dialog window detected: %lu\n", win);
                            // 画面の右半分に配置
                            XMoveResizeWindow(dpy, win, width / 2, 0, width / 2, height);
                            XMapWindow(dpy, win);
                            // ダイアログにフォーカスを当てる
                            XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
                            break;

                        case WTYPE_POPUP: // その他のポップアップの場合
                        default:
                            // サイズや位置は変更せず、そのまま表示
                            XMapWindow(dpy, win);
                            break;
                    }
                    break;
                }
                case ClientMessage: { //フォーカスってことは複数ウィンドウ弄るやつか
         
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
                case UnmapNotify: //ブラウザプロセスぶっ殺しゾーン
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
            }  //switch

            if (should_restart) { //フラグ付けして確実にループ抜ける
                break;
            } 
        }  //while

        printf("Browser process terminated. Relaunching...\n");
        
        if (browser_pid != -1) {
           waitpid(browser_pid, NULL, 0);
        }
    } 

    XCloseDisplay(dpy);
    return 0;
}