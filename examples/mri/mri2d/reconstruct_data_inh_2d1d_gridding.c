#include "nfft3.h"
#include "util.h"
#include "math.h"

void nfft (char* filename,int N,int M,int iteration , int weight)
{
  int j,l;
  double weights;
  double time,min_time,max_time,min_inh,max_inh;
  double t,real,imag;
  double w;
  mri_inh_2d1d_plan my_plan;
  FILE* fp,*fw,*fout_real,*fout_imag,*finh,*ftime;
  int my_N[3],my_n[3];
  int flags = PRE_PHI_HUT| PRE_PSI |MALLOC_X| MALLOC_F_HAT|
                      MALLOC_F| FFTW_INIT| FFT_OUT_OF_PLACE|
                      FFTW_MEASURE| FFTW_DESTROY_INPUT;


  double Ts;
  double W;
  int N3;


  ftime=fopen("readout_time.dat","r");
  finh=fopen("inh.dat","r");

  fprintf(stderr,"1\n");
  
  min_time=999999.0; max_time=-9999999.0;//Integer.maxValue!!!!
  for(j=0;j<M;j++)
  {
    fscanf(ftime,"%le ",&time);
    if(time<min_time)
      min_time = time;
    if(time>max_time)
      max_time = time;
  }

  fprintf(stderr,"2\n");
  
  fclose(ftime);
  
  Ts=min_time+max_time/2.0;


  min_inh=999999.0; max_inh=-9999999.0;//Integer.maxValue!!!!
  for(j=0;j<N*N;j++)
  {
    fscanf(finh,"%le ",&w);
    if(w<min_inh)
      min_inh = w;
    if(w>max_inh)
      max_inh = w;
  }
  fclose(finh);

  W=2.0*MAX(fabs(min_inh),fabs(max_inh)); //1.0+m/n!?!?!?!?!?

  N3=next_power_of_2(ceil((MAX(fabs(min_inh),fabs(max_inh))*(max_time-min_time)+6/(2*2))*4*2));
  
  
  fprintf(stderr,"3:  %i %e %e %e %e %e %e\n",N3,W,min_inh,max_inh,min_time,max_time,Ts);
  


  my_N[0]=N;my_n[0]=next_power_of_2(N)+16;
  my_N[1]=N; my_n[1]=next_power_of_2(N)+16;
  my_N[2]=N3;
  
  /* initialise nfft */ 
  mri_inh_2d1d_init_guru(&my_plan, my_N, M, my_n, 4,flags,
                      FFTW_MEASURE| FFTW_DESTROY_INPUT);

  fp=fopen(filename,"r");
  ftime=fopen("readout_time.dat","r");

  for(j=0;j<my_plan.M_total;j++)
  {

      fscanf(fp,"%le %le %le %le",&my_plan.x[2*j+0],&my_plan.x[2*j+1],&real,&imag);
    my_plan.f[j]=real+I*imag;
    fscanf(ftime,"%le ",&my_plan.t[j]);

    my_plan.t[j] = (my_plan.t[j]-Ts)*W/N3;
/*
    fscanf(fp,"%le %le %le %le",&my_plan.x[3*j+0],&my_plan.x[3*j+1],&real,&imag);
    my_plan.f[j]=real+I*imag;
    fscanf(ftime,"%le ",&my_plan.x[3*j+2]);

    my_plan.x[3*j+2] = (my_plan.x[3*j+2]-Ts)*W/my_n[2];*/
  }
  fclose(fp);
  fclose(ftime);


  finh=fopen("inh.dat","r");
  for(j=0;j<N*N;j++)
  {
    fscanf(finh,"%le ",&my_plan.w[j]);
    my_plan.w[j]/=W;
  }
  fclose(finh);  
  
  if (weight)
  {
    fw=fopen("weights.dat","r");
    for(j=0;j<my_plan.M_total;j++)
    {
      fscanf(fw,"%le ",&weights);
      my_plan.f[j]*= weights;
    }
    fclose(fw);
  }

    
  if(my_plan.nfft_flags & PRE_PSI) {
    nfft_precompute_psi((nfft_plan*)&my_plan);
  }
  if(my_plan.nfft_flags & PRE_FULL_PSI) {
      nfft_precompute_full_psi((nfft_plan*)&my_plan);
  } 

  t=second();
  /* do the transform */ 
  mri_inh_2d1d_adjoint(&my_plan);
  

    
  fout_real=fopen("output_real.dat","w");
  fout_imag=fopen("output_imag.dat","w");
  
  for (j=0;j<N*N;j++) {
    /* Verschiebung wieder herausrechnen */
    my_plan.f_hat[j]*=cexp(2.0*I*PI*Ts*my_plan.w[j]*W);
    
    fprintf(fout_real,"%le ",creal(my_plan.f_hat[j]));
    fprintf(fout_imag,"%le ",cimag(my_plan.f_hat[j]));
  }
  
  t=second()-t;
  fprintf(stderr,"time: %e seconds\n",t);

  fclose(fout_real);
  fclose(fout_imag);
  mri_inh_2d1d_finalize(&my_plan);
}


int main(int argc, char **argv)
{
  if (argc <= 3) {

    printf("usage: ./reconstruct FILENAME N M m PRE_FULL_PSI\n");
    return 1;
  }
  
  /* Allocate memory to hold every layer in memory after the
  2D-infft */

  nfft(argv[1],atoi(argv[2]),atoi(argv[3]),atoi(argv[4]),atoi(argv[5]));

  return 1;
}
