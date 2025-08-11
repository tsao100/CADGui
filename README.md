CAD GUI Demo (OpenGL + GLX + Xlib) with Fortran 77 interface
============================================================

Files:
  cad_gui.c       - C source with OpenGL/GLX window and Fortran-callable functions
  cad_fortran.f   - Fortran 77 demo that calls the C functions
  Makefile        - build script

Requirements (Ubuntu):
  sudo apt update
  sudo apt install build-essential gfortran libx11-dev libgl1-mesa-dev libglu1-mesa-dev pkg-config

Build:
  make

Run:
  ./cad_spline

Controls (while window focused):
  - Left mouse click: add control point
  - Right mouse click: sample spline from current control polygon
  - 's' key: sample spline
  - 'c' key: clear points
  - 'q' or ESC: quit

Notes:
  - This project uses an internal Catmull-Rom sampler (no TinySpline required).
  - If you want to integrate TinySpline, modify cad_gui.c to call tinyspline APIs and link to libtinyspline.
