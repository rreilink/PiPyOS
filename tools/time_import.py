import time
# 2 sec for no compression
def test():
    t0 = time.monotonic()
    import os
    t1 = time.monotonic()
    return t1 - t0

def test2():
    t0 = time.monotonic()
    a=open('/sd/python36z.zip','rb').read()
    t1 = time.monotonic()
    return t1 - t0
    