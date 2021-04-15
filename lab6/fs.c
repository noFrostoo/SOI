#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#define BLOCK_SIZE 4096
#define MAGIC_NUMBER 0xf420f420
#define INODES_PER_BLOCK 32
#define maxFileName 116

typedef struct SuperBlock 
{
    uint32_t magicNumber;   // File system magic number
    uint32_t inodesPtr;
    uint32_t dataPtr;
    uint32_t inodeNum;
    uint32_t dataBlocksNum;
    uint32_t size;
    uint32_t curInodeUsed;
    uint32_t leftSpace;
} SuperBlock ;

typedef struct Inode 
{
    uint32_t valid;         // Whether or not inode is valid
    uint32_t size;          // Size of file
    uint32_t dataPtr;
    char filename[maxFileName];
} Inode; //sizeof = 128

typedef uint8_t BlockMap[BLOCK_SIZE];

typedef struct Data
{
    uint8_t data[BLOCK_SIZE-4];
    uint32_t nextDataPtr;
} Data;

typedef union Block
{
    SuperBlock super;
    BlockMap blockMap;
    Inode inodes[INODES_PER_BLOCK];
    Data data;
}Block;

FILE* vdisc;
Block superBlock;
Block map;

int printInfo();

int commitBlockMap()
{
    fseek(vdisc, BLOCK_SIZE, SEEK_SET);
    if(fwrite(&map, sizeof(Block), 1, vdisc) != 1)
    {
        printf("Failed commiting block map");
        exit(-1);
    }
    return 0;
}

int commitSuperBlock()
{
    fseek(vdisc, 0, SEEK_SET);
    if(fwrite(&superBlock, sizeof(Block), 1, vdisc) != 1)
    {
        printf("Failed commiting super block");
        exit(-1);
    }
    return 0;
}

int createDisc(size_t size)
{
    uint8_t buffer[BLOCK_SIZE];
    int numberBlocks = size/BLOCK_SIZE;
    if (numberBlocks > 4096)
    {
        printf("size to big\n");
        exit(-1);
    }
    // int numberIndoes = numberBlocks*0.1;
    // int inodeOffset = sizeof(Block)*2;
    // int blocksWithInnodes = ceil((double)numberIndoes/INODES_PER_BLOCK);
    // int dataOffset = sizeof(Block)*2+sizeof(Block)*blocksWithInnodes;
    // int dataBlocks = numberBlocks - blocksWithInnodes - 2;
    int blocksWithInnodes = numberBlocks*0.1;
    int inodeOffset = sizeof(Block)*2;
    int numberIndoes = blocksWithInnodes * INODES_PER_BLOCK;
    int dataOffset = sizeof(Block)*2+sizeof(Block)*blocksWithInnodes;
    int dataBlocks = numberBlocks - blocksWithInnodes - 2;
    printf("numberBlocks %i\n", numberBlocks);
    printf("numberIndoes %i\n", numberIndoes);
    printf("inodeOffset %i\n", inodeOffset);
    printf("dataOffset %i\n", dataOffset);
    printf("blocksWithInnodes %i\n", blocksWithInnodes);
    printf("dataBlocks %i\n", dataBlocks);
    Block super;
    Block inodes[blocksWithInnodes];
    Block data[dataBlocks];
    Block blockMap;
    super.super.inodeNum = numberIndoes;
    super.super.magicNumber = MAGIC_NUMBER;
    super.super.dataPtr = dataOffset;
    super.super.inodesPtr = inodeOffset;
    super.super.dataBlocksNum = dataBlocks;
    super.super.size = size;
    super.super.curInodeUsed = 0;
    super.super.leftSpace = dataBlocks*(BLOCK_SIZE-4);
    for(int i = 0; i < dataBlocks; i++)
    {
        data[i].data.nextDataPtr = -1;
        memset(data[i].data.data, 0, BLOCK_SIZE-4);
    }
    for(int i = 0; i < blocksWithInnodes; i++)
    {
        for(int j = 0; j<INODES_PER_BLOCK; j++)
        {
            inodes[i].inodes[j].dataPtr = -1;
            inodes[i].inodes[j].dataPtr = 0;
            inodes[i].inodes[j].filename[0] = '\0';
            inodes[i].inodes[j].valid = 0;
        }
    }
    FILE* disc = fopen("disc", "wb+");
    if(disc == NULL)
    {
        printf("Error Creating disc\n");
        return -1;
    }
    if(fseek(disc, size-1, SEEK_SET) != 0)
    {
        printf("Error seting size\n");
        return -2;
    }
    buffer[0]='1';
    if(fwrite(buffer, sizeof(uint8_t), 1, disc) != 1)
    {
        printf("Error seting size\n");
        return -2;
    }
    if(fseek(disc, 0, SEEK_SET) != 0)
    {
        printf("Error rewinding disc\n");
        return -3;
    }
    if(fwrite(&super, sizeof(Block), 1, disc) != 1)
    {
        printf("Error saving super block");
        return -4;
    }
    if(fwrite(&blockMap, sizeof(Block), 1, disc) != 1)
    {
        printf("Error saving blockmap block");
        return -4;
    }
    for(int i = 0; i < blocksWithInnodes; i++)
    {
        if(fwrite(&inodes[i], sizeof(Block), 1, disc) != 1)
        {
            printf("Error saving inode block %i", i);
            return -4;
        }
    }
    for(int i = 0; i < dataBlocks; i++)
    {
        if(fwrite(&data[i], sizeof(Block), 1, disc) != 1)
        {
            printf("Error saving data block %i", i);
            return -4;
        }
    }
    
    if(fclose(disc) != 0)
    {
        printf("Error cant not close disc");
        return -5;
    }
    printf("disc creatred\n");
}

