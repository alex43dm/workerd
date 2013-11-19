#ifndef UNIXSOCKETSERVER_H
#define UNIXSOCKETSERVER_H

#include <string>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>

class UnixSocketServer
{
    public:
        int queueConnLen;

        UnixSocketServer(const std::string &);
        virtual ~UnixSocketServer();
        void run();
    protected:
    private:
        std::string path;
        struct sockaddr_un address;
        int socket_fd;
};

#endif // UNIXSOCKETSERVER_H
