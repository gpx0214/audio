#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <io.h>
#include <fcntl.h>
#include "stdlib.h"

#define BLOCKSIZE 10*1024*1024
int    readSize;
int8_t blockIn[BLOCKSIZE];

#pragma pack(1)
struct fourccValue{
    char    fourcc[4];
    uint32_t size;
};

struct flacBlockHead{
    uint8_t blockType;
    uint8_t size[3];
};

struct flacStreamInfo {
	uint8_t minBlockSize[2];
	uint8_t maxBlockSize[2];
	uint8_t minFrameSize[3];
	uint8_t maxFrameSize[3];
	uint8_t SampleRate[3];
	uint8_t totalSamples[5];
	uint8_t md5[16];
};

struct flacSeekPoint {
	uint8_t sampleNumber[8];
	uint8_t offsetByte[8];
	uint8_t samplesInFrame[2];
};

struct flacFrameHead{
	uint8_t head[4];
}
#pragma pack()

int bigFourccToInt(char* str)
{
    return (str[0] << 8*3) + (str[1] << 8*2)+ (str[2] << 8*1)+ str[3];
}

int64_t big8ByteToInt64(uint8_t* str)
{
    return ( (str[0] << 8*7) + (str[1] << 8*6)+ (str[2] << 8*5)+ (str[3] << 8*4)
         + (str[4] << 8*3) + (str[5] << 8*2)+ (str[6] << 8*1)+ (str[7]) );
}

void process0(int fd, int len)
{
	struct flacStreamInfo info;
    readSize = read(fd, &info, 34);
	printf("%dHz %dch %dbit\n",
	       (info.SampleRate[0] << 12) + (info.SampleRate[1] << 4) + (info.SampleRate[2] >> 4),
		   ((info.SampleRate[2] & 0x0f) >> 1) + 1,
		   ((info.SampleRate[2] & 0x01) << 4) + (info.totalSamples[0] >> 4)+1);

    printf("md5: ");
    for (int i = 0; i < 16; i++) {
		printf("%02x ", info.md5[i] & 0xff);
	}
	printf("\n");
    lseek(fd, len - 34, SEEK_CUR);
}

void process1(int fd, int len)
{
    readSize = read(fd, blockIn, len);

    for (int i = 0; i < readSize; i++) {
        if (i % 16 == 0) {
            printf("\n0x%08x ", i);
        } else {
            printf(" ");
        }
		printf("%02x", blockIn[i] & 0xff);
	}
	printf("\n");

}

void process2(int fd, int len)
{
    int byteToRead = len;
    char id[4];
    byteToRead -= read(fd, &id, 4);
    lseek(fd, byteToRead, SEEK_CUR);
}

void process3(int fd, int len)
{
    int byteToRead = len;
    struct flacSeekPoint seek;
    while (byteToRead) {
        byteToRead -= read(fd, &seek, 18);
        printf("%9I64d %9I64d %5d\n",
               big8ByteToInt64(seek.sampleNumber),
               big8ByteToInt64(seek.offsetByte),
               ((seek.samplesInFrame[0]) << 8) + seek.samplesInFrame[1]);
    }
}

void process4(int fd, int len)
{
    int      byteToRead = len;
    uint32_t vendor_length;
    uint32_t user_comment_list_length;
    char     vendor_string[65536];

    byteToRead -= read(fd, &vendor_length, 4);
    byteToRead -= read(fd, &vendor_string, vendor_length);

    printf("%u\n", vendor_length);
    for (int i = 0; i < vendor_length; i++) {
        printf("%c", vendor_string[i]);
    }
    printf("\n");

    byteToRead -= read(fd, &user_comment_list_length, 4);

    while (byteToRead) {

        byteToRead -= read(fd, &vendor_length, 4);
        byteToRead -= read(fd, &vendor_string, vendor_length);
        printf("[%5u]", vendor_length);
        if (vendor_length < 256)
        for (int i = 0; i < vendor_length; i++) {
            printf("%c", vendor_string[i]);
        }
        printf("\n");
    }
    lseek(fd, byteToRead, SEEK_CUR);
}

