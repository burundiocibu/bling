#include <iostream>
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
}

Slave::Slave(uint16_t slave, int hatNumber, char* drillId, char *studentName)
{
	slave_no = slave;
	hat_no = hatNumber;
	drill_id = drillId;
	student_name = studentName;
	vlevel = 0;
	stateOfCharge = 0;
}

int Slave::operator==(const Slave &slave) const 
{ 
	if(this->slave_no == slave.slave_no) return 1; 
	return 0; 
} 

void Slave::checkBattStatus()
{
	messages::encode_ping(buff);
	if(tx())
	{
		rxBatt(READ_BATT);
	}
	else
	{	
		// not successful read
		stateOfCharge = 0;
	}
}

void Slave::readMissedMsgCnt()
{
	messages::encode_ping(buff);
	if(tx())
	{
		rxBatt(READ_MMC);
	}
	else
	{	
		// not successful read
		missed_message_count = 0;
	}
}

void Slave::sendAllStop()
{
	messages::encode_all_stop(buff);
	if(tx())
	{
		rxStop();
	}
}

bool Slave::isActNow()
{
	return stateOfCharge < ERROR_LEVEL;
}

bool Slave::isWarnNow()
{
	return stateOfCharge < WARN_LEVEL;
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

	t_tx = runtime.msec();
	tx_cnt++;
	bool ack = true;
	write_tx_payload(buff, sizeof(buff), (const char*)ensemble::slave_addr[slave_no], ack);
	//write_tx_payload(buff, ensemble::message_size, (const char*)ensemble::slave_addr[m_Slave_no], ack);

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


void Slave::rxBatt(ReadType type)
{
	using namespace nRF24L01;

	uint8_t pipe;
	char config = read_reg(CONFIG);
	config |= CONFIG_PRIM_RX;
	write_reg(CONFIG, config); // should still be powered on
	delay_us(1000);  // Really should be 1.5 ms at least
	set_CE();

	uint64_t t0=runtime.usec();
	uint16_t soc, missedMessageCount, vcell;
	uint8_t freshness_count;
	int8_t majorVersion, minorVersion;


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

	if(DEBUG_OUTPUT)
	{
		for (int i = 0; i <ensemble::message_size; i++)
			printf("%.2X ", buff[i]);
	}

	messages::decode_status(buff, slave_no, t_rx, majorVersion, minorVersion,
			vcell, soc, missedMessageCount, freshness_count);

	switch(type)
	{
		case READ_BATT:
			vlevel = 1e-3 * vcell;
			major_version = (int)majorVersion;
			minor_version = (int)minorVersion;
			stateOfCharge = (int)((soc >> 8) & 0xff);
			break;
		case READ_MMC:
			missed_message_count = (int)missedMessageCount;
			break;
	}

	if(DEBUG_OUTPUT)
	{
		std::cout << std::endl << "Tx Read " << slave_no << ", t_rx=" << t_rx << ", Major= " << major_version << ", Minor=" << minor_version << ", stateOfCharge=" << stateOfCharge << ", vlevel=" << vlevel << std::endl << std::endl;
	}
}


void Slave::rxStop()
{
	using namespace nRF24L01;

	uint8_t pipe;
	char config = read_reg(CONFIG);
	config |= CONFIG_PRIM_RX;
	write_reg(CONFIG, config); // should still be powered on
	delay_us(1000);  // Really should be 1.5 ms at least
	set_CE();

	uint64_t t0=runtime.usec();
	uint16_t soc, missed_message_count, vcell;
	uint8_t freshness_count;
	int8_t majorVersion, minorVersion;

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

	if(DEBUG_OUTPUT)
	{
		for (int i = 0; i <ensemble::message_size; i++)
			printf("%.2X ", buff[i]);
	}

	messages::decode_all_stop(buff);
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
