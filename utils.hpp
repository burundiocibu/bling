#ifndef _UTILS_HPP
#define _UTILS_HPP
#include <stdint.h>

#ifdef AVR
// seems like avr don't have these
#define ntohs(x) (((x>>8) & 0xFF) | ((x & 0xFF)<<8))
#define htons(x) (((x>>8) & 0xFF) | ((x & 0xFF)<<8))
#define ntohl(x) ( ((x>>24) & 0xFF) | ((x>>8) & 0xFF00) | \
                   ((x & 0xFF00)<<8) | ((x & 0xFF)<<24)   \
                   )
#define htonl(x) ( ((x>>24) & 0xFF) | ((x>>8) & 0xFF00) |      \
                   ((x & 0xFF00)<<8) | ((x & 0xFF)<<24)        \
                   )
#endif
