import mqttpub
import time
import random
topic = "guitar/stroke"
# topic = "guitar/code"
codes = ["a","c"]
client = mqttpub.publish_init()
while True:
    val = random.randint(0,1)
    # mqttpub.publish(client,topic,codes[val])
    mqttpub.publish(client,topic,val)
    time.sleep(2)