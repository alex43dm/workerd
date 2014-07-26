#!/usr/bin/python
# encoding: utf-8
try:
        from amqplib import client_0_8 as amqp
except:
        print("AMQP modyle not installed")
        print("sudo apt-get install python-pip")
        print("sudo pip install amqplib")

class MQ():

    def _get_channel(self):
        conn = amqp.Connection(host='localhost:5672', userid='guest', password='guest', virtual_host="/", insist=False)
        ch = conn.channel()
        ch.exchange_declare(exchange="indexator", type="topic", durable=False, auto_delete=True)
        return ch

    def offer_start(self, offer_Id, campaign_id):
        ch = self._get_channel()
        msg = 'Offer:%s,Campaign:%s' % (offer_Id, campaign_id)
        msg = amqp.Message(msg)
        ch.basic_publish(msg, exchange='getmyad', routing_key='advertise.start')
        ch.close()

    def offer_update(self, offer_Id, campaign_id):
        ch = self._get_channel()
        msg = 'Offer:%s,Campaign:%s' % (offer_Id, campaign_id)
        msg = amqp.Message(msg)
        ch.basic_publish(msg, exchange='getmyad', routing_key='advertise.update')
        ch.close()

    def offer_delete(self, offer_Id, campaign_id):
        ch = self._get_channel()
        msg = 'Offer:%s,Campaign:%s' % (offer_Id, campaign_id)
        msg = amqp.Message(msg)
        ch.basic_publish(msg, exchange='getmyad', routing_key='advertise.delete')
        ch.close()

    def campaign_start(self, campaign_id):
        ''' Отправляет уведомление о запуске рекламной кампании ``campaign_id`` '''
        ch = self._get_channel()
        msg = amqp.Message(campaign_id)
        ch.basic_publish(msg, exchange='getmyad', routing_key='campaign.start')
        ch.close()

    def campaign_stop(self, campaign_id):
        ''' Отправляет уведомление об остановке рекламной кампании ``campaign_id`` '''
        ch = self._get_channel()
        msg = amqp.Message(campaign_id)
        ch.basic_publish(msg, exchange='getmyad', routing_key='campaign.stop')
        ch.close()

    def campaign_update(self, campaign_id):
        ''' Отправляет уведомление об обновлении рекламной кампании ``campaign_id`` '''
        ch = self._get_channel()
        msg = amqp.Message(campaign_id)
        ch.basic_publish(msg, exchange='getmyad', routing_key='campaign.update')
        ch.close()

    def informer_update(self, informer_id):
        ''' Отправляет уведомление о том, что информер ``informer_id`` был изменён '''
        ch = self._get_channel()
        msg = amqp.Message(informer_id)
        ch.basic_publish(msg, exchange='getmyad', routing_key='informer.update')
        ch.close()

    def informer_rating_update(self, id):
        ''' Отправляет уведомление об изменении в аккаунте ``login`` '''
        ch = self._get_channel()
        msg = amqp.Message(id)
        ch.basic_publish(msg, exchange='getmyad', routing_key='informer.updateRating')
        ch.close()


mq = MQ()
#mq.campaign_start("90c8d8a7-5f97-44a5-9497-e5b402eeacdf");
#mq.informer_update("b950c64c-34a6-11e3-a616-00e081bad46b");
mq.offer_update("bc598212-f38d-4818-be72-85dbe2a023a1","90c8d8a7-5f97-44a5-9497-e5b402eeacdf");
#mq.informer_rating_update("773540683");
