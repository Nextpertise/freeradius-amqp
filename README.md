# freeradius-amqp
Freeradius with AMQP module

How to start:
1. Build freeradius container:
```
cd src
./build.sh
```

2. Build consumer
```
cd data/python
./build.sh
```

3. Start the whole shebang:
```
docker-compose up -d

# Set up exchange/queue
docker exec -it app /app/appinit.py

# Start consumer
docker exec -it app /app/worker.py
```

4. Execute radius request:
```
docker exec -it freeradius /usr/local/etc/raddb/test-radius.sh
```

5. Stop?
```
docker-compose down
```

## Commands / Cheatlist
```
docker exec -it rabbitmq rabbitmqctl list_queues
docker exec -it rabbitmq rabbitmqctl list_queues --vhost "/"
```
