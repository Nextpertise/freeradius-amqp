#!/bin/bash

cp -R ../packages/ .
docker build -t freeradius-amqp-runtime:latest $@ .

