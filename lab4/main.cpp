#include "dataTypes.hpp"

int RUN;

sharedmem* shmem;

Source* sources[numberOfSources];
Cannal* canals [numberOfComCanals];

// buffes are kept in double linked list
Buffer* head;
Buffer* tail;

Buffer* packetBuffer;
Buffer* steamBufferStart;

pid_t wpid;

Buffer* SteamBufforAlocation()
{
    Buffer* bufferWithMaxEmptyPlaces = steamBufferStart;
    Buffer* iterator = steamBufferStart;
    int maxEmpty = 0;
    int val;
    for(int i = indexStartStreamBuffers; i < buffersAmount; i++)
    {
        val = iterator->getCount();
        if(val > 400)
            return iterator;
        if(val > maxEmpty+nextBufferCutOff)
        {
            maxEmpty = 0;
            bufferWithMaxEmptyPlaces = iterator;
        }
    }
    return bufferWithMaxEmptyPlaces;
}

Buffer* AskForBuffor(Source *s)
{
    if(s->SendsStream())
    {
       return SteamBufforAlocation();
    }
    return packetBuffer;
} 

Buffer::Buffer()
{
    hasStreamData = false;
    sourceId = -1;
    count = 0;
    failed = 0;
    top = 0;
    sem_init(&s, SHARED, 1);
}

Buffer::Buffer(int id)
{

    this->id = id;
    hasStreamData = false;
    sourceId = -1;
    count = 0;
    failed = 0;
    top = 0;
    sem_init(&s, SHARED, 1);
}

Buffer::~Buffer()
{
    sem_destroy(&s);
}

void Buffer::enter()
{
    printf("enter \n");
    sem_wait(&s);
}

void Buffer::leave()
{
    printf("leave \n");
    sem_post(&s);
}

void Buffer::wait(Condition& cond)
{
    printf("wait \n");
    ++cond.waitingCount;
    leave();
    cond.wait();
}

void Buffer::signal(Condition& cond)
{
    printf("SIGNAL \n");
    if(cond.signal())
        enter();    
}

void Buffer::printBuffer()
{
    int val;
    sem_getvalue(&s, &val);
    printf("Buffer id: %d ", id);
    printf("Lock: %d ", val);
    printf("Count: %d \n", count);
}

int Buffer::getCount() 
{
    return count;
}

int Buffer::getId() 
{
    return id;    
}

void Buffer::SetId(int id)
{
    this->id = id;
}


void Buffer::PrintInside() 
{
    std::cout << "BUFFER INSIDE: \n";
    for(int i = 0; i < top; i++)
    {
        std::cout << (int) buf[i] << "\n";
    }
}

Source::Source(int id)
{
    srand((unsigned) id);
    int timeBetweenSends;
    int amountOfData;
    this->id = id;
    data = 101;
    sendsStream = rand() % 2;
    sourceBreak = ((rand() % 41) + 80)*1000;
    attemtedSends = 0;
    sendingTime = bufforSize;
    sending = 0;
    timeBetweenSends = 500;
    amountOfData = (rand() % 101);
    if(sendsStream)
    {
        timeBetweenSends = 100;
        amountOfData = (rand() % 471) + 30;
        data = id;
    }
    timeBetweenSends = timeBetweenSends;
    amountDataToSent = amountOfData;
    lastNotGood = true;
}

Source::~Source()
{

}

void Source::PrintSource()
{
        printf("Source %d ", id);
        printf(", %d", (int)sendsStream);
        printf(", %d", data);
        printf(", %d", sourceBreak);
        printf(", %d \n", timeBetweenSends);
}

int Source::Action(void* s, int i)
{
    Source* source = (Source*) s;
    printf("STARTING SOURCE ACTION id: %d \n", source->id);
    usleep(source->sourceBreak);
    while (shmem->RUN == 1)
    {
        printf("First %d \n", source->id);
        Buffer *bufferToSend = AskForBuffor(source);
        while(source->amountDataToSent >= 0)
        {
            printf("source\n");
            usleep(source->sendingTime);
            bufferToSend->SendData(source);
            printf("sent2\n");
            shmem->attemtedSentd[i]++;
            source->attemtedSends++;
            source->amountDataToSent--;
            usleep(source->timeBetweenSends);
        }

        usleep(source->sourceBreak);
        source->UpdateSource();
    }
    return source->attemtedSends;
}

