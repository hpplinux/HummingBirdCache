//#include "dh_thread_pool.h"
#include "Humming_bird_work.h"

using namespace std;

int port=PORT;	// default port
char * mbegin;	// map begin address

int main(int argc, char ** argv) {
	/* init client*/
	if (argc > 1)
		port = atoi(argv[1]);

	int info_fd = initClient(SERVER_IP);

	char fileName[FILENAME_MAXLEN] = {0};
	cout << "Input file name: ";
	cin >> fileName;
	int fd = 0;
	if ((fd = open(fileName, O_RDWR)) == -1) {
		cout << strerror(errno) << " in " << __FUNCTION__ << endl;
		exit(-1);
	}

	/* send file info*/
	struct stat filestate;
	fstat(fd, &filestate);
	int last_bs = 0;
	struct fileinfo finfo;
	send_fileinfo(info_fd, fileName, &filestate, &finfo, &last_bs);

	/* receive allocated freeid*/
	char id_buf[INT_SIZE] = {0};
	for (int i=0; i<INT_SIZE; ++i) {
		read(info_fd, &id_buf[i], 1);
	}
	int freeid = *((int*)id_buf);

	/* map the file, close fd*/
	mbegin = (char *)mmap(NULL, filestate.st_size, PROT_WRITE|PROT_READ, MAP_SHARED, fd , 0);
	close(info_fd);
	close(fd);

	/* add task to tpool*/
	//DhThreadPool * pool = new DhThreadPool(THREAD_NUM);
	//pool->Activate();

	int num = finfo.count, offset = 0;
	int head_len = sizeof(struct head);
	/* add task to thread queue*/
	int i = 0;
	pthread_t pid[num];
	memset(pid, 0, num*sizeof(pthread_t));
	/* file is able to divide to standard block*/
	if (last_bs == 0) {
		for(i=0; i<num; ++i) {
			struct head * p_fhead = new_fb_head(fileName, freeid, &offset);
			if (pthread_create(&pid[i], NULL, send_filedata, (void *)p_fhead) != 0) {
				cout << strerror(errno) << "in " << __FUNCTION__ << endl;
				exit(-1);
			}
			//pool->AddAsynTask(send_filedata, (void *)p_fhead);
		}
	} 
	/* file is unable to divide to standard block*/
	else {
		for(i=0; i<num-1; ++i) {
			struct head* p_fhead = new_fb_head(fileName, freeid, &offset);
			if (pthread_create(&pid[i], NULL, send_filedata, (void *)p_fhead) != 0) {
				cout << strerror(errno) << "in " << __FUNCTION__ << endl;
				exit(-1);
			}
			//pool->AddAsynTask(send_filedata, (void *)p_fhead);
		}
		/* the last block*/
		struct head * p_fhead = (struct head*)malloc(head_len);
		memset(p_fhead, 0, head_len);
		strcpy(p_fhead->fileName, fileName);
		p_fhead->id = freeid;
		p_fhead->offset = offset;
		p_fhead->bs = last_bs; // Actual block size
		if (pthread_create(&pid[i], NULL, send_filedata, (void *)p_fhead) != 0) {
			cout << strerror(errno) << "in " << __FUNCTION__ << endl;
			exit(-1);
		}
		//pool->AddAsynTask(send_filedata, (void *)p_fhead);
	}

	/* wait for every worker thread finish and exit normally*/
	for (int i=0; i<num; ++i) {
		pthread_join(pid[i], NULL);
	}

	//pool->Destroy();
	//delete pool;

	return 0;
}