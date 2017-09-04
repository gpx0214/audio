#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <io.h>
#include <fcntl.h>
#include "stdlib.h"
#include "lame.h"

#define BLOCKSIZE 1152*2*2
int    readSize;
int8_t blockIn[BLOCKSIZE];

struct lame_global_struct* m_LameGlobalFlags = NULL;

#pragma pack(1)
struct fourccValue{
    unsigned char    fourcc[4];
    uint32_t size;
};

struct fmtHead{
    uint16_t format;
    uint16_t channel;
    uint32_t samplerate;
    uint32_t bytePerSec;
    uint16_t bytePerSample;
    uint16_t bitDepth;
};

int fmtProcess(int fd, int len, struct fmtHead* head)
{
    int ret = 0;
    int readSize = read(fd, head, 16);
    if (readSize) ret += readSize;
    printf("format:0x%04x %dch %dHz %dByte/s %dByte/Sample %dbit\n",
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
	
	m_LameGlobalFlags = lame_init();
    lame_set_in_samplerate(m_LameGlobalFlags, head->samplerate);
    lame_set_out_samplerate(m_LameGlobalFlags, head->samplerate);
    lame_set_num_channels(m_LameGlobalFlags, head->channel);
    lame_set_mode(m_LameGlobalFlags, STEREO);
    lame_set_brate(m_LameGlobalFlags, 128);

    char currentDate[5] = "2017";

    lame_set_write_id3tag_automatic(m_LameGlobalFlags, 1);

    char title[30]    = "";
    char artist[30]   = "";
    char album[30]    = "";

    id3tag_init(m_LameGlobalFlags);
    id3tag_space_v1(m_LameGlobalFlags);
    id3tag_set_artist(m_LameGlobalFlags, artist);
    id3tag_set_title(m_LameGlobalFlags, title);
    id3tag_set_album(m_LameGlobalFlags, album);
    id3tag_set_track(m_LameGlobalFlags, "1");
    id3tag_set_year(m_LameGlobalFlags, currentDate);
    id3tag_set_comment(m_LameGlobalFlags, "");
    id3tag_set_genre(m_LameGlobalFlags, "Unknown");

    lame_init_params(m_LameGlobalFlags);
	
	return ret;
}

int dataProcess(int fd, int len, const struct fmtHead &head) {
    int byteToRead = len;
    int readSize = 0;

	int sampleSize = lame_get_framesize(m_LameGlobalFlags);
	int frameSize = sampleSize * head.bitDepth /8 * head.channel;

    int blockInSize = 10000;//frameSize*4;
    char* blockIn = new char[blockInSize];
	
	unsigned char *MP3OutputBuffer = new unsigned char[1024*1024*1];
	
	int encode_bytes = 0;
	int mp3fd = open("1.mp3", O_WRONLY|O_BINARY|O_CREAT);
    while (byteToRead) {
        readSize = read(fd, blockIn, blockInSize);
        if (readSize > 0) {
            byteToRead -= readSize;
            //printf("%d %d %d\n",fd, readSize, byteToRead);
			encode_bytes = lame_encode_buffer_interleaved(m_LameGlobalFlags,
                                                          (short int*)blockIn,
                                                          readSize / (head.bitDepth /8 * head.channel),//sampleSize,
                                                          MP3OutputBuffer,
                                                          frameSize);
			printf("readSize %5d encode_bytes %5d\n", readSize, encode_bytes);
            if (encode_bytes > 0) write(mp3fd, MP3OutputBuffer, encode_bytes);														  
														  

        } else {
            delete[] blockIn;
			delete[] MP3OutputBuffer;
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
	close(mp3fd);
    delete[] blockIn;
    delete[] MP3OutputBuffer;
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
    char filename[1000] = "1.wav";
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
