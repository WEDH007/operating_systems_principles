#include "bitmap.h"
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

// Define a structure to pass data to threads
typedef struct {
    struct bitmap *bm;
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    int max;
    int start_line;
    int end_line;
} thread_data_t;

int iteration_to_color( int i, int max );
int iterations_at_point( double x, double y, int max );
void *compute_image_thread(void *thread_arg);

void show_help()
{
    printf("Use: mandel [options]\n");
    printf("Where options are:\n");
    printf("-m <max>    The maximum number of iterations per point. (default=1000)\n");
    printf("-x <coord>  X coordinate of image center point. (default=0)\n");
    printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
    printf("-s <scale>  Scale of the image in Mandlebrot coordinates. (default=4)\n");
    printf("-W <pixels> Width of the image in pixels. (default=500)\n");
    printf("-H <pixels> Height of the image in pixels. (default=500)\n");
    printf("-o <file>   Set output file. (default=mandel.bmp)\n");
    printf("-n <threads> Number of threads. (default=1)\n");
    printf("-h          Show this help text.\n");
    printf("\nSome examples are:\n");
    printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
    printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
    printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}

void compute_image( struct bitmap *bm, double xmin, double xmax, double ymin, double ymax, int max )
{
	int i,j;

	int width = bitmap_width(bm);
	int height = bitmap_height(bm);

	// For every pixel in the image...

	for(j=0;j<height;j++) {

		for(i=0;i<width;i++) {

			// Determine the point in x,y space for that pixel.
			double x = xmin + i*(xmax-xmin)/width;
			double y = ymin + j*(ymax-ymin)/height;

			// Compute the iterations at that point.
			int iters = iterations_at_point(x,y,max);

			// Set the pixel in the bitmap.
			bitmap_set(bm,i,j,iters);
		}
	}
}

