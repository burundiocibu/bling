
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>

#include "pb_helpers.hpp"

using boost::asio::ip::tcp;
using namespace std;


void hexdump(void *ptr, int buflen) {
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

void hexdump(string str)
{
   char buff[str.size()];
   strncpy(buff, str.c_str(), str.size());
   hexdump(buff, str.size());
}

string read_string(tcp::socket& s, size_t len)
{
   char buff[len];
   boost::asio::read(s, boost::asio::buffer(buff, len));
   hexdump(buff, len);
   return string(buff, len);
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

   bling::header hdr;

   if (typeid(msg) == typeid(bling::get_slave_list))
      hdr.set_msg_id(bling::header::GET_SLAVE_LIST);
   else if (typeid(msg) == typeid(bling::slave_list))
      hdr.set_msg_id(bling::header::SLAVE_LIST);
   else if (typeid(msg) == typeid(bling::set_slave_tlc))
      hdr.set_msg_id(bling::header::SET_SLAVE_TLC);
   else if (typeid(msg) == typeid(bling::start_effect))
      hdr.set_msg_id(bling::header::START_EFFECT);
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
   bling::header hdr;
   string str = read_string(s, header_length);
   if (!hdr.ParseFromString(str))
      return false;
   cout << "hdr::" << hdr.ShortDebugString() << endl;

   str = read_string(s, hdr.len());

   if (hdr.msg_id() == bling::header::SLAVE_LIST && 
       (typeid(msg) == typeid(bling::slave_list)))
   {
      msg.ParseFromString(str);
      return true;
   }

   cout << "Received message doesn't match expected." << endl;
   return false;
}

