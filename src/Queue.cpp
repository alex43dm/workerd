#include <iostream>

#include "Queue.h"

Queue::Queue()
{
	pthread_mutex_init(&mtx, NULL);
	pthread_cond_init(&not_empty, NULL);
	q = new std::queue<Message*>();
}

Queue::~Queue()
{
    pthread_cond_destroy(&not_empty);
	pthread_mutex_destroy(&mtx);
	delete q;
}


bool Queue::put( Message *pkt )
{
	pthread_mutex_lock(&mtx);
	q->push(pkt);
	pthread_cond_signal(&not_empty);
	pthread_mutex_unlock(&mtx);
	return true;
}

Message *Queue::get()
{
    Message *ret = NULL;

	pthread_mutex_lock(&mtx);
    if(q->empty())
    {
        pthread_cond_wait(&not_empty,&mtx);
    }
    else
    {
        ret = q->front();
        q->pop();
    }
	pthread_mutex_unlock(&mtx);

	return ret;
}

bool Queue::empty()
{
	return q->empty();
}

int Queue::size()
{
	return q->size();
}
