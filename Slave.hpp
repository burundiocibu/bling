#ifndef _SLAVE_HPP
#define _SLAVE_HPP

#include <string>
#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"
#include "ensemble.hpp"

class Slave
{
	public:
		uint16_t slave_no;
		int hat_no;
		char drill_id[10];
		char student_name[120];

		unsigned tx_cnt;
		unsigned retry_cnt;
		unsigned tx_err;  // number of transmit errors
		unsigned nack_cnt;  // number of transmissions with ACK req set that don't get responses.
		unsigned no_resp; // number of ping responses we expected but didn't get
		unsigned tx_dt, rx_dt;  // time to complete tx/rx in usec
		uint32_t t_tx, t_rx; // time of most recent send and the time in the most recent received packet
		int major_version; 
		int minor_version;
		bool validRead;

		//uint8_t id;
		uint16_t vlevel;
		uint16_t stateOfCharge;

		// used for the tx & rx functions
		uint8_t buff[ensemble::message_size];

		Slave();
		Slave(uint16_t slave, int hatNumber, char* drillId, char *studentName);
		void checkBattStatus();
		bool readStatusSuccess();
		bool isActNow();
		bool isWarnNow();

		bool tx(); // Returns true if it was successfull
		void rx();
		std::string status() const;
};

#endif
