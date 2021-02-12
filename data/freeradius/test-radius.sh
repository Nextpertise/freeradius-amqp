#!/bin/bash

echo "User-Name=2408LH11,User-Password=password,Framed-Protocol=PPP,Calling-Station-Id=2401LH000110101" | /usr/bin/radclient localhost:1812 auth testing123 -x
