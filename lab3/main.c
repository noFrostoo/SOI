#include "dataTypes.h"

int RUN;

int dataSent;
int recivedData;
int faildPackets;
int faildStreams;

Source* sources[numberOfSources];
Cannal* canals [numberOfComCanals];

// buffes are kept in double linked list
Buffer* head;
Buffer* tail;

Buffer* packetBuffer;
Buffer* steamBufferStart;


void InitSources()
{
    printf("Init Sources...\n");
    __time_t t;
    int i = 0;
    int timeBetweenSends;
    int amountOfData;
    for( ; i < numberOfSources; i++)
    {
        srand((unsigned) i);
        sources[i] = (Source*) malloc(sizeof(Source));
        sources[i]->id = i;
        sources[i]->data = 101;
        sources[i]->sendsStream = rand() % 2;
        sources[i]->sourceBreak = ((rand() % 41) + 80)*1000;
        sources[i]->attemtedSends = 0;
        sources[i]->sendingTime = bufforSize;
        sources[i]->sending = 0;
        sources[i]->last = NULL;
        timeBetweenSends = 500;
        amountOfData = (rand() % 101);
        if(sources[i]->sendsStream)
        {
            timeBetweenSends = 100;
            amountOfData = (rand() % 471) + 30;
            sources[i]->data = i;
        }
        sources[i]->timeBetweenSends = timeBetweenSends;
        sources[i]->amountDataToSent = amountOfData;

    }
}

void FreeSources()
{
    int i = 0;
    for( ; i< numberOfSources; i++)
    {
        free((void*) sources[i]);
    }
}

void PrintSources()
{
    for(int i = 0; i < numberOfSources; i++)
    {
        printf("Source %d ", sources[i]->id);
        printf(", %d", (int)sources[i]->sendsStream);
        printf(", %d", sources[i]->data);
        printf(", %d", sources[i]->sourceBreak);
        printf(", %d \n", sources[i]->timeBetweenSends);
    }
}

void UpdateSource(Source* s)
{
    int timeBetweenSends;
    int amountOfData;
    srand(time(NULL)+s->id);
    s->sourceBreak = (rand() % 41) + 80;
    s->sending = 0;
    timeBetweenSends = 500;
    amountOfData = (rand() % 471) + 30;
    if(s->sendsStream)
    {
        timeBetweenSends = 100;
        amountOfData = (rand() % 101);
    }
    s->timeBetweenSends = timeBetweenSends;
    s->amountDataToSent = amountOfData;
}

void InitCanals()
{
    printf("Init Cannals...\n");
    for(int i = 0; i < numberOfComCanals; i++)
    {
        canals[i] = (Cannal*) malloc(sizeof(Cannal));
        canals[i]->id = i;
        canals[i]->curSize = 0;
        canals[i]->canalReciveTimeData = bufforSize;
        canals[i]->canalReciveTimeData = packetLen;
    }
}

void FreeCanals()
{
    for(int i = 0; i<numberOfComCanals; i++)
    {
        free((void*)canals[i]);
    }
}

