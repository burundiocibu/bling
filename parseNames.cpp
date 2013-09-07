
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <sstream>

#include <string.h>

using namespace std;

int parseCsvHatListFile();
void writeHppFile();
void writeCppFile(int tableSize);

int main ()
{
	int numEntries = parseCsvHatListFile();
	writeHppFile();
	writeCppFile(numEntries);
	return 0;
}

int parseCsvHatListFile()
{
	// File information
	ifstream inFile;
	ofstream outFile;
	const char outFileName[] = "midFile.hpp";
	string inFileName = "hatList.csv";
	inFile.open(inFileName, ifstream::in);
	outFile.open(outFileName, ofstream::trunc);

	// variables used during parsing
	char lastName[80];
	char firstName[80];
	char hatNumber[10];
	char circuitNumber[10];
	char drillId[10];
	int numTableEntries = 0;
	// Parse Input file one line at a time
	string line;
	while (getline(inFile, line))
	{
		/* 
		 * Check for lines that don't include student
		 * information and for empty lines
		 */
		if(line.find("Anderson") != string::npos)
		{
			// Skip Anderson Band headline
			continue;
		}
		if(line.find("Last Name") != string::npos)
		{
			// Skip column header line
			continue;
		}
		if(line.size() < 15)
		{
			// Skip empty lines (or all commas)
			continue;
		}

		/* 
		 * If real line, parse necessary information
		 * Assumes following column setup
		 * [0] = last name
		 * [1] = first name
		 * [5] = hat number
		 * [7] = circuit board number
		 * [8] = drill ID
		 */
		stringstream ss(line);
		string item;
		int columnCount=0;
		bool validLine = true;
		while (getline(ss, item, ',') && validLine==true)
		{
			switch(columnCount)
			{
				case 0:
					strcpy(lastName, item.c_str());
					if((strlen(lastName) < 1) || strstr(lastName, "Complete") != NULL)
					{
						validLine = false;
					}
					break;
				case 1:
					strcpy(firstName, item.c_str());
					break;
				case 5:
					strcpy(hatNumber, item.c_str());
					break;
				case 7:
					strcpy(circuitNumber, item.c_str());
					if (strlen(circuitNumber) < 1) 
					{
						validLine = false;
					}
					else if((circuitNumber[0] == 'x') || (circuitNumber[0] == 'X'))
					{
						strcpy(circuitNumber, "999");
					}
					break;
				case 8:
					strcpy(drillId, item.c_str());
					if (strlen(drillId) < 1) strcpy(drillId, "XX");
					break;
				default:
					break;
			}
			columnCount++;
		}
		// Only write the information out if there is a valid hat number and circuit board is not "N/A"
		if (validLine == true)
		{
			if(((strlen(hatNumber) > 0) && (hatNumber[0] != 'N')) && (circuitNumber[0] != 'N'))
			{
				outFile << "\"" << firstName << " " << lastName << "\", " 
					<< hatNumber << ", " << circuitNumber << ", \"" << drillId << "\"," << endl;
				numTableEntries++;
			}
		}
	}    
	inFile.close();
	outFile.close();
	return numTableEntries;
}

void writeHppFile()
{
	const char outHppFileName[] = "nameList.hpp";
	ofstream outHppFile;
	outHppFile.open(outHppFileName, ofstream::trunc);

	// Write file header (hpp file)
	outHppFile << "// auto-generated file.  DO NOT EDIT!" << endl << endl;
	outHppFile << "#ifndef _PARSE_NAMES_HPP" << endl << "#define _PARSE_NAMES_HPP" << endl << endl;
	outHppFile << "namespace nameList" << endl << "{" << endl;
	outHppFile << "typedef struct nameHatInfo NameHatInfo;" << endl << endl;
	outHppFile << "extern const int numberEntries;" << endl;
	outHppFile << "extern NameHatInfo nameList[];" << endl << endl;
	outHppFile << "typedef struct nameHatInfo" << endl <<  "{" << endl;
	outHppFile << "    char name[120];" << endl;
	outHppFile << "    int hatNumber;" << endl;
	outHppFile << "    int circuitBoardNumber;" << endl;
	outHppFile << "    char drillId[4];" << endl;
	outHppFile << "    }  NameHatInfo;" << endl;
	outHppFile << "}" << endl;
	outHppFile << "#endif" << endl;

	outHppFile.close();
}

void writeCppFile(int tableSize)
{
	ifstream inFile;
	ofstream outCppFile;
	const char outCppFileName[] = "nameList.cpp";
	const char inFileName[] = "midFile.hpp";
	inFile.open(inFileName, ifstream::in);
	outCppFile.open(outCppFileName, ofstream::trunc);

	// Parse Input file one line at a time

	outCppFile << "// auto-generated file.  DO NOT EDIT!" << endl << endl;
	outCppFile << "#include \"nameList.hpp\"" << endl << endl;
	outCppFile << "namespace nameList" << endl;
	outCppFile << "{" << endl;
	outCppFile << "const int numberEntries = " << tableSize << ";" << endl;
	outCppFile << "NameHatInfo nameList[numberEntries] =" << endl;
	outCppFile << "{" << endl;
	string line;
	while (getline(inFile, line))
	{
		outCppFile << "    " << line << endl;
	}
	outCppFile << "};" << endl;
	outCppFile << "}" << endl;
}

