#!/bin/sh

set +e;

ip netns add red; ip netns exec red ip link set dev lo up
ip netns add blue; ip netns exec blue ip link set dev lo up

ip link add br0 type bridge
ip link set dev br0 up

ip link add tapR type veth peer name br-tapR
ip link add tapB type veth peer name br-tapB

ip link set tapR netns red
ip link set tapB netns blue

ip link set br-tapR master br0; ip link set dev br-tapR up
ip link set br-tapB master br0; ip link set dev br-tapB up


ip netns exec red ip addr add 10.0.0.1/24 dev tapR
ip netns exec blue ip addr add 10.0.0.2/24 dev tapB
ip netns exec red ip link set dev tapR up
ip netns exec blue ip link set dev tapB up

xterm -e "echo NC SERVER; ip netns exec red nc -lu 20203; echo EXIT $?; read -n 1" &
xterm -e "echo SERVER; ip netns exec red ./udp_txp -l -e 443 -u 20203; echo EXIT $?; read -n 1" &
xterm -e "echo CLIENT; ip netns exec blue ./udp_txp -c -d 10.0.0.1 -e 443 -s 9090 -u 1234; echo EXIT $?; read -n 1" &
xterm -e "sleep .5; echo NC CLIENT; ip netns exec blue nc -u 127.0.0.1 1234; echo EXIT $?; read -n 1" &


ip netns exec red nft flush table ip txp
ip netns exec red nft add table ip txp
ip netns exec red nft add chain ip txp output "{ type filter hook output priority 0 ; }"
ip netns exec red nft add rule ip txp output tcp sport 443 tcp flags \& rst == rst drop

ip netns exec blue nft flush table ip txp
ip netns exec blue nft add table ip txp
ip netns exec blue nft add chain ip txp output "{ type filter hook output priority 0 ; }"
ip netns exec blue nft add rule ip txp output tcp sport 9090 tcp flags \& rst == rst drop

ip netns exec red ss -tunp
ip netns exec blue ss -tunp

ip netns exec red nft list ruleset
ip netns exec blue nft list ruleset

# ip netns exec red wireshark &
# ip netns exec blue wireshark &


