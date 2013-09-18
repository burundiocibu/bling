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
const int NUM_MMC_MSGS = 5;

ostream& operator<<(std::ostream& strm, const list<Slave> sv)
{
	list<Slave>::const_iterator i;
	for (i=sv.begin(); i != sv.end(); i++)
		strm << left << setw(3) << i->slave_no << " " << setw(3) << i->drill_id << "  "
			<< setw(3) << i->missed_message_count << " "
			<< right << setw(3) << i->stateOfCharge << "%  "
			<< left << i->student_name << endl;
}

nameList::NameHatInfo testJonNameList[] =
{
	"Hingle McCringleberry", 207, 7, "F19",
	"Ebreham Moisesus", 208, 8, "F8",
	"Bob Mellon",    202, 2, "F1",
	"Rob Burgundy",  205, 5, "F5",
	"006",           206, 6, "F10",
	"OneThirtyNine", 239, 139, "F6",
	"OneThirty",     230, 130, "F2",
	"EightyThree",   283, 83, "F3",
	"TwentyNine",    229, 29,  "F4",
	"OneTwentySix",  226, 126, "F7",
	"OneThirtyFive", 235, 135, "F9",
};

nameList::NameHatInfo testPattiNameList[] =
{
	"Bridgette Abbott", 56, 4, "F20",
	"Caroline Acuna", 198, 106, "F5",
	"Fake Name", 345, 999, "B55",
	"Emily Acuna", 177, 3, "B19",
	"Another Fake", 444, 8, "B44",
};

int main (int argc, char* argv[])
{
	int debug=0;
	bool testJon=false;
	bool testPatti=false;
	opterr = 0;
	int c;
	string lvalue = "";

	while ((c = getopt(argc, argv, "di:s:tpl:")) != -1)
		switch (c)
		{
			case 'd': debug++; break;
			case 't': testJon=true; break;
			case 'p': testPatti=true; break;
			case 'l':
				  lvalue = optarg;
				  break;
			default:
				  cout << "Usage " << argv[0] << "[-d] [-t] [-p] [-l'C']" << endl;
				  cout << "      where 'C' is a character limiting the section to check ex. F=Flute, T=Tuba" << endl;
				  exit(-1);
		}

	list<Slave> slaveList;
	if (testJon)
	{
		size_t ll = sizeof(testJonNameList)/sizeof(nameList::NameHatInfo);
		for(int i = 0; i < ll; i++)
			slaveList.push_back(Slave(testJonNameList[i].circuitBoardNumber,
						testJonNameList[i].hatNumber,
						testJonNameList[i].drillId,
						testJonNameList[i].name));
	}
	else if (testPatti)
	{
		size_t ll = sizeof(testPattiNameList)/sizeof(nameList::NameHatInfo);
		for(int i = 0; i < ll; i++)
			slaveList.push_back(Slave(testPattiNameList[i].circuitBoardNumber,
						testPattiNameList[i].hatNumber,
						testPattiNameList[i].drillId,
						testPattiNameList[i].name));
	}
	else
		for(int entry = 0; entry < nameList::numberEntries; entry++)
		{
			if(lvalue.size() > 0)
			{
				if (nameList::nameList[entry].drillId[0] != lvalue.at(0))
					continue;
			}
			slaveList.push_back(Slave(nameList::nameList[entry].circuitBoardNumber,
						nameList::nameList[entry].hatNumber,
						nameList::nameList[entry].drillId,
						nameList::nameList[entry].name));
		}


	if (debug)
		cout << "slaveList:" << endl << slaveList;

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
	cout << "Start initial read of Slaves" << endl;
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
		cout << "Retry of unsuccessful. Retry #" << retry+1 << ", num slaves = " << unreachableList.size() << endl;

		// Cycle through all unreachable slaves querying for battery
		for(i=unreachableList.begin(); i != unreachableList.end();)
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
						errorList.push_back(slave);
						i = unreachableList.erase(i);
					}
				}
				else if(slave.isWarnNow())
				{
					warnList.push_back(slave);
					i = unreachableList.erase(i);
				}
				else
				{
					okList.push_back(slave);
					i = unreachableList.erase(i);
				}
			}
			else
			{
				i++;
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
		for(iter=okList.begin(); iter != okList.end(); iter++)
		{
			Slave& slave(*iter);
			if (slave.slave_no == 999)
				continue;
			slave.sendAllStop();
		}
	}
	for(iter=okList.begin(); iter != okList.end(); iter++)
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
