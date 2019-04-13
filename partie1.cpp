#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <pthread.h>
#include <ctime>
#include <fcntl.h> 
#include <unistd.h>


/* CONSTANTES *****************************************************************/

#define IMG_W 400
#define IMG_H 400 //768
#define MAX_ITER 300    // 200
#define MAX_NORM 4        // 2


#define LIMIT_LEFT -1
#define LIMIT_RIGHT 1
#define LIMIT_TOP -1
#define LIMIT_BOTTOM 1

pthread_mutex_t fifo = PTHREAD_MUTEX_INITIALIZER;

/* MANIPULER LES NOMBRES COMPLEXES ********************************************/

typedef struct {
    long double real;
    long double imag;
} complex;
complex c; // GLOBALE

int nb_echantillon;
int nb_thr,I;

cv::Mat newImg(IMG_H, IMG_W, CV_8UC3);

complex new_complex(long double real, long double imag) {
    complex c;
    c.real = real;
    c.imag = imag;
    return c;
}

complex add_complex(complex a, complex b) {
    a.real += b.real;
    a.imag += b.imag;
    return a;
}

complex mult_complex(complex a, complex b) {
    complex m;
    m.real = a.real * b.real - a.imag * b.imag;
    m.imag = a.real * b.imag + a.imag * b.real;
    return m;
}

long double module_complex(complex c) {
    return c.real * c.real + c.imag * c.imag;
}

/* FRACTALE DE JULIA *****************************************************/

complex convert(int x, int y) {
   return new_complex(
        ((long double) x / IMG_W * (LIMIT_RIGHT - LIMIT_LEFT)) + LIMIT_LEFT,
        ((long double) y / IMG_H * (LIMIT_BOTTOM - LIMIT_TOP)) + LIMIT_TOP );
}

int juliaDot(complex z, int iter) {
	int i;
    for (i = 0; i < iter; i++) {
        z = add_complex(mult_complex(z, z), c);
        long double norm = module_complex(z);
        if (norm > MAX_NORM) {
            break;
        }
    }
    return i * 255 / iter; // on met i dans l'intervalle 0 à 255
}

void* julia(void* _p) {
	//long num = (long) _p;
    int x,y,limite;
    int nb;
    pthread_mutex_lock(&fifo);
    while(I <= nb_echantillon){
        nb = I;
        I += 1;
        pthread_mutex_unlock(&fifo);
        x = ((nb - 1) * ((IMG_H*IMG_W)/nb_echantillon)) / IMG_H;
        y = ((nb - 1) * ((IMG_H*IMG_W)/nb_echantillon)) % IMG_H;
        limite = (nb * ((IMG_H*IMG_W)/nb_echantillon)) - 1;
        while((x * IMG_H) + y <= limite){
            int j = juliaDot(convert(x, y), MAX_ITER);
            cv::Vec3b color(j, j, j);
            newImg.at<cv::Vec3b>(cv::Point(x, y)) = color;
            y+=1;
            if(y == IMG_H){
                y = 0;
                x += 1;
            }
        }
        pthread_mutex_lock(&fifo);
    }
    pthread_mutex_unlock(&fifo);
    return NULL;
}


/* MAIN ***********************************************************************/

int main(int argc, char * argv[]) {

   	long double i,w;
    long j;
    nb_thr = atoi(argv[1]);
    nb_echantillon = atoi(argv[2]);
    pthread_t pid[nb_thr];
    for (i = -1 ; i < 1.01; i = i + 0.1) {
    	for(w = -1; w < 1.01; w = w + 0.1){
        	c = new_complex(i,w);
            pthread_mutex_lock(&fifo);
            I = 1;
            pthread_mutex_unlock(&fifo);
            for(j = 0; j < nb_thr; j++){
                pthread_create(&pid[j], NULL, julia, (void*)j);
            }
            for(j = 0; j < nb_thr; j++){
                pthread_join(pid[j], 0);
            }
    	}
   	}
    pthread_mutex_destroy(&fifo);
    return 0;
}
