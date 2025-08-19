#define TEST013
#ifdef TEST01N
#endif
#ifdef TEST013
// glx_xlib_ui.c
// Minimal Xlib + GLX UI with menu bar, toolbar, GLX draw area, cmd input bar, status bar,
// plus a tiny message bus routing events among components.
//
// Build (Linux / *BSD):
//   gcc glx_xlib_ui.c -o glx_xlib_ui -lX11 -lGL
//
// Run:
//   ./glx_xlib_ui
//
// What it does:
// - Lays out 5 child windows in a parent X window
// - Attaches a GLX context to the draw area child
// - Sends/receives messages between components:
//     * Typing in the cmd bar + Enter sends a CMD_EXEC -> draw area (changes color)
//     * Clicking a toolbar button sends TOOL_CLICK -> status bar (updates text)
//     * Clicking a menu item sends MENU_ACTION -> status bar (updates text)
//
// Notes:
// - This is intentionally compact; a real app would split files and add error checks.
// - Uses only Xlib + GLX (no Xt, no Motif, no Xft). Text via XDrawString.
// - All children share the same visual/colormap selected for GLX for simplicity.

#define _POSIX_C_SOURCE 200809L
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define APP_MIN_W 640
#define APP_MIN_H 400

// Layout (pixels)
enum {
    MENU_H   = 24,
    TOOL_H   = 32,
    INPUT_H  = 28,
    STATUS_H = 20,
    PAD      = 0
};

// Components
typedef enum {
    COMP_MENU,
    COMP_TOOLBAR,
    COMP_DRAW,
    COMP_CMD,
    COMP_STATUS,
    COMP_COUNT
} ComponentID;

// Message types
typedef enum {
    MSG_NONE = 0,
    MSG_CMD_EXEC,      // payload: command string
    MSG_TOOL_CLICK,    // payload: "Tool<n>"
    MSG_MENU_ACTION,   // payload: "File|Edit|Help:<item>"
    MSG_STATUS_SET,    // payload: status text
    MSG_DRAW_SET_COLOR // payload: "r g b"
} MessageType;

typedef struct {
    MessageType type;
    ComponentID from;
    ComponentID to;        // COMP_COUNT means broadcast
    char payload[256];
} Message;

// Very small single-producer/single-consumer queue for demo
#define MSGQ_MAX 64
typedef struct {
    Message q[MSGQ_MAX];
    int head, tail, count;
} MsgQueue;

static void msgq_init(MsgQueue *mq){ mq->head=mq->tail=mq->count=0; }
static bool msgq_push(MsgQueue *mq, Message m){
    if(mq->count==MSGQ_MAX) return false;
    mq->q[mq->tail]=m; mq->tail=(mq->tail+1)%MSGQ_MAX; mq->count++; return true;
}
static bool msgq_pop(MsgQueue *mq, Message *out){
    if(mq->count==0) return false;
    *out = mq->q[mq->head]; mq->head=(mq->head+1)%MSGQ_MAX; mq->count--; return true;
}

// Global app state
typedef struct {
    Display *dpy;
    int screen;
    Window root;
    XVisualInfo *vi;
    Colormap cmap;
    GLXContext glctx;

    Window main;
    Window comps[COMP_COUNT]; // child windows

    GC gc;

    // UI state
    int win_w, win_h;
    char status_text[256];
    char cmd_buffer[256];
    int  cmd_len;

    // Draw area state
    float clear_r, clear_g, clear_b;

    // Message bus
    MsgQueue bus;
} App;

static void fatal(const char* msg){ fprintf(stderr, "FATAL: %s\n", msg); exit(1); }

static void send_msg(App* app, ComponentID from, ComponentID to, MessageType t, const char* payload){
    Message m = { .type=t, .from=from, .to=to };
    if(payload) strncpy(m.payload, payload, sizeof(m.payload)-1);
    if(!msgq_push(&app->bus, m)){
        fprintf(stderr, "Message queue full, dropping message\n");
    }
}

static void set_status(App* app, const char* txt){
    strncpy(app->status_text, txt, sizeof(app->status_text)-1);
    XClearWindow(app->dpy, app->comps[COMP_STATUS]);
    XDrawString(app->dpy, app->comps[COMP_STATUS], app->gc, 6, STATUS_H-6, app->status_text, (int)strlen(app->status_text));
}

static void layout_children(App* app){
    int w = app->win_w, h = app->win_h;
    if(w<APP_MIN_W) w=APP_MIN_W;
    if(h<APP_MIN_H) h=APP_MIN_H;

    int y = 0;
    XMoveResizeWindow(app->dpy, app->comps[COMP_MENU],    0, y, w, MENU_H); y += MENU_H + PAD;
    XMoveResizeWindow(app->dpy, app->comps[COMP_TOOLBAR], 0, y, w, TOOL_H); y += TOOL_H + PAD;

    int draw_h = h - (MENU_H+TOOL_H+INPUT_H+STATUS_H + 3*PAD);
    if(draw_h<50) draw_h=50;
    XMoveResizeWindow(app->dpy, app->comps[COMP_DRAW],    0, y, w, draw_h); y += draw_h + PAD;

    XMoveResizeWindow(app->dpy, app->comps[COMP_CMD],     0, y, w, INPUT_H); y += INPUT_H + PAD;
    XMoveResizeWindow(app->dpy, app->comps[COMP_STATUS],  0, y, w, STATUS_H);
}

static void draw_menu(App* app){
    // Fake static menu: [File] [Edit] [Help]
    XClearWindow(app->dpy, app->comps[COMP_MENU]);
    XDrawString(app->dpy, app->comps[COMP_MENU], app->gc, 10, MENU_H-8, "File  Edit  Help", 16);
}

static void draw_toolbar(App* app){
    // Fake buttons: [Tool1] [Tool2] [Tool3]
    XClearWindow(app->dpy, app->comps[COMP_TOOLBAR]);
    const char* labels[] = {"[Tool1]", "[Tool2]", "[Tool3]"};
    int x = 8;
    for(int i=0;i<3;i++){
        XDrawString(app->dpy, app->comps[COMP_TOOLBAR], app->gc, x, TOOL_H-10, labels[i], (int)strlen(labels[i]));
        x += 80;
    }
}

static void draw_cmdbar(App* app){
    XClearWindow(app->dpy, app->comps[COMP_CMD]);
    char prompt[300];
    snprintf(prompt, sizeof(prompt), "> %s", app->cmd_buffer);
    XDrawString(app->dpy, app->comps[COMP_CMD], app->gc, 6, INPUT_H-8, prompt, (int)strlen(prompt));
}

