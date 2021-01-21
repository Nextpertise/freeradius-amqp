FROM ubuntu:20.04

ARG APT_OPTS="-y --option=Dpkg::options::=--force-unsafe-io --no-install-recommends"

RUN DEBIAN_FRONTEND="noninteractive" apt-get update && apt-get -y install tzdata

RUN DEBIAN_FRONTEND="noninteractive" apt-get update && apt-get install $APT_OPTS \
git \
libssl-dev \
cmake \
g++ \
libtalloc-dev \
libkqueue-dev \
ca-certificates \
libcurl4-openssl-dev libcap-dev libgdbm-dev libiodbc2-dev libkrb5-dev libldap2-dev libpam0g-dev libpcap-dev libperl-dev libmysqlclient-dev libpq-dev libreadline-dev libsasl2-dev libsqlite3-dev libssl-dev libtalloc-dev libwbclient-dev libyubikey-dev libykclient-dev libmemcached-dev libhiredis-dev python-dev samba-dev \
libmysqlclient-dev \
freeradius-krb5 \
freeradius-ldap \
freeradius-mysql \
freeradius-postgresql \
freeradius-rest \
freeradius-utils \
libfreeradius-dev 

RUN set -x \
&& ( git clone https://github.com/alanxz/rabbitmq-c.git /tmp/rabbitmq \
&& cd /tmp/rabbitmq \
&& mkdir build && cd build \
&& cmake .. \
&& cmake --build . --target install ) \
&& rm -rf /tmp/rabbitmq 

RUN set -x \
&& ( git clone https://github.com/json-c/json-c.git /tmp/json-c \
&& cd /tmp/json-c \
&& mkdir json-c-build \
&& cd json-c-build \
&& cmake .. \
&& make \
&& make test \
&& make install ) \
&& rm -rf /tmp/json-c

RUN git clone -b v3.0.x https://github.com/FreeRADIUS/freeradius-server.git /tmp/freeradius-server 
ADD rlm_amqp /tmp/freeradius-server/src/modules/rlm_amqp

RUN set -x \
&& (cd /tmp/freeradius-server \
&& ./configure)

 RUN ls -ltr /tmp/freeradius-server/src/include \
 && (cd /tmp/freeradius-server \
 && make \
 && make install ) \
 && rm -rf /tmp/freeradius-server

ARG STEP1=true
COPY amqp /usr/local/etc/raddb/mods-available 
RUN ln -s /usr/local/etc/raddb/mods-available/amqp /usr/local/etc/raddb/mods-enabled/amqp  
COPY default /usr/local/etc/raddb/sites-available
RUN printf "client 0.0.0.0/0 {\nsecret = 12345\nipv4addr = 0.0.0.0\n}" >> /usr/local/etc/raddb/clients.conf \
&& sed -i 's/#bob/bob/g' /usr/local/etc/raddb/mods-config/files/authorize 
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64:/usr/local/lib:/usr/local/lib/x86_64-linux-gnu
RUN echo $LD_LIBRARY_PATH

CMD radiusd -X

