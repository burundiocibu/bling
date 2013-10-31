#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ncurses.h>

#include <bcm2835.h>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"

#include "Slave.hpp"
#include "ensemble.hpp"

extern RunTime runtime;


void checkBattStatus();
bool isActNow();
bool isWarnNow();
void printFatal();
void printWarn();

const int WARN_LEVEL = 50;
const int ERROR_LEVEL = 30;
const bool DEBUG_OUTPUT = false;


Slave::Slave()
{
   vcell = 0;
   soc = 0;
   version = "unk";
}

Slave::Slave(uint16_t slave, int hatNumber, const char* drillId, const char *studentName)
{
   slave_no = slave;
   hat_no = hatNumber;
   drill_id = drillId;
   student_name = studentName;
   vcell = 0;
   soc = 0;
   version = "unk";
}

bool Slave::operator==(const Slave &slave) const 
{ 
   return this->slave_no == slave.slave_no;
} 

bool Slave::operator<(const Slave& slave) const
{
   return this->drill_id < slave.drill_id;
}

void Slave::checkBattStatus()
{
   messages::encode_ping(buff);
   if(tx())
      rx_ping();
}

void Slave::readMissedMsgCnt()
{
   messages::encode_ping(buff);
   if(tx())
      rx_ping();
}

void Slave::sendAllStop()
{
   messages::encode_all_stop(buff);
   tx();
}

void Slave::sendReboot()
{
   messages::encode_reboot(buff);
   tx();
}

bool Slave::isActNow()
{
   return soc < ERROR_LEVEL;
}

bool Slave::isWarnNow()
{
   return soc < WARN_LEVEL;
}

bool Slave::readStatusSuccess()
{
   return validRead;
}

bool Slave::tx()
{
   validRead = false;
   using namespace nRF24L01;
   if (slave_no >= ensemble::num_slaves)
   {
      std::cout << "Skipped slave " << slave_no << std::endl;
      return validRead;
   }
   else
   {
      if (DEBUG_OUTPUT) std::cout << "Slave: " << slave_no << std::endl;
   }

   t_tx = runtime.msec();
   tx_cnt++;
   bool ack = true;
   write_tx_payload(buff, sizeof(buff), (const char*)ensemble::slave_addr[slave_no], ack);

   uint64_t t0=runtime.usec();

   uint8_t status;
   int j;
   for(j=0; j<100; j++)
   {
      status = read_reg(STATUS);
      if (status & STATUS_TX_DS)
         break;
      delay_us(5);
   }

   if (status & STATUS_MAX_RT)
   {
      nack_cnt++;
      write_reg(STATUS, STATUS_MAX_RT);
      // data doesn't automatically removed...
      flush_tx();
   }
   else if (status & STATUS_TX_DS)
   {
      tx_dt = runtime.usec() - t0;
      write_reg(STATUS, STATUS_TX_DS); //Clear the data sent notice
      validRead = true;
   }
   else
      tx_err++;

   uint8_t obs_tx = read_reg(OBSERVE_TX);
   retry_cnt += obs_tx & 0x0f;

   return validRead;
}


void Slave::rx_ping()
{
   using namespace nRF24L01;

   uint8_t pipe;
   char config = read_reg(CONFIG);
   config |= CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // should still be powered on
   delay_us(1000);  // Really should be 1.5 ms at least
   set_CE();

   uint64_t t0=runtime.usec();


   int i;
   for (i=0; i<100; i++)
   {
      if (read_reg(STATUS) & STATUS_RX_DR)
      {
         read_rx_payload((char*)buff, ensemble::message_size, pipe);
         write_reg(STATUS, STATUS_RX_DR); // clear data received bit
         break;
      }
      delay_us(200);
   }

   config &= ~CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // should still be powered on
   clear_CE();

   if (i==100)
   {
      validRead = false;
      vcell = 0;
      soc = 0;
      missed_message_count = 0;
      version = "unk";
      return;
   }

   uint16_t rx_slave_no;
   uint8_t rx_fc;
   int8_t major, minor;
   uint16_t rx_vcell, rx_soc, rx_mmc;
   messages::decode_status(buff, rx_slave_no, t_rx, major, minor,
                           rx_vcell, rx_soc, rx_mmc, rx_fc);

   vcell = 1e-3 * rx_vcell;
   soc = (int)((rx_soc >> 8) & 0xff);
   version = std::to_string(major) + "." + std::to_string(minor);
   missed_message_count = (int)rx_mmc;
}


std::string Slave::status() const
{
   if (tx_cnt == 0)
      return std::string();
   float nack_pct = 100.0 * nack_cnt/tx_cnt;
   char buff[160];
   sprintf(buff, "%3d  %9.3f  %5d  %5d   %5d  %5d(%7.3f)  %9.3f  %5d  %6d  ",
           slave_no, 0.001*t_tx, tx_dt, tx_cnt, retry_cnt, nack_cnt, nack_pct, 0.001*t_rx, rx_dt, no_resp);
   //	if (id==6)
   //		sprintf(buff+80, "%5d", vlevel);
   return std::string(buff);
}


std::ostream& operator<<(std::ostream& s, const Slave& slave)
{
   s << std::left << std::setw(3) << slave.slave_no
     << "  " << std::setw(4) << slave.drill_id;
   if (slave.vcell != 0 && slave.soc != 0)
      s << "  " << std::setfill('0') << std::setw(5) << slave.vcell << "V" << std::setfill(' ')
        << "  " << std::right << std::setw(3) << int(slave.soc) << "%"
        << "  " << slave.version;
   else
      s << "                   ";
   s << "  " << std::left << slave.student_name;
   return s;
}