static void glx_redraw(App* app){
    glClearColor(app->clear_r, app->clear_g, app->clear_b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // Draw a simple triangle for proof-of-life
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glBegin(GL_TRIANGLES);
        glColor3f(1,0,0); glVertex2f(-0.6f, -0.6f);
        glColor3f(0,1,0); glVertex2f( 0.6f, -0.6f);
        glColor3f(0,0,1); glVertex2f( 0.0f,  0.6f);
    glEnd();

    glXSwapBuffers(app->dpy, app->comps[COMP_DRAW]);
}

static void handle_menu_click(App* app, int x, int y){
    // Super crude hit test: 3 zones
    const char* which = "Unknown";
    if(x<60) which="File:Open";
    else if(x<120) which="Edit:Copy";
    else which="Help:About";
    send_msg(app, COMP_MENU, COMP_STATUS, MSG_MENU_ACTION, which);
}

static void handle_toolbar_click(App* app, int x, int y){
    int idx = x/80;
    if(idx<0) idx=0; if(idx>2) idx=2;
    char buf[64]; snprintf(buf, sizeof(buf), "Tool%d", idx+1);
    send_msg(app, COMP_TOOLBAR, COMP_STATUS, MSG_TOOL_CLICK, buf);
}

static void handle_cmd_input(App* app, XKeyEvent* kev){
    KeySym ks; char txt[32]; int n = XLookupString(kev, txt, sizeof(txt)-1, &ks, NULL);
    if(n>0){
        txt[n] = '\0';
        if(txt[0] == '\r' || txt[0] == '\n'){
            // Execute command -> draw area
            send_msg(app, COMP_CMD, COMP_DRAW, MSG_CMD_EXEC, app->cmd_buffer);
            app->cmd_len = 0; app->cmd_buffer[0] = '\0';
            draw_cmdbar(app);
            return;
        }else if((unsigned char)txt[0] >= 32){
            if(app->cmd_len < (int)sizeof(app->cmd_buffer)-1){
                app->cmd_buffer[app->cmd_len++] = txt[0];
                app->cmd_buffer[app->cmd_len] = '\0';
            }
        }
    }else{
        if(ks == XK_BackSpace && app->cmd_len>0){
            app->cmd_len--; app->cmd_buffer[app->cmd_len]='\0';
        }
    }
    draw_cmdbar(app);
}

static void process_message(App* app, const Message* m){
    // Simple routing per recipient
    if(m->to != COMP_COUNT){
        // direct
        switch(m->to){
            case COMP_STATUS:
                if(m->type==MSG_TOOL_CLICK){
                    char out[512]; snprintf(out, sizeof(out), "Toolbar: %s clicked", m->payload);
                    set_status(app, out);
                }else if(m->type==MSG_MENU_ACTION){
                    char out[512]; snprintf(out, sizeof(out), "Menu: %s", m->payload);
                    set_status(app, out);
                }else if(m->type==MSG_STATUS_SET){
                    set_status(app, m->payload);
                }
                break;
            case COMP_DRAW:
                if(m->type==MSG_CMD_EXEC){
                    // Parse command: "color r g b" or "status <text>"
                    if(strncmp(m->payload, "color", 5)==0){
                        float r=0, g=0, b=0;
                        if(sscanf(m->payload+5, "%f %f %f", &r,&g,&b)==3){
                            if(r<0)r=0; if(r>1)r=1;
                            if(g<0)g=0; if(g>1)g=1;
                            if(b<0)b=0; if(b>1)b=1;
                            app->clear_r=r; app->clear_g=g; app->clear_b=b;
                            glx_redraw(app);
                            char s[128]; snprintf(s, sizeof(s), "Set color to %.2f %.2f %.2f", r,g,b);
                            send_msg(app, COMP_DRAW, COMP_STATUS, MSG_STATUS_SET, s);
                        }else{
                            send_msg(app, COMP_DRAW, COMP_STATUS, MSG_STATUS_SET, "Usage: color r g b (0..1)");
                        }
                    }else if(strncmp(m->payload, "status ", 7)==0){
                        send_msg(app, COMP_DRAW, COMP_STATUS, MSG_STATUS_SET, m->payload+7);
                    }else{
                        send_msg(app, COMP_DRAW, COMP_STATUS, MSG_STATUS_SET, "Unknown command. Try: color r g b");
                    }
                }else if(m->type==MSG_DRAW_SET_COLOR){
                    float r,g,b;
                    if(sscanf(m->payload, "%f %f %f", &r,&g,&b)==3){
                        app->clear_r=r; app->clear_g=g; app->clear_b=b;
                        glx_redraw(app);
                    }
                }
                break;
            default: break;
        }
    }else{
        // broadcast (unused in this demo)
    }
}

int main(void){
    App app = {0};
    app.dpy = XOpenDisplay(NULL);
    if(!app.dpy) fatal("Cannot open display");
    app.screen = DefaultScreen(app.dpy);
    app.root = RootWindow(app.dpy, app.screen);

    // Choose a GLX visual
    int attr[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 24, None };
    app.vi = glXChooseVisual(app.dpy, app.screen, attr);
    if(!app.vi) fatal("No appropriate GLX visual");
    app.cmap = XCreateColormap(app.dpy, app.root, app.vi->visual, AllocNone);

    XSetWindowAttributes swa = {0};
    swa.colormap = app.cmap;
    swa.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;
    app.main = XCreateWindow(app.dpy, app.root,
                             100, 100, APP_MIN_W, APP_MIN_H, 0,
                             app.vi->depth, InputOutput, app.vi->visual,
                             CWColormap | CWEventMask, &swa);
    XStoreName(app.dpy, app.main, "GLX + Xlib UI Demo");
    Atom wm_delete = XInternAtom(app.dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(app.dpy, app.main, &wm_delete, 1);

    // Create child windows (all same visual/colormap)
    unsigned long child_mask = CWColormap | CWEventMask;
    XSetWindowAttributes cwa = {0};
    cwa.colormap = app.cmap;
    cwa.event_mask = ExposureMask | ButtonPressMask | KeyPressMask;

    for(int i=0;i<COMP_COUNT;i++){
        app.comps[i] = XCreateWindow(app.dpy, app.main, 0,0, 100,100, 0,
                                     app.vi->depth, InputOutput, app.vi->visual,
                                     child_mask, &cwa);
        XMapWindow(app.dpy, app.comps[i]);
    }
    XMapWindow(app.dpy, app.main);

    // Create GLX context for draw area
    app.glctx = glXCreateContext(app.dpy, app.vi, NULL, True);
    if(!app.glctx) fatal("Failed to create GLX context");
    glXMakeCurrent(app.dpy, app.comps[COMP_DRAW], app.glctx);

    // GC for text rendering
    XGCValues gcv = {0};
    app.gc = XCreateGC(app.dpy, app.main, 0, &gcv);

    // Initial state
    app.win_w = APP_MIN_W; app.win_h = APP_MIN_H;
    app.clear_r = 0.1f; app.clear_g = 0.1f; app.clear_b = 0.12f;
    app.cmd_len = 0; app.cmd_buffer[0]='\0';
    snprintf(app.status_text, sizeof(app.status_text), "Ready. Try: color 0.2 0.5 0.8");
    msgq_init(&app.bus);

    layout_children(&app);
    draw_menu(&app);
    draw_toolbar(&app);
    draw_cmdbar(&app);
    set_status(&app, app.status_text);
    glx_redraw(&app);

    // Event loop
    for(;;){
        while(XPending(app.dpy)){
            XEvent ev; XNextEvent(app.dpy, &ev);
            if(ev.xany.window == app.main){
                if(ev.type == ConfigureNotify){
                    app.win_w = ev.xconfigure.width;
                    app.win_h = ev.xconfigure.height;
                    layout_children(&app);
                }else if(ev.type == ClientMessage){
                    if((Atom)ev.xclient.data.l[0] == (Atom)wm_delete){
                        goto cleanup;
                    }
                }
            }else{
                // Child windows
                if(ev.type == Expose){
                    if(ev.xany.window == app.comps[COMP_MENU]) draw_menu(&app);
                    else if(ev.xany.window == app.comps[COMP_TOOLBAR]) draw_toolbar(&app);
                    else if(ev.xany.window == app.comps[COMP_CMD]) draw_cmdbar(&app);
                    else if(ev.xany.window == app.comps[COMP_STATUS]) set_status(&app, app.status_text);
                    else if(ev.xany.window == app.comps[COMP_DRAW]) { glXMakeCurrent(app.dpy, app.comps[COMP_DRAW], app.glctx); glx_redraw(&app); }
                }else if(ev.type == ButtonPress){
                    if(ev.xany.window == app.comps[COMP_MENU]){
                        handle_menu_click(&app, ev.xbutton.x, ev.xbutton.y);
                    }else if(ev.xany.window == app.comps[COMP_TOOLBAR]){
                        handle_toolbar_click(&app, ev.xbutton.x, ev.xbutton.y);
                    }
                }else if(ev.type == KeyPress){
                    if(ev.xany.window == app.comps[COMP_CMD]){
                        handle_cmd_input(&app, &ev.xkey);
                    }
                }
            }
        }

        // Handle messages
        Message m;
        while(msgq_pop(&app.bus, &m)){
            process_message(&app, &m);
        }

        // Simple idle: redraw draw area occasionally (optional)
        // Here we keep it event-driven; uncomment for animation:
        // struct timespec ts={0, 16*1000*1000}; nanosleep(&ts, NULL);
    }

cleanup:
    glXMakeCurrent(app.dpy, None, NULL);
    glXDestroyContext(app.dpy, app.glctx);
    XFreeColormap(app.dpy, app.cmap);
    XFree(app.vi);
    XFreeGC(app.dpy, app.gc);
    XDestroyWindow(app.dpy, app.main);
    XCloseDisplay(app.dpy);
    return 0;
}

#endif
#ifdef TEST012 //freeglut (./mainwindow_demo): ERROR:  No display callback registered for window 1
//gcc -g mainwindow_demo.c -o mainwindow_demo -lGL -lGLU -lglut
#include <GL/glut.h>
#include <GL/freeglut.h>
#include <stdio.h>
#include <stdlib.h>

/* Window IDs */
int win_main, win_menu, win_draw;

/* Dimensions */
int WIN_W = 800, WIN_H = 600;
int H_MENU = 30;  // simulated menubar height

/* Menu selection handler */
void menu_select(int id) {
    switch(id) {
        case 1: printf("File->New\n"); break;
        case 2: printf("File->Open\n"); break;
        case 3: printf("File->Exit\n"); exit(0); break;
    }
}

/* Simulated menubar */
void menubar_display() {
    glClearColor(0.8,0.8,0.8,1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0,0,0);
    glRasterPos2i(10, 20);
    glutBitmapString(GLUT_BITMAP_HELVETICA_18, (const unsigned char *)"File");
    glutSwapBuffers();
}

void draw_display() {
    glClearColor(0.1,0.1,0.3,1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_TRIANGLES);
        glColor3f(1,0,0); glVertex2f(-0.5,-0.5);
        glColor3f(0,1,0); glVertex2f(0.5,-0.5);
        glColor3f(0,0,1); glVertex2f(0.0,0.5);
    glEnd();
    glutSwapBuffers();
}

void display_cb(void) {
    glClearColor(0.9, 0.9, 0.9, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Example drawing
    glColor3f(1, 0, 0);
    glBegin(GL_TRIANGLES);
        glVertex2f(-0.5f, -0.5f);
        glVertex2f(0.5f, -0.5f);
        glVertex2f(0.0f, 0.5f);
    glEnd();

    glutSwapBuffers();
}


int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_W, WIN_H);
    win_main = glutCreateWindow("GLUT Simulated Menubar Example");

    /* Create simulated menubar as a subwindow */
    win_menu = glutCreateSubWindow(win_main, 0, 0, WIN_W, H_MENU);
    glutDisplayFunc(menubar_display);
    int menu = glutCreateMenu(menu_select);
    glutAddMenuEntry("New", 1);
    glutAddMenuEntry("Open", 2);
    glutAddMenuEntry("Exit", 3);
    glutAttachMenu(GLUT_LEFT_BUTTON); // left-click to open menubar menu

    /* Create drawing area subwindow */
    win_draw = glutCreateSubWindow(win_main, 0, H_MENU, WIN_W, WIN_H - H_MENU);
    //glutDisplayFunc(draw_display);
    glutDisplayFunc(display_cb);  // ✅ REQUIRED

    glutMainLoopEvent();
    return 0;
}

#endif
#ifdef TEST011
/* Minimal CAD-like UI using pure GLUT subwindows.
 * Layout:
 *  +--------------------------------------------+
 *  | Toolbar (subwindow)                        |
 *  +--------------------------------------------+
 *  | Draw Area (subwindow, OpenGL)              |
 *  |                                            |
 *  +--------------------------------------------+
 *  | Command Input (subwindow)                  |
 *  +--------------------------------------------+
 *  | Status Bar (subwindow)                     |
 *  +--------------------------------------------+
 *
 * Requires freeglut or GLUT.
 */
#include <GL/glut.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* --- Layout constants --- */
static int WIN_W = 1000, WIN_H = 700;
static const int H_TOOLBAR = 44;
static const int H_CMD     = 28;
static const int H_STATUS  = 22;

/* --- Subwindow IDs --- */
static int win_main = 0;
static int win_toolbar = 0;
static int win_draw = 0;
static int win_cmd = 0;
static int win_status = 0;

/* --- State --- */
static char status_text[256] = "Ready";
static char cmd_text[256]    = "";
static int  cmd_cursor       = 0;

/* Toolbar buttons */
typedef struct { int x, y, w, h; const char* label; int id; } Button;
enum { BTN_NEW=1, BTN_LINE, BTN_SPLINE, BTN_CLEAR };
static Button toolbar_btns[4];

/* Draw area content: a simple polyline we “collect” with mouse clicks */
#define MAX_PTS 2048
static int n_pts = 0;
static float pts[MAX_PTS][2];  /* normalized device coords (-1..1) */

/* Helper: draw bitmap text (fixed 10px height, approx) */
static void drawBitmapString(float x, float y, const char *s)
{
    glRasterPos2f(x, y);
    for (const char* p=s; *p; ++p) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);
}

