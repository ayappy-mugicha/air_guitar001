import mqttpub
import time
import random
topic1 = "guitar/stroke"
topic2 = "guitar/code"
# codes = ["am","c","dm","d","f"]
codes = ["c","d","e","f","g","a","em","am","dm","bm"]
client = mqttpub.publish_init()
while 1:
    for i in range(len(codes)):
        val = random.randint(0,1)
        # val = random.randint(0,len(codes))
        mqttpub.publish(client,topic2,codes[i])
        # mqttpub.publish(client,topic1,val)
        time.sleep(2)