int open()
{
    printf("opening disc\n");
    vdisc  = fopen("disc", "rb+");
    if(vdisc == NULL)
    {
        printf("Error opeing disc\n");
        return -1;
    }
    int read = fread(&superBlock, sizeof(Block), 1, vdisc);
    if(read != 1 )
    {
        printf("Error reading super block, %i", read);
        return -2;
    }
    if(superBlock.super.magicNumber != MAGIC_NUMBER) //magic number on the disc is diffent so we assume that the rest of the disc is propably corrupted as well
    {
        printf("disc is corupeter aboorting...\n");
        exit(-1); //we dont work with corrupted disc 
    }
    if(fread(&map, sizeof(Block), 1, vdisc) != 1)
    {
        printf("Error reading super block, %i", read);
        return -2;
    }
    printf("disc opened\n");
    //printInfo();
}

int close()
{
    commitBlockMap();
    commitSuperBlock();
    printf("trying to close disc \n");
    if(fclose(vdisc) != 0)
    {
        printf("Error cant not close disc");
        return -5;
    }
    vdisc = NULL;
    printf("disc closed \n");
    return 0;
}

int findFreeBlock()
{
    for(int i = 0; i< superBlock.super.dataBlocksNum; i++)
    {
        if(map.blockMap[i]==0)
        {
            return i;
        }
    }
    return -1;
}

