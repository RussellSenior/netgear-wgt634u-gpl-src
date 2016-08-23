#!/bin/sh

NNTP_PORT=119
HTTP_PORT=80

iptables -t mangle -D PREROUTING -p tcp -m multiport --dports $HTTP_PORT,$NNTP_PORT -j QUEUE
