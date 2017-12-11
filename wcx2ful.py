#!/usr/bin/python3
import os 

def getSize(filename):
	st = os.stat(filename)
	return st.st_size

for fn in os.listdir('.'):
	if os.path.isfile(fn):
		if getSize(fn) == 264:
			with open(fn, 'rb') as f:
				payload = b"0" * 520
				payload += f.read()
				with open(fn[:4] + '.wcxfull', 'wb') as fw:
					fw.write(payload)