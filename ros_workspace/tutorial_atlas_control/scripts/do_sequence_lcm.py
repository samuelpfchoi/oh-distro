#!/usr/bin/python
import roslib; roslib.load_manifest('tutorial_atlas_control')
import rospy, yaml, sys
import csv_parse
import carry_out_sequence 

import os,sys
import lcm
import time
from lcm import LCM
import math
import csv_parse
import carry_out_sequence 


home_dir =os.getenv("HOME")
sys.path.append(home_dir + "/drc/software/build/lib/python2.7/site-packages")
sys.path.append(home_dir + "/drc/software/build/lib/python2.7/dist-packages")

from drc.utime_t import utime_t


def on_utime(channel, data):
  print "... a giant leap for robotkind"
  carry_out_sequence.carry_out_sequence("projectial_ingress.csv", 0)

####################################################################
lc = lcm.LCM()
print "started"
sub1 = lc.subscribe("PROJECTILE_LEAP", on_utime)
while True:
  lc.handle()

lc.unsubscribe(sub)
