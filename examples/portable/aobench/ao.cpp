/*
  Copyright (c) 2010-2011, Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.


   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
   TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
*/

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#pragma warning (disable: 4244)
#pragma warning (disable: 4305)
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#ifdef __linux__
#include <malloc.h>
#endif
#include <math.h>
#include <map>
#include <string>
#include <algorithm>
#include <sys/types.h>

#include "ao_ispc.h"

#include "timing.h"
#include "ispc_malloc.h"

#define NSUBSAMPLES        2

static unsigned int test_iterations[] = {3, 7, 1};
static unsigned int width, height;
static unsigned char *img;
static float *fimg;


static unsigned char
clamp(float f)
{
    int i = (int)(f * 255.5);

    if (i < 0) i = 0;
    if (i > 255) i = 255;

    return (unsigned char)i;
}


static void
savePPM(const char *fname, int w, int h)
{
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++)  {
            img[3 * (y * w + x) + 0] = clamp(fimg[3 *(y * w + x) + 0]);
            img[3 * (y * w + x) + 1] = clamp(fimg[3 *(y * w + x) + 1]);
            img[3 * (y * w + x) + 2] = clamp(fimg[3 *(y * w + x) + 2]);
        }
    }

    FILE *fp = fopen(fname, "wb");
    if (!fp) {
        perror(fname);
        exit(1);
    }

    fprintf(fp, "P6\n");
    fprintf(fp, "%d %d\n", w, h);
    fprintf(fp, "255\n");
    fwrite(img, w * h * 3, 1, fp);
    fclose(fp);
    printf("Wrote image file %s\n", fname);
}


int main(int argc, char **argv)
{
    if (argc < 3) {
        printf ("%s\n", argv[0]);
        printf ("Usage: ao [width] [height] [ispc iterations] [tasks iterations] [serial iterations]\n");
        getchar();
        exit(-1);
    }
    else {
        if (argc == 6) {
            for (int i = 0; i < 3; i++) {
                test_iterations[i] = atoi(argv[3 + i]);
            }
        }
        width = atoi (argv[1]);
        height = atoi (argv[2]);
    }

    // Allocate space for output images
    img = new unsigned char[width * height * 3];
    fimg = new float[width * height * 3];

    //
    // Run the ispc + tasks path, test_iterations times, and report the
    // minimum time for any of them.
    //
    double minTimeISPCTasks = 1e30;
    for (unsigned int i = 0; i < test_iterations[1]; i++) {
        ispc_memset(fimg, 0, sizeof(float) * width * height * 3);
        assert(NSUBSAMPLES == 2);

        reset_and_start_timer();
        ispc::ao_ispc_tasks(width, height, NSUBSAMPLES, fimg);
        double t = get_elapsed_msec();
        printf("@time of ISPC + TASKS run:\t\t\t[%.3f] msec\n", t);
        minTimeISPCTasks = std::min(minTimeISPCTasks, t);
    }

    // Report results and save image
    printf("[aobench ispc + tasks]:\t\t[%.3f] msec (%d x %d image)\n", 
           minTimeISPCTasks, width, height);
    savePPM("ao-ispc-tasks.ppm", width, height); 

    delete img;
    delete fimg;
        
    return 0;
}
