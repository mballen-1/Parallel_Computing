#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/gpu/gpu.hpp>
#include<opencv2/gpu/gpmat.hpp>

#define MAX_THREAD 16

using namespace cv;

Mat image;
int N_THREADS;
int KERNEL_SIZE;
pthread_t threads[MAX_THREAD];

struct data{
	int ib, ie;
}d[MAX_THREAD];

char add_c[3] = {'b','_',0};

__global__ void
blur(gpu::GpuMat image){

	int x = blockDim.x * blockIdx.x + threadIdx.x;

	struct data *dat;
	dat = args;

	for(int i = dat->ib; i< dat->ie; i++){
		for(int j = 0; j< image.cols; j++){
			if(i > KERNEL_SIZE && j > KERNEL_SIZE && i + KERNEL_SIZE < image.rows && j + KERNEL_SIZE < image.cols){
				int b = 0.0, r = 0.0, g = 0.0;
				int c = 0;
				for(int k = i - KERNEL_SIZE; k < i + KERNEL_SIZE; k++){
					for(int l = j - KERNEL_SIZE; l<  j + KERNEL_SIZE; l++){
						b += image.at<Vec3b>(k,l)[0]; //blue
						g += image.at<Vec3b>(k,l)[1]; //green
						r += image.at<Vec3b>(k,l)[2]; //red
						c++;
					}
				}
				image.at<Vec3b>(i,j)[0] = (b/c);
				image.at<Vec3b>(i,j)[1] = (g/c);
				image.at<Vec3b>(i,j)[2] = (r/c);
			}
		}
	}
	pthread_exit(NULL);
}




int main(int argc, const char *argv[]){

	cudaError_t err = cudaSuccess;

	if( argc < 3){
		printf("Usage: <img_path> <kernel_size> <thread_number> \n");
		return -1;
	}
	sscanf(argv[3],"%d", &KERNEL_SIZE);
	sscanf(argv[2],"%d", &N_THREADS);

	image = imread(argv[1]);
	gpu::GpuMat dst;
	dst.upload(image);

	int threadsPerBlock = 256;
	int blocksPerGrid =(numElements + threadsPerBlock - 1) / threadsPerBlock;
	blur<<blocksPerGrid, threadsPerBlock>>(&dst);
//	cudaMemcpy2DFrom








	if(!image.data){
		printf("no image data\n");
		return -1;
	}

	namedWindow("before blur", WINDOW_AUTOSIZE);
	imshow("before blur", image);

	for(int i = 0; i < N_THREADS; i++){
		d[i].ib = (int)((image.rows/N_THREADS)*i);
		d[i].ie = d[i].ib + (image.rows/N_THREADS) -1;
	}

  	for(int i = 0; i <N_THREADS; i++){
  	  if( pthread_create(&threads[i], NULL, blur, (void *)&d[i]) != 0)
  	    perror("could not create thread\n");
  	}

  	for(int i = 0; i <N_THREADS; i++){
  	  if( pthread_join(threads[i], NULL) != 0)
  	    perror("could not create thread\n");
  	}

	char * name = malloc(sizeof(argv[1]) + sizeof(add_c) + 1);
	name[0] = 0;
	strcat(name,add_c);
	strcat(name,argv[1]);
	imwrite(name, image);

	return 0;
}