/* Helper: draw rect outline */
static void drawRect(int x, int y, int w, int h)
{
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x+w, y);
    glVertex2f(x+w, y+h);
    glVertex2f(x, y+h);
    glEnd();
}

/* Helper: fill rect */
static void fillRect(int x, int y, int w, int h)
{
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x+w, y);
    glVertex2f(x+w, y+h);
    glVertex2f(x, y+h);
    glEnd();
}

/* Convert window coords (pixels) to NDC [-1,1] in draw subwindow */
static void toNDC(int px, int py, float *x, float *y)
{
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    *x = ( (float)px / (float)w ) * 2.0f - 1.0f;
    *y = 1.0f - ( (float)py / (float)h ) * 2.0f;
}

/* ----- Status Bar ----- */
static void status_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(status_text, sizeof(status_text), fmt, ap);
    va_end(ap);
    if (win_status) {
        glutSetWindow(win_status);
        glutPostRedisplay();
    }
}

/* ----- Toolbar ----- */
static void toolbar_init_buttons(void)
{
    int x = 8, y = 6, w = 90, h = H_TOOLBAR - 12, gap = 8;
    toolbar_btns[0] = (Button){x, y, w, h, "New",     BTN_NEW};
    x += w+gap;
    toolbar_btns[1] = (Button){x, y, w, h, "Line",    BTN_LINE};
    x += w+gap;
    toolbar_btns[2] = (Button){x, y, w, h, "Spline",  BTN_SPLINE};
    x += w+gap;
    toolbar_btns[3] = (Button){x, y, w, h, "Clear",   BTN_CLEAR};
}

static void toolbar_display(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, glutGet(GLUT_WINDOW_WIDTH),
            glutGet(GLUT_WINDOW_HEIGHT), 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* background */
    glColor3f(0.92f, 0.92f, 0.95f);
    fillRect(0,0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));

    /* buttons */
    for (int i=0;i<4;i++){
        glColor3f(0,0,0);
        drawRect(toolbar_btns[i].x, toolbar_btns[i].y, toolbar_btns[i].w, toolbar_btns[i].h);
        /* label */
        glColor3f(0,0,0);
        float tx = toolbar_btns[i].x + 10;
        float ty = toolbar_btns[i].y + toolbar_btns[i].h/2 + 4;
        drawBitmapString(tx, ty, toolbar_btns[i].label);
    }
    glutSwapBuffers();
}

static void toolbar_mouse(int button, int state, int x, int y)
{
    if (state != GLUT_DOWN) return;
    for (int i=0;i<4;i++){
        Button *b = &toolbar_btns[i];
        if (x>=b->x && x<=b->x+b->w && y>=b->y && y<=b->y+b->h) {
            switch (b->id) {
                case BTN_NEW:
                    n_pts = 0;
                    status_printf("New: start sketching (click in Draw Area).");
                    break;
                case BTN_LINE:
                    status_printf("Line mode (click to add points).");
                    break;
                case BTN_SPLINE:
                    status_printf("Spline mode (demo draws simple bezier over points).");
                    break;
                case BTN_CLEAR:
                    n_pts = 0;
                    status_printf("Cleared.");
                    break;
            }
            glutPostRedisplay();
            return;
        }
    }
}

static void toolbar_reshape(int w, int h)
{
    glViewport(0,0,w,h);
}

