      program cad_fortran_demo
      implicit none
      integer i, n
      double precision xs(8), ys(8)

      ! declare external C functions
      external cg_init_window
      external cg_draw_spline_from_ctrlp
      external cg_run
      external cg_clear

      call cg_init_window()

      ! example control polygon
      n = 8
      xs(1) = 100.d0; ys(1) = 100.d0
      xs(2) = 200.d0; ys(2) = 50.d0
      xs(3) = 300.d0; ys(3) = 150.d0
      xs(4) = 400.d0; ys(4) = 100.d0
      xs(5) = 500.d0; ys(5) = 200.d0
      xs(6) = 600.d0; ys(6) = 120.d0
      xs(7) = 650.d0; ys(7) = 300.d0
      xs(8) = 700.d0; ys(8) = 100.d0

      ! draw spline from arrays
      call cg_draw_spline_from_ctrlp(xs, ys, n)

      print *, 'Controls: left-click add point; right-click sample; s sa
     Cmple; c clear; q quit'
      call cg_run()

      end
