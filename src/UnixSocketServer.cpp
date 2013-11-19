#include <unistd.h>
#include <stdlib.h>

#include "Log.h"
#include "UnixSocketServer.h"
#include "Queue.h"

UnixSocketServer::UnixSocketServer(const std::string &_path):
    queueConnLen(20),
    path(_path)
{
    socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if(socket_fd < 0)
    {
        printf("socket() failed\n");
        exit(-1);
    }

    memset(&address, 0, sizeof(struct sockaddr_un));

    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, path.size(), "%s", path.c_str());
    unlink(address.sun_path);

    if(bind(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0)
    {
        printf("bind() failed\n");
        exit(-1);
    }

    if(listen(socket_fd, queueConnLen) != 0)
    {
        printf("listen() failed\n");
        exit(-1);
    }
}

UnixSocketServer::~UnixSocketServer()
{
    close(socket_fd);
}

void UnixSocketServer::run()
{
    int fd;
    socklen_t address_length;
    Queue *q = new Queue();

    while((fd = accept(socket_fd, (struct sockaddr *) &address, &address_length)) > -1)
    {
        q->put(new Message(fd));
        Log::debug("new connection");
    }

    delete q;
}
