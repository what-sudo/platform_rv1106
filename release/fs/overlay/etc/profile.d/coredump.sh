#!/bin/sh

ulimit -c unlimited
echo "/userdata/core-%p-%e" > /proc/sys/kernel/core_pattern
