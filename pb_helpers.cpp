#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>

#include "pb_helpers.hpp"

using boost::asio::ip::tcp;
using namespace std;


void hexdump(const void *ptr, int buflen)
{
   printf("len:%d\n", buflen);
   unsigned char *buf = (unsigned char*)ptr;
   int i, j;
   for (i=0; i<buflen; i+=16) {
      printf("%06x: ", i);
      for (j=0; j<16; j++) 
         if (i+j < buflen)
            printf("%02x ", buf[i+j]);
         else
            printf("   ");
      printf(" ");
      for (j=0; j<16; j++) 
         if (i+j < buflen)
            printf("%c", isprint(buf[i+j]) ? buf[i+j] : '.');
      printf("\n");
   }
}

void hexdump(const string& str)
{
   hexdump(str.c_str(), str.size());
}

void hexdump(const vector<char>& str)
{
   hexdump(&str[0], str.size());
}

string read_string(tcp::socket& s, size_t len)
{
   vector<char> buff(len);
   boost::asio::read(s, boost::asio::buffer(buff));
   hexdump(buff);
   return string(&buff[0], len);
}


size_t write_string(tcp::socket& s, string& str)
{
   return boost::asio::write(s, boost::asio::buffer(str.c_str(), str.size()));
}


bool write_message(boost::asio::ip::tcp::socket& s, ::google::protobuf::Message& msg)
{
   string msg_str;
   if (!msg.SerializeToString(&msg_str))
      return false;

   bling_pb::header hdr;

   if (typeid(msg) == typeid(bling_pb::get_slave_list))
      hdr.set_msg_id(bling_pb::header::GET_SLAVE_LIST);
   else if (typeid(msg) == typeid(bling_pb::slave_list))
      hdr.set_msg_id(bling_pb::header::SLAVE_LIST);
   else if (typeid(msg) == typeid(bling_pb::set_slave_tlc))
      hdr.set_msg_id(bling_pb::header::SET_SLAVE_TLC);
   else if (typeid(msg) == typeid(bling_pb::start_effect))
      hdr.set_msg_id(bling_pb::header::START_EFFECT);
   else
      cout << "Unidentified message type!!!" << endl;

   hdr.set_len(msg_str.size());
   string hdr_str;
   if (!hdr.SerializeToString(&hdr_str))
      return false;
   if (hdr_str.size() != header_length)
      cout << "Header length=" << hdr_str.size() << endl;
   string tx_buffer = hdr_str + msg_str;
   size_t n = write_string(s, tx_buffer);
   return n == tx_buffer.size();
}


bool read_message(boost::asio::ip::tcp::socket& s, ::google::protobuf::Message& msg)
{
   bling_pb::header hdr;
   string str = read_string(s, header_length);
   if (!hdr.ParseFromString(str))
      return false;
   cout << "hdr::" << hdr.ShortDebugString() << endl;

   str = read_string(s, hdr.len());
   hexdump(str);
   if (hdr.msg_id() == bling_pb::header::SLAVE_LIST && 
       (typeid(msg) == typeid(bling_pb::slave_list)))
      return msg.ParseFromString(str);

   cout << "Received message doesn't match expected." << endl;
   return false;
}