/* ----- Draw Area ----- */
static void draw_display(void)
{
    glViewport(0,0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
    glClearColor(0.12f,0.12f,0.14f,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Axes */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1,1,-1,1,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glLineWidth(1.0f);
    glColor3f(0.25f,0.25f,0.3f);
    glBegin(GL_LINES);
      glVertex2f(-1,0); glVertex2f(1,0);
      glVertex2f(0,-1); glVertex2f(0,1);
    glEnd();

    /* Points */
    glPointSize(6.f);
    glColor3f(1,0.6f,0.1f);
    glBegin(GL_POINTS);
      for (int i=0;i<n_pts;i++) glVertex2f(pts[i][0], pts[i][1]);
    glEnd();

    /* Polyline */
    if (n_pts >= 2) {
      glColor3f(0.0f, 0.8f, 1.0f);
      glBegin(GL_LINE_STRIP);
        for (int i=0;i<n_pts;i++) glVertex2f(pts[i][0], pts[i][1]);
      glEnd();
    }

    /* Very simple "spline": sample quadratic Bezier over first 3 points if present */
    if (n_pts >= 3) {
        float p0x=pts[0][0], p0y=pts[0][1];
        float p1x=pts[1][0], p1y=pts[1][1];
        float p2x=pts[2][0], p2y=pts[2][1];
        glColor3f(0.2f, 1.0f, 0.2f);
        glBegin(GL_LINE_STRIP);
        for (int i=0;i<=100;i++){
            float t = i/100.0f;
            float u = 1.0f - t;
            float x = u*u*p0x + 2*u*t*p1x + t*t*p2x;
            float y = u*u*p0y + 2*u*t*p1y + t*t*p2y;
            glVertex2f(x,y);
        }
        glEnd();
    }

    glutSwapBuffers();
}

static void draw_mouse(int button, int state, int x, int y)
{
    if (button==GLUT_LEFT_BUTTON && state==GLUT_DOWN) {
        if (n_pts < MAX_PTS) {
            float nx, ny;
            toNDC(x, y, &nx, &ny);
            pts[n_pts][0] = nx;
            pts[n_pts][1] = ny;
            n_pts++;
            status_printf("Point %d: (%.3f, %.3f)", n_pts, nx, ny);
            glutPostRedisplay();
        } else {
            status_printf("Point buffer full.");
        }
    }
}

static void draw_reshape(int w, int h)
{
    glViewport(0,0,w,h);
    glutPostRedisplay();
}

/* ----- Command Input ----- */
static void cmd_display(void)
{
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(0.95f,0.95f,1.0f);
    fillRect(0,0,w,h);
    glColor3f(0,0,0);
    drawRect(0,0,w,h);

    glColor3f(0,0,0);
    drawBitmapString(8, h/2 + 4, cmd_text);

    /* cursor (simple) */
    int cursor_px = 8 + 8*cmd_cursor;
    glBegin(GL_LINES);
      glVertex2f(cursor_px, 4);
      glVertex2f(cursor_px, h-4);
    glEnd();

    glutSwapBuffers();
}

static void cmd_key(unsigned char key, int x, int y)
{
    if (key == 13) { /* Enter */
        if (strcmp(cmd_text,"CLEAR")==0 || strcmp(cmd_text,"clear")==0) {
            n_pts = 0;
            status_printf("CLEARED via command.");
        } else if (strncmp(cmd_text,"NEW",3)==0 || strncmp(cmd_text,"new",3)==0) {
            n_pts = 0;
            status_printf("NEW sketch.");
        } else {
            status_printf("CMD: %s", cmd_text);
        }
        cmd_text[0] = '\0';
        cmd_cursor = 0;
        glutPostRedisplay();
        if (win_draw) { glutSetWindow(win_draw); glutPostRedisplay(); }
        return;
    } else if (key == 8 || key == 127) { /* backspace */
        if (cmd_cursor > 0) {
            cmd_cursor--;
            cmd_text[cmd_cursor] = '\0';
        }
    } else if (key >= 32 && key < 127) {
        if (cmd_cursor < (int)sizeof(cmd_text)-1) {
            cmd_text[cmd_cursor++] = key;
            cmd_text[cmd_cursor] = '\0';
        }
    }
    glutPostRedisplay();
}

static void cmd_reshape(int w, int h)
{
    glViewport(0,0,w,h);
}

/* ----- Status Bar ----- */
static void status_display(void)
{
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(0.90f,0.92f,0.95f);
    fillRect(0,0,w,h);
    glColor3f(0,0,0);
    drawRect(0,0,w,h);

    glColor3f(0.1f,0.1f,0.1f);
    drawBitmapString(8, h/2 + 4, status_text);

    glutSwapBuffers();
}

static void status_reshape(int w, int h)
{
    glViewport(0,0,w,h);
}

/* ----- Menu (right-click) ----- */
static void menu_select(int id)
{
    switch (id) {
        case 1: n_pts = 0; status_printf("New"); break;
        case 2: status_printf("Line tool"); break;
        case 3: status_printf("Spline tool"); break;
        case 4: exit(0); break;
    }
}

static void create_menu(void)
{
    int m = glutCreateMenu(menu_select);
    glutAddMenuEntry("New",   1);
    glutAddMenuEntry("Line",  2);
    glutAddMenuEntry("Spline",3);
    glutAddMenuEntry("Quit",  4);
    /* Attach to the draw area window */
    glutSetWindow(win_draw);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

/* ----- Window layout management ----- */
static void layout_subwindows(void)
{
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    int y_toolbar = 0;
    int y_draw    = y_toolbar + H_TOOLBAR;
    int h_draw    = h - H_TOOLBAR - H_CMD - H_STATUS;
    int y_cmd     = y_draw + h_draw;
    int y_status  = y_cmd + H_CMD;

    glutSetWindow(win_toolbar);
    glutPositionWindow(0, y_toolbar);
    glutReshapeWindow(w, H_TOOLBAR);

    glutSetWindow(win_draw);
    glutPositionWindow(0, y_draw);
    glutReshapeWindow(w, h_draw);

    glutSetWindow(win_cmd);
    glutPositionWindow(0, y_cmd);
    glutReshapeWindow(w, H_CMD);

    glutSetWindow(win_status);
    glutPositionWindow(0, y_status);
    glutReshapeWindow(w, H_STATUS);
}

/* main window reshape -> update layout of subwindows */
static void main_reshape(int w, int h)
{
    WIN_W = w; WIN_H = h;
    layout_subwindows();
}

/* main display is unused (we draw in subwindows) */
static void main_display(void) {}

/* ----- Entry point ----- */
int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    glutInitWindowSize(WIN_W, WIN_H);
    win_main = glutCreateWindow("GLUT CAD UI (Toolbar/Draw/Command/Status)");

    /* main window callbacks */
    glutDisplayFunc(main_display);
    glutReshapeFunc(main_reshape);

    /* create subwindows */
    win_toolbar = glutCreateSubWindow(win_main, 0, 0, WIN_W, H_TOOLBAR);
    glutDisplayFunc(toolbar_display);
    glutReshapeFunc(toolbar_reshape);
    glutMouseFunc(toolbar_mouse);

    win_draw = glutCreateSubWindow(win_main, 0, H_TOOLBAR,
                                   WIN_W, WIN_H - H_TOOLBAR - H_CMD - H_STATUS);
    glutDisplayFunc(draw_display);
    glutReshapeFunc(draw_reshape);
    glutMouseFunc(draw_mouse);

    win_cmd = glutCreateSubWindow(win_main, 0, WIN_H - H_CMD - H_STATUS, WIN_W, H_CMD);
    glutDisplayFunc(cmd_display);
    glutReshapeFunc(cmd_reshape);
    glutKeyboardFunc(cmd_key);

    win_status = glutCreateSubWindow(win_main, 0, WIN_H - H_STATUS, WIN_W, H_STATUS);
    glutDisplayFunc(status_display);
    glutReshapeFunc(status_reshape);

    toolbar_init_buttons();
    create_menu();
    status_printf("Ready");

    glutMainLoop(); /* blocks */
    return 0;
}

#endif
#ifdef TEST010
//gcc -g mainwindow_demo.c -o mainwindow_demo -lXm -lXt -lX11 -lGL -lGLU -lglut
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>   /* Needed for xmCascadeButtonWidgetClass */
#include <Xm/PushB.h>      /* Needed for xmPushButtonWidgetClass */
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/TextF.h>
#include <GL/glx.h>          /* for glXGetCurrentDrawable */
#include <GL/freeglut.h>     /* for glutMainLoopEvent etc. */
#include <GL/glut.h>
#include <X11/Xlib.h>
#include <stdio.h>

typedef struct {
    Widget top, mainWin, menuBar, toolBar, drawForm, cmdInput, statusBar;
} AppWidgets;

static AppWidgets app;

/* GLUT globals */
static int glutWinID = 0;
static Display *dpy;
static Window glutXWin;

/* Draw a triangle */
void glut_display(void) {
    glClearColor(0.1, 0.1, 0.3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0); glVertex2f(-0.6, -0.4);
        glColor3f(0, 1, 0); glVertex2f( 0.6, -0.4);
        glColor3f(0, 0, 1); glVertex2f( 0.0,  0.6);
    glEnd();

    glutSwapBuffers();
}

void glut_idle(void) {
    glutPostRedisplay();
}

/* Sync GLUT window with Motif form */
void sync_glut_position() {
    if (!glutXWin || !app.drawForm) return;

    Window root;
    int x, y;
    unsigned int width, height, border, depth;
    XWindowAttributes attr;

    Window formWin = XtWindow(app.drawForm);

    /* Get size */
    XGetWindowAttributes(dpy, formWin, &attr);
    width = attr.width;
    height = attr.height;

    /* Translate position to root coords */
    XTranslateCoordinates(dpy, formWin, RootWindow(dpy, DefaultScreen(dpy)),
                          0, 0, &x, &y, &root);

    /* Move GLUT window */
    XMoveResizeWindow(dpy, glutXWin, x, y, width, height);
}

/* Quit callback */
void quit_cb(Widget w, XtPointer c, XtPointer call) {
    if (glutWinID) glutDestroyWindow(glutWinID);
    exit(0);
}

/* Menu */
Widget create_menu_bar(Widget parent) {
    Widget menuBar = XmCreateMenuBar(parent, "menuBar", NULL, 0);
    Widget fileMenu = XmCreatePulldownMenu(menuBar, "fileMenu", NULL, 0);
    Widget fileCascade = XtVaCreateManagedWidget(
        "File", xmCascadeButtonWidgetClass, menuBar,
        XmNsubMenuId, fileMenu, NULL);

    Widget quitBtn = XtVaCreateManagedWidget(
        "Quit", xmPushButtonWidgetClass, fileMenu, NULL);
    XtAddCallback(quitBtn, XmNactivateCallback, quit_cb, NULL);

    return menuBar;
}

/* Toolbar */
Widget create_tool_bar(Widget parent) {
    Widget toolBar = XmCreateRowColumn(parent, "toolBar", NULL, 0);
    XtVaSetValues(toolBar, XmNorientation, XmHORIZONTAL, NULL);
    XtVaCreateManagedWidget("Tool1", xmPushButtonWidgetClass, toolBar, NULL);
    XtVaCreateManagedWidget("Tool2", xmPushButtonWidgetClass, toolBar, NULL);
    return toolBar;
}

Widget create_status_bar(Widget parent) {
    return XtVaCreateManagedWidget("Status: Ready", xmLabelWidgetClass, parent, NULL);
}

Widget create_cmd_input(Widget parent) {
    return XtVaCreateManagedWidget("cmdInput", xmTextFieldWidgetClass, parent, NULL);
}

int main(int argc, char **argv) {
    XtAppContext appCtx;

    /* Create Motif UI */
    app.top = XtVaAppInitialize(&appCtx, "MotifGLUTAlign", NULL, 0, &argc, argv, NULL, NULL);
    app.mainWin = XmCreateMainWindow(app.top, "mainWin", NULL, 0);
    XtManageChild(app.mainWin);

    app.menuBar = create_menu_bar(app.mainWin);
    XtManageChild(app.menuBar);

    app.toolBar = create_tool_bar(app.mainWin);
    XtManageChild(app.toolBar);

    /* Form for OpenGL drawing */
    app.drawForm = XtVaCreateManagedWidget("drawForm", xmFormWidgetClass, app.mainWin, NULL);

    app.cmdInput = create_cmd_input(app.mainWin);
    XtManageChild(app.cmdInput);

    app.statusBar = create_status_bar(app.mainWin);
    XtManageChild(app.statusBar);

    XmMainWindowSetAreas(app.mainWin, app.menuBar, app.toolBar, NULL, app.statusBar, app.drawForm);

    XtRealizeWidget(app.top);

    /* ---- GLUT window ---- */
    int gArgc = 1;
    char *gArgv[] = {"glut", NULL};
    glutInit(&gArgc, gArgv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(400, 300);
    glutWinID = glutCreateWindow("GLUT Area");
    glutDisplayFunc(glut_display);
    glutIdleFunc(glut_idle);

    /* Get X window of GLUT */
    glutXWin = glXGetCurrentDrawable(); // works in freeglut
    dpy = XOpenDisplay(NULL);

    /* ---- Combined loop ---- */
    while (1) {
        while (XtAppPending(appCtx)) {
            XtAppProcessEvent(appCtx, XtIMAll);
        }
        sync_glut_position();
        glutMainLoopEvent();
    }

    return 0;
}

#endif
#ifdef TEST009 //X Error of failed request:  BadMatch (invalid parameter attributes)
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>
#include <Xm/Form.h>
#include <Xm/DrawingA.h>
#include <Xm/TextF.h>

#include <GL/gl.h>
#include <GL/glx.h>

#include <stdio.h>
#include <stdlib.h>

/* ===== GLX Globals ===== */
static Display     *g_dpy   = NULL;
static XVisualInfo *g_vi    = NULL;
static GLXContext   g_ctx   = 0;
static Window       g_glwin = 0;

/* ===== Widgets we update ===== */
static Widget g_status = NULL;   /* status bar label */
static Widget g_gl_da  = NULL;   /* GL DrawingArea   */

/* ====== Simple scene state ====== */
static int g_width  = 800;
static int g_height = 600;

void set_status(const char *msg)
{
    if (!g_status) return;
    XmString s = XmStringCreateLocalized((char*)msg);
    XtVaSetValues(g_status, XmNlabelString, s, NULL);
    XmStringFree(s);
}

/* ====== GL drawing ====== */
void draw_scene(void)
{
    glViewport(0, 0, g_width, g_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /* simple ortho */
    glOrtho(-1, 1, -1, 1, -1, 1);

    glClearColor(0.12f, 0.12f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0); glVertex3f(-0.6f, -0.4f, 0);
        glColor3f(0, 1, 0); glVertex3f( 0.6f, -0.4f, 0);
        glColor3f(0, 0, 1); glVertex3f( 0.0f,  0.6f, 0);
    glEnd();

    glXSwapBuffers(g_dpy, g_glwin);
}

/* ====== Callbacks ====== */
void da_realize_cb(Widget w, XtPointer client, XtPointer call)
{
    /* Create GLX context and bind to this DrawingArea’s window */
    g_dpy   = XtDisplay(w);
    g_glwin = XtWindow(w);

    g_ctx = glXCreateContext(g_dpy, g_vi, NULL, True);
    if (!g_ctx) {
        fprintf(stderr, "glXCreateContext failed\n");
        exit(1);
    }
    if (!glXMakeCurrent(g_dpy, g_glwin, g_ctx)) {
        fprintf(stderr, "glXMakeCurrent failed\n");
        exit(1);
    }

    glEnable(GL_DEPTH_TEST);
    set_status("Ready.");
}

void da_expose_cb(Widget w, XtPointer client, XtPointer call)
{
    draw_scene();
}

void da_resize_cb(Widget w, XtPointer client, XtPointer call)
{
    Dimension ww=0, hh=0;
    XtVaGetValues(w, XmNwidth, &ww, XmNheight, &hh, NULL);
    g_width  = (int)ww;
    g_height = (int)hh;
    draw_scene();
}

void file_exit_cb(Widget w, XtPointer client, XtPointer call)
{
    set_status("Exiting...");
    if (g_ctx) {
        glXMakeCurrent(g_dpy, None, NULL);
        glXDestroyContext(g_dpy, g_ctx);
        g_ctx = 0;
    }
    exit(0);
}

void tool_btn_cb(Widget w, XtPointer client, XtPointer call)
{
    const char *name = (const char*)client;
    char buf[128];
    snprintf(buf, sizeof(buf), "Tool pressed: %s", name ? name : "unknown");
    set_status(buf);
    draw_scene();
}

void cmd_run_cb(Widget w, XtPointer client, XtPointer call)
{
    Widget txt = (Widget)client;
    char *val = XmTextFieldGetString(txt);
    if (val) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Command: %s", val);
        set_status(buf);
        XtFree(val);
    } else {
        set_status("Command: <empty>");
    }
    draw_scene();
}

/* ====== Helpers to build UI ====== */
Widget build_menubar(Widget parent)
{
    Widget menubar = XmCreateMenuBar(parent, "menubar", NULL, 0);

    /* File menu */
    Widget file_pane = XmCreatePulldownMenu(menubar, "file_pane", NULL, 0);
    Widget file_cascade = XmCreateCascadeButton(menubar, "File", NULL, 0);
    XtVaSetValues(file_cascade, XmNsubMenuId, file_pane, NULL);

    Widget exit_item = XmCreatePushButton(file_pane, "Exit", NULL, 0);
    XtAddCallback(exit_item, XmNactivateCallback, file_exit_cb, NULL);
    XtManageChild(exit_item);

    XtManageChild(file_cascade);
    XtManageChild(menubar);
    return menubar;
}

/* The “command area” we give to MainWindow will contain:
   - A toolbar row (buttons)
   - A command row (label + text + run button)
   We pack both rows in a Form and return it. */
Widget build_command_area(Widget parent)
{
    Widget form = XmCreateForm(parent, "cmdArea", NULL, 0);
    XtManageChild(form);

    /* Toolbar row (top) */
    Arg a[6]; int n=0;
    XtSetArg(a[n], XmNorientation, XmHORIZONTAL); n++;
    XtSetArg(a[n], XmNmarginWidth, 4); n++;
    XtSetArg(a[n], XmNmarginHeight, 4); n++;
    Widget toolbar = XmCreateRowColumn(form, "toolbar", a, n);
    XtVaSetValues(toolbar,
                  XmNtopAttachment,    XmATTACH_FORM,
                  XmNleftAttachment,   XmATTACH_FORM,
                  XmNrightAttachment,  XmATTACH_FORM,
                  NULL);
    XtManageChild(toolbar);

    Widget btn1 = XmCreatePushButton(toolbar, "Zoom", NULL, 0);
    Widget btn2 = XmCreatePushButton(toolbar, "Pan",  NULL, 0);
    Widget btn3 = XmCreatePushButton(toolbar, "Fit",  NULL, 0);
    XtManageChild(btn1); XtManageChild(btn2); XtManageChild(btn3);
    XtAddCallback(btn1, XmNactivateCallback, tool_btn_cb, (XtPointer)"Zoom");
    XtAddCallback(btn2, XmNactivateCallback, tool_btn_cb, (XtPointer)"Pan");
    XtAddCallback(btn3, XmNactivateCallback, tool_btn_cb, (XtPointer)"Fit");

    /* Separator between toolbar and command row */
    Widget sep = XmCreateSeparator(form, "sep", NULL, 0);
    XtVaSetValues(sep,
                  XmNtopAttachment,   XmATTACH_WIDGET,
                  XmNtopWidget,       toolbar,
                  XmNleftAttachment,  XmATTACH_FORM,
                  XmNrightAttachment, XmATTACH_FORM,
                  NULL);
    XtManageChild(sep);

    /* Command row (bottom of command area) */
    n = 0;
    XtSetArg(a[n], XmNorientation, XmHORIZONTAL); n++;
    XtSetArg(a[n], XmNmarginWidth, 4); n++;
    XtSetArg(a[n], XmNmarginHeight, 4); n++;
    Widget cmdrow = XmCreateRowColumn(form, "cmdrow", a, n);
    XtVaSetValues(cmdrow,
                  XmNtopAttachment,   XmATTACH_WIDGET,
                  XmNtopWidget,       sep,
                  XmNleftAttachment,  XmATTACH_FORM,
                  XmNrightAttachment, XmATTACH_FORM,
                  XmNbottomAttachment,XmATTACH_FORM,
                  NULL);
    XtManageChild(cmdrow);

    Widget lbl  = XmCreateLabel(cmdrow, "Command:", NULL, 0);
    Widget text = XmCreateTextField(cmdrow, "cmdText", NULL, 0);
    Widget run  = XmCreatePushButton(cmdrow, "Run", NULL, 0);
    XtManageChild(lbl); XtManageChild(text); XtManageChild(run);

    XtAddCallback(run, XmNactivateCallback, cmd_run_cb, (XtPointer)text);

    return form;
}

/* Work region: a Form containing the GL DrawingArea (fills most space)
   and a status bar Label attached at the bottom. */
Widget build_work_region(Widget parent, Colormap cmap)
{
    Widget form = XmCreateForm(parent, "workForm", NULL, 0);
    XtManageChild(form);

    /* Status bar at the bottom */
    g_status = XmCreateLabel(form, "Ready.", NULL, 0);
    XtVaSetValues(g_status,
                  XmNbottomAttachment, XmATTACH_FORM,
                  XmNleftAttachment,   XmATTACH_FORM,
                  XmNrightAttachment,  XmATTACH_FORM,
                  XmNrecomputeSize,    False,
                  NULL);
    XtManageChild(g_status);

    /* GL DrawingArea above the status bar, filling space */
    g_gl_da = XtVaCreateManagedWidget(
        "glArea",
        xmDrawingAreaWidgetClass, form,
        XmNvisual,   g_vi->visual,
        XmNcolormap, cmap,
        XmNdepth,    g_vi->depth,
        XmNwidth,    g_width,
        XmNheight,   g_height,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNrightAttachment,  XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_WIDGET,
        XmNbottomWidget,     g_status,
        NULL
    );

    XtAddCallback(g_gl_da, XmNrealizeCallback, da_realize_cb, NULL);
    XtAddCallback(g_gl_da, XmNexposeCallback,  da_expose_cb,  NULL);
    XtAddCallback(g_gl_da, XmNresizeCallback,  da_resize_cb,  NULL);

    return form;
}

int main(int argc, char **argv)
{
    XtAppContext app;
    Widget toplevel, mainwin, menubar, cmd_area, work_region;
    Colormap cmap;

    /* === 1) Choose a GLX visual BEFORE creating the toplevel shell === */
    Display *probe = XOpenDisplay(NULL);
    if (!probe) { fprintf(stderr, "Cannot open display\n"); return 1; }

    int attribs[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 16, None };
    g_vi = glXChooseVisual(probe, DefaultScreen(probe), attribs);
    if (!g_vi) { fprintf(stderr, "No suitable GLX visual\n"); return 1; }

    cmap = XCreateColormap(probe,
                           RootWindow(probe, g_vi->screen),
                           g_vi->visual, AllocNone);

    /* === 2) Create the top-level with that visual/colormap === */
    Arg args[10]; int n = 0;
    XtSetArg(args[n], XtNvisual,   g_vi->visual); n++;
    XtSetArg(args[n], XtNcolormap, cmap);         n++;
    XtSetArg(args[n], XtNdepth,    g_vi->depth);  n++;

    toplevel = XtAppInitialize(&app, "MotifGLXCAD",
                               NULL, 0, &argc, argv,
                               NULL, args, n);

    /* From here on, use Xt’s Display */
    g_dpy = XtDisplay(toplevel);

    /* === 3) MainWindow === */
    mainwin = XmCreateMainWindow(toplevel, "mainwin", NULL, 0);
    XtManageChild(mainwin);

    /* === 4) Menu bar === */
    menubar = build_menubar(mainwin);

    /* === 5) Command area (toolbar + command row) === */
    cmd_area = build_command_area(mainwin);

    /* === 6) Work region: GL DrawingArea + Status bar === */
    work_region = build_work_region(mainwin, cmap);

    /* === 7) Place widgets into MainWindow ===
       menu    -> menubar
       command -> cmd_area
       hscroll -> NULL
       vscroll -> NULL
       work    -> work_region (contains GL + status) */
    XmMainWindowSetAreas(mainwin, menubar, cmd_area, NULL, NULL, work_region);

    /* === 8) Realize and loop === */
    XtRealizeWidget(toplevel);
    XtAppMainLoop(app);
    return 0;
}

#endif
#ifdef TEST008 //freeglut (./mainwindow_demo):  ERROR:  Function <glutPostRedisplay> called with no current window defined.
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/DrawingA.h>
#include <GL/freeglut.h>
#include <GL/freeglut_ext.h>
//#include <GL/glut.h>
#include <GL/glx.h>
#include <stdio.h>

static Display *dpy;
static Window win;

void display(void)
{
    glClearColor(0.1, 0.1, 0.3, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0); glVertex2f(-0.6, -0.4);
        glColor3f(0, 1, 0); glVertex2f( 0.6, -0.4);
        glColor3f(0, 0, 1); glVertex2f( 0.0,  0.6);
    glEnd();

    glutSwapBuffers();
}

void realize_cb(Widget w, XtPointer client, XtPointer call)
{
    dpy = XtDisplay(w);
    win = XtWindow(w);

    /* Tell GLUT to use this Display/Window */
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutSetWindowData((void*)w); // optional data
    glutSetWindow(glutCreateSubWindow(win, 0, 0, 800, 600));

    glutDisplayFunc(display);
}

int main(int argc, char **argv)
{
    XtAppContext app;
    Widget toplevel, mainwin, gl_area;

    /* Init Xt and GLUT */
    toplevel = XtVaAppInitialize(&app, "MotifGLUT", NULL, 0, &argc, argv, NULL, NULL);
    glutInit(&argc, argv);

    mainwin = XmCreateMainWindow(toplevel, "mainwin", NULL, 0);
    XtManageChild(mainwin);

    gl_area = XmCreateDrawingArea(mainwin, "glArea", NULL, 0);
    XtManageChild(gl_area);

    XtAddCallback(gl_area, XmNrealizeCallback, realize_cb, NULL);

    XmMainWindowSetAreas(mainwin, NULL, NULL, NULL, NULL, gl_area);

    XtRealizeWidget(toplevel);

    /* Event loop — drive GLUT manually */
    while (1) {
        while (XtAppPending(app)) {
            XtAppProcessEvent(app, XtIMAll);
        }
        glutMainLoopEvent();   /* process any GLUT events */
        glutPostRedisplay();   /* trigger redraw */
    }

    return 0;
}

#endif
#ifdef TEST007 //Error: XtCreateWidget "glArea" requires non-NULL widget class
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/GLwMDrawA.h>   /* Motif OpenGL DrawingArea widget */

static void expose_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    glClearColor(0.1, 0.1, 0.3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0); glVertex3f(-0.6, -0.4, 0);
        glColor3f(0, 1, 0); glVertex3f( 0.6, -0.4, 0);
        glColor3f(0, 0, 1); glVertex3f( 0.0,  0.6, 0);
    glEnd();

    glFlush();
    GLwDrawingAreaSwapBuffers(w); /* for double buffering */
}

static void init_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 600);
}

