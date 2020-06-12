#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "thrmgr.h"

pools_t pools;

void thrmgr_init()
{
    pools.head = NULL;
    pools.tail = NULL;
}

static void add_node(queue_header_t *queue, item_header_t *item)
{
    item->next = NULL;

    if(!queue->head && !queue->tail)
    {
        queue->head = item;
        queue->tail = item;
        return;
    }

    // add node at tail
    queue->tail->next = item;
    
    queue->tail = item;
}

static int rm_node(queue_header_t *queue, item_header_t *item)
{
    if(!queue->head || !queue->tail)
    {
        LOG_ERR("internal err:queue dont have head or tail\n");
        return FAILURE;
    }

    if (queue->head == item)
    {
        queue->head = NULL;
        queue->tail = NULL;

        return SUCCESS;
    }

    item_header_t *pitem = queue->head;

    while(pitem->next)
    {
        if (pitem->next == item)
        {
            pitem->next = pitem->next->next;
            return SUCCESS;
        }
    }

    LOG_ERR("queue dont have this node\n");
    return FAILURE;
}

static void *pop_headnode(queue_header_t *queue)
{
    void *node = queue->head;

    if (NULL == node)
    {
        return NULL;
    }
    if (node == queue->tail)
    {
        queue->head = NULL;
        queue->tail = NULL;
    
        return node;
    }

    queue->head = queue->head->next;

    return node;
}

static void print_queue(queue_header_t *queue)
{
    if (NULL == queue->head)
    {
        return;
    }
    
    item_header_t *tmp = queue->head;
    int i = 0;
    while(tmp)
    {
        printf("[%d] -> ", i);
        tmp = tmp->next;
        i++;
    }
    printf("\n");
    return;
}

threadpool_t *thrmgr_new(int max, int idle_timeout, void (*handler)(void *))
{
    threadpool_t *threadpool = NULL;

    if (max < 0)
    {
        LOG_ERR("[%s]param invalid", __FUNCTION__);
        return NULL;
    }

    threadpool = (threadpool_t *)malloc(sizeof(threadpool_t));

    threadpool->thr_max = max;
    threadpool->thr_idle_timeout = idle_timeout;
    threadpool->idle_count = 0;
    threadpool->alive_count = 0;
    threadpool->handler = handler;
    
    threadpool->queue.head = NULL;
    threadpool->queue.tail = NULL;
    threadpool->queue.item_count = 0;

    if (pthread_mutex_init(&(threadpool->pool_mutex), NULL) != 0) 
    {
        LOG_ERR("init mutex failed.\n");
        free(threadpool);
        return NULL;
    }

    if (pthread_cond_init(&(threadpool->pool_cond), NULL) != 0)
    {
        LOG_ERR("init cond failed.\n");
        pthread_mutex_destroy(&(threadpool->pool_mutex));
        free(threadpool);
        return NULL;
    }

    add_node((queue_header_t *)&pools, (item_header_t *)threadpool);

    threadpool->state = POOL_VALID;

    return threadpool;
}

void thrmgr_destroy(threadpool_t *threadpool)
{
    if (!threadpool)
    {
        return;
    }

    if (pthread_mutex_lock(&threadpool->pool_mutex) != 0)
    {
        LOG_ERR("lock failed\n");
        return;
    }

    threadpool->state = POOL_EXIT;

    rm_node((queue_header_t *)&pools, (item_header_t *)threadpool);

    if (threadpool->alive_count > 0)
    {
        if (pthread_cond_broadcast(&(threadpool->pool_cond)) != 0)
        {
            pthread_mutex_unlock(&threadpool->pool_mutex);
            return;
        }
    }
	while (threadpool->alive_count > 0) {
		if (pthread_cond_wait(&threadpool->pool_cond, &threadpool->pool_mutex) != 0)
        {
			pthread_mutex_unlock(&threadpool->pool_mutex);
			return;
		}
	}

    if (pthread_mutex_unlock(&threadpool->pool_mutex) != 0)
    {
        LOG_ERR("unlock failed\n");
        return;
    }
    pthread_mutex_destroy(&threadpool->pool_mutex);
    pthread_cond_destroy(&threadpool->pool_cond);
    free(threadpool);
}

