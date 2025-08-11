#ifndef PSLIB_H
#define PSLIB_H

#ifdef __cplusplus
extern "C" {
#endif

/* 以 Fortran 77 呼叫慣例命名（底線）*/
/* TinySpline 相關函數 */

/* 建立新的 B-spline
   deg: degree, dim: dimension, n_ctrlp: 控制點數量, type: tsBSplineType
   status: 0 = success, 1 = fail */
void ps_spline_new_(int *deg, int *dim, int *n_ctrlp, int *type, int *status);

/* 設定控制點 (double array) */
void ps_spline_set_ctrlp_(double *ctrlp, int *status);

/* 取出控制點 (double array) */
void ps_spline_get_ctrlp_(double *ctrlp_out, int *status);

/* 依給定點做自然三次插值，建立 spline */
void ps_spline_interpolate_(double *points, int *n, int *dim, int *status);

/* 產生 spline 取樣點 */
void ps_bspline_sample_f_(int *num_samples, double *samples, int *actual_samples, int *status);

/* 使用 X11 繪製 spline 曲線
   X, Y 為 double precision array，N 為點數 */
void ps_draw_spline_(double *X, double *Y, int *N);

/* 初始化 X11 視窗及資源 */
void ps_init_window_();

/* 處理 X11 事件（可在 Fortran 中迴圈呼叫） */
void ps_handle_events_();

/* 釋放 X11 資源 */
void ps_close_window_();

/* 顯示下拉選單（可擴充）*/
void ps_create_menu_();

/* 其他可能函式依需求添加 */

#ifdef __cplusplus
}
#endif

#endif /* PSLIB_H */
