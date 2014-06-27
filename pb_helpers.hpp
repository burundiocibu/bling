#ifndef _PB_HELPERS_HPP
#define _PB_HELPERS_HPP

#include <iostream>
#include <string.h>

#include <boost/asio.hpp>

#include "server_msg.pb.h"

const size_t header_length=7;
const size_t max_length=1024;

void hexdump(void *ptr, int buflen);
void hexdump(std::string str);


// These are synchronous reads/write helpers.
std::string read_string(boost::asio::ip::tcp::socket& s, size_t len);

size_t write_string(boost::asio::ip::tcp::socket& s, std::string& str);

bool write_message(boost::asio::ip::tcp::socket& s, ::google::protobuf::Message& msg);

bool read_message(boost::asio::ip::tcp::socket& s, ::google::protobuf::Message& msg);

#endif
