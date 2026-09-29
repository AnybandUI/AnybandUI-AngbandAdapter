#!/bin/sh
set -eu
build_root=/tmp/anybandui-ubuntu-2204
test "$(readlink -f "$build_root")" = /tmp/anybandui-ubuntu-2204
mount -o remount,rw /mnt/host/wslg/distro
mount -o remount,rw /
rm -rf -- "$build_root"
rm -f -- /tmp/ubuntu-base-22.04.5-base-amd64.tar.gz /tmp/anybandui-ubuntu-SHA256SUMS
test ! -e "$build_root"
test ! -e /tmp/ubuntu-base-22.04.5-base-amd64.tar.gz
test ! -e /tmp/anybandui-ubuntu-SHA256SUMS
df -h /mnt/host/wslg/distro
