#!/bin/bash

dev="xpu"
major="$(awk -F ' ' '$2 == "'"$dev"'" { print $1 }' /proc/devices)"
mknod "/dev/$dev" c "$major" 0
