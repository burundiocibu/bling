#include "Effect.hpp"
#include "messages.hpp"
#include "avr_tlc5940.hpp"
#include "avr_rtc.hpp"


Effect::Effect(uint16_t slave)
{
   state = complete;
   slave_id = slave;
};

void Effect::init(uint8_t* buff)
{
   messages::decode_start_effect(buff, id, start_time, duration);
   dt = 0;
   prev_dt = 0;
   state = unstarted;
}


void Effect::e0()
{
   int v = 4096 - dt*4;
   if (v<0)
      v=0;
   for (unsigned ch=0; ch<12; ch++)
      avr_tlc5940::set_channel(ch, v);
}


// Ramp up to vmax and down to zero every cl ms
void Effect::e1()
{
   long cl = 3000; // length of cycle in ms
   long phi = 0; // offset into cycle
   long vmax = 4095; // intensity at peak
   long cl2 = cl >> 1; 

   long dtp = dt-phi;
   long cldt = dtp<=0 ? 0 : dtp % cl;

   int v;
   if (cldt < cl2)
      v = 2 * (vmax * cldt) / cl;
   else
      v = -2 * (vmax * cldt) / cl + 2*vmax;

   // red starts at ch 1, green starts at ch 0
   for (unsigned ch=1; ch<12; ch+=3)
      avr_tlc5940::set_channel(ch, v);

   phi = 1000;
   dtp = dt-phi;
   cldt = dtp<=0 ? 0 : dtp % cl;
   if (cldt < cl2)
      v = 2 * (vmax * cldt) / cl;
   else
      v = -2 * (vmax * cldt) / cl + 2*vmax;

   for (unsigned ch=2; ch<12; ch+=3)
      avr_tlc5940::set_channel(ch, v);

   phi = 2000;
   dtp = dt-phi;
   cldt = dtp<=0 ? 0 : dtp % cl;
   if (cldt < cl2)
      v = 2 * (vmax * cldt) / cl;
   else
      v = -2 * (vmax * cldt) / cl + 2*vmax;

   // red starts at ch 1, green starts at ch 0
   for (unsigned ch=0; ch<12; ch+=3)
      avr_tlc5940::set_channel(ch, v);
}


void Effect::execute()
{
   if (state==complete)
      return;

   dt = avr_rtc::t_ms - start_time;

   if (dt>int(duration) && state==started)
   {
      state=complete;
      for (unsigned ch=0; ch<14; ch++)
         avr_tlc5940::set_channel(ch, 0);
      return;
   }
   else if (dt>0 && dt<int(duration) && state==unstarted)
      state=started;

   // Don't really need to update the LEDs more often than 50 Hz
   if (prev_dt>0 && dt - prev_dt < 20)
      return;

   if (state==started)
      switch(id)
      {
         case 0: e0(); break;
         case 1: e1(); break;
      }
   prev_dt = dt;
}


void Effect::all_stop()
{
   state = complete;
   for (int ch=0; ch<14; ch++)
      avr_tlc5940::set_channel(ch, 0);
}