void InitBuffers()
{
    printf("Init Buffers...\n");
    Buffer* temp_ptr;
    
    head = (Buffer*) malloc(sizeof(Buffer));
    head->id = 0;
    sem_init(&(head->lock), SHARED, 1);
    sem_init(&(head->full), SHARED, 0);
    sem_init(&(head->empty), SHARED, bufforSize);

    head->hasStreamData = true;
    head->sourceId = -1;

    head->head = (stailhead*) malloc(sizeof(stailhead));
    head->head->stqh_first = NULL;
    head->head->stqh_last = &(*(head->head)).stqh_first;
    
    STAILQ_INIT(head->head);     

    head->prev = NULL;

    temp_ptr = head;
    for(int i = 1; i<=buffersAmount; i++)
    {
        // to do make freeing buffer corect
        temp_ptr->next = (Buffer*) malloc(sizeof(Buffer));

        temp_ptr->next->id = i;
        sem_init(&(temp_ptr->next->lock), SHARED, 1);
        sem_init(&(temp_ptr->next->full), SHARED, 0);
        sem_init(&(temp_ptr->next->empty), SHARED, bufforSize);

        temp_ptr->next->hasStreamData = true;
        temp_ptr->next->sourceId = -1;

        temp_ptr->next->head = (stailhead*) malloc(sizeof(stailhead));
        temp_ptr->next->head->stqh_first = NULL;
        temp_ptr->next->head->stqh_last = &(*(temp_ptr->next->head)).stqh_first;
        
        STAILQ_INIT(temp_ptr->next->head);     

        temp_ptr->next->prev = temp_ptr;
        temp_ptr = temp_ptr->next;
    }
    tail = temp_ptr;
    tail->next = NULL;
}

void FreeBuffers()
{
    //!!!!!!!!
    //!!!!!!!!
    //!!!!!!!!!!!
    // to do make freeing buffer corect
    Buffer* temp_ptr = head->next;
    while (temp_ptr != NULL)
    {  
        sem_destroy(&(temp_ptr->lock));
        sem_destroy(&(temp_ptr->full));
        sem_destroy(&(temp_ptr->empty));
        ///!!!!!! list free !!!!!!
        free((void*)temp_ptr->head);
        free((void*) temp_ptr->prev);
        temp_ptr = temp_ptr->next;
    }
    free((void*)tail);
}

void PrintBuffers()
{
    int emptyCount;
    int fullCount;
    Buffer* temp_ptr = head->next;
    while (temp_ptr != NULL)
    {
        sem_getvalue(&(temp_ptr->empty), &emptyCount);
        sem_getvalue(&(temp_ptr->full), &fullCount);
        printf("Buffer id: %d ", temp_ptr->id);
        printf("Lock: %d ", temp_ptr->lock);
        printf("Empty count: %d ", emptyCount);
        printf("Fill count: %d\n", fullCount);
        temp_ptr = temp_ptr->next;
    }
}

Buffer* SteamBufforAlocation()
{
    Buffer* bufferWithMaxEmptyPlaces = steamBufferStart;
    Buffer* iterator = steamBufferStart;
    int maxEmpty = 0;
    int semVal;
    for(int i = indexStartStreamBuffers; i < buffersAmount; i++)
    {
        sem_getvalue(&(iterator->empty), &semVal);
        if(semVal > 500)
            return iterator;
        if(semVal > maxEmpty+nextBufferCutOff)
        {
            maxEmpty = 0;
            bufferWithMaxEmptyPlaces = iterator;
        }
    }
    return bufferWithMaxEmptyPlaces;
}

Buffer* AskForBuffor(Source *s)
{
    if(s->sendsStream)
    {
       return SteamBufforAlocation();
    }
    return packetBuffer;
} 

void SendFistStream(Source *s, Buffer *b) //used when in case of sending first stream data 
{
    // printf("Sending  first steam  source: %d", s->id);
    // printf(" to buffer %d \n", b->id);
    double elapsed = 0.0;

    entry* e = (entry*) malloc(sizeof(entry));
    e->data = s->data;
    e->sourceID = s->id;
    clock_gettime(CLOCK_MONOTONIC, &(s->start));
    
    sem_wait(&(b->empty));
    
    clock_gettime(CLOCK_MONOTONIC, &(s->end));
    elapsed = s->end.tv_sec - s->start.tv_sec;
    elapsed += ((s->end.tv_nsec - s->start.tv_nsec) / 1000000000.0); 
    if(elapsed > 0.00013)
    {
        faildStreams++;
        sem_post(&(b->empty));
        free(e);
        return;
    }

    sem_wait(&(b->lock));
    
    STAILQ_INSERT_TAIL(b->head, e, entries);
    dataSent++;

    sem_post(&(b->lock));
    sem_post(&(b->full));

    s->last = e;
    s->sending = 1;
}


