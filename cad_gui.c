#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Simple OpenGL + GLX CAD-like demo with Fortran-callable interface.
   Provides:
     cg_init_window_()
     cg_add_point_(double *x, double *y)
     cg_clear_()
     cg_draw_spline_from_ctrlp_(double *x, double *y, int *n)
     cg_run_()   -- enters event loop (blocking)
   Uses internal Catmull-Rom sampler if TinySpline is not linked.
*/

#define MAX_POINTS 1024
#define MAX_SAMPLES 8192

typedef struct { float x,y,z; } P3;

static Display *display = NULL;
static Window win = 0;
static GLXContext glc = NULL;
static int win_w = 800, win_h = 600;

static P3 ctrl_pts[MAX_POINTS];
static int ctrl_n = 0;

static P3 spline_samples[MAX_SAMPLES];
static int sample_n = 0;
static int show_spline = 1;

/* --- Utilities --- */

static void ensure_gl() {
    if (!display) return;
    glViewport(0,0,win_w,win_h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, win_w, 0, win_h, -1, 1); /* origin at bottom-left */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

/* Simple Catmull-Rom centripetal sampler for 2D control points */
static void sample_catmull_rom_2d(const double *x, const double *y, int n, int samples_per_seg) {
    sample_n = 0;
    if (n < 2) return;
    /* For endpoints, duplicate ends to create tangents */
    for (int i=0;i<n-1;i++) {
        /* p0 p1 p2 p3 for segment between p1 and p2 (we iterate i from 0..n-2 and use p_{i-1},p_i,p_{i+1},p_{i+2}) */
    }
    /* We'll use standard Catmull-Rom with endpoints repeated */
    for (int i=0;i<n-1;i++) {
        double x0 = (i==0)? x[0] : x[i-1];
        double y0 = (i==0)? y[0] : y[i-1];
        double x1 = x[i];
        double y1 = y[i];
        double x2 = x[i+1];
        double y2 = y[i+1];
        double x3 = (i+2>=n)? x[n-1] : x[i+2];
        double y3 = (i+2>=n)? y[n-1] : y[i+2];

        for (int j=0;j<samples_per_seg;j++) {
            double t = (double)j/(double)samples_per_seg;
            double t2 = t*t;
            double t3 = t2*t;
            double a0 = -0.5*t3 + t2 - 0.5*t;
            double a1 = 1.5*t3 - 2.5*t2 + 1.0;
            double a2 = -1.5*t3 + 2.0*t2 + 0.5*t;
            double a3 = 0.5*t3 - 0.5*t2;
            double sx = a0*x0 + a1*x1 + a2*x2 + a3*x3;
            double sy = a0*y0 + a1*y1 + a2*y2 + a3*y3;
            if (sample_n < MAX_SAMPLES) {
                spline_samples[sample_n].x = (float)sx;
                spline_samples[sample_n].y = (float)sy;
                spline_samples[sample_n].z = 0.0f;
                sample_n++;
            }
        }
    }
    /* add last control point as final sample */
    if (sample_n < MAX_SAMPLES) {
        spline_samples[sample_n].x = (float)x[n-1];
        spline_samples[sample_n].y = (float)y[n-1];
        spline_samples[sample_n].z = 0.0f;
        sample_n++;
    }
}

/* --- Fortran-callable interface (trailing underscore) --- */

void cg_init_window_() {
    if (display) return; /* already inited */
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "cg_init_window_: cannot open X display\n");
        return;
    }
    int screen = DefaultScreen(display);
    static int visual_attribs[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    XVisualInfo *vi = glXChooseVisual(display, screen, visual_attribs);
    if (!vi) {
        fprintf(stderr, "cg_init_window_: no appropriate visual\n");
        return;
    }
    Colormap cmap = XCreateColormap(display, RootWindow(display, vi->screen), vi->visual, AllocNone);

    XSetWindowAttributes swa;
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask;

    win = XCreateWindow(display, RootWindow(display, vi->screen),
                        100, 100, win_w, win_h, 0, vi->depth, InputOutput,
                        vi->visual, CWColormap | CWEventMask, &swa);

    XStoreName(display, win, "CAD GLX Demo (Fortran-callable)");
    XMapWindow(display, win);

    glc = glXCreateContext(display, vi, NULL, GL_TRUE);
    glXMakeCurrent(display, win, glc);

    glEnable(GL_DEPTH_TEST);
    glClearColor(1,1,1,1);

    ensure_gl();

    XFree(vi);
}

