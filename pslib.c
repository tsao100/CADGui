// pslib.c
#include <stdio.h>
#include <stdlib.h>
#include <tinyspline.h>

static tsBSpline spline;
static int spline_ready = 0;

void ps_spline_new_(int *degree, int *dim, int *n_ctrlp, int *type, int *status) {
    tsStatus stat;
    tsError err = ts_bspline_new((size_t)(*n_ctrlp), (size_t)(*dim), (size_t)(*degree), (tsBSplineType)(*type), &spline, &stat);
    spline_ready = (err == TS_SUCCESS);
    *status = spline_ready ? 0 : 1;
}

void ps_spline_set_ctrlp_(double *ctrlp, int *status) {
    if (!spline_ready) { *status=1; return; }
    tsStatus stat;
    tsError err = ts_bspline_set_control_points(&spline, ctrlp, &stat);
    *status = (err == TS_SUCCESS) ? 0 : 1;
}

void ps_spline_sample_(int *sample_count, double *samples_out, int *status_out) {
    tsReal *samples = NULL;
    size_t actual = 0;
    tsStatus status;

    tsError err = ts_bspline_sample(&spline, (size_t)*sample_count, &samples, &actual, &status);
    if (err != TS_SUCCESS) {
        *status_out = status.code;
        return;
    }

    size_t dim = ts_bspline_dimension(&spline);
    for (size_t i = 0; i < actual * dim; i++) {
        samples_out[i] = samples[i];
    }

    free(samples);
    *status_out = 0;
}

void ps_spline_free_() {
    if (spline_ready) ts_bspline_free(&spline);
    spline_ready = 0;
}
