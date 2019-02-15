#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os, errno, io, platform, re, sys, shutil, logging, socket, subprocess

print u'PROGRAM START'

basedisk = u'c:'
batcmd = u'vol D:'
result = subprocess.check_output(batcmd, shell=True)
result = result.decode('cp866')
if(re.search(u'Серийный номер тома', result)):
	basedisk = u'd:'
if(re.search(u'Volume Serial Number is', result)):
	basedisk = u'd:'	
print u'basedisk = %s' % basedisk

basedir = u'%s\\_fakeStations' % basedisk

for root, dirnames, filenames in os.walk(basedir):
	for fn in filenames:
		if(re.search(u"run.bat", fn)):
			runbat = os.path.join(root, fn)
			cmd = u'start cmd.exe /c %s > NUL' % runbat
			print cmd
			os.system(cmd)

print u'PROGRAM END'