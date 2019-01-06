#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "raw_tcp.h"
#include "udp.h"

struct env {
    int      pid;
    int      sock;
    uint32_t sip;
    uint16_t sport;
    uint16_t dport;
    uint16_t uport;
};

static void handler(uint32_t sip, uint16_t sport, uint8_t *data, size_t datalen, void *e){
    struct env *env=e;
    if(!datalen) {
        return;
    }

    if (env->sip == sip && env->sport == sport) {
        struct sockaddr_in sin;
        sin.sin_family = AF_INET;
        sin.sin_port = env->uport;
        sin.sin_addr.s_addr = inet_addr("127.0.0.1");
        
        int c = sendto(env->sock, data,  datalen, 0,
                       (struct sockaddr *) &sin, sizeof (sin));
        if(c == -1){
            perror("UDP sendto failed");
            env->sock=0;
            exit(EXIT_FAILURE);
        }
    }
}


int udp_txp_client(uint32_t dst, uint16_t dport, uint16_t sport, uint16_t uport) {
    int s = udp4_srv(htons(uport));
    if(s == -1){
        perror("Opening UDP server failed");
        exit(EXIT_FAILURE);
    }

    static uint8_t datagram[PKT_MAX_SIZE]; // 64 kB
    struct env env = {0};
    struct sockaddr_in cliaddr;
    socklen_t addrlen = sizeof(cliaddr);

    while(1){
        int n = recvfrom(s, datagram, PKT_MAX_SIZE,  0,
                         (struct sockaddr *) &cliaddr, &addrlen);
        if(n == -1){
            perror("UDP recv failed");
            exit(EXIT_FAILURE);
        }

        int status;
        if(env.sock == 0 || waitpid(env.pid, &status, WNOHANG) != 0){
            env.sock=s;
            env.sip = dst;
            env.sport = htons(dport);
            env.dport = htons(sport);
            env.uport = cliaddr.sin_port;
            pid_t pid = fork();
            if(pid == -1){
                perror("Fork failed");
                exit(EXIT_FAILURE);
            } else if ( pid == 0 ) {
                /*child*/
                fprintf(stderr, "FORKED listening on %d\n", sport);
                tcp4_recv(htons(sport), handler, &env);
                exit(EXIT_FAILURE);
            } else {
                env.pid=pid;
            }
        }

        tcp4_send(htons(sport), dst, htons(dport), datagram, n, TCP_SYN);
    }
    return 1;
}
