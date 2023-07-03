#define _POSIX_C_SOURCE 200809L
#define START_TIMER(S) struct timeval start_ ## S , end_ ## S ; gettimeofday(&start_ ## S , NULL);
#define STOP_TIMER(S,T) gettimeofday(&end_ ## S, NULL); T->S += (double)(end_ ## S .tv_sec-start_ ## S.tv_sec)+(double)(end_ ## S .tv_usec-start_ ## S .tv_usec)/1000000;

#include "stdlib.h"
#include "math.h"
#include "sys/time.h"
#include "xmmintrin.h"
#include "pmmintrin.h"
#include "omp.h"

struct dataobj
{
  void *restrict data;
  unsigned long * size;
  unsigned long * npsize;
  unsigned long * dsize;
  int * hsize;
  int * hofs;
  int * oofs;
  void * dmap;
} ;

struct profiler
{
  double section0;
  double section1;
  double section2;
} ;


int Forward(struct dataobj *restrict damp_vec, struct dataobj *restrict rec_vec, struct dataobj *restrict rec_coords_vec, struct dataobj *restrict src_vec, struct dataobj *restrict src_coords_vec, struct dataobj *restrict u_vec, struct dataobj *restrict vp_vec, const int x_M, const int x_m, const int y_M, const int y_m, const float dt, const float o_x, const float o_y, const int p_rec_M, const int p_rec_m, const int p_src_M, const int p_src_m, const int time_M, const int time_m, const int nthreads, const int nthreads_nonaffine, struct profiler * timers)
{
  float (*restrict damp)[damp_vec->size[1]] __attribute__ ((aligned (64))) = (float (*)[damp_vec->size[1]]) damp_vec->data;
  float (*restrict rec)[rec_vec->size[1]] __attribute__ ((aligned (64))) = (float (*)[rec_vec->size[1]]) rec_vec->data;
  float (*restrict rec_coords)[rec_coords_vec->size[1]] __attribute__ ((aligned (64))) = (float (*)[rec_coords_vec->size[1]]) rec_coords_vec->data;
  float (*restrict src)[src_vec->size[1]] __attribute__ ((aligned (64))) = (float (*)[src_vec->size[1]]) src_vec->data;
  float (*restrict src_coords)[src_coords_vec->size[1]] __attribute__ ((aligned (64))) = (float (*)[src_coords_vec->size[1]]) src_coords_vec->data;
  float (*restrict u)[u_vec->size[1]][u_vec->size[2]] __attribute__ ((aligned (64))) = (float (*)[u_vec->size[1]][u_vec->size[2]]) u_vec->data;
  float (*restrict vp)[vp_vec->size[1]] __attribute__ ((aligned (64))) = (float (*)[vp_vec->size[1]]) vp_vec->data;

  /* Flush denormal numbers to zero in hardware */
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);

  float r4 = 1.0F/(dt*dt);
  float r5 = 1.0F/dt;

  for (int time = time_m; time <= time_M; time += 1)
  {
    /* Begin section0 */
    START_TIMER(section0)
    #pragma omp parallel num_threads(nthreads)
    {
      #pragma omp for collapse(1) schedule(dynamic,1)
      for (int x = x_m; x <= x_M; x += 1)
      {
        #pragma omp simd aligned(damp,u,vp:64)
        for (int y = y_m; y <= y_M; y += 1)
        {
          float r6 = 1.0F/(vp[x + 4][y + 4]*vp[x + 4][y + 4]);
          u[time + 1][x + 4][y + 4] = (r5*damp[x + 1][y + 1]*u[time][x + 4][y + 4] + r6*(-r4*(-2.0F*u[time][x + 4][y + 4]) - r4*u[time - 1][x + 4][y + 4]) + 8.33333315e-4F*(-u[time][x + 2][y + 4] - u[time][x + 4][y + 2] - u[time][x + 4][y + 6] - u[time][x + 6][y + 4]) + 1.3333333e-2F*(u[time][x + 3][y + 4] + u[time][x + 4][y + 3] + u[time][x + 4][y + 5] + u[time][x + 5][y + 4]) - 4.99999989e-2F*u[time][x + 4][y + 4])/(r4*r6 + r5*damp[x + 1][y + 1]);
        }
      }
    }
    STOP_TIMER(section0,timers)
    /* End section0 */

    /* Begin section1 */
    START_TIMER(section1)
    #pragma omp parallel num_threads(nthreads_nonaffine)
    {
      int chunk_size = (int)(fmax(1, (1.0F/3.0F)*(p_src_M - p_src_m + 1)/nthreads_nonaffine));
      #pragma omp for collapse(1) schedule(dynamic,chunk_size)
      for (int p_src = p_src_m; p_src <= p_src_M; p_src += 1)
      {
        float posx = -o_x + src_coords[p_src][0];
        float posy = -o_y + src_coords[p_src][1];
        int ii_src_0 = (int)(floor(1.0e-1*posx));
        int ii_src_1 = (int)(floor(1.0e-1*posy));
        int ii_src_2 = 1 + (int)(floor(1.0e-1*posy));
        int ii_src_3 = 1 + (int)(floor(1.0e-1*posx));
        float px = (float)(posx - 1.0e+1F*(int)(floor(1.0e-1F*posx)));
        float py = (float)(posy - 1.0e+1F*(int)(floor(1.0e-1F*posy)));
        if (ii_src_0 >= x_m - 1 && ii_src_1 >= y_m - 1 && ii_src_0 <= x_M + 1 && ii_src_1 <= y_M + 1)
        {
          float r0 = (dt*dt)*(vp[ii_src_0 + 4][ii_src_1 + 4]*vp[ii_src_0 + 4][ii_src_1 + 4])*(1.0e-2F*px*py - 1.0e-1F*px - 1.0e-1F*py + 1)*src[time][p_src];
          #pragma omp atomic update
          u[time + 1][ii_src_0 + 4][ii_src_1 + 4] += r0;
        }
        if (ii_src_0 >= x_m - 1 && ii_src_2 >= y_m - 1 && ii_src_0 <= x_M + 1 && ii_src_2 <= y_M + 1)
        {
          float r1 = (dt*dt)*(vp[ii_src_0 + 4][ii_src_2 + 4]*vp[ii_src_0 + 4][ii_src_2 + 4])*(-1.0e-2F*px*py + 1.0e-1F*py)*src[time][p_src];
          #pragma omp atomic update
          u[time + 1][ii_src_0 + 4][ii_src_2 + 4] += r1;
        }
        if (ii_src_1 >= y_m - 1 && ii_src_3 >= x_m - 1 && ii_src_1 <= y_M + 1 && ii_src_3 <= x_M + 1)
        {
          float r2 = (dt*dt)*(vp[ii_src_3 + 4][ii_src_1 + 4]*vp[ii_src_3 + 4][ii_src_1 + 4])*(-1.0e-2F*px*py + 1.0e-1F*px)*src[time][p_src];
          #pragma omp atomic update
          u[time + 1][ii_src_3 + 4][ii_src_1 + 4] += r2;
        }
        if (ii_src_2 >= y_m - 1 && ii_src_3 >= x_m - 1 && ii_src_2 <= y_M + 1 && ii_src_3 <= x_M + 1)
        {
          float r3 = 1.0e-2F*px*py*(dt*dt)*(vp[ii_src_3 + 4][ii_src_2 + 4]*vp[ii_src_3 + 4][ii_src_2 + 4])*src[time][p_src];
          #pragma omp atomic update
          u[time + 1][ii_src_3 + 4][ii_src_2 + 4] += r3;
        }
      }
    }
    STOP_TIMER(section1,timers)
    /* End section1 */

    /* Begin section2 */
    START_TIMER(section2)
    #pragma omp parallel num_threads(nthreads_nonaffine)
    {
      int chunk_size = (int)(fmax(1, (1.0F/3.0F)*(p_rec_M - p_rec_m + 1)/nthreads_nonaffine));
      #pragma omp for collapse(1) schedule(dynamic,chunk_size)
      for (int p_rec = p_rec_m; p_rec <= p_rec_M; p_rec += 1)
      {
        float posx = -o_x + rec_coords[p_rec][0];
        float posy = -o_y + rec_coords[p_rec][1];
        int ii_rec_0 = (int)(floor(1.0e-1*posx));
        int ii_rec_1 = (int)(floor(1.0e-1*posy));
        int ii_rec_2 = 1 + (int)(floor(1.0e-1*posy));
        int ii_rec_3 = 1 + (int)(floor(1.0e-1*posx));
        float px = (float)(posx - 1.0e+1F*(int)(floor(1.0e-1F*posx)));
        float py = (float)(posy - 1.0e+1F*(int)(floor(1.0e-1F*posy)));
        float sum = 0.0F;
        if (ii_rec_0 >= x_m - 1 && ii_rec_1 >= y_m - 1 && ii_rec_0 <= x_M + 1 && ii_rec_1 <= y_M + 1)
        {
          sum += (1.0e-2F*px*py - 1.0e-1F*px - 1.0e-1F*py + 1)*u[time][ii_rec_0 + 4][ii_rec_1 + 4];
        }
        if (ii_rec_0 >= x_m - 1 && ii_rec_2 >= y_m - 1 && ii_rec_0 <= x_M + 1 && ii_rec_2 <= y_M + 1)
        {
          sum += (-1.0e-2F*px*py + 1.0e-1F*py)*u[time][ii_rec_0 + 4][ii_rec_2 + 4];
        }
        if (ii_rec_1 >= y_m - 1 && ii_rec_3 >= x_m - 1 && ii_rec_1 <= y_M + 1 && ii_rec_3 <= x_M + 1)
        {
          sum += (-1.0e-2F*px*py + 1.0e-1F*px)*u[time][ii_rec_3 + 4][ii_rec_1 + 4];
        }
        if (ii_rec_2 >= y_m - 1 && ii_rec_3 >= x_m - 1 && ii_rec_2 <= y_M + 1 && ii_rec_3 <= x_M + 1)
        {
          sum += 1.0e-2F*px*py*u[time][ii_rec_3 + 4][ii_rec_2 + 4];
        }
        rec[time][p_rec] = sum;
      }
    }
    STOP_TIMER(section2,timers)
    /* End section2 */
  }

  return 0;
}