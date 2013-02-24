// A catch-all place for code to be shared between the rpi and mcu430

#include <cstdio>

#include "rt_utils.hpp"

RunTime::RunTime()
{
   if (!initialized)
   {
      initialized=true;
      gettimeofday(&tv0, NULL);
   }
}

float RunTime::sec()
{
   struct timeval dt_tv;
   tv(dt_tv);
   return dt_tv.tv_sec + dt_tv.tv_usec/1e6;
}

uint32_t RunTime::msec()
{
   struct timeval dt_tv;
   tv(dt_tv);
   return dt_tv.tv_sec*1000 + dt_tv.tv_usec/1000;
}

uint64_t RunTime::usec()
{
   struct timeval dt_tv;
   tv(dt_tv);
   return dt_tv.tv_sec*1000000 + dt_tv.tv_usec;
}

void RunTime::tv(struct timeval& dt_tv)
{
   gettimeofday(&dt_tv, NULL);
   dt_tv.tv_sec -= tv0.tv_sec;
   dt_tv.tv_usec -= tv0.tv_usec;
}

void RunTime::puts()
{
   printf("%.6f us", RunTime::sec());
}

struct timeval RunTime::tv0;
bool RunTime::initialized=false;



void dump(const void* buff, size_t len)
{
   for (int ret = 0; ret <len; ret++)
      printf("%.2X ", ((char*)buff)[ret]);
   puts("");
}