void Source::UpdateSource()
{
    int timeBetweenSends;
    int amountOfData;
    srand(time(NULL)+id);
    sourceBreak = (rand() % 41) + 80;
    sending = 0;
    timeBetweenSends = 500;
    amountOfData = (rand() % 471) + 30;
    if(sendsStream)
    {
        timeBetweenSends = 100;
        amountOfData = (rand() % 101);
    }
    timeBetweenSends = timeBetweenSends;
    amountDataToSent = amountOfData;
}

bool Source::SendsStream() 
{
    return sendsStream;
}

void Source::startTiming() 
{
    clock_gettime(CLOCK_MONOTONIC, &(start));
}

double Source::stopTiming() 
{
    double elapsed = 0.0;
    clock_gettime(CLOCK_MONOTONIC, &(end));
    elapsed = end.tv_sec - start.tv_sec;
    elapsed += ((end.tv_nsec - start.tv_nsec) / 1000000000.0); 
    return elapsed;
}

char Source::getData() 
{
    sending = 1;
    return data;
}

int Source::getId()
{
    return id;
}

void Buffer::SendFistStream(Source *s) //used when in case of sending first stream data 
{
    s->startTiming();
    enter();
    // printf("Sending  first steam  source: %d", s->id);
    // printf(" to buffer %d \n", b->id);
    double elapsed = 0.0;

    if(count == bufforSize)
    {
        wait(full);    
    }
    elapsed = s->stopTiming();

    if(elapsed > 0.00013)
    {
        shmem->faildStreams++;
        leave();
        return;
    }

    buf[top] = s->getData();
    ++top;
    ++count;
    shmem->dataSent++;

    s->lastNotGood = false;
    leave();
}


void Buffer::SendStream(Source *s)
{
    s->startTiming();
    enter();
    // printf("Sending  steam  source: %d", s->id);
    // printf(" to buffer %d \n", b->id);
    double elapsed = 0.0;

    if(s->lastNotGood)
    {
        return SendFistStream(s);
    }

    if(count == bufforSize)
    {
        wait(full);    
    }
    elapsed = s->stopTiming();

    if(elapsed > 0.00013)
    {
        shmem->faildStreams++;
        leave();
        return;
    }

    buf[top] = s->getData();
    ++top;
    ++count;
    shmem->dataSent++;

    leave();
}


void Buffer::SendPacket(Source* s)
{
    s->startTiming();
    enter();           
    double elapsed = 0;

    if(count == bufforSize)
    {
        wait(full);    
    }
    elapsed = s->stopTiming();

    if(elapsed > 0.00053)
    {
        shmem->faildPackets++;
        leave();
        return;
    }
    
    buf[top] = s->getData();
    ++top;
    ++count;
    shmem->dataSent++;

    leave();
}

void Buffer::SendData(Source *s)
{
    if(s->SendsStream() && s->lastNotGood)
    {
       return SendFistStream(s);       
    }
    else if(s->SendsStream())
    {
        return SendStream(s);      
    }
    else
        return SendPacket(s);
}

char Buffer::RecivePacket() 
{
    int semVal;
    
    enter();
    if(count == 0)
    {
        leave();
        return '1';
    }
    char data = buf[top];
    --top;
    --count;
    shmem->recivedData++;

    if(count == bufforSize-1)
        signal(full);
    leave();
}


Cannal::Cannal(int id)
{
    this->id = id;
    this->curSize = 0;
    this->canalReciveTimeData = bufforSize;
    this->canalReciveTimeData = packetLen;
}

Cannal::~Cannal()
{
}

void Cannal::Action(void* c, int id) 
{
    Cannal* cannal = (Cannal*) c;
    Buffer* b = head;
    int val;
    printf("Cannal %d \n", cannal->id);
    while (shmem->RUN == 1)
    {
        int i = 0;
        while (cannal->curSize < packetLen)
        {
            b->RecivePacket();
            usleep(cannal->canalReciveTimeData);
            cannal->curSize++;
            val = b->getCount();
            if(val == 0)
            {
                b = b->next;
                if(b==NULL)
                {
                    b = head;
                }
                val = b->getCount();
            }
        }
        
        usleep(cannal->canalSendDataTime);
        cannal->curSize = 0;
    }
}



void InitSources()
{
    printf("Init Sources...\n");
    __time_t t;
    int i = 0;
    int timeBetweenSends;
    int amountOfData;
    for( ; i < numberOfSources; i++)
    {
        
        sources[i] = new Source(i);
    }
}

void FreeSources()
{
    int i = 0;
    for( ; i< numberOfSources; i++)
    {
        delete sources[i];
    }
}

