
// gcc ~/Downloads/portaudio/src/common/pa_ringbuffer.c paex_pink2.c libportaudio.a -lrt -lm -lasound -pthread -o paex_pink2 -I ~/Downloads/portaudio/src/common/ -I $PWD
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "portaudio.h"
#include "pa_ringbuffer.h"
#include "pa_util.h"

#define PINK_MAX_RANDOM_ROWS   (30)
#define PINK_RANDOM_BITS       (24)
#define PINK_RANDOM_SHIFT      ((sizeof(long)*8)-PINK_RANDOM_BITS)

uint8_t GBL_SPIKE_STATE = 0x00;

typedef struct
{
    long      pink_Rows[PINK_MAX_RANDOM_ROWS];
    long      pink_RunningSum;   /* Used to optimize summing of generators. */
    int       pink_Index;        /* Incremented each sample. */
    int       pink_IndexMask;    /* Index wrapped by ANDing with this mask. */
    float     pink_Scalar;       /* Used to scale within range of -1.0 to +1.0 */
} PinkNoise;

/* Prototypes */
static unsigned long GenerateRandomNumber( void );
void InitializePinkNoise( PinkNoise *pink, int numRows );
float GeneratePinkNoise( PinkNoise *pink );
/************************************************************/
/* Calculate pseudo-random 32 bit number based on linear congruential method. */
static unsigned long GenerateRandomNumber( void )
{
    /* Change this seed for different random sequences. */
    static unsigned long randSeed = 22222;
    randSeed = (randSeed * 196314165) + 907633515;
    return randSeed;
}
/************************************************************/
/* Setup PinkNoise structure for N rows of generators. */
void InitializePinkNoise( PinkNoise *pink, int numRows )
{
    int i;
    long pmax;
    pink->pink_Index = 0;
    pink->pink_IndexMask = (1<<numRows) - 1;
    
    /* Calculate maximum possible signed random value. Extra 1 for white noise always added. */
    pmax = (numRows + 1) * (1<<(PINK_RANDOM_BITS-1));
    pink->pink_Scalar = 1.0f / pmax;
    
    /* Initialize rows. */
    for( i=0; i<numRows; i++ ) { pink->pink_Rows[i] = 0; }
    pink->pink_RunningSum = 0;
}

