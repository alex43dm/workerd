#ifndef QUEUE_H
#define QUEUE_H

#include <queue>

#include <pthread.h>

#include "Message.h"

class Queue
{
	public:
		Queue();
		virtual ~Queue();
		bool put( Message *pkt );
		Message * get();
		bool empty();
		int size();

	protected:
	private:
		pthread_mutex_t mtx;
		pthread_cond_t  not_empty;
		std::queue<Message*> *q;
};

#endif // QUEUE_H