void SendStream(Source *s, Buffer *b)
{
    // printf("Sending  steam  source: %d", s->id);
    // printf(" to buffer %d \n", b->id);
    double elapsed = 0.0;

    if(s->last == NULL)
    {
        return SendFistStream(s, b);
    }
    entry* e = (entry*) malloc(sizeof(entry));
    e->data = s->data;
    e->sourceID = s->id;
    clock_gettime(CLOCK_MONOTONIC, &(s->start));
    
    sem_wait(&(b->empty));
    
    clock_gettime(CLOCK_MONOTONIC, &(s->end));
    elapsed = s->end.tv_sec - s->start.tv_sec;
    elapsed += ((s->end.tv_nsec - s->start.tv_nsec) / 1000000000.0); 
    if(elapsed > 0.00013)
    {
        faildStreams++;
        sem_post(&(b->empty));
        free(e);
        return;
    }

    sem_wait(&(b->lock));
    
    STAILQ_INSERT_TAIL(b->head, e, entries);
    dataSent++;

    sem_post(&(b->lock));
    sem_post(&(b->full));
    s->last = e;
}


void SendPacket(Source *s, Buffer *b)
{           
    double elapsed = 0;

    entry* e = (entry*) malloc(sizeof(entry));
    e->data = s->data;
    e->sourceID = s->id;
    clock_gettime(CLOCK_MONOTONIC, &(s->start));
    
    sem_wait(&(b->empty));
    
    clock_gettime(CLOCK_MONOTONIC, &(s->end));

    elapsed = (((s->end).tv_sec) - ((s->start).tv_sec));
    elapsed += (s->end.tv_nsec - s->start.tv_nsec) / 1000000000.0; 
    if(elapsed > 0.00053)
    {
        faildPackets++;
        sem_post(&(b->empty));
        free(e);
        return;
    }
    
    sem_wait(&(b->lock));

    STAILQ_INSERT_HEAD(b->head, e, entries);
    dataSent++;

    sem_post(&(b->lock));
    sem_post(&(b->full));
    s->sending = 1;
}

void SendData(Source *s, Buffer *b)
{
    if(s->sendsStream && s->last == NULL)
    {
        return SendFistStream(s, b);       
    }
    else if(s->sendsStream)
    {
        return SendStream(s, b);      
    }
    else
        return SendPacket(s, b);
}


void *SourceAction(void* s)
{
    Source* source = (Source*) s;
    printf("STARTING SOURCE ACTION id: %d \n", source->id);
    usleep(source->sourceBreak);
    while (RUN == 1)
    {
        source->bufferToSend = AskForBuffor(source);
        while(source->amountDataToSent >= 0)
        {

            usleep(source->sendingTime);
            SendData(source, source->bufferToSend);
            source->attemtedSends++;
            source->amountDataToSent--;
            usleep(source->timeBetweenSends);
        }

        usleep(source->sourceBreak);
        UpdateSource(source);
    }
}

char RecivePacket(Buffer* b, Cannal* c)
{
    // printf("Reciving packet or stream canal: %d", c->id);
    // printf(" from buffer %d \n", b->id);
    int semVal;
    entry* e = (entry*) malloc(sizeof(entry));
    
    sem_getvalue(&(b->full), &semVal);
    
    if(semVal != 0)
    {
    sem_wait(&(b->full));
    sem_wait(&(b->lock));
    
    e = STAILQ_FIRST(b->head);
    if(e != NULL)
    {
        if(sources[e->sourceID]->last == e && e != NULL)
        {
            sources[e->sourceID]->last = STAILQ_NEXT(e, entries);
        }
        STAILQ_REMOVE_HEAD(b->head, entries);
        recivedData++;
    }

    sem_post(&(b->lock));
    sem_post(&(b->empty));
    c->curSize++;
    }
    free(e);
    
}

