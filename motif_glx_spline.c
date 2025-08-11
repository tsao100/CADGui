#include <stdio.h>
#include <stdlib.h>
#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/MainW.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>

#include <GL/gl.h>
#include <GL/glx.h>

#include "pslib.h"  // 你的 spline Fortran 接口 C 包裝

Display *display;
Window glx_window;
GLXContext glx_context;
Widget drawing_area;
XtAppContext app_context;

int win_width = 800;
int win_height = 600;

double *sampled_points = NULL;
int sampled_count = 0;

void draw_gl_scene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // 簡單 3D camera 固定視角
    glTranslated(0.0, 0.0, -3.0);

    // 畫控制點（紅色）
    glColor3f(1.0, 0.0, 0.0);
    glPointSize(6.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < sampled_count; i++) {
        double *p = &sampled_points[i * 3];  // 3D
        glVertex3d(p[0], p[1], p[2]);
    }
    glEnd();

    // 畫曲線（綠色線）
    glColor3f(0.0, 1.0, 0.0);
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < sampled_count; i++) {
        double *p = &sampled_points[i * 3];
        glVertex3d(p[0], p[1], p[2]);
    }
    glEnd();

    glXSwapBuffers(display, glx_window);
}

void realize_callback(Widget w, XtPointer client_data, XtPointer call_data) {
    display = XtDisplay(w);
    glx_window = XtWindow(w);

    int screen = DefaultScreen(display);
    int attribs[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 24, None };
    XVisualInfo *vi = glXChooseVisual(display, screen, attribs);
    if (!vi) {
        fprintf(stderr, "No appropriate visual found\n");
        exit(1);
    }

    glx_context = glXCreateContext(display, vi, NULL, GL_TRUE);
    if (!glx_context) {
        fprintf(stderr, "Failed to create GLX context\n");
        exit(1);
    }

    glXMakeCurrent(display, glx_window, glx_context);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2, 0.3, 0.4, 1.0);

    // 建立範例 spline
    int status;
    int degree = 3, dim = 3, n_ctrlp = 4, type = 0;
    ps_spline_new_(&degree, &dim, &n_ctrlp, &type, &status);

    double ctrlp[12] = {
        -0.8, -0.8, 0.0,
        -0.4,  0.8, 0.2,
         0.4, -0.8, 0.6,
         0.8,  0.8, 0.0
    };
    ps_spline_set_ctrlp_(ctrlp, &status);

    int samples = 100;
    if (sampled_points) free(sampled_points);
    sampled_points = malloc(sizeof(double)*samples*dim);
    ps_spline_sample_(&samples, sampled_points, &sampled_count, &status);
}

void expose_callback(Widget w, XtPointer client_data, XtPointer call_data) {
    XExposeEvent *event = &((XEvent *)call_data)->xexpose;
    if (event->count == 0) {
        draw_gl_scene();
    }
}

void menu_callback(Widget w, XtPointer client_data, XtPointer call_data) {
    char *item = (char*)client_data;
    printf("Menu item selected: %s\n", item);

    // TODO: 根據選單改變 spline 參數或行為
    // 這裡可重新設定控制點、曲線類型、重算曲線

    draw_gl_scene();
}

Widget create_menu(Widget parent) {
    Widget menu_pane = XmCreatePulldownMenu(parent, "menu_pane", NULL, 0);
    Widget cascade = XmCreateCascadeButton(parent, "SplineType", NULL, 0);
    XmString label = XmStringCreateLocalized("SplineType");
    //XtVaSetValues(cascade, XmNsubMenuId, menu_pane, XmNlabelString, label, NULL);
    //XmStringFree(label);

    char *items[] = {"Linear", "Quadratic", "Cubic", NULL};
    for (int i = 0; items[i] != NULL; i++) {
        Widget item = XmCreatePushButton(menu_pane, items[i], NULL, 0);
        XtAddCallback(item, XmNactivateCallback, menu_callback, items[i]);
        XtManageChild(item);
    }

    return cascade;
}

int main(int argc, char **argv)
{
    Widget toplevel, mainwin, menu_bar, menu;
    Arg args[10];
    int n;

    /* 初始化 Xt/Motif 應用程式 */
    toplevel = XtVaAppInitialize(
        &app_context,
        "MotifGLXSpline",
        NULL, 0,
        &argc, argv,
        NULL, NULL);

    /* 建立 MainWindow */
    mainwin = XmCreateMainWindow(toplevel, "mainwin", NULL, 0);
    XtManageChild(mainwin);

    /* 建立 MenuBar 與選單 */
    menu_bar = XmCreateMenuBar(mainwin, "menubar", NULL, 0);
    XtManageChild(menu_bar);

    menu = create_menu(menu_bar);
//    XtManageChild(menu);

    XtVaSetValues(menu_bar, XmNmenuHelpWidget, NULL, NULL);

    /* 建立 DrawingArea，準備嵌入 OpenGL */
    n = 0;
    XtSetArg(args[n], XmNwidth, win_width); n++;
    XtSetArg(args[n], XmNheight, win_height); n++;
    drawing_area = XmCreateDrawingArea(mainwin, "drawingArea", args, n);
    XtManageChild(drawing_area);

    /* 綁定 Callback */
    XtAddCallback(drawing_area, XmNexposeCallback, expose_callback, NULL);
    XtAddCallback(drawing_area, XmNrealizeCallback, realize_callback, NULL);

    /* MainWindow 配置區域 */
    XmMainWindowSetAreas(mainwin, menu_bar, NULL, NULL, NULL, drawing_area);

    /* Realize widget（產生 X Window） */
    XtRealizeWidget(toplevel);

    /* 驗證 GLX context 是否正確建立（可放在 realize_callback 裡） */
    if (!glXGetCurrentContext()) {
        fprintf(stderr, "[錯誤] GLX context 建立失敗，請檢查 X 視窗或 OpenGL 初始化程式碼。\n");
    }

    /* 進入事件迴圈 */
    XtAppMainLoop(app_context);

    return 0;
}