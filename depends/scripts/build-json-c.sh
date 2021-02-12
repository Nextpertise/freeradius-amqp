#!/bin/bash

APT_OPTS="-y --option=Dpkg::options::=--force-unsafe-io --no-install-recommends"
BUILD_VERSION="0.15"

#rm -rf json-c
#rm -rf json-c-build
#rm -rf json-c-${BUILD_VERSION}-amd64.deb

#apt-get update && apt-get install $APT_OPTS \
#git \
#libssl-dev \
#cmake \
#g++ \
#libtalloc-dev \
#libkqueue-dev \
#ca-certificates \
#libcurl4-openssl-dev libcap-dev libgdbm-dev libiodbc2-dev libkrb5-dev libldap2-dev \
#libpam0g-dev libpcap-dev libperl-dev libmysqlclient-dev libpq-dev libreadline-dev libsasl2-dev \
#libsqlite3-dev libssl-dev libtalloc-dev libwbclient-dev libyubikey-dev libykclient-dev libmemcached-dev libhiredis-dev python-dev samba-dev \
#devscripts quilt debhelper fakeroot equivs build-essential libsystemd-dev dh-systemd libjson-c3 git libjson-c-dev 

#set -x \
#&& ( git clone https://github.com/json-c/json-c.git json-c \
#&& mkdir json-c-build && cd json-c-build \
#&& cmake ../json-c \
#&& make \
#&& make test \
#&& fakeroot make install DESTDIR=.)

cd json-c-build

INSTALL_SIZE=$(du -s ./usr | awk '{ print $1 }')

mkdir -p ./DEBIAN

cp ../control.json.in ./DEBIAN/control

sed -i "s/__VERSION__/${BUILD_VERSION}/g" ./DEBIAN/control
sed -i "s/__FILESIZE__/${INSTALL_SIZE}/g" ./DEBIAN/control

shopt -s extglob
rm -rfv !("DEBIAN"|"usr"|"doc")

cd ../

fakeroot dpkg-deb -b "./json-c-build"

mv ./json-c-build.deb /build/json-c-${BUILD_VERSION}-amd64.deb
