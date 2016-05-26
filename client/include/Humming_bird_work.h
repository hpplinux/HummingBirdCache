#ifndef HUMMING_BIRD_WORK_H
#define HUMMING_BIRD_WORK_H

#include <iostream>

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <time.h>
#include <errno.h>

#define SERVER_IP "127.0.0.1"	// Server IP
#define PORT 10001		// Server port
#define THREAD_NUM 4		// thread pool size
#define FILENAME_MAXLEN 30	// file name max length
#define INT_SIZE 4		// int type size

#define SEND_SIZE 65536	// 64K

#define BLOCKSIZE 536870912	// 512M

/* File information*/
struct fileinfo {
	char fileName[FILENAME_MAXLEN];	// file name
	int fileSize;				// file size
	int count;				// block count
	int bs;					// standard block size
};

/* Block head information*/
struct head {
	char fileName[FILENAME_MAXLEN];	// file name
	int id;					// the file id that the block belong to, index of gconn[CONN_MAX]
	int offset;				// offset of the block of file
	int bs;					// block Actual size
};

/* Create file with size bytes*/
int createFile(char *fileName, int size);

/* Set file descriptor non-blocking*/
void set_fd_nonblocking(int fd);

/* Initialize client*/
int initClient(char *ip);

/* Send file info to server, and initialize block head info*/
/* last_bs == 0: all blocks are standard block; last_bs > 0: the last block isn't standard block, last_bs is the size of last block*/
void send_fileinfo(int sock_fd			// the socket to send file info
		, char *fileName 		// file name
		, struct stat* p_fstat		// file state struct
		, struct fileinfo *p_finfo		// used for initialized file information
		, int *p_last_bs);			// last_bs indicates whether the last block is a standard block, 0 yes, last block size otherwise.

/* Send file data block*/
void * send_filedata(void *args);

/* Creata file block head*/
struct head * new_fb_head(char * fileName, int freeid, int *offset);

#endif