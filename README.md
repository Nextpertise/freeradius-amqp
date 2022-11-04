# freeradius-amqp
Freeradius with AMQP module

How to start:
1. Checkout
```
cd depends
git clone git@github.com:FreeRADIUS/freeradius-server.git
git checkout 0da4faf84733e276bf5265c2d72c807996766a37
```

2. Build depends / packages
```
./build-all.sh
```

3. Build freeradius runtime container
```
cd freeradius-runtime
./build.sh
```

4. Build consumer
```
cd data/python
./build.sh
```

5. Start the whole shebang:
```
docker-compose up -d

# Set up exchange/queue
docker exec -it app /app/appinit.py

# Start consumer
docker exec -it app /app/worker.py
```

6. Execute radius request:
```
docker exec -it freeradius /etc/freeradius/test-radius.sh
```

7. Stop?
```
docker-compose down
```

## Radperf 

Using radperf to load-test:
```
docker exec -it freeradius bash -c 'echo "User-Name=2408LH11,User-Password=password,Framed-Protocol=PPP,Calling-Station-Id=2401LH000110101" | /usr/sbin/radperf -c 10 localhost:1812 auth testing123'
```

## Commands / Cheatlist
```
docker exec -it rabbitmq rabbitmqctl list_queues
docker exec -it rabbitmq rabbitmqctl list_queues --vhost "/"
echo "User-Name=2408LH11,User-Password=password,Framed-Protocol=PPP,Calling-Station-Id=2401LH000110101" | radperf -c 10 localhost:1812 auth testing123
```
