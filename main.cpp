#include <pthread.h>
#include <bits/local_lim.h>
#include <unistd.h>

#include <Log.h>
#include <Config.h>
#include <Queue.h>
#include <DataBase.h>
#include <Worker.h>

#define MY_PRIORITY (49)
#define THREAD_STACK_SIZE PTHREAD_STACK_MIN + 10 * 1024

int main(int argc, char *argv[])
{
    std::string config = "/home/alex/Projects/workerd/config.xml";

    //open syslog
    Log(LOG_LOCAL0);

    Config *cfg = Config::Instance();
    cfg->LoadConfig(config);

    Log::info("use config: %s",config.c_str());

    DataBase *db1 = new DataBase();

    //make queue
    Queue *q = new Queue();

    //dbDatabase *db = new dbDatabase(dbDatabase::dbReadOnly);
    //db->open(cfg->dbpath_.c_str());
    /*
        struct sched_param param;
        param.sched_priority = MY_PRIORITY;
        if(sched_setscheduler(0, SCHED_FIFO, &param) == -1)
        {
            perror("sched_setscheduler failed");
            exit(-1);
        }
    */

    pthread_attr_t* attributes = (pthread_attr_t*) malloc(sizeof(pthread_attr_t));
    pthread_attr_init(attributes);
    pthread_attr_setstacksize(attributes, THREAD_STACK_SIZE);


    //threads
    int i;
    pthread_t *threads = new pthread_t[cfg->server_children_];

    for(i = 0; i < cfg->server_children_; i++)
    {
        Worker *w = new Worker(q, db1->pDatabase);

        if(pthread_create(&threads[i], attributes, &w->run, w))
        {
            Log::err("creating thread failed");
        }
    }

    pthread_attr_destroy(attributes);

    //main loop
    db1->gen(0, 1024);
    Message *m = new Message(1);
    int j = 0;
    while(1)
    {
        for(i = 0; i < cfg->server_children_; i++)
        {
            q->put(m);
        }

        printf("put\n");
        db1->gen(++j * 1024, 1024);
        sleep(1);
    }

    //main loop end
    for(i = 0; i < cfg->server_children_; i++)
    {
        pthread_join(threads[i], 0);
    }

    return 0;
}
