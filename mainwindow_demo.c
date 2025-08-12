#define TEST001
#ifdef TEST001
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

    /* === Step 1: Get GLX visual BEFORE creating top-level shell === */
    dpy = XtDisplay(toplevel); /* now same Display as Xt */
    if (!dpy) {
        fprintf(stderr, "Cannot open display.\n");
        return 1;
    }
    int attribs[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None };
    vi = glXChooseVisual(dpy, DefaultScreen(dpy), attribs);
    if (!vi) {
        fprintf(stderr, "No suitable visual found.\n");
        return 1;
    }
    cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen),
                           vi->visual, AllocNone);

    /* === Step 2: Create shell with matching visual === */
    XtSetArg(args[n], XtNvisual,   vi->visual);   n++;
    XtSetArg(args[n], XtNcolormap, cmap);         n++;
    XtSetArg(args[n], XtNdepth,    vi->depth);    n++;
    toplevel = XtAppInitialize(&app, "MotifGLX", NULL, 0, &argc, argv,
                               NULL, args, n);


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
#ifdef TEST002
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
