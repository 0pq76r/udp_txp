#pragma once

#include <stdlib.h>
#include <inttypes.h>

#ifndef PKT_MAX_SIZE
#  define PKT_MAX_SIZE (1<<16)
#endif

#define TCP_SYN 0x1
#define TCP_ACK 0x2
#define TCP_RST 0x4

int tcp4_send(uint16_t sport, uint32_t dst_ip, uint16_t dport,
              uint8_t* data, size_t datalen, uint8_t ct);

__attribute__((noreturn)) void tcp4_recv(
    uint16_t dport,
    void (handler(uint32_t sip, uint16_t sport, uint8_t *data, size_t datalen, void *env)),
    void *env
    );
