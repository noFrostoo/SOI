#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"
#include "time.h"
#include "math.h"
#include "sys/queue.h"
#include "semaphore.h"
#include "pthread.h"
#include "unistd.h"
#include "time.h"
#include "signal.h"

#define bufforSize 3770
#define buffersAmount 3

#define numberOfSources 100
#define packetLen 64
#define numberOfComCanals 3

#define SHARED 1 // used to tell if semaphore is sherad by threads

#define indexStartStreamBuffers 1
#define nextBufferCutOff 50 //we want to have many sources sent to buffers closer to the start so next buffer is deed as good place for data if has more than cufoff empty place then current min

typedef STAILQ_HEAD(stailhead, entry) stailhead;

typedef struct entry {
    char data;
    int sourceID;
    STAILQ_ENTRY(entry) entries;    /* Tail queue. */

} entry;

typedef struct Buffer
{
    int id;

    int sourceId;
    bool hasStreamData;

    sem_t lock; // buffer is currently in use  
    sem_t full;       
    sem_t empty;

    struct stailhead* head; 
   
    struct Buffer* next;
    struct Buffer* prev;
}Buffer;

typedef struct Source
{
    int id;
    pthread_t tid;
    
    bool sendsStream;
    
    struct timespec start, end;

    int attemtedSends;
    //data form 0 - 99 means stream and 101 means packet 200 means empty
    char data;
    int amountDataToSent;

    Buffer* bufferToSend;

    entry* last; //used to keep steam data in order
    int sending; //already sendig has began

    
    int sendingTime;
    int timeBetweenSends;

    int sourceBreak;

} Source;

typedef struct Cannal
{
    int id;
    pthread_t tid;

    int curSize;

    int canalReciveTimeData;
    int canalSendDataTime;

} Cannal;


#define OK 0 // things are cool
#define ERROR -1
#define NBLEFT 2 //no buffors left 
#define NYDEF 99 // no yet defined error
