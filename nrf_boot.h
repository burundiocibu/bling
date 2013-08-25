// here are some defines and such that need to be shared with the programmer.
#include <stdint.h>
#include <stddef.h>

// These two probabaly should be used form ensemble.hpp
const unsigned default_channel = 2;
const size_t addr_len = 4;

const size_t boot_message_size = 22;
const size_t boot_chunk_size = 16; // bytes
const size_t boot_page_size = 128; // bytes
const uint16_t boot_magic_word = 0xbabe;
enum bootloader_msg_ids
{
   bl_no_op = 0,
   bl_load_flash_chunk = 1,
   bl_write_flash_page = 2,
   bl_check_write_complete = 3,
   bl_write_eeprom = 4,
   bl_start_app = 5,
   bl_read_flash_request = 12,
   bl_read_flash_chunk = 13,
   bl_read_eeprom = 14
};   
