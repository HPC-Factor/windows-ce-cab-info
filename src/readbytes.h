#ifndef INCLUDE_READBYTES_H
#define INCLUDE_READBYTES_H

#include <stdint.h>
#include <string.h>

uint32_t read_uint32_be(const unsigned char *bytes)
{
	return((bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | (bytes[0]));
}

uint32_t read_uint32_le(const unsigned char *bytes)
{
	return((bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3]));
}

uint16_t read_uint16_be(const unsigned char *bytes)
{
	return((bytes[1]) | (bytes[0] << 8));
}

uint16_t read_uint16_le(const unsigned char *bytes)
{
	return((bytes[0]) | (bytes[1] << 8));
}

#endif
