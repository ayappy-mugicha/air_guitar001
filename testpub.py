import mqttpub
import time
import random
# topic = "guitar/stroke"
topic = "guitar/code"
client = mqttpub.publish_init()
while True:
    val = random.randint(0,1)
    mqttpub.publish(client,topic,val)
    # mqttpub.publish("0")
    time.sleep(2)