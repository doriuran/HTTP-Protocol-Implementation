#include "threadpool.h"

threadpool *create_threadpool(int num_threads_in_pool)
{
    if (num_threads_in_pool >= MAXT_IN_POOL)
    {
        perror("Number of threads is bigger then the max size (200)\n");
        return NULL;
    }
    threadpool *thpool = (threadpool *)malloc(sizeof(threadpool));
    if (thpool == NULL)
    {
        perror("Error:malloc\n");
        return NULL;
    }
    thpool->num_threads = num_threads_in_pool;
    thpool->qsize = 0;
    int i, cthread = 0;
    thpool->threads = malloc(thpool->num_threads * sizeof(thpool->threads));
    if (thpool == NULL)
    {
        perror("Error:malloc\n");
        return NULL;
    }
    thpool->qhead = NULL;
    thpool->qtail = NULL;

    int checkInit = 0;
    checkInit = pthread_mutex_init(&(thpool->qlock), NULL);
    if (checkInit != 0)
    {
        perror("error:pthread_mutex_init\n");
        return NULL;
    }
    checkInit = pthread_cond_init(&(thpool->q_empty), NULL);
    if (checkInit != 0)
    {
        perror("error:pthread_cond_init\n");
        return NULL;
    }
    checkInit = pthread_cond_init(&(thpool->q_not_empty), NULL);
    if (checkInit != 0)
    {
        perror("error:pthread_cond_init\n");
        return NULL;
    }
    thpool->dont_accept = 0;
    thpool->shutdown = 0;

    for (i = 0; i < thpool->num_threads; i++)
    {
        cthread = pthread_create(&(thpool->threads[i]), NULL, do_work, thpool);
        if (cthread != 0)
        {
            perror("error:pthread_create\n");
            return NULL;
        }
    }

    return thpool;
};

void dispatch(threadpool *from_me, dispatch_fn dispatch_to_here, void *arg)
{

    int temp = 0;
    temp = pthread_mutex_lock(&(from_me->qlock));
    if (temp != 0)
    {
        perror("ERROR:pthread_mutex_lock\n");
        return;
    }
    work_t *newNode = (work_t *)malloc(sizeof(work_t) * 1);
    if (newNode == NULL)
    {
        perror("error:malloc\n");
        return;
    }

    //init the new node

    if (from_me->dont_accept == 0)
    {
        newNode->routine = dispatch_to_here;
        newNode->arg = arg;
        newNode->next = NULL;
        if (from_me->qhead == NULL)
        {
            from_me->qhead = newNode;
            from_me->qtail = newNode;
        }
        else
        {
            from_me->qtail->next = newNode;
            from_me->qtail = newNode;
        }
        from_me->qsize++;
    }
    else
    {
        temp = pthread_mutex_unlock(&(from_me->qlock));
        if (temp != 0)
        {
            perror("ERROR:pthread_mutex_unlock\n");
            return;
        }
        destroy_threadpool(from_me);
    }

    temp = pthread_mutex_unlock(&(from_me->qlock));
    if (temp != 0)
    {
        perror("ERROR:pthread_mutex_unlock\n");
        return;
    }
    temp = pthread_cond_signal(&(from_me->q_not_empty));
    if (temp != 0)
    {
        perror("ERROR:pthread_cond_signal\n");
        return;
    }
}

void *do_work(void *p)
{
    int temp = 0; //for checking the fx
    threadpool *thpool = (threadpool *)p;
    while (1)
    {
        if (thpool->shutdown == 1)
        {
            temp = pthread_mutex_unlock(&(thpool->qlock));
            if (temp != 0)
            {
                perror("ERROR:pthread_exit\n");
                return NULL;
            }

            return NULL; //pthread_exit(&(thpool->qlock)); //maybe need return null
        }


        temp = pthread_mutex_lock(&(thpool->qlock));
        if (temp != 0)
        {
            perror("ERROR:pthread_mutex_unlock\n");
            return NULL;
        }
        
        if (thpool->shutdown == 1)
        {

            temp = pthread_mutex_unlock(&(thpool->qlock));
            if (temp != 0)
            {
                perror("ERROR:pthread_mutex_unlock\n");
                return NULL;
            }
            return NULL; //pthread_exit(&(thpool->qlock)); //maybe need return null
        }
        if (thpool->qsize == 0)
        {
            temp = pthread_cond_wait(&(thpool->q_not_empty), &(thpool->qlock));
            if (temp != 0)
            {
                perror("ERROR:pthread_cond_wait\n");
                return NULL;
            }
        }
        if (thpool->shutdown == 1)
        {
            temp = pthread_mutex_unlock(&(thpool->qlock));
            if (temp != 0)
            {
                perror("ERROR:pthread_exit\n");
                return NULL;
            }

            return NULL; //pthread_exit(&(thpool->qlock)); //maybe need return null
        }

        work_t *work = thpool->qhead;
        
        thpool->qhead = thpool->qhead->next;
        if (thpool->qsize == 1)
        {
            thpool->qhead = NULL;
            thpool->qtail = NULL;
        }   
        thpool->qsize--;
        
        if (thpool->qsize == 0 && thpool->dont_accept == 1)
        {
            temp = pthread_cond_signal(&(thpool->q_empty));
            if (temp != 0)
            {
                perror("ERROR:pthread_cond_signal\n");
                return NULL;
            }
        }
        temp = pthread_mutex_unlock(&(thpool->qlock));
        if (temp != 0)
        {
            perror("ERROR:pthread_mutex_unlock\n");
            return NULL;
        }
        work->routine(work->arg);
        free(work);
    }
}

void destroy_threadpool(threadpool *destroyme)
{
    int temp = 0; //for check the fx.

    temp = pthread_mutex_lock(&(destroyme->qlock));
    if (temp != 0)
    {
        perror("ERROR:pthread_mutex_lock\n");
        return;
    }

    destroyme->dont_accept = 1;
    //if (destroyme->qsize != 0)
    //{
        pthread_cond_wait(&(destroyme->q_empty), &(destroyme->qlock));
        if (temp != 0)
        {
            perror("ERROR:pthread_cond_wait\n");
            return;
        }
    //}
    
    destroyme->shutdown = 1;
    
    temp = pthread_mutex_unlock(&(destroyme->qlock));
    if (temp != 0)
    {
        perror("ERROR:pthread_mutex_lock\n");
        return;
    }

    temp = pthread_cond_broadcast(&(destroyme->q_not_empty));
    if (temp != 0)
    {
        perror("ERROR:pthread_mutex_lock\n");
        return;
    }
    for (int t = 0; t < destroyme->num_threads; t++)
    {
        temp = pthread_join(destroyme->threads[t], NULL);
        if (temp != 0)
        {
            perror("ERROR:pthread_join \n");
            return;
        }
    }


    pthread_cond_destroy(&destroyme->q_empty);
    pthread_cond_destroy(&destroyme->q_not_empty);
    pthread_mutex_destroy(&destroyme->qlock);

    free(destroyme->threads);
    free(destroyme);
}