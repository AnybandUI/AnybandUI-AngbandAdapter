#!/bin/sh
set -eu
build_root=/tmp/anybandui-ubuntu-2204
test "$(readlink -f "$build_root")" = /tmp/anybandui-ubuntu-2204
# This is solely the partial filesystem created by setup-release-toolchain.sh.
rm -rf -- "$build_root"
mkdir "$build_root"
# Keep the toolchain in temporary RAM storage, not Docker's distribution disk.
mount -t tmpfs -o size=2g,nosuid tmpfs "$build_root"
tar -xzf /tmp/ubuntu-base-22.04.5-base-amd64.tar.gz -C "$build_root"
cp /etc/resolv.conf "$build_root/etc/resolv.conf"
mount --bind /dev "$build_root/dev"
chroot "$build_root" sh -c 'apt-get update -qq && DEBIAN_FRONTEND=noninteractive apt-get install -y -qq gcc-mingw-w64-i686 autoconf automake make git cmake ninja-build' > "$build_root/toolchain-install.log" 2>&1
tail -n 5 "$build_root/toolchain-install.log"