int main( int argc, char *argv[] )
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    char c;

    // These are the default configuration values used
    // if no command line arguments are given.

    const char *outfile = "mandel.bmp";
    double xcenter = 0;
    double ycenter = 0;
    double scale = 4;
    int image_width = 500;
    int image_height = 500;
    int max = 1000;
    int num_threads = 1;

    // For each command line argument given,
    // override the appropriate configuration value.

    while((c = getopt(argc,argv,"x:y:s:W:H:m:o:n:h"))!=-1) {
        switch(c) {
            case 'x':
                xcenter = atof(optarg);
                break;
            case 'y':
                ycenter = atof(optarg);
                break;
            case 's':
                scale = atof(optarg);
                break;
            case 'W':
                image_width = atoi(optarg);
                break;
            case 'H':
                image_height = atoi(optarg);
                break;
            case 'm':
                max = atoi(optarg);
                break;
            case 'o':
                outfile = optarg;
                break;
            case 'n':
                num_threads = atoi(optarg);
                break;
            case 'h':
                show_help();
                exit(1);
                break;
        }
    }

    // // Display the configuration of the image.
    // printf("mandel: x=%lf y=%lf scale=%lf max=%d outfile=%s threads=%d\n",xcenter,ycenter,scale,max,outfile,num_threads);

    // Create a bitmap of the appropriate size.
    struct bitmap *bm = bitmap_create(image_width,image_height);


    // Fill it with a dark blue, for debugging
    bitmap_reset(bm,MAKE_RGBA(0,0,255,0));

    // Compute the Mandelbrot image
    if (num_threads == 1) {
        // If only one thread, compute image in the main thread.
        compute_image(bm,xcenter-scale,xcenter+scale,ycenter-scale,ycenter+scale,max);
    } else {
        // If multiple threads, create and launch threads to compute the image.
        pthread_t threads[num_threads];
        thread_data_t thread_data[num_threads];
        int lines_per_thread = image_height / num_threads;
        for (int i = 0; i < num_threads; i++) {
            thread_data[i].bm = bm;
            thread_data[i].xmin = xcenter - scale;
            thread_data[i].xmax = xcenter + scale;
            thread_data[i].ymin = ycenter - scale;
            thread_data[i].ymax = ycenter + scale;
            thread_data[i].max = max;
            thread_data[i].start_line = i * lines_per_thread;
            thread_data[i].end_line = (i == num_threads - 1) ? image_height : (i + 1) * lines_per_thread;
            pthread_create(&threads[i], NULL, compute_image_thread, &thread_data[i]);
        }
        // Wait for all threads to complete.
        for (int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("mandel: x=%lf y=%lf scale=%lf max=%d outfile=%s threads=%d Time taken: %f seconds\n",xcenter,ycenter,scale,max,outfile,num_threads, elapsed);

    // Save the image in the stated file.
    if(!bitmap_save(bm,outfile)) {
        fprintf(stderr,"mandel: couldn't write to %s: %s\n",outfile,strerror(errno));
        return 1;
    }

    return 0;
}


/*
Compute an entire Mandelbrot image using threads, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/

void *compute_image_thread(void *thread_arg)
{
    thread_data_t *data = (thread_data_t *)thread_arg;
    struct bitmap *bm = data->bm;
    double xmin = data->xmin;
    double xmax = data->xmax;
    double ymin = data->ymin;
    double ymax = data->ymax;
    int max = data->max;
    int start_line = data->start_line;
    int end_line = data->end_line;

    int i,j;

    int width = bitmap_width(bm);
    int height = bitmap_height(bm);

    // For every pixel in the image...

    for(j = start_line; j < end_line; j++) {

        for(i = 0; i < width; i++) {

            // Determine the point in x,y space for that pixel.
            double x = xmin + i*(xmax-xmin)/width;
            double y = ymin + j*(ymax-ymin)/height;

            // Compute the iterations at that point.
            int iters = iterations_at_point(x,y,max);

            // Set the pixel in the bitmap.
            bitmap_set(bm,i,j,iters);
        }
    }

    return NULL;
}

/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point( double x, double y, int max )
{
    double x0 = x;
    double y0 = y;

    int iter = 0;

    while( (x*x + y*y <= 4) && iter < max ) {

        double xt = x*x - y*y + x0;
        double yt = 2*x*y + y0;

        x = xt;
        y = yt;

        iter++;
    }

    return iteration_to_color(iter,max);
}

/*
Convert a iteration number to an RGBA color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/

// int iteration_to_color( int i, int max )
// {
//     int gray = 255*i/max;
//     return MAKE_RGBA(gray,gray,gray,0);
// }

// int iteration_to_color(int i, int max) {
//     if (i == max) {
//         // If it never escaped, color it black.
//         return MAKE_RGBA(0, 0, 0, 0);
//     } else {
//         // As a simple example, let's make a gradient of blue-to-red based on iteration count.
//         int red = (255 * i) / max;
//         int blue = 255 - red;
//         return MAKE_RGBA(red, 0, blue, 0);
//     }
// }

// Convert HSV to RGB (assuming H in [0, 360], S and V in [0, 1])
void hsv_to_rgb(float h, float s, float v, int *r, int *g, int *b) {
    int i = (int)(h / 60.0f) % 6;
    float f = (h / 60.0f) - i;
    float p = v * (1 - s);
    float q = v * (1 - s * f);
    float t = v * (1 - s * (1 - f));

    switch(i) {
        case 0: *r = v*255, *g = t*255, *b = p*255; break;
        case 1: *r = q*255, *g = v*255, *b = p*255; break;
        case 2: *r = p*255, *g = v*255, *b = t*255; break;
        case 3: *r = p*255, *g = q*255, *b = v*255; break;
        case 4: *r = t*255, *g = p*255, *b = v*255; break;
        case 5: *r = v*255, *g = p*255, *b = q*255; break;
    }
}

int iteration_to_color(int i, int max) {
    if (i == max) {
        return MAKE_RGBA(0, 0, 0, 0);  // black
    } else {
        float hue = (360.0f * i) / max;  // Change this formula as needed
        int r, g, b;
        hsv_to_rgb(hue, 1.0f, 1.0f, &r, &g, &b);
        return MAKE_RGBA(r, g, b, 0);
    }
}