int upload(const char* name)
{
    if(strlen(name) >= maxFileName || strlen(name) <= 0)
    {
        printf("name wrong len");
        return -1;
    }
    if(vdisc == NULL)
        open();
    printf("uploading...\n");
    //*checking name
    fseek(vdisc, superBlock.super.inodesPtr, SEEK_SET);
    uint32_t innodeAdrr = superBlock.super.inodesPtr;
    int i = 0;
    int j = 0;
    Block inodeBlock;
    while(i < superBlock.super.inodeNum)
    {

        if(fread(&inodeBlock, sizeof(Block), 1, vdisc) != 1)
        {
            printf("Error reading disc\n");
            return -2;
        }
        j = 0;
        while (j < INODES_PER_BLOCK && i < superBlock.super.inodeNum)
        {
            if(strcmp(inodeBlock.inodes[j].filename, name) == 0)
            {
                printf("not unique name\n");
                exit(-1);
            }
            ++j;
            ++i;
        }
        innodeAdrr += BLOCK_SIZE;
    }
    innodeAdrr -= BLOCK_SIZE; //it's always one block too far

    // * find free innode
    fseek(vdisc, superBlock.super.inodesPtr, SEEK_SET);
    innodeAdrr = superBlock.super.inodesPtr;
    i = 0;
    j = 0;
    int found = 0;
    while(i < superBlock.super.inodeNum)
    {

        if(fread(&inodeBlock, sizeof(Block), 1, vdisc) != 1)
        {
            printf("Error reading disc\n");
            return -2;
        }
        j = 0;
        while (j < INODES_PER_BLOCK && i < superBlock.super.inodeNum)
        {
            if(inodeBlock.inodes[j].valid == 0)
            {
                //found free innode
                inodeBlock.inodes[j].valid = 1;
                strcpy(inodeBlock.inodes[j].filename, name);
                i = superBlock.super.inodeNum;
                found = 1;
                break;
            }
            ++j;
            ++i;
        }
        innodeAdrr += BLOCK_SIZE;
    }
    innodeAdrr -= BLOCK_SIZE; //it's always one block too far

    if(found == 0)
    {
        printf("no left inodes aborting\n");
        exit(1);
    }
    superBlock.super.curInodeUsed += 1;

    //! open file for coping
    FILE *toCopy = fopen(name, "r");
    // * find size of the file to copy
    fseek(toCopy, 0, SEEK_END);
    int size = ftell(toCopy);
    if(size > superBlock.super.leftSpace)
    {
        printf("no left space for file aborting\n");
        exit(1);
    }
    superBlock.super.leftSpace -= size;
    printf("222\n");
    fseek(toCopy, 0, SEEK_SET); //return cursor to the beginning  
    inodeBlock.inodes[j].size = size;

    // * FIND FREE BLOCKS AND COPY INFOMATION
    uint8_t buffer[BLOCK_SIZE-4];
    memset(buffer, 0, BLOCK_SIZE-4);
    uint32_t lastBlockNumber = 1;
    uint32_t blockNumber;
    int sizeToCopy = (BLOCK_SIZE - 4);
    blockNumber = findFreeBlock(); 
    if(blockNumber == -1)
    {
        printf("no free blocks\n");
        return -1;
    }
    inodeBlock.inodes[j].dataPtr = superBlock.super.dataPtr+BLOCK_SIZE*blockNumber; 
    printf("data adr %i \n", inodeBlock.inodes[j].dataPtr);
    if(blockNumber == -1)
    {
        printf("no free blocks\n");
        return -1;
    }
    // Todo: add jumping to the next block data and set pointer 
    while (size>0)
    {
        if(size > (BLOCK_SIZE-4))
        {
            sizeToCopy = (BLOCK_SIZE - 4);
        }
        else
        {
            sizeToCopy = size;   
        }
        
        if(fread(&buffer, sizeToCopy, 1, toCopy) != 1)
        {
            printf("Error reading file fails\n");
            return -1;
        }
        uint32_t dataAdrr = superBlock.super.dataPtr+BLOCK_SIZE*blockNumber;
        fseek(vdisc, dataAdrr, SEEK_SET); 
        map.blockMap[blockNumber] = 1;
        lastBlockNumber = blockNumber;
        blockNumber = findFreeBlock(); //* find next block so we can set ptr
        if(blockNumber == -1)
        {
            printf("no free blocks\n");
            return -1;
        }
        size -= (BLOCK_SIZE-4);
        Block toSave;
        memcpy(toSave.data.data, buffer,BLOCK_SIZE-4);
        if(size > 0)
        {
            toSave.data.nextDataPtr = superBlock.super.dataPtr+BLOCK_SIZE*blockNumber;
        }else
        {
            toSave.data.nextDataPtr = -1;
        }
    
        if(fwrite(&toSave, sizeof(Block), 1, vdisc) != 1)
        { // saving data to the disc
            printf("writing to the disc failed\n");
            return -1;
        }

    }
    fclose(toCopy); //!closing file 
    //*hopefully data copied 
    //*save inode with data
    fseek(vdisc, innodeAdrr, SEEK_SET);
    fwrite(&inodeBlock, sizeof(Block), 1, vdisc);
    //todo copy block map to disc
    //everything copied i think
    commitBlockMap();
    printf("copied\n");
    close();
    return 0;
}

