#pragma once

#ifdef __APPLE__
#include <sys/_types/_ssize_t.h>
#endif

#include <sys/types.h>

#include <cstddef>

namespace tcp {
int createListener(int port, int backlog);
int acceptClient(int listenerFd);
}  // namespace tcp
