import time
import subprocess
import os
from os import listdir
from os.path import isfile, join

path = "test"
files = [ f for f in listdir(path) if isfile(join(path,f)) ]

os.chdir(path)

for f in files:

  if f[:4] == "pass":
    print 'TEST:',f
    p = subprocess.Popen(("../emulator","-r",f),stdout=subprocess.PIPE)
    p.wait();
    assert p.returncode == 0
    print "PASS:", f
  elif f[:4] == "fail":
    print 'TEST:',f
    p = subprocess.Popen(("../emulator","-r",f),stdout=subprocess.PIPE)
    p.wait();
    assert p.returncode == 1
    print "PASS:", f

before = time.time()
p = subprocess.Popen(("../emulator","-r","perfo.test"),stdout=subprocess.PIPE)
p.wait()
assert p.returncode == 0
after = time.time()
print 'Performance: 1 million instructions in:', after-before, 'seconds'