int download(const char* name)
{
    if(vdisc == NULL)
        open();
    printf("downloading...\n");
   //*find by name 
    fseek(vdisc, superBlock.super.inodesPtr, SEEK_SET);
    uint32_t innodeAdrr = superBlock.super.inodesPtr;
    uint32_t dataAdrr;
    Block inodeBlock;
    int i = 0;
    int j = 0;
    int found = 0;
    while(i < superBlock.super.inodeNum)
    {
        if(fread(&inodeBlock, sizeof(Block), 1, vdisc) != 1)
        {
            printf("Error reading disc\n");
            return -2;
        }
        j = 0;
        while (j < INODES_PER_BLOCK && i < superBlock.super.inodeNum)
        {
            if(strcmp(inodeBlock.inodes[j].filename, name) == 0)
            {
                //found correct inode
                found = 1;
                dataAdrr = inodeBlock.inodes[j].dataPtr;
                // printf("i %i j %i \n", i, j);
                i=superBlock.super.inodeNum;
                // printf("filename %s , %s\n", inodeBlock.inodes[j].filename, name);
                // printf("data adr %i \n", inodeBlock.inodes[j].dataPtr);
                // printf("found file\n");
                break;
            }
            ++j;
            ++i;
        }
        innodeAdrr += BLOCK_SIZE;
    }
    innodeAdrr -= BLOCK_SIZE; //it's always one block too far

    if(found == 0)
    {
        close();
        printf("file not found \n");
        exit(-1);
    }
    //*open new file
    FILE* fp = fopen(name, "wb+");

    //*copy file
    uint32_t size = inodeBlock.inodes[j].size;
    uint32_t sizeToCopy = 0;
    uint32_t buffer[BLOCK_SIZE-4];
    uint32_t nextDataAdrr;
    Block dataBlock; 
    uint32_t blockNumber;
    fseek(vdisc, dataAdrr, SEEK_SET);
    if(fread(&dataBlock, sizeof(Block), 1, vdisc) != 1)
    {
        printf("failded reading data while coping from disc\n");
        return -1;
    }
    while (1)
    {
        if( size > (BLOCK_SIZE - 4))
        {
            sizeToCopy = (BLOCK_SIZE-4);
        }
        else
        {
            sizeToCopy = size;
        }
        nextDataAdrr = dataBlock.data.nextDataPtr;
        memcpy(buffer, dataBlock.data.data, sizeToCopy);
        if(fwrite(buffer, sizeToCopy, 1, fp) != 1)
        {
            printf("failded saving new file\n");
            return -1;
        }             
        size -= (BLOCK_SIZE - 4);
        if(dataBlock.data.nextDataPtr == -1)
            break;      
        if(size < 0)
            break;
        dataAdrr = nextDataAdrr;
        fseek(vdisc, dataAdrr, SEEK_SET);
        if(fread(&dataBlock, sizeof(Block), 1, vdisc) != 1)
        {
            printf("failded reading data while coping from disc\n");
            return -1;
        }

    }
    fclose(fp);
    close();
}

