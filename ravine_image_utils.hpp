#ifndef RAVINE_IMAGE_UTILS_HPP_
#define RAVINE_IMAGE_UTILS_HPP_

#include <cinttypes>
#include <cstdio>

/* ========================================================================= */
void normalize(uint8_t* buffer, int32_t width, int32_t height)
{
    float mx = 0.0f;
    float mn = 255.0f;
    for(int32_t k = 0; k < (width*height); ++k)
    {
        float tmp = (float)buffer[k];
        if (tmp > mx)
        {
            mx = tmp;
        }
        else if (tmp < mn)
        {
            mn = tmp;
        }
    }
    
    mx -= mn;
    
    if (mx > 0.0f)
    {
        for(int32_t k = 0; k < (width*height); ++k)
        {
            buffer[k] = (uint8_t)((((float)buffer[k] - mn) / mx) * 255.0f);
        }
    }
    
}
/* ========================================================================= */
void yuv422_to_gray(uint8_t* buffer, int32_t width, int32_t height, uint8_t* obuffer)
{

    for (int32_t k = 0; k < (width*height); ++k)
    {
        //low byte of every other 16-bit sample is y-channel
        obuffer[k] = buffer[k*2];
    }
}
/* ========================================================================= */
int write_pgm(uint8_t* buffer, int32_t width, int32_t height, const char* ofile)
{
    FILE* fp = fopen(ofile, "wb");
    
    if (fp == NULL)
    {
        return 1;
    }
    
    fprintf(fp, "P5\n%d %d\n255\n", width, height);
    fwrite(buffer, sizeof(uint8_t), (width*height), fp);
    fclose(fp);
    
    return 0;
}
/* ========================================================================= */
#endif