#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#define MAX_EVENTS (0xff)
#define IP_LENGTH (0xf)
#define BUFFER_LENGTH (0xffff)

using namespace std;

char host[1024];
char uri[1024] = "/";
int fds[MAX_EVENTS] = {0};
int nconnect = 0;
int quit = 0;
int count = 0;

int reads(int fd);
int writes(int fd, const char *uri, const char *host);
int setnonblocking(int fd);
int do_use_fd();

void usage(const char * argv0) {
	cout << __FILE__ << ":" << __LINE__ << ":" << __func__ <<endl;
	cout << argv0 << " 127.0.0.1 80 \"/\"" << endl;
}

int main(int argc, char **argv) {
	cout << __FILE__ << ":" << __LINE__ << ":" << __func__ <<endl;
	int retval = 0;
	do {
		if (argc < 3) {
			usage(argv[0]);
			break;
		}

		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0) {
			cout << strerror(errno) << endl;
			break;
		}

		// init host
		memset(host, 0, sizeof(host));
		memcpy(host, argv[1], sizeof(host) - 1);

		// init uri
		memset(uri, 0, sizeof(uri));
		memcpy(uri, argv[3], sizeof(uri) - 1);

		struct sockaddr_in addr;
		struct sockaddr_in local;
		memset(&addr, 0, sizeof(struct sockaddr_in));

		addr.sin_family = AF_INET;
		addr.sin_port = htons(atoi(argv[2]));

		struct hostent *ht;
		ht = gethostbyname(argv[1]);
		if (ht == NULL) {
			// print error followed by getthehostbyname
			herror("gethostbyname");
			break;
		}

		char tmp_addr[1024] = {0};
		// 
		socklen_t socklen = sizeof(tmp_addr);
		char *ri;

		// On success, inet_ntop() returns a non-null pointer to dst, NULL is returned if there was an error,
		// with errno set to indicate the error.
		// TODO:
		inet_ntop(AF_INET, (void *)*(ht->h_addr_list), tmp_addr, socklen);
		if (tmp_addr == NULL) {
			cout << strerror(errno) << endl;
			break;
		}

		retval = inet_pton(AF_INET, tmp_addr, (struct sockaddr *)&addr.sin_addr.s_addr);
		if (retval == 0) {
			cout << "0 is retruned if src does not contain a character string representing a valid network address in the \
				specified address family." << endl;
			break;
		} else if (retval == -1) {
			cout << strerror(errno) << endl;
			break;
		}

		socklen_t addrlen = sizeof(struct sockaddr_in);
		retval = connect(fd, (struct sockaddr *)&addr, addrlen);
		if (retval == -1) {
			cout << strerror(errno) <<endl;
			break;
		}

		fds[nconnect++] = fd;

		socklen_t locallen = sizeof(struct sockaddr_in);
		retval = getsockname(fd, (struct sockaddr *) &local, &locallen);

		if (retval == -1) {
			cout << strerror(errno) << endl;
			break;
		}

		char local_ip[IP_LENGTH];
		memset(local_ip, 0, sizeof(local_ip));
		// TODO: non-sense
		if (inet_ntop(AF_INET, (void *) &local.sin_addr.s_addr, local_ip, locallen) == NULL) {
			cout << strerror(errno) << endl;
			break;
		}

		// from...to...
		cout << "local " << local_ip << ":" << ntohs(local.sin_port) << ", remote " << tmp_addr << ":" << ntohs(addr.sin_port) << endl;

		// set non blocking
		retval = setnonblocking(fd);
		if (retval < 0) {
			break;
		}

		int efd = epoll_create(MAX_EVENTS);
		if (efd == -1) {
			cout <<	strerror(errno) << endl;
			break;
		}

		struct epoll_event ev, events[MAX_EVENTS];
		ev.events = EPOLLIN | EPOLLET; // epoll edge triggered
		ev.data.fd = fd;
		retval = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev);
		if (retval == -1) {
			cout << strerror(errno) << endl;
			break;
		}

		do_use_fd();

		int n = 0;
		while (1) {
			if (quit == 1) {
				break;
			}
			int nfds = epoll_wait(efd, events, MAX_EVENTS, 1000);
			if (nfds == -1) {
				cout << strerror(errno) << endl;
				break;
			}

			for (n = 0; n < nfds; ++n) {
				if (events[n].events & EPOLLHUP) {
					cout << "events["<< n << "].events = 0x" << hex << events[n].events << endl;
					cout << "Hang up happened on the associated file descriptor." << endl;
					retval = epoll_ctl(efd, EPOLL_CTL_DEL, events[n].data.fd, &events[n]);
					if (retval == -1) {
						cout << strerror(errno) << endl;
						break;
					}
					close(events[n].data.fd);
					quit = 1;
					break;
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
					quit = 1;
					break;
				}

				if (events[n].events & EPOLLIN) {
					cout << "events[" << n << "].events = 0x" << hex << events[n].events << endl;
					cout << "The associated file is available for read(2) operations. " << endl;
					retval = reads(events[n].data.fd);
					if (retval < 0) {
						break;
					}
				}
			}
		}
	} while (0);
	return retval;
}

int setnonblocking(int fd) {
	cout << __FILE__ << ":" << __LINE__ << ":" << __func__ << endl;
	int retval = 0;
	do {
		int flags = fcntl(fd, F_GETFL);
		if (flags == -1) {
			retval = flags;
			cout << strerror(errno) << endl;
			break;
		}
		flags |= O_NONBLOCK; // non block
		retval |= fcntl(fd, F_SETFL, flags);
		if (retval == -1) {
			cout << strerror(errno) << endl;
			break;
		}
	} while (0);
	return retval;
}

int reads(int fd) {
	cout << __FILE__ << ":" << __LINE__ << ":" << __func__ << endl;
	int retval = 0;
	do {
		char buffer[BUFFER_LENGTH];
		memset(buffer, 0, sizeof(buffer));
		retval = read(fd, buffer, BUFFER_LENGTH);
		if (retval < 0) {
			cout << strerror(errno) << endl;
			if (errno == EAGAIN) {
				cout << "count = " << count << endl;
				quit = 1;
			}
			break;
		}
		cout << "fd = " << fd << endl;
		cout << "---response begin---" << endl;
		cout << buffer << endl;
		cout << "---response end---" << endl;
	} while (0);
	count++;
	return retval;
}

int writes(int fd, const char* uri, const char *host) {
	cout << __FILE__ << ":" << __LINE__ << ":" << __func__ << endl;
	int retval = 0;
	do {
		char buffer[BUFFER_LENGTH];
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, 
/* -http protocol request message header START- */
"GET %s HTTP/1.1\r\n"
"User-Agent: Wget/1.12 (linux-gnu)\r\n"
"Accept: */*\r\n"
"Host: %s\r\n"
"Connection: Keep-Alive\r\n"
"\r\n"
/* -http protocol request message header ENDED- */
, uri, host);
		size_t length = strlen(buffer);
		retval = write(fd, buffer, length);
		if (retval < 0) {
			cout << strerror(errno) << endl;
			break;
		}
		cout << "fd = " << fd << endl;
		cout << "---request begin---" << endl;
		cout << buffer << endl;
		cout << "---request end---" << endl;
	} while(0);
	return retval;
}

int do_use_fd() {
	cout << __FILE__ << ":" << __LINE__ << ":" << __func__ << endl;
	int retval = 0;
	do {
		for (int n=0; n<nconnect; ++n) {
			retval = writes(fds[n], uri, host);
			if (retval < 0) {
				break;
			}
		}
	} while (0);
	return retval;
}