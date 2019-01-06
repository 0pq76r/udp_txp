#pragma once

#ifndef PKT_MAX_SIZE
#  define PKT_MAX_SIZE (1<<16)
#endif

int udp4_srv(uint16_t uport);
int udp4_cli(uint16_t uport);
