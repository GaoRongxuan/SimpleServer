#ifndef __THRMGR_H__
#define __THRMGR_H__

#include <pthread.h>
#include "common.h"

typedef enum {
	POOL_INVALID,
	POOL_VALID,
	POOL_EXIT
} pool_state_t;

typedef struct item_header_tag {
    struct item_header_tag *next;
} item_header_t;

typedef struct queue_header_tag {
    struct item_header_tag *head;
    struct item_header_tag *tail;
} queue_header_t;

typedef struct work_item_tag {
    struct work_item_tag *next;
    void *data;

} work_item_t;

typedef struct work_queue_tag {
    struct work_item_tag *head;
    struct work_item_tag *tail;
    
    int item_count;

} work_queue_t;

typedef struct threadpool_tag {
    struct threadpool_tag *next;

    pthread_mutex_t pool_mutex;
    pthread_cond_t pool_cond;

    int thr_max;
    int thr_idle_timeout;
    int state;

    int alive_count;
    int idle_count;

    void (*handler)(void *);

    struct work_queue_tag queue;

} threadpool_t;

typedef struct pools_tag {
    struct threadpool_tag *head;
    struct threadpool_tag *tail;

} pools_t;

void thrmgr_init();
threadpool_t *thrmgr_new(int max, int idle_timeout, void (*handler)(void *));
void thrmgr_destroy(threadpool_t *threadpool);
int thrmgr_dispatch(threadpool_t *threadpool, void *user_data);

#endif