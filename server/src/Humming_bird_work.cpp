#include "Humming_bird_work.h"

using namespace std;

int freeid = 0;
/* array gconn[] stores connection information, with mutex lock*/
struct conn gconn[CONN_MAX];
pthread_mutex_t conn_lock = PTHREAD_MUTEX_INITIALIZER;

/* struct len*/
int fileinfo_len = sizeof(struct fileInfo);
socklen_t sockaddr_len = sizeof(struct sockaddr);
int head_len = sizeof(struct head);
int conn_len = sizeof(struct conn);

/* Create file with size bytes*/
int createFile(char *fileName, int size) {
	int fd = open(fileName, O_RDWR | O_CREAT);
	fchmod(fd, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	lseek(fd, size-1, SEEK_SET);
	write(fd, "", 1);
	close(fd);
}

/* Initialize server, listen request, and return fd*/
int initServer(int port) {
	int listen_fd;
	struct sockaddr_in server_addr;
	if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		std::cout << "Creating server socket error..." << std::endl;
		exit(-1);
	}
	set_fd_nonblock(listen_fd);

	memset(&server_addr, 0, sockaddr_len);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listen_fd, (struct sockaddr*)&server_addr, sockaddr_len) == -1) {
		std::cout << strerror(errno) << " in initServer" << std::endl;
		exit(-1);
	}

	if (listen(listen_fd, LISTEN_QUEUE_LEN) == -1) {
		std::cout << strerror(errno) << " in initServer" << std::endl;
		exit(-1);
	}

	return listen_fd;
}

/* Set fd non-blocking*/
void set_fd_nonblock(int fd) {
	int flag = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flag | O_NONBLOCK);
	return;
}

/**
  * Receive file infomation, and then send used freeid during this file transfering period.
  * Insert connection to gconn[], create related file and map to memory. 
  */
void recv_finfo(int sockfd) {
	// Receiving file information
	char fileinfo_buf[100] = {0};
	memset(fileinfo_buf, 0, fileinfo_len);
	for (int i=0; i<fileinfo_len; ++i) {
		recv(sockfd, &fileinfo_buf[i], 1, 0);
	}

	struct fileInfo finfo;
	memcpy(&finfo, fileinfo_buf, fileinfo_len);

	/* Create filling file, and map to memory*/
	char filePath[100] = {0};
	strcpy(filePath, finfo.fileName);
	createFile(filePath, finfo.fileSize);
	int fd = 0;
	if ((fd = open(filePath, O_RDWR)) == -1) {
		std::cout << strerror(errno) << " in recv_finfo" << std::endl;
		exit(-1);
	}

	char *map = (char*)mmap(NULL, finfo.fileSize, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	close(fd);

	/* Insert connection to gconn[]*/
	pthread_mutex_lock(&conn_lock);

	while (gconn[freeid].used) {
		++freeid;
		freeid %= CONN_MAX;
	}

	memset(&gconn[freeid].fileName, 0, FILENAME_MAXLEN);
	gconn[freeid].info_fd = sockfd;
	strcpy(gconn[freeid].fileName, finfo.fileName);
	gconn[freeid].fileSize = finfo.fileSize;
	gconn[freeid].bs = finfo.bs;
	gconn[freeid].count = finfo.count;
	gconn[freeid].mbegin = map;
	gconn[freeid].recvCount = 0;
	gconn[freeid].used = 1;

	pthread_mutex_unlock(&conn_lock);

	std::cout << "Connect " << freeid << " invoke, start to receive file: " << gconn[freeid].fileName << std::endl;

	/**
	  * Send allocated freeid to client (the index of gconn[]),
	  * and then every block will carry this id to identify the connection
	  */
	char freeid_buf[INT_SIZE] = {0};
	memcpy(freeid_buf, &freeid, INT_SIZE);
	send(sockfd, freeid_buf, INT_SIZE, 0);

	return;
}

/* Receive file block*/
void recv_fdata(int sockfd) {
	/* Read block head information */
	int recv_size = 0;
	char head_buf[100] = {0};
	char *p = head_buf;
	while (1) {
		if (recv(sockfd, p, 1, 0) == 1) {
			++recv_size;
			if (recv_size == head_len)
				break;
			++p;
		}
	}

	struct head fhead;
	memcpy(&fhead, head_buf, head_len);
	std::cout << "head----id: " << fhead.id << " | offset: " << fhead.offset << " | bs: " << fhead.bs << std::endl;
	int recv_id = fhead.id;

	/* Calculate current block start address fp in the map */
	int recv_offset = fhead.offset;
	char *fp = gconn[recv_id].mbegin+recv_offset;

	/* Receive data, and write to map */
	int remain_size = fhead.bs;	// remaining size of data
	int size = 0;			// data size in one recv
	while (remain_size > 0) {
		if ((size = recv(sockfd, fp, RECVBUF_SIZE, 0)) > 0) {
			fp += size;
			remain_size -= size;
		} 
	}

	/* Increment recvCount, and check whether it is the last block. If not, synchronize map and file, and release gconn */
	pthread_mutex_lock(&conn_lock);
	gconn[recv_id].recvCount++;
	if (gconn[recv_id].recvCount == gconn[recv_id].count) {
		munmap((void*)gconn[recv_id].mbegin, gconn[recv_id].fileSize);
		
		close(gconn[recv_id].info_fd);
		cout << "Connect " << recv_id << " upload successful, file: " << gconn[recv_id].fileName << endl;
		/* reset gonn[recv_id], the file fucked up*/
		memset(&gconn[recv_id], 0, conn_len);
	}

	pthread_mutex_lock(&conn_lock);
	close(sockfd);
	return;
}

/* Thread function, choose specified function according type*/
void * worker(void *arg) {
	struct worker_args *pw = (struct worker_args*)arg;
	int conn_fd = pw->fd;

	char type_buf[INT_SIZE] = {0};
	char *p = type_buf;
	int recv_size = 0;
	while (1) {
		if (recv(conn_fd, p, 1, 0)	== 1) {
			++recv_size;
			if (recv_size == INT_SIZE)
				break;
			++p;
		}
	}

	int type = *((int *)type_buf);
	switch(type) {
		/* Receiving file information*/
		case 0:
			pw->recv_finfo(conn_fd);
			break;
		/* Receiving file block*/
		case 255:
			pw->recv_fdata(conn_fd);
			break;
		default:
			return NULL;
	}
	return NULL;
}