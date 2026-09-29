#!/bin/sh
set -eu
build_root=/tmp/anybandui-ubuntu-2204
test ! -e "$build_root"
mkdir "$build_root"
cd /tmp
wget -q https://cdimage.ubuntu.com/ubuntu-base/releases/22.04/release/ubuntu-base-22.04.5-base-amd64.tar.gz
wget -qO anybandui-ubuntu-SHA256SUMS https://cdimage.ubuntu.com/ubuntu-base/releases/22.04/release/SHA256SUMS
grep 'ubuntu-base-22.04.5-base-amd64.tar.gz' anybandui-ubuntu-SHA256SUMS | sha256sum -c -
tar -xzf ubuntu-base-22.04.5-base-amd64.tar.gz -C "$build_root"
cp /etc/resolv.conf "$build_root/etc/resolv.conf"
mount --bind /dev "$build_root/dev"
chroot "$build_root" sh -c 'apt-get update -qq && DEBIAN_FRONTEND=noninteractive apt-get install -y -qq gcc-mingw-w64-i686 autoconf automake make git cmake ninja-build' > /tmp/anybandui-toolchain-install.log 2>&1
tail -n 5 /tmp/anybandui-toolchain-install.log
