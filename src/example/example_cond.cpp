/*
 * FileName : example_cond.cpp
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-06-14
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <queue>
#include <libpc/pc_routine.h>

using namespace std;

struct stTask_t
{
	int id;
};

struct stEnv_t
{
	stPcCond_t *cond;
	queue<stTask_t *> task_queue;
};

void *Producer(void* args)
{
	pc_enable_hook_sys();
	stEnv_t *env=  (stEnv_t *)args;
	int id = 0;
	while (true) {
		stTask_t *task = (stTask_t *)calloc(1, sizeof(stTask_t));
		task->id = id++;
		env->task_queue.push(task);
		printf("%s:%d produce task %d\n", __func__, __LINE__, task->id);
		pc_cond_signal(env->cond);
		poll(NULL, 0, 1000);
	}
	return NULL;
}

void *Consumer(void *args)
{
	pc_enable_hook_sys();
	stEnv_t *env = (stEnv_t *)args;
	while (true) {
		if (env->task_queue.empty()) {
			pc_cond_timewait(env->cond, -1);
			continue;
		}
		stTask_t *task = env->task_queue.front();
		env->task_queue.pop();
		printf("%s:%d consume task %d\n", __func__, __LINE__, task->id);
		free(task);
	}
	return NULL;
}

int main()
{
	stEnv_t* env = new stEnv_t;
	env->cond = pc_cond_alloc();

	stPcRoutine_t* consumer_routine;
	pc_create(&consumer_routine, NULL, Consumer, env);
	pc_resume(consumer_routine);

	stPcRoutine_t* producer_routine;
	pc_create(&producer_routine, NULL, Producer, env);
	pc_resume(producer_routine);
	
	pc_eventloop(pc_get_epoll_ct(), NULL, NULL);

	return 0;
}
