#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os, errno, io, platform, re, sys, shutil, logging, socket, subprocess

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
transferpy = u'%s\\transfer.py' % basedisk
execrunbatpy = u'%s\\execRunBat.py' % basedisk
cdrgenpy = u'%s\\cdrgen.py' % basedisk

if os.path.exists(basedir):
	shutil.rmtree(basedir)
if os.path.exists(transferpy):
	os.remove(transferpy)
if os.path.exists(execrunbatpy):
	os.remove(execrunbatpy)
if os.path.exists(cdrgenpy):
	os.remove(cdrgenpy)

cmd = u'schtasks /Delete /F /TN "создание CDR файлов"'
cmd = cmd.encode('cp1251')
os.system(cmd)

cmd = u'schtasks /Delete /F /TN "передача CDR файлов"'
cmd = cmd.encode('cp1251')
os.system(cmd)