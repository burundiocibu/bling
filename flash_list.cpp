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
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <list>

#include "Slave.hpp"
#include "nameList.hpp"
#include "Flasher.hpp"

nameList::NameHatInfo testNameList[] =
{
   "S002",   2,   2, "X1",
   "S167", 167, 167, "X2",
   "S006",   6,   6, "X3",
   "S007",   7,   7, "X4",
   "S008",   8,   8, "X5",
   "S005",   5,   5, "X6",
   "S083",  83,  83, "X7",
   "S017",  17,  17, "X8"
};


std::ostream& operator<<(std::ostream& s, const std::list<Slave>& slave_list)
{
   std::list<Slave>::const_iterator j;
   for (j=slave_list.begin(); j != slave_list.end(); j++)
      std::cout << *j << std::endl;
   return s;
}


using namespace std;

int main(int argc, char **argv)
{
   int debug=0;
   char *input_fn;
   bool test=false;
   bool ignore_battery=true;
   unsigned slave_no=0;
   string version;
   opterr = 0;
   int c;
   while ((c = getopt(argc, argv, "di:s:tv:")) != -1)
      switch (c)
      {
         case 'd': debug++; break;
         case 'i': input_fn = optarg; break;
         case 't': test=true; break;
         case 's': slave_no=atoi(optarg); break;
         case 'v': version=string(optarg); break;
         case 'b': ignore_battery=false; break;
         default:
            cout << "Usage " << argv[0] << " -i fn [-d] [-s slave_no] [-t] [-b]" << endl
                 << " -b  Don't skip programming battery " << endl;
            exit(-1);
      }

   if (input_fn==NULL)
   {
      cout << "Please specify slave image hex file. Exiting." << endl;
      exit(-1);
   }

   FILE* fp = fopen(input_fn, "r");
   if (fp == NULL)
   {
      cout << "Could not open " << input_fn << ". Terminating." << endl;
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
 
   list<Slave> todo, done, all;
   if (slave_no!=0)
      todo.push_back(Slave(slave_no, 1, "FXX", "Hingle McCringleberry"));
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

   for (int pass=1; todo.size() > 0 && pass < 10; pass++)
   {
      cout << "Pass " << pass << " remaining boards:" << todo.size() << endl;
      list<Slave>::iterator i;
      for (i = todo.begin(); i != todo.end(); )
      {
         if (ignore_battery && 
             (i->drill_id[0] == 'O' || i->drill_id[0] == 'Q' || i->drill_id[1] == 'N'))
         {
            done.push_back(*i);
            i = todo.erase(i);
         }
         else if (flasher.prog_slave(i->slave_no, image_buff, image_size, version))
         {
            done.push_back(*i);
            cout <<  *i  << " Programmed." << endl;
            i = todo.erase(i);
         }
         else
         {
            cout << *i << " Failed programming." << endl;
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

   return 0;
}
