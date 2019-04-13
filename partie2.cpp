#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <pthread.h>
#include <ctime>
#include <semaphore.h>
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

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond;

/* MANIPULER LES NOMBRES COMPLEXES ********************************************/

typedef struct {
    long double real;
    long double imag;
} complex;
complex c; // GLOBALE

long double c_real = 0;
long double c_imag = 0;
int color_in[3];
float vertical = 0;
float horizontal = 0;
float scale = 0;
int s = 0;
//int r = 0;
//int c_imag = 0;

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
        ((long double) x / IMG_W * (LIMIT_RIGHT+scale - LIMIT_LEFT+scale)) + LIMIT_LEFT+horizontal-scale,
        ((long double) y / IMG_H * (LIMIT_BOTTOM+scale - LIMIT_TOP+scale)) + LIMIT_TOP+vertical-scale);
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
    int x,y;
    int nb;
	while(1){
        pthread_mutex_lock(&mutex);
        while(I < nb_echantillon){
            nb = I;
            I += 1;
            pthread_mutex_unlock(&mutex);
            x = ((nb - 1) * ((IMG_W*IMG_H)/nb_echantillon)) / IMG_H;
            y = ((nb - 1) * ((IMG_W*IMG_H)/nb_echantillon)) % IMG_H;
            while((x * IMG_H) + y <= (nb * ((IMG_W*IMG_H)/nb_echantillon)) - 1){
            	int j = juliaDot(convert(x, y), MAX_ITER);
            	cv::Vec3b color(j+color_in[0], j+color_in[1], j+color_in[2]);
            	newImg.at<cv::Vec3b>(cv::Point(x, y)) = color;
                y+=1;
                if(y == IMG_H){
                    y = 0;
                    x += 1;
                }
            }
            pthread_mutex_lock(&mutex);
        }
        pthread_cond_wait(&cond, &mutex);
        pthread_mutex_unlock(&mutex);
        if (s == 1){
        	break;
        }
	}

    return NULL;
}

void change_color(cv::Mat newImg, int key){
    for (int i = 0; i < 3; ++i){
        if (key == 'c')
            color_in[i] = (rand() % 1000 + 1);
        if (key == 'n')
            color_in[i] = 0;
    }
}

void change_real(cv::Mat newImg){
    c_real += 0.1;
    if (c_real >= 1)
        c_real = -1;
    c = new_complex(c_real, c_imag);
    printf("New value real : %Le\n", c_real);
}

void change_imag(cv::Mat newImg){
    c_imag += 0.1;
    if (c_imag >=1)
        c_imag = -1;
    c = new_complex(c_real, c_imag);
    printf("New value imag : %Le\n", c_imag);
}

void button_zoom(cv::Mat newImg, int key){
    if (scale > -0.9 && key == 'z') //zoom
        scale -= 0.1;

    if (scale < 1 && key == 'd') //dezoom
        scale += 0.1;
}

void button_moove (cv::Mat newImg, int key){
    if (horizontal+0.1 < 1 && key == 'm') //Right
        horizontal+= 0.1;

    if (vertical-0.1 > -1 && key == 'o') //Up
        vertical -= 0.1;

    if (vertical-0.1 < 1 && key == 'l') //Down
        vertical += 0.1;

    if (horizontal+0.1 > -1 && key == 'k') //Left
        horizontal-= 0.1;
}


/* MAIN ***********************************************************************/

int main(int argc, char * argv[]) {
    int key = -1;
    int save = 0;
    long j;
    nb_thr = atoi(argv[1]);
    nb_echantillon = atoi(argv[2]);
    pthread_t pid[nb_thr];
    pthread_cond_init(&cond, NULL);
    c = new_complex(c_real, c_imag);
    for(j = 0; j < nb_thr; j++){
        pthread_create(&pid[j], NULL, julia, (void*)j);
    }


    while( (key = (char)cvWaitKey(30)) ) {

    	if (key == 'z' || key == 'd')
        	button_zoom(newImg, key);

        if (key == 'm' || key == 'o' || key == 'l' || key == 'k')
            button_moove(newImg, key);

        if (key == 'c' || key == 'n')
            change_color(newImg, key);

        if (key == 's'){
            char name[15];
            sprintf(name, "image_bw%d.bmp", save);
            imwrite(name, newImg); // sauve une copie de l'image
            save++;
        }

        if (key == 'r')
            change_real(newImg);

        if (key == 'i')
            change_imag(newImg);

        if (key == 'q'){
        	pthread_mutex_lock(&mutex);
            s = 1;
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);
            break;
        }
        
        if (key != -1){
            pthread_mutex_lock(&mutex);
           	I = 1;
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);
		}
        imshow("image", newImg); // met à jour l'image
    }

   	for(j = 0; j < nb_thr; j++){
   		pthread_cancel(pid[j]);
   		pthread_join(pid[j], 0);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    return 0;
}
