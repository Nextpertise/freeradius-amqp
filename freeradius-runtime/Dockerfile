FROM ubuntu:18.04

ARG APT_OPTS="-y --option=Dpkg::options::=--force-unsafe-io --no-install-recommends"

RUN apt-get update && apt-get install $APT_OPTS \
vim

ARG STEP1=true
COPY packages /tmp

WORKDIR /tmp
RUN dpkg -i *.deb || \
  apt-get install $APT_OPTS -f && \
  dpkg -i *.deb

COPY amqp /etc/freeradius/mods-available
RUN ln -s /etc/freeradius/mods-available/amqp /etc/freeradius/mods-enabled/amqp
COPY default /etc/freeradius/sites-available
RUN printf "client 0.0.0.0/0 {\nsecret = 12345\nipv4addr = 0.0.0.0\n}" >> /etc/freeradius/clients.conf \
&& sed -i 's/#bob/bob/g' /etc/freeradius/mods-config/files/authorize
#ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64:/usr/local/lib:/usr/local/lib/x86_64-linux-gnu
#RUN echo $LD_LIBRARY_PATH

CMD freeradius -X

