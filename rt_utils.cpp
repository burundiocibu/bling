#include <cstdio>
#include <iostream>

#include "rt_utils.hpp"

using namespace std;

RunTime::RunTime()
{
   if (!initialized)
   {
      initialized=true;
      gettimeofday(&tv0, NULL);
      // step(2140000); // to help test rollovers
   }
}

void RunTime::step(uint32_t t_ms)
{
   int32_t dt_ms = t_ms - RunTime::msec();
   int32_t dt_sec = dt_ms/1000;
   int32_t dt_usec = dt_ms*1000 - dt_sec*1000000;
   RunTime::tv0.tv_sec -= dt_sec;
   RunTime::tv0.tv_usec -= dt_usec;
   normalize(tv0);
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
   uint64_t us = dt_tv.tv_sec;
   us *= 1000000L;
   return  us + dt_tv.tv_usec;
}

void RunTime::tv(struct timeval& dt_tv)
{
   gettimeofday(&dt_tv, NULL);
   dt_tv.tv_sec -= tv0.tv_sec;
   dt_tv.tv_usec -= tv0.tv_usec;
   normalize(dt_tv);
}

void RunTime::normalize(struct timeval& tv)
{
   if (tv.tv_usec < 0)
   {
      tv.tv_sec -= 1;
      tv.tv_usec += 1000000;
   }
   else if (tv.tv_usec > 999999)
   {
      tv.tv_sec += 1;
      tv.tv_usec -= 1000000;
   }
}

struct timeval RunTime::tv0;
bool RunTime::initialized=false;


void dump(const void* buff, size_t len)
{
   for (int ret = 0; ret <len; ret++)
      printf("%.2X ", ((char*)buff)[ret]);
   puts("");
}

