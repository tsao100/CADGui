#define TEST008
#ifdef TEST00N
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
