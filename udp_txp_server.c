#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#include "udp.h"
#include "raw_tcp.h"


struct env {
    int      pid;
    int      sock;
    uint32_t sip;
    uint16_t sport;
    uint16_t dport;
    uint16_t uport;
};

static void udp_listener(struct env *env){
    static uint8_t datagram[PKT_MAX_SIZE]; // 64 kB
    struct sockaddr_in cliaddr;
    socklen_t addrlen = sizeof(cliaddr);

    while(1){
        int n = recvfrom(env->sock, datagram, PKT_MAX_SIZE,  0,
                         (struct sockaddr *) &cliaddr, &addrlen);
        if(n == -1){
            perror("UDP recv failed");
            exit(EXIT_FAILURE);
        }
        tcp4_send(env->dport, env->sip, env->sport, datagram, n, TCP_SYN|TCP_ACK);
    }
}

static void handler(uint32_t sip, uint16_t sport, uint8_t *data, size_t datalen, void *e){
    struct env *env=e;
    if(!datalen) {
        return;
    }

    int status;
    if(env->sock == 0 || waitpid(env->pid, &status, WNOHANG) != 0){
        int s = udp4_cli(env->uport);
        if(s == -1){
            perror("Opening UDP client failed");
            exit(EXIT_FAILURE);
        }
        env->sock=s;
        env->sip = sip;
        env->sport = sport;

        pid_t pid = fork();
        if(pid == -1){
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if ( pid == 0 ) {
            /*child*/
            udp_listener(env);
            exit(EXIT_FAILURE);
        } else {
            env->pid=pid;
        }
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
            kill(env->pid, SIGKILL);
        }
    } else {
        fprintf(stderr, "TODO: multiclient support\n"           \
                "Connection currently used by  %s:%d\n"         \
                "Connection request denied for %s:%d\n",
                inet_ntoa(*((struct in_addr*)&env->sip)), ntohs(env->sport),
                inet_ntoa(*((struct in_addr*)&sip)), ntohs(sport)
            );
    }
}


int udp_txp_server(uint16_t dport, uint16_t uport) {
    struct env env = {0};
    env.dport=htons(dport);
    env.uport=htons(uport);
    tcp4_recv(htons(dport), handler, &env);
    return 1;
}
