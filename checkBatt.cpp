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

const int MAX_RETRY_COUNT = 10;

ostream& operator<<(std::ostream& strm, const list<Slave> sv)
{
	list<Slave>::const_iterator i;
	for (i=sv.begin(); i != sv.end(); i++)
		strm << left << setw(3) << i->slave_no << " " << setw(3) << i->drill_id << " "
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
	ofstream okOutFile;
	const char okOutFileName[] = "ok.txt";
	list<Slave> warnList;
	ofstream warnOutFile;
	const char warnOutFileName[] = "warn.txt";
	list<Slave> errorList;
	ofstream errorOutFile;
	const char errorOutFileName[] = "error.txt";
	list<Slave> unreachableList;
	ofstream unreachableOutFile;
	const char unreachableOutFileName[] = "unreachable.txt";


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
					// assume read error for now
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
					// Don't move to error list if charge=0 and not last read
					if(slave.stateOfCharge != 0)
					{
						unreachableList.remove(slave);
						errorList.push_back(slave);
					}
					else if (lastRead == true)
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


	// Output list of Slaves that we failed to read
	if(unreachableList.size() > 0)
	{
		unreachableOutFile.open(unreachableOutFileName, ofstream::trunc);
		unreachableOutFile << endl << "Unreachable Slaves:" << endl
			<< unreachableList;
		unreachableOutFile.close();
	}
	else
	{
		// remove any existing file
		remove(unreachableOutFileName);
	}

	// Output list of Slaves with battery level requiring replacement
	if(errorList.size() > 0)
	{
		errorOutFile.open(errorOutFileName, ofstream::trunc);
		errorOutFile << endl << "Batteries Needing Replacement:" << endl
			<< errorList;
		errorOutFile.close();
		cout << endl << "Batteries Needing Replacement:" << endl
			<< errorList;
	}
	else
	{
		// remove any existing file
		remove(errorOutFileName);
	}

	// Output list of Slaves with battery level low, but not critical
	if(warnList.size() > 0)
	{
		warnOutFile.open(warnOutFileName, ofstream::trunc);
		warnOutFile << endl << "Batteries low but not critical:" << endl
			<< warnList;
		warnOutFile.close();
	}
	else
	{
		// remove any existing file
		remove(warnOutFileName);
	}

	// Output list of OK Slaves
	if(okList.size() > 0)
	{
		okOutFile.open(okOutFileName, ofstream::trunc);
		okOutFile << endl << "OK Batteries:" << endl
			<< okList;
		okOutFile.close();
	}
	else
	{
		// remove any existing file
		remove(okOutFileName);
	}

}
