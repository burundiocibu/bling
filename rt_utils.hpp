#ifndef _RT_UTILS_HPP
#define _RT_UTILS_HPP

#include <cstdint>
#include <sys/time.h>

class RunTime
{
public:
   RunTime();

   // Updates the off set to match the provided time.
   static void step(uint32_t t_ms);

   // returns the time since start in seconds
   static float sec();
   // returns the time since start in miliseconds
   static uint32_t msec();
   // returns the time since start in microseconds
   static uint64_t usec();
   // returns the time since start in a timeval
   static void tv(struct timeval& dt_tv);
   // prints to stdout the time 
   static void puts();
   // keeps the tv_usec between 0..999999
   static void normalize(struct timeval& dt_tv);


private:
   static struct timeval tv0;
   static bool initialized;
};

// does a hex dump of the memory pointed to
void dump(const void* buff, size_t len);

// prints current time as HH:MM:SS.fff 
std::string timestamp(void);

#endif