/* Generate Pink noise values between -1.0 and +1.0 */
float GeneratePinkNoise( PinkNoise *pink )
{
    long newRandom;
    long sum;
    float output;
    /* Increment and mask index. */
    pink->pink_Index = (pink->pink_Index + 1) & pink->pink_IndexMask;
    
    /* If index is zero, don't update any random values. */
    if( pink->pink_Index != 0 )
    {
        /* Determine how many trailing zeros in PinkIndex. */
        /* This algorithm will hang if n==0 so test first. */
        int numZeros = 0;
        int n = pink->pink_Index;
        while( (n & 1) == 0 )
        {
            n = n >> 1;
            numZeros++;
        }
        
        /* Replace the indexed ROWS random value.
         * Subtract and add back to RunningSum instead of adding all the random
         * values together. Only one changes each time.
         */
        pink->pink_RunningSum -= pink->pink_Rows[numZeros];
        newRandom = ((long)GenerateRandomNumber()) >> PINK_RANDOM_SHIFT;
        pink->pink_RunningSum += newRandom;
        pink->pink_Rows[numZeros] = newRandom;
    }
    
    /* Add extra white noise value. */
    newRandom = ((long)GenerateRandomNumber()) >> PINK_RANDOM_SHIFT;
    sum = pink->pink_RunningSum + newRandom;
    
    /* Scale to range of -1.0 to 0.9999. */
    output = pink->pink_Scalar * sum;

    return output;
}
/*******************************************************************/
/* Context for callback routine. */
typedef struct
{
    PinkNoise   leftPink;
    // PinkNoise   rightPink;
    // unsigned int sampsToGo;
    
     /* Ring buffer (FIFO) for "communicating" towards audio callback */
     PaUtilRingBuffer    rBufToRT;
     void*               rBufToRTData;
     
     /* Ring buffer (FIFO) for "communicating" from audio callback */
     PaUtilRingBuffer    rBufFromRT;
     void*               rBufFromRTData;

} paTestData;

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int patestCallback(const void*                     inputBuffer,
                          void*                           outputBuffer,
                          unsigned long                   framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags           statusFlags,
                          void*                           userData)
{
    char spike = 0;
    
    paTestData *data = (paTestData*)userData;
    
    float *out = (float*)outputBuffer;
    (void) inputBuffer; /* Prevent "unused variable" warnings. */

    // double tnow = PaUtil_GetTime();
    // PaUtil_WriteRingBuffer(&data->rBufFromRT, &tnow, 1);
            
    for(int k = 0; k < framesPerBuffer; ++k)
    {
        if (GBL_SPIKE_STATE)
        {
            // PaUtil_ReadRingBuffer(&data->rBufToRT, &spike, 1);
            double tnow = PaUtil_GetTime();
            PaUtil_WriteRingBuffer(&data->rBufFromRT, &tnow, 1);
            GBL_SPIKE_STATE = 0x00;
            spike = 1;
        }
        
        if (spike)
        {
            *out++ = k < 8 ? 0.999f : -0.999f;
        }
        else
        {
            *out++ = GeneratePinkNoise( &data->leftPink );
            // *out++ = GeneratePinkNoise( &data->rightPink );
        }
    }
    
    // if (GBL_SPIKE_STATE)
    // {
    //     GBL_SPIKE_STATE = 0x00;
    // }
    
    return paContinue;
}
/*******************************************************************/
int main(void)
{
    PaStream*           stream;
    PaError             err;
    paTestData          data = {0};
    PaStreamParameters  outputParameters;
    // int                 totalSamps;
    
    static const double SR  = 44100.0;
    static const int    FPB = 16;//2048; /* Frames per buffer: 46 ms buffers. */
    
    const double duration = 5.2;
    
    /* Initialize communication buffers (queues) */
    data.rBufToRTData = PaUtil_AllocateMemory(sizeof(char) * 256);
    if (data.rBufToRTData == NULL)
    {
        return 1;
    }
     
    PaUtil_InitializeRingBuffer(&data.rBufToRT, sizeof(char), 256, data.rBufToRTData);
    
    data.rBufFromRTData = PaUtil_AllocateMemory(sizeof(double) * 256);
    if (data.rBufFromRTData == NULL)
    {
        PaUtil_FreeMemory(data.rBufToRTData);
        return 1;
    }
     
    PaUtil_InitializeRingBuffer(&data.rBufFromRT, sizeof(double), 256, data.rBufFromRTData);
    
    /* Initialize two pink noise signals with different numbers of rows. */
    InitializePinkNoise( &data.leftPink,  12 );
    // InitializePinkNoise( &data.rightPink, 16 );

    
    // data.sampsToGo = totalSamps = (int)(5.0 * SR);   /* Play a 15 seconds. */
    
    err = Pa_Initialize();
    if( err != paNoError ) { goto error; }
    
    /* Open a stereo PortAudio stream so we can hear the result. */
    outputParameters.device = Pa_GetDefaultOutputDevice(); /* Take the default output device. */
    
    if (outputParameters.device == paNoDevice)
    {
      fprintf(stderr,"Error: No default output device.\n");
      goto error;
    }
    
    outputParameters.channelCount = 1;                     /* Stereo output, most likely supported. */
    outputParameters.hostApiSpecificStreamInfo = NULL;
    outputParameters.sampleFormat = paFloat32;             /* 32 bit floating point output. */
    outputParameters.suggestedLatency =
                     Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    
    err = Pa_OpenStream(&stream,
                        NULL,                              /* No input. */
                        &outputParameters,
                        SR,                                /* Sample rate. */
                        FPB,                               /* Frames per buffer. */
                        paClipOff, /* we won't output out of range samples so don't bother clipping them */
                        patestCallback,
                        &data);
    
    if( err != paNoError ) { goto error; }
    
    err = Pa_StartStream( stream );
    if( err != paNoError ) { goto error; }
    
    printf("Stereo pink noise for %.3f seconds...\n", duration);
    
    char spike = 1;
    int nspike = 0;
    
    double tstart, last, req;
    tstart = last = PaUtil_GetTime();
    
    // while( ( err = Pa_IsStreamActive( stream ) ) == 1 )
    // {
    //     int check = 0;
    //     double now = PaUtil_GetTime();
    //     double elap = now - tstart;
    //     if (now >= last + 0.25 && (GBL_SPIKE_STATE < 0x01))
    //     {
    //         req = PaUtil_GetTime();
    //         // PaUtil_WriteRingBuffer(&data.rBufToRT, &spike, 1);
    //         GBL_SPIKE_STATE = 0x01;
    //         last = now;
    //         ++nspike;
    //         check = 1;
    //     }
        
    //     if (check > 0)
    //     {
    //         while (PaUtil_GetRingBufferReadAvailable(&data.rBufFromRT) < 1 && ((now - tstart) < duration))
    //         {
    //             now = PaUtil_GetTime();
    //             Pa_Sleep(1);
    //         }
    //         elap = now - tstart;
            
    //         if (PaUtil_GetRingBufferReadAvailable(&data.rBufFromRT))
    //         {
    //             double sent;
    //             PaUtil_ReadRingBuffer(&data.rBufFromRT, &sent, 1);
    //             printf("Latency b/t rec and send: %f\n", sent - req);
    //         }
    //     }
        
    //     if (elap > duration)
    //     {
    //         break;
    //     }printf("lag1: %f\n", tgot - tsent);
        
    // }
    
    // printf("[INFO]: %d spikes in %f seconds\n", nspike, PaUtil_GetTime() - tstart);
    
    // double ts[100] = {0};
    
    Pa_Sleep(500);
    
    double tsent;
    double tgot, tgotr;
    
    tsent = PaUtil_GetTime();
    // PaUtil_WriteRingBuffer(&data.rBufToRT, &spike, 1);
    GBL_SPIKE_STATE = 0x01;
    // tsent = (tsent + PaUtil_GetTime()) / 2;
    
    while (GBL_SPIKE_STATE > 0x00)//(PaUtil_GetRingBufferReadAvailable(&data.rBufFromRT) < 1)
    {
        Pa_Sleep(1);
    }
    
    tgotr = PaUtil_GetTime();
    
    if (PaUtil_GetRingBufferReadAvailable(&data.rBufFromRT) > 0)
    {
        PaUtil_ReadRingBuffer(&data.rBufFromRT, &tgot, 1);
        // ts[k] = tgot - tsent;
    }
    
    Pa_Sleep(500);
    
    printf("lag1: %f\n", tgot - tsent);
    printf("lag2: %f\n", tgotr - tsent);
    
    if( err < 0 ) { goto error; }

    err = Pa_StopStream( stream );
    if( err != paNoError ) { goto error; }

    err = Pa_CloseStream( stream );
    if( err != paNoError ) { goto error; }
    
    // double mn = 0.0;
    // for (int k = 1; k < 100; ++k)
    // {
    //     // double diff = (ts[k] - ts[k-1]);
    //     mn += ts[k];
    //     printf("ts[%d] = %f\n", k, ts[k]);
    // }
    

    // printf("Mean lag: %f\n", mn / 100.0);
    
    
    if (data.rBufToRTData)
    {
        PaUtil_FreeMemory(data.rBufToRTData);
    }
    
    if (data.rBufFromRTData)
    {
        PaUtil_FreeMemory(data.rBufFromRTData);
    }

    Pa_Terminate();
    return 0;

error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return 0;
}