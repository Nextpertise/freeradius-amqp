FROM ubuntu:18.04 as builder

RUN mkdir /root/builds
WORKDIR /root/builds

ARG APT_OPTS="-y --option=Dpkg::options::=--force-unsafe-io --no-install-recommends"

RUN apt-get update && apt-get install $APT_OPTS git libssl-dev cmake g++ libtalloc-dev libkqueue-dev ca-certificates libcurl4-openssl-dev libcap-dev libgdbm-dev libiodbc2-dev libkrb5-dev libldap2-dev libpam0g-dev libpcap-dev libperl-dev libmysqlclient-dev libpq-dev libreadline-dev libsasl2-dev libsqlite3-dev libssl-dev libtalloc-dev libwbclient-dev libyubikey-dev libykclient-dev libmemcached-dev libhiredis-dev python-dev samba-dev \
devscripts quilt debhelper fakeroot equivs build-essential libsystemd-dev dh-systemd libjson-c3 git libjson-c-dev wget

RUN set -x \
	&& ( git clone https://github.com/json-c/json-c.git json-c \
	&& mkdir json-c-build && cd json-c-build \
	&& cmake ../json-c \
	&& make \
	&& make test \
	&& fakeroot make install DESTDIR=.)

RUN set -x \
	&& ( wget https://github.com/alanxz/rabbitmq-c/archive/refs/tags/v0.13.0.tar.gz \
	&& tar zxvf v0.13.0.tar.gz -C rabbitmq
	&& mkdir rabbitmq-c-build && cd rabbitmq-c-build \
	&& cmake $AMQP_BUILD_OPTIONS ../rabbitmq \
	&& make \
	&& fakeroot make install DESTDIR=. )

COPY ./scripts/control.json.in /root/builds/control.json.in
COPY ./scripts/build-json-c.sh /root/builds/build-json-c.sh
COPY ./scripts/control.rmq.in /root/builds/control.rmq.in
COPY ./scripts/build-rabbitmq-c.sh /root/builds/build-rabbitmq-c.sh

RUN chmod +x build-json-c.sh && \
	chmod +x build-rabbitmq-c.sh
COPY ./scripts/freeradius-amqp.install /root/builds/freeradius-amqp.install
COPY ./scripts/freeradius-amqp.postinst /root/builds/freeradius-amqp.postinst
COPY ./scripts/freeradius-amqp.prerm /root/builds/freeradius-amqp.prerm
COPY ./scripts/freeradius-amqp.lintian-overrides /root/builds/freeradius-amqp.lintian-overrides
COPY ./scripts/control.in.update /root/builds/control.in.update
COPY ./scripts/build-deb.sh /root/builds/build-deb.sh
RUN chmod +x build-deb.sh

RUN git clone https://github.com/FreeRADIUS/freeradius-server.git && \
	cd freeradius-server && \
	git checkout 0da4faf84733e276bf5265c2d72c807996766a37
ADD rlm_amqp /root/builds/freeradius-server/src/modules/rlm_amqp

RUN mkdir /build && \
	/root/builds/build-json-c.sh && \
	/root/builds/build-rabbitmq-c.sh && \
	/root/builds/build-deb.sh


FROM ubuntu:18.04

ARG APT_OPTS="-y --option=Dpkg::options::=--force-unsafe-io --no-install-recommends"

RUN apt-get update && apt-get install $APT_OPTS \
vim \
wget

ARG STEP1=true
COPY --from=builder /build /tmp

WORKDIR /tmp
RUN dpkg -i *.deb || \
  apt-get install $APT_OPTS -f && \
  dpkg -i *.deb

COPY config/amqp /etc/freeradius/mods-available

RUN wget --no-check-certificate https://networkradius.com/assets/packages/radperf/radperf_2.0.1_amd64.deb && \
	dpkg -i radperf_2.0.1_amd64.deb
RUN rm /tmp/* && \
	cd /etc/freeradius/mods-enabled && \
	ln -s ../mods-available/amqp .

CMD freeradius -X
