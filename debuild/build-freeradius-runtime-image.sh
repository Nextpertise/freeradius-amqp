#!/bin/bash

docker build -f Dockerfile-freeradius-runtime-image -t freeradius-amqp-runtime:latest $@ .
