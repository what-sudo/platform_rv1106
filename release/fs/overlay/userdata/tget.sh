#!/bin/sh

ipaddr=192.168.5.11

rm -rf $1

tftp -g -r $1 $ipaddr

chmod 755 $1