int main(int argc, char **argv)
{
    XtAppContext app;
    Widget toplevel, mainwin, gl_area;

    toplevel = XtVaAppInitialize(&app, "MotifGLwDemo", NULL, 0,
                                 &argc, argv, NULL, NULL);

    mainwin = XmCreateMainWindow(toplevel, "mainwin", NULL, 0);
    XtManageChild(mainwin);

    /* Create the OpenGL/Motif DrawingArea */
    gl_area = XtVaCreateManagedWidget("glArea",
                                      glwMDrawingAreaWidgetClass, mainwin,
                                      GLwNrgba, True,
                                      GLwNdoublebuffer, True,
                                      GLwNdepthSize, 16,
                                      XmNwidth, 800,
                                      XmNheight, 600,
                                      NULL);

    XtAddCallback(gl_area, GLwNexposeCallback, expose_cb, NULL);
    XtAddCallback(gl_area, GLwNresizeCallback, init_cb, NULL);
    XtAddCallback(gl_area, GLwNginitCallback, init_cb, NULL);

    XmMainWindowSetAreas(mainwin, NULL, NULL, NULL, NULL, gl_area);

    XtRealizeWidget(toplevel);
    XtAppMainLoop(app);
    return 0;
}

#endif
#ifdef TEST006 //X Error of failed request:  BadMatch (invalid parameter attributes)
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/RowColumn.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrollBar.h>
#include <Xm/DrawingA.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>