void *CannalAction(void* c)
{
    Cannal* cannal = (Cannal*) c;
    Buffer* b = head;
    int semVal;
    while (RUN == 1)
    {
        int i = 0;
        while (cannal->curSize < packetLen)
        {
            RecivePacket(b, cannal);
            usleep(cannal->canalReciveTimeData);
            cannal->curSize++;
            sem_getvalue(&(b->full), &semVal);
            if(semVal == 0)
            {
                b = b->next;
                if(b==NULL)
                {
                    b = head;
                }
                sem_getvalue(&(b->full), &semVal);
            }
        }
        
        usleep(cannal->canalSendDataTime);
        cannal->curSize = 0;
    }
    
}

void InitSourceThreads()
{
    printf("Init sources Threds...\n");
    for(int i = 0; i < numberOfSources; i++)
    {
        pthread_create(&(sources[i]->tid), NULL, SourceAction, (void*) sources[i]);
        printf("SOURCE THREAD CREATED sourceid: %d \n", sources[i]->id);
    }
}

void InitCanalThreads()
{
    printf("Init cannals Threds...\n");
    for(int i = 0; i<numberOfComCanals; i++)
    {
        pthread_create(&(canals[i]->tid), NULL, CannalAction, (void*)canals[i]);
        printf("CANNAl THREAD CREATED canal id: %d \n", canals[i]->id);
    }
    
}



void Init()
{
    printf("Init Starting...\n");
    dataSent = 0;
    recivedData = 0;
    faildPackets = 0;
    faildStreams = 0;
    InitBuffers();
    packetBuffer = head;
    steamBufferStart = head->next;
    InitSources();
    InitCanals();
    InitSourceThreads();
    InitCanalThreads();
}

void JoinSourcers()
{
    for(int i = 0; i<numberOfSources; i++)
    {
        pthread_join(sources[i]->tid, NULL);
    }
}

void JoinCanals()
{
    for(int i = 0; i<numberOfComCanals; i++)
    {
        pthread_join(canals[i]->tid, NULL);
    }
}

void Join()
{
    JoinSourcers();
    JoinCanals(); 
}

void FreeResourses()
{
    FreeCanals();
    FreeSources();
    FreeBuffers();
}

void GatherStats()
{
    int totalPacktetSource = 0;
    int totalStreamSources = 0;
    long attemtedSents = 0;
    for(int i =0; i<numberOfSources; i++)
    {
        attemtedSents+= sources[i]->attemtedSends;

    }
    double attemsToTotal = (double) dataSent/attemtedSents * 100;
    printf("**********************************************************\n");
    printf("**********  Percentage: %f \n", attemsToTotal);
    printf("(taking into account all attems also the ones that hasn't be droped yet)\n");
    printf("**********  Total Attems: %d\n", attemtedSents);
    printf("**********  Data Sent: %d\n", dataSent);
    printf("**********  RecivedData: %d\n", recivedData);
    printf("**********  faildPackets: %d\n", faildPackets);
    printf("**********  faildStreams: %d\n", faildStreams);
    printf("**********  faildPackets: %f\n", (double)faildPackets/dataSent * 100);
    printf("**********  faildStreams: %f\n", (double)faildStreams/dataSent * 100);
    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    printf("**********************************************************\n");

}

void signalHandler(int sig)
{
    printf("INTERUPT SIGNAL\n");
    printf("WRAPING UP AND EXITING\n");
    RUN = 0;
    GatherStats();
    printf("Exiting ...\n");
    exit(0);
}

int main(int argc, char const *argv[])
{
    signal(SIGINT, signalHandler);
    RUN = 1;
    int end;
    Init();
    // PrintSources();
    // PrintBuffers();
    printf("PRESS CTRL+C to EXIT PROGRAM\n");
    Join();
    GatherStats();
    FreeResourses();
    printf("exit\n");
    return 0;
}
