# freeradius-amqp
Freeradius with AMQP module

## Commands / Cheatlist
```
docker exec -it rabbitmq | jq -r . | grep rabbitmq) rabbitmqctl list_queues
docker exec -it rabbitmq | jq -r . | grep rabbitmq) rabbitmqctl list_queues --vhost "/"
docker exec -it app /app/appinit.py
docker exec -it app /app/worker.py
docker exec -it freeradius ls /usr/local/etc/radddb/test-radius.sh
```
