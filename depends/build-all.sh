#!/bin/bash

docker build -f Dockerfile -t freeradius-amqp-depends:latest $@ .
docker run  -v "$(pwd)/../packages:/build" -it freeradius-amqp-depends:latest
