#ifndef _LOCK_HPP
#define _LOCK_HPP
#include <cstring>
#include <sys/file.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>

class Lock
{
public:
   Lock()
   {
      fd = open("/tmp/nRF.lock", O_CREAT, 0644);
      int rc = flock(fd, LOCK_EX | LOCK_NB);
      if (rc<0)
      {
         std::cout << "Could not acquire lock. "
                   << strerror(errno) << std::endl;
         exit(0);
      }
   };

   ~Lock()
   {
      int rc = flock(fd, LOCK_UN | LOCK_NB);
      close(fd);
   };

private:
   int fd;
};

#endif
