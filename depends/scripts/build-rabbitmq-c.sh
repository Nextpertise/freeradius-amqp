#!/bin/bash

APT_OPTS="-y --option=Dpkg::options::=--force-unsafe-io --no-install-recommends"
AMQP_BUILD_OPTIONS="-DAMQP_DEFAULT_LOGIN_TIMEOUT_SEC=5"
BUILD_VERSION="0.10.0"

#rm -rf rabbitmq
#rm -rf rabbitmq-c-build
#rm -rf rabbitmq-c-${BUILD_VERSION}-amd64.deb

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
#&& ( git clone https://github.com/alanxz/rabbitmq-c.git rabbitmq \
#&& mkdir rabbitmq-c-build && cd rabbitmq-c-build \
#&& cmake $AMQP_BUILD_OPTIONS ../rabbitmq \
#&& make \
#&& fakeroot make install DESTDIR=. ) 

cd rabbitmq-c-build

INSTALL_SIZE=$(du -s ./usr | awk '{ print $1 }')

mkdir -p ./DEBIAN

cp ../control.rmq.in ./DEBIAN/control

sed -i "s/__VERSION__/${BUILD_VERSION}/g" ./DEBIAN/control
sed -i "s/__FILESIZE__/${INSTALL_SIZE}/g" ./DEBIAN/control

shopt -s extglob
rm -rfv !("DEBIAN"|"usr"|"doc")

cd ../

fakeroot dpkg-deb -b "./rabbitmq-c-build"

mv ./rabbitmq-c-build.deb /build/rabbitmq-c-${BUILD_VERSION}-amd64.deb
