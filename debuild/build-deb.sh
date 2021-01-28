#!/bin/bash

#APT_OPTS="-y --option=Dpkg::options::=--force-unsafe-io --no-install-recommends"
#AMQP_BUILD_OPTIONS="-DAMQP_DEFAULT_LOGIN_TIMEOUT_SEC=5"
#
#rm -rf rabbitmq
#rm -rf json-c
#rm -rf freeradius-server
#rm -rf *.deb
#
#apt-get update && apt-get install $APT_OPTS \
#git \
#libssl-dev \
#cmake \
#g++ \
#libtalloc-dev \
#libkqueue-dev \
#ca-certificates \
#libcurl4-openssl-dev libcap-dev libgdbm-dev libiodbc2-dev libkrb5-dev libldap2-dev libpam0g-dev libpcap-dev libperl-dev libmysqlclient-dev libpq-dev libreadline-dev libsasl2-dev libsqlite3-dev libssl-dev libtalloc-dev libwbclient-dev libyubikey-dev libykclient-dev libmemcached-dev libhiredis-dev python-dev samba-dev \
#devscripts quilt debhelper fakeroot equivs build-essential libsystemd-dev dh-systemd libjson-c3 git libjson-c-dev
#
#
#set -x \
#&& ( git clone https://github.com/alanxz/rabbitmq-c.git rabbitmq \
#&& cd rabbitmq \
#&& mkdir build && cd build \
#&& cmake $AMQP_BUILD_OPTIONS .. \
#&& cmake --build . --target install ) \
#
#cd /root/freeradius
#
#set -x \
#&& ( git clone https://github.com/json-c/json-c.git json-c \
#&& cd json-c \
#&& mkdir json-c-build \
#&& cd json-c-build \
#&& cmake .. \
#&& make \
#&& make test \
#&& make install )
#

#cd /root/freeradius

#git clone -b v3.0.x https://github.com/FreeRADIUS/freeradius-server.git freeradius-server
#cd freeradius-server \
#&& git checkout v3.0.x \

cd /root/freeradius

cp -r rlm_amqp freeradius-server/src/modules/rlm_amqp
cp freeradius-amqp.* freeradius-server/debian
cat control.in.update >> freeradius-server/debian/control.in

ls -ltr freeradius-server/debian
cat freeradius-server/debian/control.in

cd freeradius-server

fakeroot debian/rules debian/control
fakeroot debian/rules clean
mk-build-deps -ir debian/control

# Workaroount to dependancy check for custom libraries
sed -i 's/dh_shlibdeps -l$(freeradius_dir)\/usr\/lib\/freeradius/dh_shlibdeps --dpkg-shlibdeps-params=--ignore-missing-info -l$(freeradius_dir)\/usr\/lib\/freeradius/g' debian/rules

make deb

mv ../*.deb /build

