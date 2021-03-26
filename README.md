# freeradius-amqp
Freeradius with AMQP module

How to start:
1. Build depends / packages
```
cd depends
./build-all.sh
```

1. Build freeradius runtime container
```
cd freeradius-runtime
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

4. Execute radius request:
```
docker exec -it freeradius /etc/freeradius/test-radius.sh
```

5. Stop?
```
docker-compose down
```

## Commands / Cheatlist
```
docker exec -it rabbitmq rabbitmqctl list_queues
docker exec -it rabbitmq rabbitmqctl list_queues --vhost "/"
echo "User-Name=2408LH11,User-Password=password,Framed-Protocol=PPP,Calling-Station-Id=2401LH000110101" | radperf -c 10 localhost:1812 auth testing123
```