/* Add a control point (from Fortran) */
void cg_add_point_(double *x, double *y) {
    if (ctrl_n >= MAX_POINTS) return;
    ctrl_pts[ctrl_n].x = (float)(*x);
    ctrl_pts[ctrl_n].y = (float)(*y);
    ctrl_pts[ctrl_n].z = 0.0f;
    ctrl_n++;
}

/* Clear points and samples */
void cg_clear_() {
    ctrl_n = 0;
    sample_n = 0;
}

/* Draw immediate: control polygon + sample spline (uses Catmull-Rom sampler internally) */
void cg_draw_spline_from_ctrlp_(double *x, double *y, int *n) {
    int ni = *n;
    if (ni < 2) return;
    /* copy control pts */
    for (int i=0;i<ni && i<MAX_POINTS;i++) {
        ctrl_pts[i].x = (float)x[i];
        ctrl_pts[i].y = (float)y[i];
        ctrl_pts[i].z = 0.0f;
    }
    ctrl_n = (ni < MAX_POINTS) ? ni : MAX_POINTS;
    /* sample */
    sample_catmull_rom_2d(x,y,ni,30); /* 30 samples per segment */
    /* request redraw by swapping in next draw call (we draw immediately here) */
    ensure_gl();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* draw control polygon */
    glColor3f(0.2f,0.2f,0.2f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_STRIP);
    for (int i=0;i<ctrl_n;i++) glVertex3f(ctrl_pts[i].x, ctrl_pts[i].y, 0.0f);
    glEnd();

    /* draw control points */
    glColor3f(1.0f,0.0f,0.0f);
    glPointSize(6.0f);
    glBegin(GL_POINTS);
    for (int i=0;i<ctrl_n;i++) glVertex3f(ctrl_pts[i].x, ctrl_pts[i].y, 0.0f);
    glEnd();

    /* draw spline samples */
    if (sample_n>1) {
        glColor3f(0.0f,0.0f,1.0f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_STRIP);
        for (int i=0;i<sample_n;i++) glVertex3f(spline_samples[i].x, spline_samples[i].y, 0.0f);
        glEnd();
    }

    glXSwapBuffers(display, win);
}

