#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/ip_icmp.h>

#include "raw_tcp.h"

struct pseudo_header
{
    u_int32_t source_address;
    u_int32_t dest_address;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t tcp_length;
};

/*
  checksum
*/
static unsigned short csum(unsigned short *ptr,int nbytes) 
{
    register long sum = 0;

    while(nbytes>1) {
        sum+=*ptr++;
        nbytes-=2;
    }

    if(nbytes==1) {
        unsigned short oddbyte = 0;
        oddbyte=0;
        *((u_char*)&oddbyte)=*(u_char*)ptr;
        sum+=oddbyte;
    }

    sum = (sum>>16)+(sum & 0xffff);
    sum = sum + (sum>>16);
	
    return (short)~sum;
}

/*
 * USES STATIC BUFFER
 * NOT SUITABLE FOR MULTITHREADING
 */
int tcp4_send(uint16_t sport, uint32_t dst_ip, uint16_t dport,
              uint8_t* data, size_t datalen, uint8_t ct) {
    //Datagram to represent the packet
    static char datagram[PKT_MAX_SIZE]; // 64 kB
    static char pseudogram[PKT_MAX_SIZE]; // 64 kB

    //TCP header
    struct tcphdr *tcph = (struct tcphdr *) (datagram);
    struct sockaddr_in sin={0}, tin={0};
    struct pseudo_header psh;

    if(datalen > (PKT_MAX_SIZE)-sizeof(struct tcphdr)){
        fprintf(stderr, "datalen exceeds ip packet limit\n");
        return -1;
    }
    
    //Create a raw socket
    int s = socket (PF_INET, SOCK_RAW, IPPROTO_TCP);
    if(s == -1) {
        perror("Failed to create socket");
        return -1;
    }
 
    //some address resolution
    sin.sin_family = AF_INET;
    sin.sin_port = dport;
    sin.sin_addr.s_addr = dst_ip;

    int c = connect(s, (struct sockaddr*)&sin, sizeof(sin));
    if(c == -1)
    {
        perror("Failed to connect");
        close(s);
        return -1;
    }

    socklen_t tinlen = sizeof(tin);
    c = getsockname(s, (struct sockaddr*) &tin, &tinlen);
    if(c == -1)
    {
        perror("Failed to retrieve own ip");
        close(s);
        return -1;
    }
        
    //zero out the header part of the buffer
    memset (datagram, 0, sizeof (struct tcphdr));

    //Data part
    memcpy(datagram + sizeof(struct tcphdr), data, datalen);
	
    //TCP Header
    tcph->source = sport;
    tcph->dest = dport;
    tcph->seq = 1;
    tcph->ack_seq = 0;
    tcph->doff = 5;	//tcp header size
    tcph->fin=0;
    tcph->syn=(ct & 0x1)?1:0;
    tcph->rst=(ct & 0x4)?1:0;
    tcph->psh=0;
    tcph->ack=(ct & 0x2)?1:0;
    tcph->urg=0;
    tcph->window = htons (5840);	/* maximum allowed window size */
    tcph->check = 0;	//leave checksum 0 now, filled later by pseudo header
    tcph->urg_ptr = 0;
 
    //Now the TCP checksum
    psh.source_address = tin.sin_addr.s_addr;
    psh.dest_address = sin.sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons( sizeof(struct tcphdr) + datalen );
	
    int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr) + datalen;

    memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header) , tcph , sizeof(struct tcphdr) + datalen);

    tcph->check = csum( (unsigned short*) pseudogram , psize);

    //Send the packet
    if (sendto(s, datagram,  sizeof(struct tcphdr) + datalen,
               0, (struct sockaddr *) &sin, sizeof (sin)) < 0) {
        perror("sendto failed");
    }
    
    close(s);
    return 0;
}

/*
 * USES STATIC BUFFER
 * NOT SUITABLE FOR MULTITHREADING
 */
__attribute__((noreturn)) void tcp4_recv(
    uint16_t dport,
    void (handler(uint32_t sip, uint16_t sport, uint8_t *data, size_t datalen, void *env)),
    void *env
    ) {

    static uint8_t datagram[PKT_MAX_SIZE]; // 64 kB

    struct sockaddr saddr; 
    unsigned int saddrlen = sizeof(struct sockaddr);

    //Create a raw socket
    int s = socket (PF_INET, SOCK_RAW, IPPROTO_TCP);
    if(s == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    while(1) {
        int datalen = recvfrom(s, datagram , PKT_MAX_SIZE , 0 , &saddr , &saddrlen);
        if(datalen < 0 ) {
            perror("Recvfrom error , failed to get packets");
            exit(EXIT_FAILURE);
        }

        //Get the IP Header part of this packet
        struct iphdr *iph = (struct iphdr*)datagram;
        if (iph->protocol == IPPROTO_TCP) {
            struct tcphdr *tcph=(struct tcphdr*)(datagram + iph->ihl*4);
            if(tcph->dest == dport){
                handler(iph->saddr, tcph->source,
                        datagram+tcph->doff*4+iph->ihl*4 , datalen-tcph->doff*4-iph->ihl*4,
                        env);
            }
        }
    }
}

