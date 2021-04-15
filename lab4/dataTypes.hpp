#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <boost/interprocess/anonymous_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>


#define bufforSize 3860
#define buffersAmount 3

#define numberOfSources 100
#define packetLen 64
#define numberOfComCanals 3

#define SHARED 1 // used to tell if semaphore is sherad by threads

#define indexStartStreamBuffers 1
#define nextBufferCutOff 30 //we want to have many sources sent to buffers closer to the start so next buffer is deed as good place for data if has more than cufoff empty place then current min


class Condition
{
  friend class Buffer;

public:
	Condition()
	{
		waitingCount = 0;
        sem_init(&w, SHARED, 0);
	}

    ~Condition()
    {
        sem_destroy(&w);
    }

	void wait()
	{
		sem_wait(&w);
	}

	bool signal()
	{
		if( waitingCount )
		{
			-- waitingCount;
			sem_post(&w);
			return true;
		}//if
		else
			return false;
	}

private:
	sem_t w;
	int waitingCount; //liczba oczekujacych watkow
};

class Source
{
private:
    int id;
    
    bool sendsStream;
    
    struct timespec start, end;

    //data form 0 - 99 means stream and 101 means packet 200 means empty
    char data;
    int amountDataToSent;

    int sending; //already sendig has began

    
    int sendingTime;
    int timeBetweenSends;

    int sourceBreak;
public:
    int attemtedSends;
    bool lastNotGood;

    Source(int id);
    ~Source();
    void PrintSource();
    void UpdateSource();
    void Start();
    static int Action(void* s, int id);
    bool SendsStream();
    void startTiming();
    double stopTiming();
    char getData();
    int getId();
};


class Buffer
{
private:

    int id;

    int sourceId;
    bool hasStreamData;
    
    int failed;

    int count;

    char buf[bufforSize];
    int top;

    sem_t s;
    Condition full, empty;

    void SendFistStream(Source *s);
    void SendStream(Source *s);
    void SendPacket(Source *s);
    void enter();
    void leave();
    void wait(Condition& cond);
    void signal(Condition& cond);
public:
    //puclic cuz i didn't have energy to rewirte parts of code
    struct Buffer* next;
    struct Buffer* prev;

    Buffer();
    Buffer(int id);
    ~Buffer();
    void printBuffer();
    int getCount();
    int getId();
  
    void SendData(Source *s);
    char RecivePacket();
  
    void PrintInside();
    void SetId(int id);
};


class Cannal
{
private:
    int id;

    int curSize;

    int canalReciveTimeData;
    int canalSendDataTime;
public:
    static void Action(void* c, int id);
    Cannal(int id);
    ~Cannal();
};

struct sharedmem
{
    Buffer buffers[buffersAmount];
    int attemtedSentd[numberOfSources];
    int dataSent;
    int recivedData;
    int faildPackets;
    int faildStreams;
    int RUN;

    sharedmem()
    {
        dataSent = 0;
        recivedData = 0;
        faildPackets = 0;
        faildStreams = 0;
        RUN = 1;
    }
};