/* Blocking event loop (Fortran calls this to run GUI) */
void cg_run_() {
    if (!display) return;
    XEvent ev;
    int running = 1;
    while (running) {
        XNextEvent(display, &ev);
        switch (ev.type) {
            case Expose:
                /* redraw current scene */
                ensure_gl();
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                /* draw control polygon */
                if (ctrl_n>0) {
                    glColor3f(0.2f,0.2f,0.2f);
                    glLineWidth(1.0f);
                    glBegin(GL_LINE_STRIP);
                    for (int i=0;i<ctrl_n;i++) glVertex3f(ctrl_pts[i].x, ctrl_pts[i].y, 0.0f);
                    glEnd();

                    glColor3f(1.0f,0.0f,0.0f);
                    glPointSize(6.0f);
                    glBegin(GL_POINTS);
                    for (int i=0;i<ctrl_n;i++) glVertex3f(ctrl_pts[i].x, ctrl_pts[i].y, 0.0f);
                    glEnd();
                }
                if (sample_n>1) {
                    glColor3f(0.0f,0.0f,1.0f);
                    glLineWidth(2.0f);
                    glBegin(GL_LINE_STRIP);
                    for (int i=0;i<sample_n;i++) glVertex3f(spline_samples[i].x, spline_samples[i].y, 0.0f);
                    glEnd();
                }
                glXSwapBuffers(display, win);
                break;
            case ConfigureNotify:
                win_w = ev.xconfigure.width;
                win_h = ev.xconfigure.height;
                ensure_gl();
                break;
            case ButtonPress:
                /* left button add point */
                if (ev.xbutton.button == Button1) {
                    float x = (float)ev.xbutton.x;
                    float y = (float)(win_h - ev.xbutton.y);
                    if (ctrl_n < MAX_POINTS) {
                        ctrl_pts[ctrl_n].x = x;
                        ctrl_pts[ctrl_n].y = y;
                        ctrl_pts[ctrl_n].z = 0.0f;
                        ctrl_n++;
                    }
                }
                /* right button: sample spline from current control polygon */
                if (ev.xbutton.button == Button3) {
                    if (ctrl_n >= 2) {
                        double *xs = malloc(sizeof(double)*ctrl_n);
                        double *ys = malloc(sizeof(double)*ctrl_n);
                        for (int i=0;i<ctrl_n;i++) { xs[i]=ctrl_pts[i].x; ys[i]=ctrl_pts[i].y; }
                        sample_catmull_rom_2d(xs, ys, ctrl_n, 30);
                        free(xs); free(ys);
                    }
                }
                /* redraw after interaction */
                ensure_gl();
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                /* draw control polygon */
                if (ctrl_n>0) {
                    glColor3f(0.2f,0.2f,0.2f);
                    glLineWidth(1.0f);
                    glBegin(GL_LINE_STRIP);
                    for (int i=0;i<ctrl_n;i++) glVertex3f(ctrl_pts[i].x, ctrl_pts[i].y, 0.0f);
                    glEnd();

                    glColor3f(1.0f,0.0f,0.0f);
                    glPointSize(6.0f);
                    glBegin(GL_POINTS);
                    for (int i=0;i<ctrl_n;i++) glVertex3f(ctrl_pts[i].x, ctrl_pts[i].y, 0.0f);
                    glEnd();
                }
                if (sample_n>1) {
                    glColor3f(0.0f,0.0f,1.0f);
                    glLineWidth(2.0f);
                    glBegin(GL_LINE_STRIP);
                    for (int i=0;i<sample_n;i++) glVertex3f(spline_samples[i].x, spline_samples[i].y, 0.0f);
                    glEnd();
                }
                glXSwapBuffers(display, win);
                break;
            case KeyPress: {
                KeySym ks;
                ks = XLookupKeysym(&ev.xkey, 0);
                if (ks == XK_q || ks == XK_Q || ks == XK_Escape) {
                    running = 0;
                } else if (ks == XK_c || ks == XK_C) {
                    /* clear */
                    ctrl_n = 0; sample_n = 0;
                    ensure_gl(); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); glXSwapBuffers(display, win);
                } else if (ks == XK_s || ks == XK_S) {
                    /* sample using current ctrl_pts */
                    if (ctrl_n >= 2) {
                        double *xs = malloc(sizeof(double)*ctrl_n);
                        double *ys = malloc(sizeof(double)*ctrl_n);
                        for (int i=0;i<ctrl_n;i++){ xs[i]=ctrl_pts[i].x; ys[i]=ctrl_pts[i].y; }
                        sample_catmull_rom_2d(xs, ys, ctrl_n, 30);
                        free(xs); free(ys);
                        ensure_gl(); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); glXSwapBuffers(display, win);
                    }
                }
                break;
            }
        }
    }

    /* cleanup */
    glXMakeCurrent(display, None, NULL);
    glXDestroyContext(display, glc);
    XDestroyWindow(display, win);
    XCloseDisplay(display);
    display = NULL;
}