int deleteFile(const char* name)
{
    if(vdisc == NULL)
        open();
    //*find by name 
    fseek(vdisc, superBlock.super.inodesPtr, SEEK_SET);
    uint32_t innodeAdrr = superBlock.super.inodesPtr;
    uint32_t dataAdrr;
    Block inodeBlock;
    int i = 0;
    int j = 0;
    int found = 0;
    while(i < superBlock.super.inodeNum)
    {

        if(fread(&inodeBlock, sizeof(Block), 1, vdisc) != 1)
        {
            printf("Error reading disc\n");
            return -2;
        }
        j = 0;
        while (j < INODES_PER_BLOCK && i < superBlock.super.inodeNum)
        {
            if(strcmp(inodeBlock.inodes[j].filename, name) == 0)
            {
                //found correct inode
                inodeBlock.inodes[j].valid = 0;
                strcpy(inodeBlock.inodes[j].filename, "\0");
                dataAdrr = inodeBlock.inodes[j].dataPtr;
                superBlock.super.leftSpace +=  inodeBlock.inodes[j].size;
                superBlock.super.curInodeUsed -= 1;
                inodeBlock.inodes[j].size = 0;
                inodeBlock.inodes[j].dataPtr = 0;
                i=superBlock.super.inodeNum;
                found = 1;
                break;
            }
            ++j;
            ++i;
        }
        innodeAdrr += BLOCK_SIZE;
    }
    innodeAdrr -= BLOCK_SIZE; //it's always one block too far
    if(found == 0)
    {
        printf("not such file \n");
        exit(1);
    }
    //* Deleting data from disc
    uint32_t nextDataAdrr;
    Block dataBlock; 
    uint32_t blockNumber;
    fseek(vdisc, dataAdrr, SEEK_SET);
    fread(&dataBlock, sizeof(Block), 1, vdisc);
    while (1)
    {
        nextDataAdrr = dataBlock.data.nextDataPtr;
        blockNumber = (dataAdrr - superBlock.super.dataPtr)/BLOCK_SIZE;
        
        map.blockMap[blockNumber] = 0; 
        
        dataBlock.data.nextDataPtr = -1;
        memset(dataBlock.data.data, 0, BLOCK_SIZE-4);
        fseek(vdisc, dataAdrr, SEEK_SET);
        if(fwrite(&dataBlock,sizeof(Block),1,vdisc)!=1)
        {
            printf("failded writing data while deleting\n");
            return -1;
        }
        if(nextDataAdrr == -1)
        {
            printf("next -1 brake\n");
            break;
        }
        dataAdrr = nextDataAdrr;
        fseek(vdisc, dataAdrr, SEEK_SET);
        if(fread(&dataBlock, sizeof(Block), 1, vdisc) != 1)
        {
            printf("failded reading data while deleting\n");
            return -1;
        }
    }
    //*save map to disc
    commitBlockMap();
    //*save inode block to disc
    fseek(vdisc, innodeAdrr, SEEK_SET);
    fwrite(&inodeBlock, sizeof(Block), 1, vdisc);
    close();
}

int destroyDisc()
{
    if(vdisc != NULL)
        close();
    if(remove("disc")==0)
    {
        printf("disc removed\n");
        return 0;
    }
    else
    {
        printf("Failed disc removing\n");
        perror(NULL);
        return -1;
    }
}

