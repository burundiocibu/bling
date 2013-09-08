#include <iostream>
#include <fstream>
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

vector<Slave> createSlaveList();

int main ()
{
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

	vector<Slave> slaveList = createSlaveList();


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


	for(int i=0; i < slaveList.size(); i++)
	{
		slaveList[i].checkBattStatus();
		if(slaveList[i].readStatusSuccess())
		{
			if(slaveList[i].isActNow())
			{
				errorList.push_back(slaveList[i]);
			}
			else if(slaveList[i].isWarnNow())
			{
				warnList.push_back(slaveList[i]);
			}
			else
			{
				okList.push_back(slaveList[i]);
			}
		}
		else
		{
			unreachableList.push_back(slaveList[i]);
		}
	}

	// Output list of Slaves that we failed to read
	if(unreachableList.size() > 0)
	{
		unreachableOutFile.open(unreachableOutFileName, ofstream::trunc);
		unreachableOutFile << endl << "Unreachable Slaves:" << endl;
		for(int i=0; i < unreachableList.size(); i++)
		{
			unreachableOutFile << "  Student: " << unreachableList[i].student_name << ", Board Number " << unreachableList[i].slave_no << ", Hat Number " << unreachableList[i].hat_no << endl;
		}
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
		cout << endl << "Batteries Needing Replacement:" << endl;
		errorOutFile << endl << "Batteries Needing Replacement:" << endl;
		for(int i=0; i < errorList.size(); i++)
		{
			errorOutFile << "  Student: " << errorList[i].student_name << ", Board Number " << errorList[i].slave_no << ", Hat Number " << errorList[i].hat_no << ", Battery Level " << errorList[i].stateOfCharge << endl;
			cout << "  Student: " << errorList[i].student_name << ", Board Number " << errorList[i].slave_no << ", Hat Number " << errorList[i].hat_no << ", Battery Level " << errorList[i].stateOfCharge << endl;
		}
		errorOutFile.close();
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
		warnOutFile << endl << "Batteries low but not critical:" << endl;
		for(int i=0; i < warnList.size(); i++)
		{
			warnOutFile << "  Student: " << warnList[i].student_name << ", Board Number " << warnList[i].slave_no << ", Hat Number " << warnList[i].hat_no << ", Battery Level " << warnList[i].stateOfCharge << endl;
		}
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
		okOutFile << endl << "OK Batteries:" << endl;
		for(int i=0; i < okList.size(); i++)
		{
			okOutFile << "  Student: " << okList[i].student_name << ", Board Number " << okList[i].slave_no << ", Hat Number " << okList[i].hat_no << ", Battery Level " << okList[i].stateOfCharge << endl;
		}
		okOutFile.close();
	}
	else
	{
		// remove any existing file
		remove(okOutFileName);
	}

}

vector<Slave> createSlaveList()
{
	vector<Slave> slaveList;

	ifstream inFile;
	string inFileName = "nameList.cpp";
	inFile.open(inFileName, ifstream::in);

	char name[120];
	char hatNumber[10];
	char circuitNumber[10];
	char drillId[10];

	for(int entry = 0; entry < nameList::numberEntries; entry++)
	{
		Slave *slave = new Slave(nameList::nameList[entry].circuitBoardNumber, nameList::nameList[entry].hatNumber, nameList::nameList[entry].drillId, nameList::nameList[entry].name);
		slaveList.push_back(*slave);
	}
	return slaveList;
}

