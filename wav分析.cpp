#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <io.h>
#include <fcntl.h>
#include "stdlib.h"

#define BLOCKSIZE 1*1024*1024
int    readSize;
int8_t blockIn[BLOCKSIZE];

#pragma pack(1)
struct fourccValue{
    char    fourcc[4];
    int32_t size;
};

struct fmtHead{
    int16_t format;
    int16_t channel;
    int32_t samplerate;
    int32_t bytePerSec;
    int16_t bytePerSample;
    int16_t bitDepth;
};

int fmtProcess(int fd, int len, struct fmtHead* head)
{
    int ret = 0;
    int readSize = read(fd, head, 16);
    if (readSize) ret += readSize;
    printf("format:0x%04x %dch %dByte %dByte %dByte %dbit\n",
            head->format & 0xffff,
            head->channel,
            head->samplerate,
            head->bytePerSec,
            head->bytePerSample,
            head->bitDepth
            );
    if (len - 16 > 0) {
        readSize = read(fd, &blockIn, len - 16);
        if (readSize) ret += readSize;
    }
}

int dataProcess(int fd, int len, const struct fmtHead &head) {
    int byteToRead = len;
    int readSize = 0;
    int blockInSize = 60 * head.bytePerSec;
    char* blockIn = new char[blockInSize];
    while (byteToRead) {
        readSize = read(fd, blockIn, blockInSize);
        if (readSize > 0) {
            byteToRead -= readSize;
            printf("%d %d %d\n",fd, readSize, byteToRead);
        } else {
            delete[] blockIn;
            return len - byteToRead;
        }
        /*
        for (int i = 0; i < readSize; i++) {
            if (i % 16 == 0) {
                printf("\n0x%08x ", i);
            } else {
                printf(" ");
            }
            printf("%02x", blockIn[i] & 0xff);
        }
        printf("\n");
        */
    }
    delete[] blockIn;
    return len - byteToRead;
}

void displayFourcc(const fourccValue& fourcc)
{
    printf("[%c%c%c%c] %08x %10d\n",
           fourcc.fourcc[0],
           fourcc.fourcc[1],
           fourcc.fourcc[2],
           fourcc.fourcc[3],
           fourcc.size,
           fourcc.size);
}

int main(int argc, const char* argv[])
{
    char filename[1000] = "";
    if (argc > 2) {
        strcpy(filename, argv[1]);
    }

    int fd = open(filename, O_RDONLY|O_BINARY);
    if (fd < 0) {
        printf("fd error!");
        return -1;
    } else {
        printf("fd=%d %s\n", fd, filename);
    }

    struct fourccValue riff;
    char               wave[4];
    struct fmtHead     head;

    readSize = read(fd, &riff, 8);
    readSize = read(fd, &wave, 4);
    displayFourcc(riff);

    while (readSize > 0) {
        struct fourccValue packHead;
        readSize = read(fd, &packHead, 8);
        if (readSize > 0) {
            displayFourcc(packHead);
        } else {
            break;
        }

        switch (*(int*)&packHead.fourcc) {
        case 0x20746d66: //'f' 'm' 't' ' '
            fmtProcess(fd, packHead.size, &head);
            break;
        case 0x61746164: //'d' 'a' 't' 'a'
            dataProcess(fd, packHead.size, head);
            break;
        default:
            lseek(fd, packHead.size, SEEK_CUR);
            break;
        }
    }

    close(fd);
    return 0;
}