void process6(int fd, int len)
{
    //flac 0x06 picture
    char* picTypeStr[21] = {
        "Other",
        "32x32 pixels 'file icon' (PNG only)",
        "Other file icon",
        "Cover (front)",
        "Cover (back)",
        "Leaflet page",
        "Media (e.g. label side of CD)",
        "Lead artist/lead performer/soloist",
        "Artist/performer",
        "Conductor",
        "Band/Orchestra",
        "Composer",
        "Lyricist/text writer",
        "Recording Location",
        "During recording",
        "During performance",
        "Movie/video screen capture",
        "A bright coloured fish",
        "Illustration",
        "Band/artist logotype",
        "Publisher/Studio logotype"
    };

    int byteToRead = len;
    int readSize = 0;

    char picType[4];
    char strSize[4];
    int size;
    char str[256];
    char picWidth[4];
    char picHeight[4];
    char picColorDepth[4];
    char picColorIndex[4];
    char picLen[4];

    readSize = read(fd, picType, 4);
    byteToRead -= readSize;
    printf("picType[%4d]%s\n", bigFourccToInt(picType), picTypeStr[bigFourccToInt(picType)]);

    readSize = read(fd, strSize, 4);
    byteToRead -= readSize;
    size = bigFourccToInt(strSize);
    readSize = read(fd, str, size);
    byteToRead -= readSize;
    printf("[%4d]", bigFourccToInt(strSize));
    for (int i = 0; i < readSize; i++) {
        printf("%c", str[i]);
    }
    printf("\n");

    readSize = read(fd, strSize, 4);
    byteToRead -= readSize;
    size = bigFourccToInt(strSize);
    readSize = read(fd, str, size);
    byteToRead -= readSize;
    printf("[%4d]", bigFourccToInt(strSize));
    for (int i = 0; i < readSize; i++) {
        printf("%c", str[i]);
    }
    printf("\n");

    readSize = read(fd, picWidth, 4);
    byteToRead -= readSize;

    readSize = read(fd, picHeight, 4);
    byteToRead -= readSize;

    readSize = read(fd, picColorDepth, 4);
    byteToRead -= readSize;

    readSize = read(fd, picColorIndex, 4);
    byteToRead -= readSize;
    printf("%5d*%5dpx %5dbit %5d\n",
           bigFourccToInt(picWidth),
           bigFourccToInt(picHeight),
           bigFourccToInt(picColorDepth),
           bigFourccToInt(picColorIndex));

    readSize = read(fd, picLen, 4);
    byteToRead -= readSize;

    readSize = read(fd, blockIn, byteToRead);
    byteToRead -= readSize;
    /*
    for (int i = 0; i < 16; i++) {
        if (i % 16 == 0) {
            printf("\n0x%08x ", i);
        } else {
            printf(" ");
        }
		printf("%02x", blockIn[i] & 0xff);
	}
	printf("\n");
	*/
    lseek(fd, byteToRead, SEEK_CUR);
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
    char filename[1000] = "D:\\1.flac";
    if (argc > 1) {
        strcpy(filename, argv[1]);
    }

    int fd = open(filename, O_RDONLY|O_BINARY);
    if (fd < 0) {
        printf("fd error!");
        return -1;
    } else {
        printf("fd=%d %s\n", fd, filename);
    }

    char                 flac[4];
    struct flacBlockHead head;
	int flacBlockHeadSize = 0;

    readSize = read(fd, &flac, 4);

    if (*(int*)&flac == 0x43614c66) {
		printf("fLaC\n");
	} else {
		return 0;
	}

    while (true) {
        readSize = read(fd, &head, 4);
    	flacBlockHeadSize = (head.size[0] << 16) + (head.size[1] << 8) + head.size[2];
    	printf("type=%2d size=%9d\n", head.blockType & 0x7f, flacBlockHeadSize);

    	switch (head.blockType & 0x7f) {
    	case 0:
    		process0(fd, flacBlockHeadSize);
    		break;
    	case 1:
    		//process1(fd, flacBlockHeadSize);
    		lseek(fd, flacBlockHeadSize, SEEK_CUR);
    		break;
    	case 3:
    		process3(fd, flacBlockHeadSize);
    		break;
    	case 4:
    		process4(fd, flacBlockHeadSize);
    		break;
    	case 6:
    		process6(fd, flacBlockHeadSize);
    		break;
        default:
            lseek(fd, flacBlockHeadSize, SEEK_CUR);
    	}

    	if (head.blockType & 0x80) {
    		printf("Last-metadata-block\n");
    		break;
    	}
    }
/*
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
*/

    readSize = read(fd, blockIn, 256);
    for (int i = 0; i < readSize; i++) {
        if (i % 16 == 0) {
            printf("\n0x%08x ", i);
        } else {
            printf(" ");
        }
        printf("%02x", blockIn[i] & 0xff);
    }
    printf("\n");

    close(fd);
    return 0;
}
