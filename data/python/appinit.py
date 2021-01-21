#!/usr/bin/env python
import pika
import time

rabbitmq_host = "rabbitmq"
rabbitmq_port = 5672
rabbitmq_user = "radiustest"
rabbitmq_password = "radiustest"
rabbitmq_virtual_host = "/"
rabbitmq_send_exchange = "fr.authorize" 
rabbitmq_rcv_exchange = "fr.authorize"
rabbitmq_rcv_queue = "queue_radius"
rabbitmq_rcv_key = "rlm_amqp_authorize"

# The binding area
credentials = pika.PlainCredentials(rabbitmq_user, rabbitmq_password)
connection = pika.BlockingConnection(pika.ConnectionParameters(rabbitmq_host, rabbitmq_port, rabbitmq_virtual_host, credentials))
channel = connection.channel()

# Create queue if not exist
channel.exchange_declare(
	exchange='fr.authorize',
	exchange_type='direct',
	passive=False,
	durable=True,
	auto_delete=False
)
channel.queue_declare(queue=rabbitmq_rcv_queue, auto_delete=False)
channel.queue_bind(queue=rabbitmq_rcv_queue, exchange=rabbitmq_rcv_exchange, routing_key=rabbitmq_rcv_key)
channel.basic_publish(
	exchange=rabbitmq_send_exchange, routing_key=rabbitmq_rcv_key,
	body="hallo", mandatory=False
)
#channel.queue_bind(exchange=rabbitmq_rcv_exchange, queue=rabbitmq_rcv_queue, routing_key=rabbitmq_rcv_key)

