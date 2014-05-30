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
#include <iomanip>
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
#include "Flasher.hpp"
#include "Lock.hpp"

using namespace std;

ostream& operator<<(ostream& s, const list<Slave>& slave_list)
{
   map<string, Slave> sm;

   list<Slave>::const_iterator j;
   for (j=slave_list.begin(); j != slave_list.end(); j++)
      sm[j->drill_id] = *j;

   if (sm.size())
   {
      map<string, Slave>::const_iterator i;
      for (i=sm.begin(); i != sm.end(); i++)
         cout << left << setw(3) << i->second.id << "  " << setw(3) << i->second.version
              << right << fixed
              << "  " << setw(6) << setprecision(3) << i->second.vcell << "V"
              << "  " << setw(5) << setprecision(2) << i->second.soc << "%"
              << "  " << i->second.student_name
              << endl;
   }

   return s;
}

void prog_list(list<Slave> todo, string fn, string version, int debug);
void ping_list(list<Slave> todo, int debug);

int main(int argc, char **argv)
{
   Lock lock; // If this fails, we can't get the hardware

   int debug=0;
   string image_fn, list_fn, version;
   bool ping=false;
   list<unsigned> slaves;
   opterr = 0;
   int c;
   while ((c = getopt(argc, argv, "di:l:s:v:p")) != -1)
      switch (c)
      {
         case 'd': debug++; break;
         case 'i': image_fn = string(optarg); break;
         case 'l': list_fn = string(optarg); break;
         case 'p': ping=true; break;
         case 's': slaves.push_back(atoi(optarg)); break;
         case 'v': version=string(optarg); break;
         default:
            cout << "Usage " << argv[0] << " -i [hex_fn] -l [slave_fn] [-d] [-s slave_no] [-v vers]" << endl
                 << " -i  specifies hex image file" << endl
                 << " -l  specifies board list file" << endl
                 << " -d  Enable debug ouput" << endl
                 << " -v  Don't program if board already has this version" << endl
                 << " -s  board id to program, repeat for more than one board." << endl
                 << " -p  Just ping, don't program" << endl;
            exit(-1);
      }


   SlaveList todo, done, all, sl;
   if (list_fn.size())
   {
      sl = read_slaves(list_fn);
      if (debug>2)
         cout << list_fn << ":" << endl << sl;
   }

   if (slaves.size() > 0)
   {
      list<unsigned>::const_iterator i;
      for (i=slaves.begin(); i != slaves.end(); i++)
      {
         bool found=false;
         for(auto j=sl.begin(); j != sl.end(); j++)
         {
            if (j->id == *i)
            {
               todo.push_back(*j);
               found = true;
               break;
            }
         }
         if (!found)
            todo.push_back(Slave(*i, ("X"+to_string(*i)).c_str(), "Hingle McCringleberry"));
      }
   }
   else
      todo = sl;

   if (debug>1)
      cout << "todo:" << endl << todo;

   if (todo.size() == 0)
   {
      cout << "No boards specified. Please use -l and/or -s" << endl;
      exit(-1);
   }

   if (ping)
      ping_list(todo, debug);
   else
      prog_list(todo, image_fn, version, debug);
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
         if (flasher.ping_slave(i->id))
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
         if (flasher.prog_slave(i->id, image_buff, image_size, version))
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
