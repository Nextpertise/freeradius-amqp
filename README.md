# freeradius-amqp
Freeradius with AMQP module

## How to build:

1. Clone repository in your project directory
```
git clone git@github.com:Nextpertise/freeradius-amqp.git
cd freeradius-amqp
```

2. Build Docker image
```
cd build
./build.sh
```

3. Build consumer
```
cd data/python
./build.sh
```

4. Start the whole shebang:
```
docker-compose up -d

# Set up exchange/queue
docker exec -it app /app/appinit.py

# Start consumer
docker exec -it app /app/worker.py
```

5. Check radius logs
```
docker logs -f freeradius
```

6. Execute radius request:
```
docker exec -it freeradius /etc/freeradius/test-radius.sh
```

7. Stop?
```
docker-compose down
```

## Commands / Cheatlist
```
# Rabbitmq
docker exec -it rabbitmq rabbitmqctl list_queues
docker exec -it rabbitmq rabbitmqctl list_queues --vhost "/"

# Single test
echo "User-Name=2408LH11,User-Password=password,Framed-Protocol=PPP,Calling-Station-Id=2401LH000110101" | radperf -c 10 localhost:1812 auth testing123

# Radperf
docker exec -it freeradius bash -c 'echo "User-Name=2408LH11,User-Password=password,Framed-Protocol=PPP,Calling-Station-Id=2401LH000110101" | /usr/sbin/radperf -c 10 localhost:1812 auth testing123'
```