void PrintSources()
{
    for(int i = 0; i < numberOfSources; i++)
    {
        sources[i]->PrintSource();
    }
}

void InitCanals()
{
    printf("Init Cannals...\n");
    for(int i = 0; i < numberOfComCanals; i++)
    {
        canals[i] = new Cannal(i);
    }
}

void FreeCanals()
{
    for(int i = 0; i<numberOfComCanals; i++)
    {
        delete (canals[i]);
    }
}

void PrintBuffers()
{
    int emptyCount;
    int fullCount;
    Buffer* temp_ptr = head;
    while (temp_ptr != NULL)
    {
        temp_ptr->printBuffer();
        printf("next: %d \n", temp_ptr->next);
        temp_ptr = temp_ptr->next;
    }
}

void InitBuffers()
{
    printf("Init Buffers...\n");
    Buffer* temp_ptr;
    
    head = &(shmem->buffers[0]);
    head->printBuffer();
    head->prev = NULL;
    temp_ptr = head;
    for (int i = 1; i < buffersAmount; i++)
    {
        temp_ptr->next = &(shmem->buffers[i]);
        temp_ptr->next->printBuffer();
        temp_ptr->next->prev = temp_ptr;
        temp_ptr = temp_ptr->next;
    }
    tail = temp_ptr;
    tail->next = NULL;
}

void FreeBuffers()
{
    Buffer* temp_ptr = head->next;
    while (temp_ptr != NULL)
    {  
        delete temp_ptr->prev;
    }
    delete tail;
}

void Init()
{
    printf("Init Starting...\n");
    shmem->dataSent = 0;
    shmem->recivedData = 0;
    shmem->faildPackets = 0;
    shmem->faildStreams = 0;
    InitBuffers();
    packetBuffer = head;
    steamBufferStart = head->next;
    InitSources();
    InitCanals();
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
        // attemtedSents+= sources[i]->attemtedSends;
        attemtedSents+= shmem->attemtedSentd[i];

    }
    double attemsToTotal = (double) shmem->dataSent/attemtedSents * 100;
    printf("**********************************************************\n");
    printf("**********  Percentage: %f \n", attemsToTotal);
    printf("(taking into account all attems also the ones that hasn't be droped yet)\n");
    printf("**********  Total Attems: %d\n", attemtedSents);
    printf("**********  Data Sent: %d\n", shmem->dataSent);
    printf("**********  RecivedData: %d\n", shmem->recivedData);
    printf("**********  faildPackets: %d\n", shmem->faildPackets);
    printf("**********  faildStreams: %d\n", shmem->faildStreams);
    printf("**********  faildPackets: %f\n", (double)shmem->faildPackets/shmem->dataSent * 100);
    printf("**********  faildStreams: %f\n", (double)shmem->faildStreams/shmem->dataSent * 100);
    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    printf("**********************************************************\n");

}

void signalHandler(int sig)
{
        printf("INTERUPT SIGNAL\n");
        printf("WRAPING UP AND EXITING\n");
        shmem->RUN = 0;
        GatherStats();
        //PrintBuffers();
        //PrintSources();
        printf("Exiting ...\n");
        exit(0);
    
}

void InitProceses()
{
    int status = 0;
    int x = 0;
    for(int i = 0; i < 103; i++)
    {
        wpid = fork();
        if(wpid == 0)
        {   
            if( i < 100)
            {
                std::cout << "SOURCE:  i: " << i << "\n";
                Source::Action((void*)sources[i], i);
            }
            else
            {
                std::cout << "CANNAL:  i: " << i << "\n";
                Cannal::Action((void*) canals[i-100], i);
            }
            //FreeResourses();
            exit(0);
        }
        else
        {
            std::cout << "Parrent: " << wpid << "\n";
            std::cout << "parent p; " << getpid() << "\n";
        }
        
    }
    if (wpid > 0)
    {
        signal(SIGINT, signalHandler);
        while ((wpid = wait(&status)) > 0);
        std::cout << " partent HEad" << head << "  i:" << x << "\n";
    }
}


int main(int argc, char const *argv[])
{
    boost::interprocess::mapped_region region(boost::interprocess::anonymous_shared_memory(sizeof(sharedmem)));
    shmem = new (region.get_address()) sharedmem();
    shmem->RUN = 1;
    int end;
    Init();
    InitProceses();
    //PrintSources();
    //PrintBuffers();
    printf("PRESS CTRL+C to EXIT PROGRAM\n");
    GatherStats();
    //FreeResourses();
    printf("exit\n");
    return 0;
}