int ls()
{
    int opened = 0;
    if(vdisc == NULL)
    {
        opened = 1;
        open();
    }
    printf("printing catalog\n");
    //* reading catalog info
    if(fseek(vdisc, superBlock.super.inodesPtr, SEEK_SET) != 0)
    {
        perror("seeking failed, ");
        return -1;
    }
    uint32_t innodeAdrr = superBlock.super.inodesPtr;
    int i = 0;
    int j = 0;
    int filesCount = 0;
    Block inodeBlock;
    while(i < superBlock.super.inodeNum)
    {
        int read = fread(&inodeBlock, sizeof(Block), 1, vdisc);
        if(read != 1)
        {
            printf("Error reading disc %i %i\n", i, read);
            perror("printing info");
            return -2;
        }
        j = 0;
        while (j < INODES_PER_BLOCK && i < superBlock.super.inodeNum)
        {
            if(inodeBlock.inodes[j].valid == 1)  
            {
                ++filesCount;
                printf("Namefile %s, size %i, data adr %i \n", inodeBlock.inodes[j].filename, inodeBlock.inodes[j].size, inodeBlock.inodes[j].dataPtr);
            }
            ++j;
            ++i;
        }
        innodeAdrr += BLOCK_SIZE;
    }
    innodeAdrr -= BLOCK_SIZE; //it's always one block too far
    if (opened == 1)
    {
        close();
    }
    return filesCount;
}

int printInfo()
{
    if(vdisc == NULL)
        open();
    fseek(vdisc, 0, SEEK_SET); //rewind the disc
    Block super;
    int read = fread(&super, sizeof(Block), 1, vdisc);
    if(read != 1 )
    {
        printf("Error reading super block, %i", read);
        return -2;
    }
    printf("magic number %i\n", super.super.magicNumber);
    printf("data block Num %i\n", super.super.dataBlocksNum);
    printf("inode num %i\n", super.super.inodeNum);
    printf("size: %i \n", super.super.size);
    printf("inodes ptr %i\n", super.super.inodesPtr);
    printf("dataptr %i\n", super.super.dataPtr);
    printf("maginc block addr %i \n", 0);
    printf("block map addr %i \n", BLOCK_SIZE);
    printf("inodes addr %i \n", superBlock.super.inodesPtr);
    printf("data addr %i \n", superBlock.super.dataPtr);
    printf("space left %i \n", superBlock.super.leftSpace);
    printf("inodes in use %i\n", superBlock.super.curInodeUsed);
    int takenBlocksCount = 0;
    for(int i = 0; i < superBlock.super.dataBlocksNum ; i++)
    {
        if(map.blockMap[i] == 1)
            ++takenBlocksCount;
    }
    printf("Data blocks taken %i out of %i \n", takenBlocksCount, superBlock.super.dataBlocksNum);
    int filesCount = ls();
    printf("total files possible(inodes on the disc) %i \n", superBlock.super.inodeNum);
    printf("files count %i \n", filesCount);
    close();
}

int printBlocks()
{
    if(vdisc == NULL)
        open();
    fseek(vdisc, superBlock.super.dataPtr, SEEK_SET);
    Block block;
    for(int i = 0; i< superBlock.super.dataBlocksNum; i++)
    {
        fread(&block, sizeof(Block), 1, vdisc);
        printf("next data ptr %i \n", block.data.nextDataPtr);
    }
    close();
}

int pareseCommand(int argc, char const *argv[])
{
    size_t optind;
    for (optind = 1; optind < argc && argv[optind][0] == '-'; optind++) {
        switch (argv[optind][1]) {
            case 'i': printInfo(); break; //info
            case 'b': printBlocks(); break; //print blocks
            case 'l': ls(); break; //ls
            case 'd': download(argv[optind+1]); break; //download
            case 'u': upload(argv[optind+1]); break; //upload
            case 'c': createDisc(atoll(argv[optind+1])); break; //create disc
            case 'r': deleteFile(argv[optind+1]); break; //remove file
            case 't': destroyDisc(); break; //remove disc
            case 'h': printf("Help: \n i print info,\n b print blocs, \n l print catalog ls, d fileName download file, \n u fileName upload file, \n c size crate disc, \n r fileName remove file, \n t remove disc\n"); break; //h help 
            default:
                fprintf(stderr, "Usage: %s [-ilduc] [-h] [fileName]\n", argv[0]);
                exit(0);
            }   
    }  
}

int main(int argc, char const *argv[])
{
    pareseCommand(argc, argv);
    return 0;
}