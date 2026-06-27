#include "include/09pthread_sockect/pthread_sockect.h"

#include "include/09pthread_sockect/server.h"
#include "include/common.h"

void PthreadSocketTest() {
    std::cout << "PthreadSocketTest" << std::endl;
    s_ptr<PServers> servers = std::make_shared<PServers>();

    servers->listen();
}
