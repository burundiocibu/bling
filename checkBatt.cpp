#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <vector>
#include <bcm2835.h>

#include <string.h>
#include "ensemble.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"
#include "Slave.hpp"
#include "nameList.hpp"

using namespace std;

ostream& operator<<(std::ostream& strm, const vector<Slave> sv)
{
   vector<Slave>::const_iterator i;
   for (i=sv.begin(); i != sv.end(); i++)
      strm << left << setw(3) << i->slave_no << " " << setw(3) << i->drill_id << " "
           << right << setw(3) << i->stateOfCharge << "%  "
           << left << i->student_name << endl;
}

int main ()
{
   vector<Slave> slaveList;
   for(int entry = 0; entry < nameList::numberEntries; entry++)
      slaveList.push_back(Slave(nameList::nameList[entry].circuitBoardNumber,
                                nameList::nameList[entry].hatNumber,
                                nameList::nameList[entry].drillId,
                                nameList::nameList[entry].name));

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


	vector<Slave> okList;
	ofstream okOutFile;
	const char okOutFileName[] = "ok.txt";
	vector<Slave> warnList;
	ofstream warnOutFile;
	const char warnOutFileName[] = "warn.txt";
	vector<Slave> errorList;
	ofstream errorOutFile;
	const char errorOutFileName[] = "error.txt";
	vector<Slave> unreachableList;
	ofstream unreachableOutFile;
	const char unreachableOutFileName[] = "unreachable.txt";

        vector<Slave>::iterator i;
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
