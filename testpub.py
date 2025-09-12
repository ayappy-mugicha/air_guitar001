import mqttpub
import time
import random
# topic = "guitar/stroke"
topic = "guitar/code"
codes = ["a","c","g","d","f"]
client = mqttpub.publish_init()
while True:
    # val = random.randint(0,1)
    val = random.randint(0,len(codes))
    mqttpub.publish(client,topic,codes[val-1])
    # mqttpub.publish(client,topic,val)
    time.sleep(2)