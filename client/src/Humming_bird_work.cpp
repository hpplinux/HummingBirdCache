#include "Humming_bird_work.h"

extern char * mbegin;
extern int port;

/* Some struct lengths*/
int fileinfo_len = sizeof(struct fileinfo);
socklen_t sockaddr_len = sizeof(struct sockaddr);
int head_len = sizeof(struct head);

/* Create file with size bytes*/
int createFile(char *fileName, int size) {
	int fd = open(fileName, O_RDWR | O_CREAT);
	fchmod(fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	lseek(fd, size-1, SEEK_SET);
	write(fd, "", 1);
	close(fd);
	return 0;
}

/* Set file descriptor non-blocking*/
void set_fd_nonblocking(int fd) {
	int flag = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flag | O_NONBLOCK);
	return;
}

/* Initialize client*/
int initClient(char *ip) {
	/* Create socket*/
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	/* init sockaddr_in struct*/
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sockaddr_len);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	inet_pton(AF_INET, ip, (struct sockaddr *)&server_addr.sin_addr.s_addr);

	/* connect to the server*/
	if (connect(sock_fd, (struct sockaddr*)&server_addr, sockaddr_len) < 0) {
		std::cout << strerror(errno) << " in init client" << std::endl;
		exit(-1);
	}
	return sock_fd;
}

/* Send file info to server, and initialize block head info*/
/* last_bs == 0: all blocks are standard block; flag > 0: the last block isn't standard block, last_bs is the size of last block*/
void send_fileinfo(int sock_fd			// the socket to send file info
		, char *fileName 		// file name
		, struct stat* p_fstat		// file state struct
		, struct fileinfo *p_finfo		// used for initialized file information
		, int *p_last_bs) {		// last_bs indicates whether the last block is a standard block, 0 yes, last block size otherwise.
	/* init fileinfo*/
	memset(p_finfo, 0, fileinfo_len);
	strcpy(p_finfo->fileName, fileName);
	p_finfo->fileSize = p_fstat->st_size;

	/* the last block may not be a standard block*/
	int count = p_fstat->st_size/BLOCKSIZE;
	if (p_fstat->st_size%BLOCKSIZE == 0) {
		p_finfo->count = count;
	} else {
		p_finfo->count = count+1;
		*p_last_bs = p_fstat->st_size - count*BLOCKSIZE;
	}
	p_finfo->bs = BLOCKSIZE;

	std::cout << "file info---file name: " << p_finfo->fileName  << " | fileSize : " << p_finfo->fileSize << " | count: " << p_finfo->count << " | bs: " << p_finfo->bs << std::endl; 

	/* send type and file information*/
	char send_buf[100] = {0};
	int type = 0;
	memcpy(send_buf, &type, INT_SIZE);
	memcpy(send_buf+INT_SIZE, p_finfo, fileinfo_len);
	send(sock_fd, send_buf, fileinfo_len+INT_SIZE, 0);
	return;
}

/* Send file data block*/
void * send_filedata(void *args) {
	struct head * p_fhead = (struct head*)args;
    	//std::cout << "data head ---- fileName: " << p_fhead->fileName << " | id: " << p_fhead->id << " | offset: " << p_fhead->offset << " | bs: " << p_fhead->bs << std::endl;
	int sock_fd = initClient(SERVER_IP);
	/* send type and block header*/
	char send_buf[100] = {0};
	memset(send_buf, 0, 100);
	int type = 255;
	memcpy(send_buf, &type, INT_SIZE);
	memcpy(send_buf+INT_SIZE, p_fhead, head_len);
	int sendsize = 0, len = head_len+INT_SIZE;
	char *p = send_buf;
	while(1) {
		if ((send(sock_fd, p, 1, 0) > 0)) {
			++sendsize;
			if (sendsize == len)
				break;
			++p;
		}
	}

	/* send data block*/
	std::cout << "send data block" << std::endl;
	int send_size = 0, num = p_fhead->bs/SEND_SIZE;
	char *fp = mbegin+p_fhead->offset;
	int remain_size = p_fhead->bs - num*SEND_SIZE;
	for (int i=0; i<num; ++i) {	
		if ((send_size = send(sock_fd, fp, SEND_SIZE, 0)) == SEND_SIZE) {
			fp += SEND_SIZE;
		} else {
			// TODO: send error here
		}
	}
	/* send remaining part*/
	while (remain_size > 0) {
		int tmp_send = send(sock_fd, fp, remain_size, 0);
		remain_size -= tmp_send;
	}

	close(sock_fd);
	/* because the file head malloc*/
	free(args);
	return NULL;
}

/* Creata file block head*/
struct head * new_fb_head(char * fileName, int freeid, int *offset) {
	struct head * p_fhead = (struct head*)malloc(head_len);
	memset(p_fhead, 0, head_len);
	strcpy(p_fhead->fileName, fileName);
	p_fhead->id = freeid;
	p_fhead->offset = *offset;
	p_fhead->bs = BLOCKSIZE;
	*offset += BLOCKSIZE;
	return p_fhead;
}