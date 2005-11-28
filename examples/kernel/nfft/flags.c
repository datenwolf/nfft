/*! \file flags.c
 *
 * \brief Testing the nfft.
 *
 * \author Stefan Kunis
 *
 * References: Time and Memory Requirements of the Nonequispaced FFT
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "nfft3.h"

void flags_cp(nfft_plan *dst, nfft_plan *src)
{
  dst->x=src->x;
  dst->f_hat=src->f_hat;
  dst->f=src->f;
  dst->g1=src->g1;
  dst->g2=src->g2;
  dst->my_fftw_plan1=src->my_fftw_plan1;
  dst->my_fftw_plan2=src->my_fftw_plan2;
}

void time_accuracy(int d, int N, int M, int n, int m, unsigned test_ndft,
                   unsigned test_pre_full_psi)
{
  int j,k,r;

  double t_ndft, t;
  complex *swapndft;

  int NN[d], nn[d];

  nfft_plan p;
  nfft_plan p_pre_phi_hut;
  nfft_plan p_fg_psi;
  nfft_plan p_pre_lin_psi;
  nfft_plan p_pre_fg_psi;
  nfft_plan p_pre_psi;
  nfft_plan p_pre_full_psi;

  printf("%d\t%d\t", d, N);

  for(r=0; r<d; r++)
    {
      NN[r]=N;
      nn[r]=n;
    }

  /** output vector ndft */
  if(test_ndft)
    swapndft=(complex*)fftw_malloc(M*sizeof(complex));

  nfft_init_guru(&p, d, NN, M, nn, m,
                 MALLOC_X| MALLOC_F_HAT| MALLOC_F|
		 FFTW_INIT| FFT_OUT_OF_PLACE,
		 FFTW_ESTIMATE| FFTW_DESTROY_INPUT);

  /** init pseudo random nodes */
  for(j=0; j<p.d*p.M_total; j++)
    p.x[j]=drand48()-0.5;

  nfft_init_guru(&p_pre_phi_hut, d, NN, M, nn, m, PRE_PHI_HUT,0);
  flags_cp(&p_pre_phi_hut, &p);
  nfft_precompute_one_psi(&p_pre_phi_hut);

  nfft_init_guru(&p_fg_psi, d, NN, M, nn, m, FG_PSI,0);
  flags_cp(&p_fg_psi, &p);
  nfft_precompute_one_psi(&p_fg_psi);

  nfft_init_guru(&p_pre_lin_psi, d, NN, M, nn, m, PRE_LIN_PSI,0);
  flags_cp(&p_pre_lin_psi, &p);
  nfft_precompute_one_psi(&p_pre_lin_psi);

  nfft_init_guru(&p_pre_fg_psi, d, NN, M, nn, m, PRE_FG_PSI,0);
  flags_cp(&p_pre_fg_psi, &p);
  nfft_precompute_one_psi(&p_pre_fg_psi);

  nfft_init_guru(&p_pre_psi, d, NN, M, nn, m, PRE_PSI,0);
  flags_cp(&p_pre_psi, &p);
  nfft_precompute_one_psi(&p_pre_psi);

  if(test_pre_full_psi)
    {
      nfft_init_guru(&p_pre_full_psi, d, NN, M, nn, m, PRE_FULL_PSI,0);
      flags_cp(&p_pre_full_psi, &p);
      nfft_precompute_one_psi(&p_pre_full_psi);
    }

  /** init pseudo random Fourier coefficients */
  for(k=0; k<p.N_total; k++)
    p.f_hat[k] = drand48() + I* drand48();

  /** NDFT */
  if(test_ndft)
    {
      SWAP_complex(p.f,swapndft);
      
      t_ndft=0;
      r=0; 
      while(t_ndft<0.01)
        {
          r++;
          t=second();
          ndft_trafo(&p);
          t=second()-t;
          t_ndft+=t;
        }
      t_ndft/=r;

      SWAP_complex(p.f,swapndft);
      printf("%.2e\t", t_ndft);
    }
  else 
    {
      printf("nan\t\t");
    }

  /** NFFT */
  nfft_trafo(&p);
  nfft_trafo(&p_pre_phi_hut);
  nfft_trafo(&p_fg_psi);
  nfft_trafo(&p_pre_lin_psi);
  nfft_trafo(&p_pre_fg_psi);
  nfft_trafo(&p_pre_psi);
  if(test_pre_full_psi)
    nfft_trafo(&p_pre_full_psi);

  if(test_ndft)
    printf("%.2e\t",error_l_2_complex(swapndft, p.f, p.M_total));
  else
    printf("nan\t\t");


  printf("%.2e\t%.2e\t%.2e\t%.2e\t%.2e\t%.2e\t%.2e\t%.2e\t%.2e\n",
         p.MEASURE_TIME_t[0],
         p_pre_phi_hut.MEASURE_TIME_t[0],

         p.MEASURE_TIME_t[1],

         p.MEASURE_TIME_t[2],
         p_fg_psi.MEASURE_TIME_t[2],
         p_pre_lin_psi.MEASURE_TIME_t[2],
         p_pre_fg_psi.MEASURE_TIME_t[2],
         p_pre_psi.MEASURE_TIME_t[2],
         p_pre_full_psi.MEASURE_TIME_t[2]);

  /** finalise */
  if(test_pre_full_psi)
    nfft_finalize(&p_pre_full_psi);
  nfft_finalize(&p_pre_psi);
  nfft_finalize(&p_pre_fg_psi);
  nfft_finalize(&p_pre_lin_psi);
  nfft_finalize(&p_fg_psi);
  nfft_finalize(&p_pre_phi_hut);
  nfft_finalize(&p);

  if(test_ndft)
    fftw_free(swapndft);
}



int main(int argc,char **argv)
{
  int l,m,d,trial,N;

  if(argc<=2)
    {
      fprintf(stderr,"flags type first last trials d m\n");
      return -1;
    }

  fprintf(stderr,"Testing different precomputation schemes for the nfft.\n");
  fprintf(stderr,"Columns: d, N=M, t_ndft, e_nfft, t_D, t_pre_phi_hut, ");
  fprintf(stderr,"t_fftw, t_B, t_fg_psi, t_pre_lin_psi, t_pre_fg_psi, ");
  fprintf(stderr,"t_pre_psi, t_pre_full_psi\n\n");

  d=atoi(argv[5]);
  m=atoi(argv[6]);

  /* time vs. N=M */
  if(atoi(argv[1])==0)
    for(l=atoi(argv[2]); l<=atoi(argv[3]); l++)
      for(trial=0; trial<atoi(argv[4]); trial++)
        if(l<=20)
          time_accuracy(d, (1U<< l), (1U<< (d*l)), (1U<< (l+1)), m, 0, 1);
        else
          time_accuracy(d, (1U<< l), (1U<< (d*l)), (1U<< (l+1)), m, 0, 0);

  d=atoi(argv[5]);
  N=atoi(argv[6]);

  /* accuracy vs. time */
  if(atoi(argv[1])==1)
    for(m=atoi(argv[2]); m<=atoi(argv[3]); m++)
      for(trial=0; trial<atoi(argv[4]); trial++)
        time_accuracy(d, N, (int)pow(N,d), 2*N, m, 1, 1);


  return 1;
}