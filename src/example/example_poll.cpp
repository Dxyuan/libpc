/*
 * FileName : example_poll.cpp
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-06-14
*/

#include <libpc/pc_routine.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <stack>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <vector>
#include <set>
#include <unistd.h>

using namespace std;

struct task_t
{
	stPcRoutine_t *pc;
	int fd;
	struct sockaddr_in addr;
};

static int SetNonBlock(int iSock)
{
    int iFlags;

    iFlags = fcntl(iSock, F_GETFL, 0);
    iFlags |= O_NONBLOCK;
    iFlags |= O_NDELAY;
    int ret = fcntl(iSock, F_SETFL, iFlags);

    return ret;
}

static void SetAddr(const char *pszIP, const unsigned short shPort, struct sockaddr_in &addr)
{
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(shPort);
	int nIP = 0;
	if( !pszIP || '\0' == *pszIP   
	    || 0 == strcmp(pszIP,"0") || 0 == strcmp(pszIP,"0.0.0.0") 
		|| 0 == strcmp(pszIP,"*") 
	  ) {
		nIP = htonl(INADDR_ANY);
	} else {
		nIP = inet_addr(pszIP);
	}
	addr.sin_addr.s_addr = nIP;
}

static int CreateTcpSocket(const unsigned short shPort  = 0, const char *pszIP  = "*", bool bReuse  = false)
{
	int fd = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
	if (fd >= 0) {
		if (shPort != 0) {
			if (bReuse) {
				int nReuseAddr = 1;
				setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&nReuseAddr,sizeof(nReuseAddr));
			}
			struct sockaddr_in addr ;
			SetAddr(pszIP,shPort,addr);
			int ret = bind(fd,(struct sockaddr*)&addr,sizeof(addr));
			if (ret != 0) {
				close(fd);
				return -1;
			}
		}
	}
	return fd;
}

static void *poll_routine(void *arg)
{
	pc_enable_hook_sys();

	vector<task_t> &v = *(vector<task_t> *)arg;
	for (size_t i=0;i<v.size();i++) {
		int fd = CreateTcpSocket();
		SetNonBlock(fd);
		v[i].fd = fd;

		int ret = connect(fd,(struct sockaddr*)&v[i].addr,sizeof( v[i].addr )); 
		printf("pc %p connect i %ld ret %d errno %d (%s)\n",
			pc_self(),i,ret,errno,strerror(errno));
	}
	struct pollfd *pf = (struct pollfd *)calloc(1, sizeof(struct pollfd) * v.size() );

	for (size_t i = 0; i < v.size(); i++) {
		pf[i].fd = v[i].fd;
		pf[i].events = ( POLLOUT | POLLERR | POLLHUP );
	}
	set<int> setRaiseFds;
	size_t iWaitCnt = v.size();
	for (;;) {
		int ret = poll(pf, iWaitCnt, 1000 );
		printf("pc %p poll wait %ld ret %d\n",
				pc_self(), iWaitCnt, ret);
		for (int i = 0; i < ret; i++) {
			printf("pc %p fire fd %d revents 0x%X POLLOUT 0x%X POLLERR 0x%X POLLHUP 0x%X\n",
					pc_self(),
					pf[i].fd,
					pf[i].revents,
					POLLOUT,
					POLLERR,
					POLLHUP
					);
			setRaiseFds.insert( pf[i].fd );
		}
		if (setRaiseFds.size() == v.size()) {
			break;
		}
		if (ret <= 0) {
			break;
		}

		iWaitCnt = 0;
		for (size_t i = 0; i < v.size(); i++) {
			if (setRaiseFds.find( v[i].fd ) == setRaiseFds.end()) {
				pf[ iWaitCnt ].fd = v[i].fd;
				pf[ iWaitCnt ].events = ( POLLOUT | POLLERR | POLLHUP );
				++iWaitCnt;
			}
		}
	}
	for (size_t i = 0; i < v.size(); i++) {
		close( v[i].fd );
		v[i].fd = -1;
	}

	printf("pc %p task cnt %ld fire %ld\n",
			pc_self(), v.size(), setRaiseFds.size());
	return 0;
}

int main(int argc, char *argv[])
{
	vector<task_t> v;
	for(int i = 1; i < argc; i += 2) {
		task_t task = {0};
		SetAddr(argv[i], atoi(argv[i+1]), task.addr);
		v.push_back(task);
	}

//------------------------------------------------------------------------------------
	printf("--------------------- main -------------------\n");
	vector<task_t> v2 = v;
	poll_routine(&v2);
	printf("--------------------- routine -------------------\n");

	for(int i = 0; i < 10; i++) {
		stPcRoutine_t *pc = 0;
		vector<task_t> *v2 = new vector<task_t>();
		*v2 = v;
		pc_create(&pc, NULL, poll_routine, v2 );
		printf("routine i %d\n", i);
		pc_resume(pc);
	}
	pc_eventloop(pc_get_epoll_ct(), 0, 0);

	return 0;
}