static void *worker(void *user_data)
{
    int ret = FAILURE;
    threadpool_t *threadpool = (threadpool_t *)user_data;
    work_item_t *node;
    struct timespec timeout;
    int must_exit = FALSE;

    timeout.tv_sec = time(NULL) + threadpool->thr_idle_timeout;
    timeout.tv_nsec = 0;

    while(1)
    {
		if (pthread_mutex_lock(&(threadpool->pool_mutex)) != 0) {
			LOG_ERR("lock failed\n");
            return NULL;
		}
        threadpool->idle_count++;
        print_queue((queue_header_t *)&(threadpool->queue));
        while( ((node = (work_item_t *)pop_headnode((queue_header_t *)&(threadpool->queue))) == NULL) &&
            (threadpool->state != POOL_EXIT) )
        {
            LOG_INFO("did not get job, wait signal\n");
            ret = pthread_cond_timedwait(&threadpool->pool_cond, &threadpool->pool_mutex, &timeout);
            if (ret == ETIMEDOUT)
            {
                LOG_INFO("idle too long time, exit\n");
                must_exit = TRUE;
                break;
            }
        }

        threadpool->idle_count--;
        if (threadpool->state == POOL_EXIT)
        {
            must_exit = TRUE;
        }

		if (pthread_mutex_unlock(&(threadpool->pool_mutex)) != 0) {
			LOG_ERR("lock failed\n");
            return NULL;
		}

        if (node != NULL)
        {
            LOG_INFO("GET JOB, DO JOB\n")
            threadpool->handler(node->data);
            free(node);
            node = NULL;
        }
        else if (TRUE == must_exit)
        {
            // 退出条件：1、超时时间内没有执行过任务；2、线程池关闭
            LOG_INFO("EXIT THREAD\n")
            break;
        }
    }
    
    if (pthread_mutex_lock(&(threadpool->pool_mutex)) != 0) {
        LOG_ERR("lock failed\n");
        return NULL;
    }

    threadpool->alive_count--;
    if (threadpool->alive_count == 0)
    {
        LOG_INFO("no alive thread, broadcast event\n")
        pthread_cond_broadcast(&threadpool->pool_cond);
    }

    if (pthread_mutex_unlock(&(threadpool->pool_mutex)) != 0) {
        LOG_ERR("lock failed\n");
        return NULL;
    }

}

int thrmgr_dispatch(threadpool_t *threadpool, void *user_data)
{
    pthread_t thr_id;

    if (NULL == threadpool || NULL == user_data)
    {
        LOG_ERR("param invalid\n");
        return FAILURE;
    }

    if (pthread_mutex_lock(&(threadpool->pool_mutex)) != 0)
    {
        LOG_ERR("lock failed\n");
        return FAILURE;
    }

    do {
        if (threadpool->state != POOL_VALID)
        {
            LOG_ERR("thr pool state is not valid\n");
            break;
        }

        work_item_t *job = (work_item_t *)malloc(sizeof(work_item_t));
        job->data = user_data;
        add_node((queue_header_t *)&(threadpool->queue), (item_header_t *)job);
        LOG_INFO("add new job\n");

        if ( threadpool->alive_count < threadpool->thr_max &&
            threadpool->idle_count == 0 )
        {
            LOG_INFO("create new thread.\n");
            if (pthread_create(&thr_id, NULL, worker, threadpool) != 0)
            {
                LOG_ERR("create thread failed\n");
            } else {
                threadpool->alive_count++;
            }
        }
        else
        {
            LOG_INFO("awake old thread.\n");
            // 没有新建线程，就发送一个信号，尝试唤醒一个闲置线程
            pthread_cond_signal(&(threadpool->pool_cond));
        }
        
    }while(0);

    if (pthread_mutex_unlock(&(threadpool->pool_mutex)) != 0)
    {
        LOG_ERR("unlock failed\n");
        return FAILURE;
    }

    return SUCCESS;
}

