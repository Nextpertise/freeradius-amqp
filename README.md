# freeradius-amqp
Freeradius with AMQP module

## Commands / Cheatlist
```
docker exec -it rabbitmq rabbitmqctl list_queues
docker exec -it rabbitmq rabbitmqctl list_queues --vhost "/"
docker exec -it app /app/appinit.py
docker exec -it app /app/worker.py
docker exec -it freeradius /usr/local/etc/raddb/test-radius.sh
```