/* GLX variables */
Display *dpy;
GLXContext glx_ctx;
Window glx_win;
XVisualInfo *vi;

void draw_scene()
{
    glClearColor(0.1, 0.1, 0.3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0);
        glVertex3f(-0.6, -0.4, 0);
        glColor3f(0, 1, 0);
        glVertex3f(0.6, -0.4, 0);
        glColor3f(0, 0, 1);
        glVertex3f(0.0, 0.6, 0);
    glEnd();

    glXSwapBuffers(dpy, glx_win);
}

void expose_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    draw_scene();
}

void realize_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    dpy = XtDisplay(w);
    glx_win = XtWindow(w);
    glx_ctx = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    if (!glx_ctx) {
        fprintf(stderr, "glXCreateContext failed\n");
        exit(1);
    }
    glXMakeCurrent(dpy, glx_win, glx_ctx);

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 600);
}

int main(int argc, char **argv)
{
    XtAppContext app;
    Widget toplevel, mainwin, menubar, hscroll, vscroll, gl_area;
    Colormap cmap;
    Arg args[10];
    int n;

    /* Init Xt/Motif */
    toplevel = XtVaAppInitialize(&app, "MotifGLX", NULL, 0, &argc, argv, NULL, NULL);

    /* Create main window (default visual) */
    mainwin = XmCreateMainWindow(toplevel, "mainwin", NULL, 0);
    XtManageChild(mainwin);

    /* Menu bar */
    menubar = XmCreateMenuBar(mainwin, "menubar", NULL, 0);
    XtManageChild(menubar);

    /* Scrollbars */
    hscroll = XmCreateScrollBar(mainwin, "hscroll", NULL, 0);
    vscroll = XmCreateScrollBar(mainwin, "vscroll", NULL, 0);

    /* Get GLX visual for DrawingArea */
    dpy = XtDisplay(toplevel);
    int screen = DefaultScreen(dpy);
    int attribs[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None };
    vi = glXChooseVisual(dpy, screen, attribs);
    if (!vi) {
        fprintf(stderr, "No suitable visual found.\n");
        exit(1);
    }
    cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);

    /* Create DrawingArea with GLX visual */
    n = 0;
    XtSetArg(args[n], XmNvisual,   vi->visual); n++;
    XtSetArg(args[n], XmNcolormap, cmap);       n++;
    XtSetArg(args[n], XmNdepth,    vi->depth);  n++;
    XtSetArg(args[n], XmNwidth,    800);        n++;
    XtSetArg(args[n], XmNheight,   600);        n++;
    gl_area = XmCreateDrawingArea(mainwin, "glArea", args, n);
    XtManageChild(gl_area);

    XtAddCallback(gl_area, XmNexposeCallback, expose_cb, NULL);
    XtAddCallback(gl_area, XmNrealizeCallback, realize_cb, NULL);

    /* Attach areas */
    XmMainWindowSetAreas(mainwin, menubar, NULL, hscroll, vscroll, gl_area);

    XtRealizeWidget(toplevel);
    XtAppMainLoop(app);
    return 0;
}

