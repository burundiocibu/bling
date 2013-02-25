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

void RunTime::step(uint32_t t_ms)
{
   int32_t dt_ms = t_ms - RunTime::msec();
   int32_t dt_sec = dt_ms/1000;
   int32_t dt_usec = dt_ms*1000 - dt_sec*1000000;
   printf("dt: %d %d\n", dt_sec, dt_usec);
   RunTime::tv0.tv_sec -= dt_sec;
   RunTime::tv0.tv_usec -= dt_usec;
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
   if (dt_tv.tv_usec < 0)
   {
      dt_tv.tv_sec -= 1;
      dt_tv.tv_usec += 1000000;
   }
}

void RunTime::puts()
{
   printf("%.6f", RunTime::sec());
}

struct timeval RunTime::tv0;
bool RunTime::initialized=false;



void dump(const void* buff, size_t len)
{
   for (int ret = 0; ret <len; ret++)
      printf("%.2X ", ((char*)buff)[ret]);
   puts("");
}
