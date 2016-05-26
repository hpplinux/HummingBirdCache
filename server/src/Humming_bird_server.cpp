#include "dh_thread_pool.h"
#include "Humming_bird_work.h"

using namespace std;

int main(int argc, char ** argv) {
	int port = PORT;
	if (argc > 1)
		port = atoi(argv[1]);

	DhThreadPool * pool = new DhThreadPool(THREAD_NUM);
	pool->Activate();

	/* Init server, listen to request*/
	int listenfd = initServer(port);
	socklen_t sockaddr_len = sizeof(struct sockaddr);

	/* epoll*/
	static struct epoll_event ev, events[EPOLL_SIZE];
	int epfd = epoll_create(EPOLL_SIZE);
	ev.events = EPOLLIN;
	ev.data.fd = listenfd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);

	while (1) {
		int event_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);

		/* Accept connection, and choose a thread to handle it*/
		for (int i=0; i<EPOLL_SIZE; ++i) {
			if (events[i].data.fd == listenfd) {
				int connfd;
				struct sockaddr_in clientaddr;
				while ((connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &sockaddr_len)) > 0) {
					struct worker_args * p_args = (struct worker_args*)malloc(sizeof(struct worker_args));
					p_args->fd = connfd;
					p_args->recv_finfo = recv_finfo;
					p_args->recv_fdata = recv_fdata;
					pool->AddAsynTask(worker, (void*)p_args);
				}
			}
		}
	}
	return 0;
}