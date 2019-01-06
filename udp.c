#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#include "udp.h"

int udp4_srv(uint16_t uport) {
    struct sockaddr_in sin={0};

    //Create a raw socket
    int s = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(s == -1) {
        perror("Failed to create socket");
        return -1;
    }

    sin.sin_family = AF_INET; // IPv4 
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    sin.sin_port = uport;
	
    // Bind the socket with the server address
    int c = bind(s, (struct sockaddr *)&sin, sizeof(sin)); 
    if (c == -1) { 
        perror("bind failed");
        return -1;
    }

    return s;
}

int udp4_cli(uint16_t uport) { 
    struct sockaddr_in sin={0};

    //Create a raw socket
    int s = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(s == -1) {
        perror("Failed to create socket");
        return -1;
    }

    sin.sin_family = AF_INET; // IPv4 
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    sin.sin_port = uport;
	
    // Bind the socket with the server address
    int c = connect(s, (struct sockaddr *)&sin, sizeof(sin)); 
    if (c == -1) { 
        perror("connection failed");
        return -1;
    }

    return s;
}
