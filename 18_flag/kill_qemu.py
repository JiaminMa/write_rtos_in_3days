#!/usr/bin/python
import os

ret = os.popen("ps aux | grep qemu")
line = ret.read().split(" ")
print line
kill_cmd = "kill -9 {}".format(line[1])
os.system(kill_cmd)
