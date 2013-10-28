#!/bin/sh

# Flush all rules.
iptables -F

# Capture every STUN-packet as identified by the magic cookie, and punt them to
# NFQUEUE number 0. The string module could be used instead for more clarity,
# but u32 is much faster in this case.
iptables -A FORWARD -p udp --match u32 --u32 "0>>22&0x3C@12=0x2112a442" -j NFQUEUE --queue-num 0
