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
   "Hingle McCringleberry", 207, 7, "F19",
   "Ebreham Moisesus", 208, 8, "F8",
   "Bob Mellon",    202, 2, "F1",
   "Ron Burgundy",  205, 5, "F5",
   "006",           206, 6, "F10",
   "OneThirtyNine", 239, 139, "F6",
   "OneThirty",     230, 130, "F2",
   "EightyThree",   283, 83, "F3",
   "TwentyNine",    229, 29,  "F4",
   "OneTwentySix",  226, 126, "F7",
   "OneThirtyFive", 235, 135, "F9",
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
   unsigned slave_no=0;
   opterr = 0;
   int c;
   while ((c = getopt(argc, argv, "di:s:t")) != -1)
      switch (c)
      {
         case 'd': debug++; break;
         case 'i': input_fn = optarg; break;
         case 't': test=true; break;
         case 's': slave_no=atoi(optarg); break;
         default:
            printf("Usage %s -i fn [-d] [-s slave_no] [-t]\n", argv[0]);
            exit(-1);
      }

   if (input_fn==NULL)
   {
      printf("Please specify slave image hex file.\n");
      exit(-1);
   }

   FILE* fp = fopen(input_fn, "r");
   if (fp == NULL)
   {
      printf("Could not open %s. Terminating.\n", input_fn);
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

   for (int pass=1; todo.size() > 0; pass++)
   {
      cout << "Pass " << pass << " remaining boards:" << todo.size() << endl;
      list<Slave>::iterator i;
      for (i = todo.begin(); i != todo.end(); )
      {
         if (i->drill_id[0] != 'F')
         {
            done.push_back(*i);
            i=todo.erase(i);
         }
         else if (flasher.prog_slave(i->slave_no, image_buff, image_size))
         {
            done.push_back(*i);
            cout <<  *i  << " Programmed." << endl;
            i = todo.erase(i);
         }
         else
         {
            if (debug)
               cout << *i << " Failed programming." << endl;
            i++;
         }
      }
      if (todo.size() && debug)
         cout << "Boards remaining: " << endl << todo;
      sleep(1);

   }

   cout << "All boards programmed." << endl;

   fclose(fp);
   return 0;
}
