#ifndef HUMMING_BIRD_WORK_H
#define HUMMING_BIRD_WORK_H

#include <iostream>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/epoll.h>
#include <sys/mman.h>

#include "dh_thread_pool.h"

#define PORT 10001                  //监听端口
#define LISTEN_QUEUE_LEN 100        //listen队列长度
#define THREAD_NUM  8               //线程池大小
#define CONN_MAX  10                //支持最大连接数，一个连接包含多个socket连接（多线程）
#define EPOLL_SIZE  50              //epoll最大监听fd数量
#define FILENAME_MAXLEN   30        //文件名最大长度
#define INT_SIZE    4               //int类型长度

/*一次rece接收数据大小*/
#define RECVBUF_SIZE    65536       //64K

/* File infomation*/
struct fileInfo {
	char fileName[FILENAME_MAXLEN];	// file name
	int fileSize;	// file Size
	int count;	// block count
	int bs;		// standard block size
};

/* BLock head infomation*/
struct head {
	char fileName[FILENAME_MAXLEN];	// file name
	int id;		// the file ID that blcok belong to, the index of array gconn[CONN_MAX]
	int offset;	// the block offset of original file
	int bs;		// the actual size of block
};

/* Connection with specified client, each connection set up only once, and shared with multiple thread.*/
struct conn {
	int info_fd;	// the socket used for communication: receiving file info, etc
	char fileName[FILENAME_MAXLEN];
	int fileSize;
	int bs;		// block size
	int count;	// block count
	int recvCount;	// the block count the already received, when recvCount == count, file transfer done.
	char *mbegin;	// mmap start addr
	int used;	// used flag, 1 is used, 0 is unused
};

/* Thread args*/
struct worker_args
{
	int fd;		// the socket discriptor
	void (*recv_finfo)(int fd);
	void (*recv_fdata)(int fd);	
};

/* Create file with size bytes*/
int createFile(char *fileName, int size);

/* Initialize server, listen request, and return fd*/
int initServer(int port);

/* Set fd non-blocking*/
void set_fd_nonblock(int fd);

/* Receive file infomation, and then send used freeid during  this file transfering period*/
void recv_finfo(int sockfd);

/* Receive file block*/
void recv_fdata(int sockfd);

/* Thread function*/
void * worker(void *arg);

#endif