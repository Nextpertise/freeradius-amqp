FROM ubuntu:18.04

RUN mkdir /root/freeradius
WORKDIR /root/freeradius

ARG APT_OPTS="-y --option=Dpkg::options::=--force-unsafe-io --no-install-recommends"
ARG AMQP_BUILD_OPTIONS="-DAMQP_DEFAULT_LOGIN_TIMEOUT_SEC=5"

RUN apt-get update && apt-get install $APT_OPTS \
git \
libssl-dev \
cmake \
g++ \
libtalloc-dev \
libkqueue-dev \
ca-certificates \
libcurl4-openssl-dev libcap-dev libgdbm-dev libiodbc2-dev libkrb5-dev libldap2-dev libpam0g-dev libpcap-dev libperl-dev libmysqlclient-dev libpq-dev libreadline-dev libsasl2-dev libsqlite3-dev libssl-dev libtalloc-dev libwbclient-dev libyubikey-dev libykclient-dev libmemcached-dev libhiredis-dev python-dev samba-dev \
devscripts quilt debhelper fakeroot equivs build-essential libsystemd-dev dh-systemd libjson-c3 git libjson-c-dev


RUN set -x \
&& ( git clone https://github.com/alanxz/rabbitmq-c.git rabbitmq \
&& cd rabbitmq \
&& mkdir build && cd build \
&& cmake $AMQP_BUILD_OPTIONS .. \
&& cmake --build . --target install )

WORKDIR /root/freeradius

RUN set -x \
&& ( git clone https://github.com/json-c/json-c.git json-c \
&& cd json-c \
&& mkdir json-c-build \
&& cd json-c-build \
&& cmake .. \
&& make \
&& make test \
&& make install )

WORKDIR /root/freeradius


ARG STEP1=true
COPY freeradius-amqp.install /root/freeradius/freeradius-amqp.install
COPY freeradius-amqp.postinst /root/freeradius/freeradius-amqp.postinst
COPY freeradius-amqp.prerm /root/freeradius/freeradius-amqp.prerm
COPY freeradius-amqp.lintian-overrides /root/freeradius/freeradius-amqp.lintian-overrides
COPY control.in.update /root/freeradius/control.in.update
COPY build-deb.sh /root/freeradius/build-deb.sh
ADD ./src/rlm_amqp /root/freeradius/rlm_amqp
ADD freeradius-server /root/freeradius/freeradius-server

RUN chmod +x build-deb.sh

VOLUME /build

CMD ./build-deb.sh