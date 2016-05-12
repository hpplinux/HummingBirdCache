#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>

#define MAX_EVENTS (0xff)
#define BUFFER_LENGTH (0xff)

using namespace std;

#include "../include/dh_thread_pool.h"

int fds[MAX_EVENTS] = {0};
int naccept = 0;
int setnonblocking(int fd);
int del_fd(int fd);
void* reads(void* arg);
void* writes(void* arg);
int do_use_fd();

int quit = 0;
DhThreadPool *pool;
time_t last = 0;

int main(int argc, char ** argv) {
	cout << __FILE__ << __LINE__ << __func__ << endl;
	pool = new DhThreadPool(10);
	pool->Activate();
	int retval = 0;
	do {
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0) {
			cout << strerror(errno) << endl;
			break;
		}

		cout << "fd = " << fd << endl;

		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(struct sockaddr_in));

		addr.sin_family = AF_INET;
		addr.sin_port = htons(atoi(argv[2]));
		retval = inet_pton(AF_INET, argv[1], (struct sockaddr *) &addr.sin_addr.s_addr);
		if (retval != 1) {
			cout << strerror(errno) << endl;
			break;
		}

		socklen_t addrlen = sizeof(struct sockaddr_in);
		retval = bind(fd, (struct sockaddr*) &addr, addrlen);
		if (retval < 0) {
			cout << strerror(errno) << endl;
			break;
		}

		int backlog = 5; // change with real demand
		retval = listen(fd, backlog);
		if (retval < 0) {
			cout << strerror(errno) << endl;
			break;
		}

		int efd = epoll_create(MAX_EVENTS);
		if (efd == -1) {
			cout << strerror(errno) << endl;
			break;
		}

		cout << "efd = " << efd << endl;

		struct epoll_event ev,  events[MAX_EVENTS];
		ev.events = EPOLLIN | EPOLLET; // epoll edge triggered
		ev.data.fd = fd; // bind & listen's fd
		retval = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev);
		if (retval == -1) {
			cout << strerror(errno) << endl;
			break;
		}

		int n = 0;
		struct  sockaddr_in peer_addr;
		socklen_t peer_addrlen = sizeof(struct sockaddr_in);
		memset(&peer_addr, 0, sizeof(sockaddr_in));

		while (1) {
			if (quit) {
				close(efd);
				close(fd);
				break;
			}

			int nfds = epoll_wait(efd, events, MAX_EVENTS, 1000);
			if (nfds == -1) {
				cout << strerror(errno) << endl;
				break;
			}

			for (n=0; n<nfds; ++n) {
				if (events[n].data.fd == fd) {
					int afd = accept(fd, (struct sockaddr*)&peer_addr, &peer_addrlen);
					if (afd == -1) {
						cout << strerror(errno) << endl;
						break;
					}

					cout << "accept: afd = " << afd << endl;
					// set non blocking
					retval = setnonblocking(afd);
					if (afd < 0) {
						break;
					}

					ev.events = EPOLLIN | EPOLLET; // epoll edge triggered
					ev.data.fd = afd;
					retval = epoll_ctl(efd, EPOLL_CTL_ADD, afd, &ev);
					if (retval == -1) {
						cout << strerror(errno) << endl;
						break;
					}
					fds[naccept++] = afd;
				} else {
					if (events[n].events & EPOLLHUP) {
						cout << "events["<< n << "].events = 0x" << hex << events[n].events << endl;
						cout << "Hang up happened on the associated file descriptor." << endl;
						retval = epoll_ctl(efd, EPOLL_CTL_DEL, events[n].data.fd, &events[n]);
						if (retval == -1) {
							cout << strerror(errno) << endl;
							break;
						}
						close(events[n].data.fd);
						del_fd(events[n].data.fd);
						continue;
					}

					if (events[n].events & EPOLLERR) {
						cout << "0x" << hex << events[n].events;
						cout << "Erorr condition happened on the associated file descriptor. epoll_wait(2) will always epoll_wait \
							for this event; it is not necessary to set it in events." << endl;
						retval = epoll_ctl(efd, EPOLL_CTL_DEL, events[n].data.fd, &events[n]);
						if (retval == -1) {
							cout << strerror(errno) << endl;
							break;
						}
						close(events[n].data.fd);
						continue;
					}

					if (events[n].events & EPOLLIN) {
						cout << "events[" << n << "].events = 0x" << hex << events[n].events << endl;
						cout << "The associated file is available for read(2) operations. " << endl;
						pool->AddAsynTask(&reads, &(events[n].data.fd));
						/*
						retval = reads(events[n].data.fd);
						if (retval < 0) {
							break;
						}
						*/
					}

					if (events[n].events & EPOLLOUT) {
						cout << "events[" << n << "].events = 0x" << hex << events[n].events << endl;
						cout << "The associated file is available for write(2) operations. " << endl;
						pool->AddAsynTask(&writes, &(events[n].data.fd));
						/*
						retval = writes(events[n].data.fd);
						if (retval < 0) {
							break;
						}
						*/
					}
				}
			}

			do_use_fd();
		}
	}while (0);
	return retval;
}

