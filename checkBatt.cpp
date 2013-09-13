#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <list>
#include <bcm2835.h>

#include <string.h>
#include "ensemble.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"
#include "Slave.hpp"
#include "nameList.hpp"

using namespace std;

void outputList(std::string text, std::string fileName, list<Slave>& slaveList);


const int MAX_RETRY_COUNT = 10;
const int NUM_MMC_MSGS = 30;

ostream& operator<<(std::ostream& strm, const list<Slave> sv)
{
	list<Slave>::const_iterator i;
	for (i=sv.begin(); i != sv.end(); i++)
		strm << left << setw(3) << i->slave_no << " " << setw(3) << i->drill_id << "  "
			<< setw(3) << i->missed_message_count << " "
			<< right << setw(3) << i->stateOfCharge << "%  "
			<< left << i->student_name << endl;
}

int main ()
{
	list<Slave> slaveList;
	for(int entry = 0; entry < nameList::numberEntries; entry++)
	{
		slaveList.push_back(Slave(nameList::nameList[entry].circuitBoardNumber,
					nameList::nameList[entry].hatNumber,
					nameList::nameList[entry].drillId,
					nameList::nameList[entry].name));
	}

	nRF24L01::channel = 2;
	memcpy(nRF24L01::master_addr,    ensemble::master_addr,   nRF24L01::addr_len);
	memcpy(nRF24L01::broadcast_addr, ensemble::slave_addr[0], nRF24L01::addr_len);
	memcpy(nRF24L01::slave_addr,     ensemble::slave_addr[2], nRF24L01::addr_len);

	nRF24L01::setup();
	if (!nRF24L01::configure_base())
	{
		cout << "Failed to find nRF24L01. Exiting." << endl;
		return -1;
	}
	nRF24L01::configure_PTX();
	nRF24L01::flush_tx();


	list<Slave> okList;
	std::string okOutFileName = "ok.txt";
	list<Slave> warnList;
	std::string warnOutFileName = "warn.txt";
	list<Slave> errorList;
	std::string errorOutFileName = "error.txt";
	std::string unreachableOutFileName = "unreachable.txt";
	list<Slave> unreachableList;

	// Do Initial read of all Slaves
	list<Slave>::iterator i;
	for(i=slaveList.begin(); i != slaveList.end(); i++)
	{
		Slave& slave(*i);
		if (slave.slave_no == 999)
			continue;
		slave.checkBattStatus();
		if(slave.readStatusSuccess())
		{
			if(slave.isActNow())
			{
				if(slave.stateOfCharge == 0)
				{
					/* 
					 * If got charge=0, try again assuming
					 * that there might be a read error
					 */
					unreachableList.push_back(*i);
				}
				errorList.push_back(slave);
			}
			else if(slave.isWarnNow())
			{
				warnList.push_back(slave);
			}
			else
			{
				okList.push_back(slave);
			}
		}
		else
		{
			unreachableList.push_back(slave);
		}
	}


	// For any Slaves not successfully read, try again
	int retry = 0;
	bool lastRead = false;
	while((unreachableList.size() > 0) && (retry < MAX_RETRY_COUNT))
	{

		// Make copy of unreachable list so not editing the list that am iterating
		list<Slave> copyUnreachable;
		list<Slave>::iterator i; 
		for(i=unreachableList.begin(); i != unreachableList.end(); ++i)
		{
			copyUnreachable.push_back(*i);
		}

		// Cycle through all unreachable slaves querying for battery
		for(i=copyUnreachable.begin(); i != copyUnreachable.end(); ++i)
		{
			Slave& slave(*i);
			slave.checkBattStatus();
			if(slave.readStatusSuccess())
			{
				if(slave.isActNow())
				{
					/* 
					 * Reached Slave and got low battery level.
					 * If charge=0, leave on unreachable list to try
					 * again in case it was an invalid read.
					 *
					 * If this is the last read, move to error list.
					 */
					if((slave.stateOfCharge != 0) || (lastRead == true))
					{
						unreachableList.remove(slave);
						errorList.push_back(slave);
					}
				}
				else if(slave.isWarnNow())
				{
					unreachableList.remove(slave);
					warnList.push_back(slave);
				}
				else
				{
					unreachableList.remove(slave);
					okList.push_back(slave);
				}
			}
		}
		lastRead = (retry == (MAX_RETRY_COUNT-1));
		retry++;
	}

	// Now test for missed messages
	list<Slave>::iterator iter;
	for(int i=0; i<NUM_MMC_MSGS; i++)
	{
		bcm2835_delay(5); // delay 5mS	
		for(iter=slaveList.begin(); iter != slaveList.end(); iter++)
		{
			Slave& slave(*iter);
			if (slave.slave_no == 999)
				continue;
			slave.sendAllStop();
		}
	}
	for(iter=slaveList.begin(); iter != slaveList.end(); iter++)
	{
		Slave& slave(*iter);
		if (slave.slave_no == 999)
			continue;
		slave.readMissedMsgCnt();
	}

	// Output list of Slaves that we failed to read
	outputList("Unreachable Slaves:", unreachableOutFileName, unreachableList);

	// Output list of Slaves with battery level requiring replacement
	outputList("Batteries Needing Replacement:", errorOutFileName, errorList);

	// Output list of Slaves with battery level low, but not critical
	outputList("Batteries low but not critical:", warnOutFileName, warnList);

	// Output list of OK Slaves
	outputList("OK Batteries:", okOutFileName, okList);

}

void outputList(std::string text, std::string fileName, list<Slave>& slaveList)
{
	ofstream OutFile;
	// Output list
	if(slaveList.size() > 0)
	{
		OutFile.open(fileName, ofstream::trunc);
		OutFile << endl << text << endl << slaveList;
		OutFile.close();
	}
	else
	{
		// remove any existing file
		remove(fileName.c_str());
	}
}
