#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <arpa/inet.h>

int udp_txp_client(uint32_t dst, uint16_t dport, uint16_t sport, uint16_t uport);
int udp_txp_server(uint16_t dport, uint16_t uport);

struct optdesc{
    const struct option opt;
    char *desc;
};

static const struct optdesc options[] = {
    {{"client", no_argument,       0,  'c' }, "Opens udp listening socket, sends tcp SYN packets"},
    {{"server", no_argument,       0,  'l' }, "Sends tcp SYN,ACK packets, uses random udp source port"},
    {{"dst",    required_argument, 0,  'd' }, "Destination IP (client only)"},
    {{"dport",  required_argument, 0,  'e' }, "Destination/listen port"},
    {{"sport",  required_argument, 0,  's' }, "Source port (client only)"},
    {{"uport",  required_argument, 0,  'u' }, "Local UDP port"},
    {{"help",   required_argument, 0,  'h' }, "Prints this help"},
    {{0,        0,                 0,  0   }, 0}
};


void usage(char *exec_name){
    fprintf(stderr, "Usage: %s -{c | l}  -d [DST] -e [PORT] -s [PORT] -u [PORT]\n", exec_name);
    for(int i=0; options[i].desc;i++){
        fprintf(stderr, "\t--%s or -%c\t%s\n",
                options[i].opt.name, options[i].opt.val, options[i].desc);
    }
    exit(1);
}

int main(int argc, char **argv) {
    int c;
    uint8_t server = 0;
    uint16_t D_PORT=0, S_PORT=0, U_PORT=0;
    uint32_t dst=0;

    static struct option long_options[sizeof(options)/sizeof(options[0])];
    for(c=0;c < (int)(sizeof(options)/sizeof(options[0]));c++) {
        long_options[c]=options[c].opt;
    }


    while (1) {
        int option_index = 0;
        c = getopt_long(argc, argv, ":cld:e:s:u:h",
                        long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'c':
            break;
        case 'd':
            dst = inet_addr(optarg);
            break;
        case 'e':
            D_PORT = atol(optarg);
            break;
        case 'l':
            server=1;
            break;
        case 's':
            S_PORT = atol(optarg);
            break;
        case 'u':
            U_PORT = atol(optarg);
            break;
        case 'h': /* fall through */
        case '?': /* fall through */
        default:
            usage(argv[0]);
        }
    }

    if( (!server && (dst*D_PORT*S_PORT*U_PORT == 0)) ||
        (server && (D_PORT * U_PORT == 0)) )
    {
        usage(argv[0]);
    }
    
    if (optind < argc) {
        printf("non-option ARGV-elements: ");
        while (optind < argc)
            printf("%s ", argv[optind++]);
        printf("\n");
        usage(argv[0]);
    }

    if(server){
        exit(udp_txp_server(D_PORT, U_PORT));
    } else {
        exit(udp_txp_client(dst, D_PORT, S_PORT, U_PORT));
    }

    exit(EXIT_SUCCESS);
}