#endif
#ifdef TEST005 //Error: XtCreateWidget "glArea" requires non-NULL widget class
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/ScrollBar.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/GLwMDrawA.h>   /* Motif OpenGL widget */
#include <stdio.h>
#include <stdlib.h>

/* Globals */
GLXContext glx_ctx;
XVisualInfo *vi;

/* Draw a simple triangle */
void draw_scene(void)
{
    glClearColor(0.1, 0.1, 0.3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0);
        glVertex3f(-0.6, -0.4, 0);
        glColor3f(0, 1, 0);
        glVertex3f(0.6, -0.4, 0);
        glColor3f(0, 0, 1);
        glVertex3f(0.0, 0.6, 0);
    glEnd();

    glFlush();
    glXSwapBuffers(XtDisplay((Widget)0), XtWindow((Widget)0)); /* Will override later */
}

/* Expose callback */
void expose_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    Display *dpy = XtDisplay(w);
    Window win = XtWindow(w);
    glXMakeCurrent(dpy, win, glx_ctx);
    draw_scene();
}

/* Realize callback */
void realize_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    Display *dpy = XtDisplay(w);
    Window win = XtWindow(w);

    glx_ctx = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    if (!glx_ctx) {
        fprintf(stderr, "glXCreateContext failed\n");
        exit(1);
    }
    glXMakeCurrent(dpy, win, glx_ctx);

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 600);
}

int main(int argc, char **argv)
{
    XtAppContext app;
    Widget toplevel, mainwin, menubar, hscroll, vscroll, gl_area;

    /* Initialize Xt/Motif */
    toplevel = XtVaAppInitialize(&app, "MotifGLX", NULL, 0, &argc, argv, NULL, NULL);

    /* Get GLX visual before creating GLwMDrawingArea */
    int attribs[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None };
    vi = glXChooseVisual(XtDisplay(toplevel), DefaultScreen(XtDisplay(toplevel)), attribs);
    if (!vi) {
        fprintf(stderr, "No suitable GLX visual found.\n");
        exit(1);
    }

    /* MainWindow + Menu */
    mainwin = XmCreateMainWindow(toplevel, "mainwin", NULL, 0);
    XtManageChild(mainwin);

    menubar = XmCreateMenuBar(mainwin, "menubar", NULL, 0);
    XtManageChild(menubar);

    /* Scroll bars */
    hscroll = XmCreateScrollBar(mainwin, "hscroll", NULL, 0);
    vscroll = XmCreateScrollBar(mainwin, "vscroll", NULL, 0);

    /* Create OpenGL widget (Motif's GLwMDrawingArea) */
    gl_area = XtVaCreateManagedWidget("glArea",
                                      glwMDrawingAreaWidgetClass, mainwin,
                                      GLwNvisualInfo, vi,
                                      XmNwidth,  800,
                                      XmNheight, 600,
                                      NULL);

    XtAddCallback(gl_area, XmNexposeCallback, expose_cb, NULL);
    XtAddCallback(gl_area, XmNrealizeCallback, realize_cb, NULL);

    /* Layout in MainWindow */
    XmMainWindowSetAreas(mainwin, menubar, NULL, hscroll, vscroll, gl_area);

    XtRealizeWidget(toplevel);
    XtAppMainLoop(app);

    return 0;
}

#endif
#ifdef TEST004 //X Error of failed request:  BadMatch (invalid parameter attributes)
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/DrawingA.h>
#include <Xm/ScrollBar.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>

/* Globals */
Display     *dpy;
GLXContext   glx_ctx;
Window       glx_win;
XVisualInfo *vi;

void draw_scene()
{
    glClearColor(0.1, 0.1, 0.3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0);
        glVertex3f(-0.6, -0.4, 0);
        glColor3f(0, 1, 0);
        glVertex3f(0.6, -0.4, 0);
        glColor3f(0, 0, 1);
        glVertex3f(0.0, 0.6, 0);
    glEnd();

    glXSwapBuffers(dpy, glx_win);
}

void expose_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    draw_scene();
}

void realize_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    dpy     = XtDisplay(w);
    glx_win = XtWindow(w);
    glx_ctx = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    if (!glx_ctx) {
        fprintf(stderr, "glXCreateContext failed\n");
        exit(1);
    }
    glXMakeCurrent(dpy, glx_win, glx_ctx);

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 600);
}

int main(int argc, char **argv)
{
    XtAppContext app;
    Widget toplevel, mainwin, menubar, hscroll, vscroll, gl_area;
    Colormap cmap;
    Arg args[10];
    int n;

    /* === Step 1: Init Xt/Motif with DEFAULT visual === */
    toplevel = XtVaAppInitialize(&app, "MotifGLX", NULL, 0, &argc, argv, NULL, NULL);

    /* === Step 2: Create MainWindow & Menu with default visual === */
    mainwin = XmCreateMainWindow(toplevel, "mainwin", NULL, 0);
    XtManageChild(mainwin);

    menubar = XmCreateMenuBar(mainwin, "menubar", NULL, 0);
    XtManageChild(menubar);

    hscroll = XmCreateScrollBar(mainwin, "hscroll", NULL, 0);
    vscroll = XmCreateScrollBar(mainwin, "vscroll", NULL, 0);

    /* === Step 3: Get GLX visual for DrawingArea only === */
    int attribs[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None };
    dpy = XtDisplay(toplevel);
    int screen = DefaultScreen(dpy);
    vi = glXChooseVisual(dpy, screen, attribs);
    if (!vi) {
        fprintf(stderr, "No suitable GLX visual found.\n");
        exit(1);
    }

    cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen),
                           vi->visual, AllocNone);

    /* === Step 4: Create DrawingArea with GLX visual & colormap === */
    n = 0;
    XtSetArg(args[n], XmNvisual,   vi->visual);   n++;
    XtSetArg(args[n], XmNcolormap, cmap);         n++;
    XtSetArg(args[n], XmNdepth,    vi->depth);    n++;
    XtSetArg(args[n], XmNwidth,    800);          n++;
    XtSetArg(args[n], XmNheight,   600);          n++;

    gl_area = XmCreateDrawingArea(mainwin, "drawingArea", args, n);
    XtManageChild(gl_area);

    //XtAddCallback(gl_area, XmNexposeCallback, expose_cb, NULL);
    //XtAddCallback(gl_area, XmNrealizeCallback, realize_cb, NULL);

    /* === Step 5: Attach widgets to MainWindow === */
    XmMainWindowSetAreas(mainwin, menubar, NULL, hscroll, vscroll, gl_area);

    /* === Step 6: Realize and run === */
    XtRealizeWidget(toplevel);
    XtAppMainLoop(app);

    return 0;
}

#endif
#ifdef TEST003 //X Error of failed request:  BadColor (invalid Colormap parameter)
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrollBar.h>
#include <Xm/DrawingA.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>

/* Globals */
Display     *dpy;
GLXContext   glx_ctx;
Window       glx_win;
XVisualInfo *vi;
Widget       gl_area;

/* Draw a simple triangle */
void draw_scene()
{
    glClearColor(0.1f, 0.1f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0);
        glVertex3f(-0.6f, -0.4f, 0.0f);
        glColor3f(0, 1, 0);
        glVertex3f(0.6f, -0.4f, 0.0f);
        glColor3f(0, 0, 1);
        glVertex3f(0.0f, 0.6f, 0.0f);
    glEnd();

    glXSwapBuffers(dpy, glx_win);
}

void expose_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    draw_scene();
}

void realize_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    dpy     = XtDisplay(w);
    glx_win = XtWindow(w);

    glx_ctx = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    if (!glx_ctx) {
        fprintf(stderr, "glXCreateContext failed\n");
        exit(1);
    }
    glXMakeCurrent(dpy, glx_win, glx_ctx);

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 600);
}

