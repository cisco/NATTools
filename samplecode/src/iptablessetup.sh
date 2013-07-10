#!/bin/sh

# Flush all rules.
iptables -F

# Capture every udp-packet with either destport or srcport as 4950,
# since that is the port we use for stun-testing.
# They are then punted to NFQUEUE number 0.
iptables -A FORWARD -p udp --dport 4950 -j NFQUEUE --queue-num 0
iptables -A FORWARD -p udp --sport 4950 -j NFQUEUE --queue-num 0
