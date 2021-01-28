#!/bin/bash

docker build -f Dockerfile-build-image -t freeradius-amqp-build:latest $@ .