int main(int argc, char **argv)
{
    XtAppContext app;
    Widget toplevel, mainwin, menubar, hscroll, vscroll;
    Colormap cmap;
    Arg args[10];
    int n;

    /* === Step 1: Choose GLX visual BEFORE creating shell === */
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    int attribs[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None };
    vi = glXChooseVisual(dpy, DefaultScreen(dpy), attribs);
    if (!vi) {
        fprintf(stderr, "No suitable visual found.\n");
        return 1;
    }

    cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);

    /* === Step 2: Create top-level shell with correct visual/colormap === */
    toplevel = XtVaAppInitialize(
        &app, "MotifGLX", NULL, 0, &argc, argv, NULL,
        XmNvisual,   vi->visual,
        XmNcolormap, cmap,
        XmNdepth,    vi->depth,
        NULL
    );

    /* === Step 3: Create MainWindow === */
    mainwin = XmCreateMainWindow(toplevel, "mainwin", NULL, 0);
    XtManageChild(mainwin);

    /* === Step 4: Create MenuBar === */
    menubar = XmCreateMenuBar(mainwin, "menubar", NULL, 0);
    XtManageChild(menubar);

    /* === Step 5: Create scrollbars === */
    hscroll = XmCreateScrollBar(mainwin, "hscroll", NULL, 0);
    vscroll = XmCreateScrollBar(mainwin, "vscroll", NULL, 0);

    /* === Step 6: Create GLX DrawingArea === */
    n = 0;
    XtSetArg(args[n], XmNvisual,   vi->visual);   n++;
    XtSetArg(args[n], XmNcolormap, cmap);         n++;
    XtSetArg(args[n], XmNdepth,    vi->depth);    n++;
    XtSetArg(args[n], XmNwidth,    800);          n++;
    XtSetArg(args[n], XmNheight,   600);          n++;
    gl_area = XmCreateDrawingArea(mainwin, "drawingArea", args, n);
    XtManageChild(gl_area);

    XtAddCallback(gl_area, XmNexposeCallback,  expose_cb,  NULL);
    XtAddCallback(gl_area, XmNrealizeCallback, realize_cb, NULL);

    /* === Step 7: Attach areas to MainWindow === */
    XmMainWindowSetAreas(mainwin, menubar, NULL, hscroll, vscroll, gl_area);

    /* === Step 8: Realize and run === */
    XtRealizeWidget(toplevel);
    XtAppMainLoop(app);

    return 0;
}

#endif
#ifdef TEST002 //X Error of failed request:  BadMatch (invalid parameter attributes)
#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/MainW.h>
#include <Xm/PushB.h>
#include <Xm/DrawingA.h>
#include <Xm/ScrollBar.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>

/* Globals */
Display      *dpy;
GLXContext    glx_ctx;
Window        glx_win;
XVisualInfo  *vi;
Widget        gl_area;

void draw_scene()
{
    glClearColor(0.1, 0.1, 0.3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0);
        glVertex3f(-0.6, -0.4, 0);
        glColor3f(0, 1, 0);
        glVertex3f(0.6, -0.4, 0);
        glColor3f(0, 0, 1);
        glVertex3f(0.0, 0.6, 0);
    glEnd();

    glXSwapBuffers(dpy, glx_win);
}

void expose_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    draw_scene();
}

void realize_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    dpy = XtDisplay(w);
    glx_win = XtWindow(w);
    glx_ctx = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    if (!glx_ctx) {
        fprintf(stderr, "glXCreateContext failed\n");
        exit(1);
    }
    glXMakeCurrent(dpy, glx_win, glx_ctx);

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 600);
}

int main(int argc, char **argv)
{
    XtAppContext app;
    Widget toplevel, mainwin, menubar;
    Widget hscroll, vscroll;
    Colormap cmap;
    Arg args[10];

       /* Init Xt/Motif */
    toplevel = XtVaAppInitialize(&app, "MotifGLX", NULL, 0, &argc, argv, NULL, NULL);

    mainwin = XmCreateMainWindow(toplevel, "mainwin", NULL, 0);
    XtManageChild(mainwin);


    /* Get GLX visual */
    int attribs[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None };
    Display *dpy = XtDisplay(toplevel);
    int screen = DefaultScreen(dpy);
    XVisualInfo *vi = glXChooseVisual(dpy, screen, attribs);
    if (!vi) {
        fprintf(stderr, "No suitable visual found.\n");
        exit(1);
    }

    /* Create colormap for the GLX visual */
    cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen),
                                    vi->visual, AllocNone);

    /* Set args for DrawingArea with correct visual + colormap */
    int n = 0, win_width = 800, win_height = 600;
    XtSetArg(args[n], XmNvisual, vi->visual); n++;
    XtSetArg(args[n], XmNcolormap, cmap); n++;
    XtSetArg(args[n], XmNdepth, vi->depth); n++;
    XtSetArg(args[n], XmNwidth, win_width); n++;
    XtSetArg(args[n], XmNheight, win_height); n++;
    gl_area = XmCreateDrawingArea(mainwin, "drawingArea", args, n);
    XtManageChild(gl_area);

     /* Menu bar */
    menubar = XmCreateMenuBar(mainwin, "menubar", NULL, 0);
    XtManageChild(menubar);

    /* Scrollbars */
    hscroll = XmCreateScrollBar(mainwin, "hscroll", NULL, 0);
    vscroll = XmCreateScrollBar(mainwin, "vscroll", NULL, 0);

    /* GLX DrawingArea */
    gl_area = XtVaCreateManagedWidget("glArea",
                                      xmDrawingAreaWidgetClass, mainwin,
                                      XtNvisual,    vi->visual,
                                      XtNdepth,     vi->depth,
                                      XtNcolormap,  cmap,
                                      XmNwidth,     800,
                                      XmNheight,    600,
                                      NULL);

    XtAddCallback(gl_area, XmNexposeCallback, expose_cb, NULL);
    XtAddCallback(gl_area, XmNrealizeCallback, realize_cb, NULL);

    /* Attach areas */
    XmMainWindowSetAreas(mainwin, menubar, NULL, hscroll, vscroll, gl_area);

    XtRealizeWidget(toplevel);
    XtAppMainLoop(app);
    return 0;
}
#endif
#ifdef TEST001 //X Error of failed request:  BadColor (invalid Colormap parameter)
#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/MainW.h>
#include <Xm/PushB.h>
#include <Xm/DrawingA.h>
#include <Xm/ScrollBar.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>

/* Globals */
Display      *dpy;
GLXContext    glx_ctx;
Window        glx_win;
XVisualInfo  *vi;
Widget        gl_area;

void draw_scene()
{
    glClearColor(0.1, 0.1, 0.3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0);
        glVertex3f(-0.6, -0.4, 0);
        glColor3f(0, 1, 0);
        glVertex3f(0.6, -0.4, 0);
        glColor3f(0, 0, 1);
        glVertex3f(0.0, 0.6, 0);
    glEnd();

    glXSwapBuffers(dpy, glx_win);
}

void expose_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    draw_scene();
}

void realize_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    dpy = XtDisplay(w);
    glx_win = XtWindow(w);
    glx_ctx = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    if (!glx_ctx) {
        fprintf(stderr, "glXCreateContext failed\n");
        exit(1);
    }
    glXMakeCurrent(dpy, glx_win, glx_ctx);

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 600);
}

int main(int argc, char **argv)
{
    XtAppContext app;
    Widget toplevel, mainwin, menubar;
    Widget hscroll, vscroll;
    Colormap cmap;
    Arg args[10];
    int n = 0;

    int attribs[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None };
    dpy = XOpenDisplay(NULL);
    if (!dpy) { fprintf(stderr, "Cannot open display\n"); exit(1); }

    int screen = DefaultScreen(dpy);
    vi = glXChooseVisual(dpy, screen, attribs);
    if (!vi) { fprintf(stderr, "No suitable visual found\n"); exit(1); }

    cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);

    /* Now create the shell with correct visual/colormap */
    toplevel = XtVaAppInitialize(
        &app, "MotifGLX", NULL, 0, &argc, argv, NULL,
        XmNvisual,   vi->visual,
        XmNcolormap, cmap,
        XmNdepth,    vi->depth,
        NULL
    );


    /* === Step 3: Create main window === */
    mainwin = XmCreateMainWindow(toplevel, "mainwin", NULL, 0);
    XtManageChild(mainwin);

    /* === Step 4: Menu bar === */
    menubar = XmCreateMenuBar(mainwin, "menubar", NULL, 0);
    XtManageChild(menubar);

    /* === Step 5: Scrollbars === */
    hscroll = XmCreateScrollBar(mainwin, "hscroll", NULL, 0);
    vscroll = XmCreateScrollBar(mainwin, "vscroll", NULL, 0);

    /* === Step 6: GL DrawingArea with correct visual === */
    gl_area = XtVaCreateManagedWidget("glArea",
                                      xmDrawingAreaWidgetClass, mainwin,
                                      XmNvisual,   vi->visual,
                                      XmNcolormap, cmap,
                                      XmNdepth,    vi->depth,
                                      XmNwidth,    800,
                                      XmNheight,   600,
                                      NULL);

    XtAddCallback(gl_area, XmNexposeCallback, expose_cb, NULL);
    XtAddCallback(gl_area, XmNrealizeCallback, realize_cb, NULL);

    /* === Step 7: Attach areas to main window === */
    XmMainWindowSetAreas(mainwin, menubar, NULL, hscroll, vscroll, gl_area);

    /* === Step 8: Show === */
    XtRealizeWidget(toplevel);
    XtAppMainLoop(app);

    return 0;
}
#endif
