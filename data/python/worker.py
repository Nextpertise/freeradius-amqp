#!/usr/bin/env python
import pika, sys, os

rabbitmq_host = "rabbitmq"
rabbitmq_port = 5672
rabbitmq_user = "radiustest"
rabbitmq_password = "radiustest"
rabbitmq_virtual_host = "/"
rabbitmq_send_exchange = "fr.authorize" 
rabbitmq_rcv_exchange = "fr.authorize"
rabbitmq_rcv_queue = "queue_radius"
rabbitmq_rcv_key = "rlm_amqp_authorize"

def main():
    credentials = pika.PlainCredentials(rabbitmq_user, rabbitmq_password)
    connection = pika.BlockingConnection(pika.ConnectionParameters(rabbitmq_host, rabbitmq_port, rabbitmq_virtual_host, credentials))
    channel = connection.channel()

    def callback(ch, method, properties, body):
        print(" [x] Received %r" % body)

    channel.basic_consume(queue=rabbitmq_rcv_queue, on_message_callback=callback, auto_ack=True)

    print(' [*] Waiting for messages. To exit press CTRL+C')
    channel.start_consuming()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print('Interrupted')
        try:
            sys.exit(0)
        except SystemExit:
            os._exit(0)
