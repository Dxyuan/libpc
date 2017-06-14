/*
 * FileName : exception_setenv.cpp
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-06-14
*/

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <queue>
#include <libpc/pc_routine.h>

const char *CGI_ENV_HOOK_LIST[] = 
{
	"CGINAME",
};

struct stRoutineArgs_t
{
	int iRoutineID;
};

void SetAndGetEnv(int iRoutineID)
{
	printf("routineid %d begin\n", iRoutineID);

	//use poll as sleep
	poll(NULL, 0, 500);

	char sBuf[128];
	sprintf(sBuf, "cgi_routine_%d", iRoutineID);
	int ret = setenv("CGINAME", sBuf, 1);
	if (ret) {
		printf("%s:%d set env err ret %d errno %d %s\n", __func__, __LINE__,
				ret, errno, strerror(errno));
		return;
	}
	printf("routineid %d set env CGINAME %s\n", iRoutineID, sBuf);

	poll(NULL, 0, 500);

	char *env = getenv("CGINAME");
	if (!env) {
		printf("%s:%d get env err errno %d %s\n", __func__, __LINE__,
				errno, strerror(errno));
		return;
	}
	printf("routineid %d get env CGINAME %s\n", iRoutineID, env);
}

void *RoutineFunc(void *args)
{
	pc_enable_hook_sys();

	stRoutineArgs_t *g = (stRoutineArgs_t *)args;
	SetAndGetEnv(g->iRoutineID);

	return NULL;
}

int main(int argc, char *argv[])
{
	pc_set_env_list(CGI_ENV_HOOK_LIST, sizeof(CGI_ENV_HOOK_LIST) / sizeof(char *));
	stRoutineArgs_t args[3];
	for (int i = 0; i < 3; i++) {
		stPcRoutine_t* pc = NULL;
		args[i].iRoutineID = i;
		pc_create(&pc, NULL, RoutineFunc, &args[i]);
		pc_resume(pc);
	}
	pc_eventloop(pc_get_epoll_ct(), NULL, NULL);

	return 0;
}
