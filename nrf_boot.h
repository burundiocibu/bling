// here are some defines and such that need to be shared with the
// programmer.

// These two probabaly should be used form ensemble.hpp
const unsigned channel = 2;
const size_t addr_len = 4;


const size_t message_size = 22;
const size_t chunk_size = 16;
const uint16_t magic_word = 0xbabe;
enum bootloader_msg_ids
{
   no_op = 0,
   load_flash_chunk = 1,
   write_flash_page = 2,
   check_write_complete = 3,
   write_eeprom = 4,
   start_app = 5,
   read_flash_request = 12,
   read_flash_chunk = 13,
   read_eeprom = 14
};   