int do_use_fd() {
	cout << __FILE__ << ":" << __LINE__ << ":" << __func__ << endl;
	int retval = 0;
	do {
		time_t t;
		time(&t);
		if (t - last < 3) {
			break;
		}
		last = t;

		for (int n=0; n<naccept; ++n) {
			pool->AddAsynTask(writes, &(fds[n]));
			/*
			retval = writes(fds[n]);
			if (retval < 0) {
				break;
			}
			*/
		}
	} while (0);
	return retval;
}

int setnonblocking(int fd) {
	cout << __FILE__ << ":" << __LINE__ <<  ":" << __func__ << endl;
	int retval = 0;
	do {
		int flags = fcntl(fd, F_GETFL);
		if (flags == -1) {
			retval = flags;
			cout << strerror(errno) << endl;
			break;
		}
		flags |= O_NONBLOCK; // non block
		retval = fcntl(fd, F_SETFL, flags);
		if (retval == -1) {
			cout << strerror(errno) << endl;
			break;
		}
	} while(0);
	return retval;
}

int del_fd(int fd) {
	cout << __FILE__ << ":" << __LINE__ << ":" << __func__ << endl;
	int retval = 0;
	do {
		int n;
		for (int n=0; n<naccept; ++n) {
			if (fds[n] == fd) {
				cout << "Remove fd = " << fd << endl;
				fds[n] = 0;
				for (; n < naccept-1; ++n) {
					fds[n] = fds[n+1];
				}
				fds[n] = 0;
				naccept--;
			}
		}
	} while (0);
	return retval;
}

void* reads(void * arg) {
	cout << __FILE__ << ":" << __LINE__ << ":" << __func__ << endl;
	int * fdp = (int *) arg;
	int retval = 0;
	do {
		char buffer[BUFFER_LENGTH];
		memset(buffer, 0, sizeof(buffer));
		retval = read(*fdp, buffer, BUFFER_LENGTH);
		if (retval < 0) {
			cout << strerror(errno) << endl;
			break;
		}
		cout << "fd = " << *fdp << ", READ [" << buffer << "]" << endl;
	} while (0);
//	return retval;
}
/*
int reads(int fd) {
	cout << __FILE__ << ":" << __LINE__ << ":" << __func__ << endl;
	int retval = 0;
	do {
		char buffer[BUFFER_LENGTH];
		memset(buffer, 0, sizeof(buffer));
		retval = read(fd, buffer, BUFFER_LENGTH);
		if (retval < 0) {
			cout << strerror(errno) << endl;
			break;
		}
		cout << "fd = " << fd << ", READ [" << buffer << "]" << endl;
	} while (0);
	return retval;
}
*/
void* writes(void * arg) {
	cout << __FILE__ << ":" << __LINE__ << ":" << __func__ << endl;
	int * fdp = (int *) arg;
	int retval = 0;
	do {
		char buffer[BUFFER_LENGTH];
		memset(buffer, 0, BUFFER_LENGTH);
		sprintf(buffer, "send");
		retval = write(*fdp, buffer, BUFFER_LENGTH);
		if (retval < 0) {
			cout <<strerror(errno) << endl;
			break;
		}
		cout << "fd = " << *fdp << ", WRITE [" << buffer << "]" << endl;
	} while(0);
//	return retval;
}
/*
int writes(int fd) {
	cout << __FILE__ << ":" << __LINE__ << ":" << __func__ << endl;
	int retval = 0;
	do {
		char buffer[BUFFER_LENGTH];
		memset(buffer, 0, BUFFER_LENGTH);
		sprintf(buffer, "send");
		retval = write(fd, buffer, BUFFER_LENGTH);
		if (retval < 0) {
			cout <<strerror(errno) << endl;
			break;
		}
		cout << "fd = " << fd << ", WRITE [" << buffer << "]" << endl;
	} while(0);
	return retval;
}
*/