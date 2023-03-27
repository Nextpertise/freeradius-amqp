#!/bin/bash

echo "User-Name=nextpertise,User-Password=password,Framed-Protocol=PPP,Calling-Station-Id=2408ZE10" | /usr/bin/radclient localhost:1812 auth testing123 -x
