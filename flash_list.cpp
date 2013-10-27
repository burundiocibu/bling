/*
  flash_list.cpp copyright Jon Little (2013)

   This program is used to program all slaves defined in nameList.hpp
  -----------------------------------------------------------------------------
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <list>
#include <map>
#include <algorithm>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>


#include "Slave.hpp"
#include "nameList.hpp"
#include "Flasher.hpp"
#include "Lock.hpp"

nameList::NameHatInfo testNameList[] =
{
   "S002", 157, 157, "X1",
   "S167", 167, 167, "X2",
   "S006",   6,   6, "X3",
   "S007",   7,   7, "X4",
   "S008",   8,   8, "X5",
   "S005",   5,   5, "X6",
   "S083",  83,  83, "X7",
   "S017",  17,  17, "X8"
};


using namespace std;

ostream& operator<<(ostream& s, const list<Slave>& slave_list)
{
   map<string, Slave> sm;

   list<Slave>::const_iterator j;
   for (j=slave_list.begin(); j != slave_list.end(); j++)
      sm[j->drill_id] = *j;

   map<string, Slave>::const_iterator i;
   for (i=sm.begin(); i != sm.end(); i++)
      cout << i->second << endl;

   return s;
}

void prog_list(list<Slave> todo, string fn, string version, int debug);
void ping_list(list<Slave> todo, int debug);

int main(int argc, char **argv)
{
   Lock lock; // If this fails, we can't get the hardware

   int debug=0;
   string fn;
   bool test=false;
   bool ping=false;
   list<unsigned> slaves;
   string version;
   opterr = 0;
   int c;
   while ((c = getopt(argc, argv, "di:s:tv:p")) != -1)
      switch (c)
      {
         case 'd': debug++; break;
         case 'i': fn = string(optarg); break;
         case 't': test=true; break;
         case 'p': ping=true; break;
         case 's': slaves.push_back(atoi(optarg)); break;
         case 'v': version=string(optarg); break;
         default:
            cout << "Usage " << argv[0] << " -i fn [-d] [-s slave_no] [-t] [-v vers]" << endl
                 << " -t  Just program test list" << endl
                 << " -p  Just ping, don't program"
                 << endl;
            exit(-1);
      }

 
   list<Slave> todo, done, all;
   if (slaves.size() > 0)
   {
      list<unsigned>::const_iterator i;
      for (i=slaves.begin(); i != slaves.end(); i++)
      {
         bool found=false;
         for(int j=0; j < nameList::numberEntries; j++)
         {
            if (nameList::nameList[j].circuitBoardNumber == *i)
            {
               todo.push_back(Slave(nameList::nameList[j].circuitBoardNumber,
                                    nameList::nameList[j].hatNumber,
                                    nameList::nameList[j].drillId,
                                    nameList::nameList[j].name));
               found = true;
               break;
            }
         }
         if (!found)
            todo.push_back(Slave(*i, 1, "Fxx", "Hingle McCringleberry"));
      }
   }
   else if (!test)
   {
      for(int i = 0; i < nameList::numberEntries; i++)
         todo.push_back(Slave(nameList::nameList[i].circuitBoardNumber,
                              nameList::nameList[i].hatNumber,
                              nameList::nameList[i].drillId,
                              nameList::nameList[i].name));
   }
   else
   {
      size_t ll = sizeof(testNameList)/sizeof(nameList::NameHatInfo);
      for(int i = 0; i < ll; i++)
         todo.push_back(Slave(testNameList[i].circuitBoardNumber,
                              testNameList[i].hatNumber,
                              testNameList[i].drillId,
                              testNameList[i].name));
   }

   if (ping)
      ping_list(todo, debug);
   else
      prog_list(todo, fn, version, debug);
}


void ping_list(list<Slave> todo, int debug)
{
   Flasher flasher(debug);
 
   list<Slave> done, all;

   list<Slave>::iterator i;
   for (int pass=1; todo.size() > 0 && pass < 10; pass++)
   {
      cout << "Pass " << pass << " ";
      for (i = todo.begin(); i != todo.end(); )
      {
         if (flasher.ping_slave(i->slave_no))
         {
            i->soc = flasher.rx_soc;
            i->vcell = flasher.rx_vcell;
            i->version = flasher.rx_version;
            done.push_back(*i);
            cout << "." << flush;
            i = todo.erase(i);
         }
         else
         {
            cout << "x" << flush;
            i++;
         }
      }

      cout << endl;
      if (todo.size())
         sleep(1);
   }

   cout << "Done" << endl
        << endl
        << "Responded:" << endl
        << done << endl
        << "No response:" << endl
        << todo << endl;
}

void prog_list(list<Slave> todo, string fn, string version, int debug)
{
   if (fn.size() == 0)
   {
      cout << "Please specify slave image hex file. Exiting." << endl;
      exit(-1);
   }

   FILE* fp = fopen(fn.c_str(), "r");
   if (fp == NULL)
   {
      cout << "Could not open " << fn << ". Terminating." << endl;
      exit(-1);
   }

   bool got_image=false;
   char line[80];
   size_t image_size=0;
   uint8_t *image_buff;
   uint8_t *p;

   const size_t ibsize = 0x8000;
   image_buff = (uint8_t*)malloc(ibsize);
   memset(image_buff, 0xff, ibsize);
   
   p = image_buff;
   while (!got_image)
   {
      if (fgets(line, sizeof(line), fp) == NULL)
         break;
      if (line[0] != ':')
         break;
      int len, addr, id;
      sscanf(line+1, "%02x%04x%02x", &len, &addr, &id);

      if (id==0)
      {
         for (int i=0; i<len; i++)
            sscanf(line+9+2*i, "%02x", p++);

         image_size+=len;
      }
      else if (id==1)
         got_image = true;
   }
   fclose(fp);
   
   if (debug)
      printf("Image length is %d bytes\n", image_size);

   if (!got_image)
   {
      printf("Failed to read image to load. Exiting.\n");
      exit(-1);
   }

   Flasher flasher(debug);
 
   list<Slave> done, all;

   list<Slave>::iterator i;
   for (int pass=1; todo.size() > 0 && pass < 10; pass++)
   {
      cout << "Pass " << pass << " remaining boards:" << todo.size() << endl;
      for (i = todo.begin(); i != todo.end(); )
      {
         if (flasher.prog_slave(i->slave_no, image_buff, image_size, version))
         {
            i->soc = flasher.rx_soc;
            i->vcell = flasher.rx_vcell;
            i->version = flasher.rx_version;
            done.push_back(*i);
            cout <<  i->student_name  << " Programmed." << endl;
            i = todo.erase(i);
         }
         else
         {
            cout << i->student_name << " Failed programming." << endl;
            i++;
         }
      }

      if (todo.size())
         sleep(2);
   }

   if (todo.size()==0)
      cout << "All boards programmed." << endl;
   else
      cout << "Giving up." << endl;

   cout << "Programmed:" << endl;
   cout << done;
}
