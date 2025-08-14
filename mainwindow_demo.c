#define TEST010
#ifdef TEST01N
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
