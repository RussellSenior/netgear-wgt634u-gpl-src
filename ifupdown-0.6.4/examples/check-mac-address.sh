#!/bin/sh

set -e

export LANG=C

iface="$1"
targetmac="$(echo "$2" | sed -e 'y/ABCDEF/abcdef/'"
mac=$(/sbin/ifconfig "$iface" | sed -n -e '/^.*HWaddr \([:[:xdigit:]]*\).*/{s//\1/;y/ABCDEF/abcdef/;p;q;}')

if [ "$targetmac" = "$mac" ]; then exit 0; else exit 1; fi
