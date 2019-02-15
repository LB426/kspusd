#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, errno, io, platform, re, sys, shutil, logging, socket, subprocess
from datetime import datetime

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
userlist = u'users.txt'

hostname = socket.gethostname()
ip = socket.gethostbyname(hostname)
userlist = u'users_%s.txt' % ip

if not os.path.exists(userlist):
	print u'файл со списком пользователей отсутствует %s' % userlist
	sys.exit(0)

if not os.path.exists(basedir):
	os.makedirs(basedir)
	
with open(userlist) as f:
	content = f.readlines()
	content = [x.strip() for x in content]
for line in content:
	a = line.split()
	login = a[0].decode('UTF-8')
	passw = a[1].decode('UTF-8')
	indir = os.path.join(basedir, login, u'Входящие CDR файлы')
	ardir = os.path.join(basedir, login, u'Архив CDR файлов')
	print "login: %s , password: %s" % (login,passw)
	if not os.path.exists(indir):
		os.makedirs(indir)
	if not os.path.exists(ardir):
		os.makedirs(ardir)
	runfile = os.path.join(basedir, login, u'run.bat')
	f = io.open(runfile, mode='w', encoding='cp866')
	f.write(r'@SET CDR_FILE_SOURCE_DIR=%s%s' % (indir, u'\r\n'))
	f.write(r'@SET CDR_FILE_ARCHIVE_DIR=%s%s' % (ardir, u'\r\n'))
	f.write(r'@SET CDR_COLLECTOR_MAIN_IP=%s%s' % (u'10.152.164.6', u'\r\n'))
	f.write(r'@SET CDR_COLLECTOR_RSRV_IP=%s%s' % (u'10.152.46.120', u'\r\n'))
	f.write(r'@SET CDR_COLLECTOR_LOGIN=%s%s' % (login, u'\r\n'))
	f.write(r'@SET CDR_COLLECTOR_PASWD=%s%s' % (passw, u'\r\n'))
	f.write(r'@SET CDR_FILENAME_PREFIX=%s%s' % (login.replace('k',''), u'\r\n'))
	f.write(r'@SET CDR_FILENAME_EXT=%s%s' % (u'ama', u'\r\n'))
	f.write(u'\r\n')
	f.write(r'c:\Python27\python.exe %s%s' % (transferpy, u'\r\n'))
	f.close()

shutil.copy(u'transfer.py',transferpy)
shutil.copy(u'execRunBat.py',execrunbatpy)
shutil.copy(u'cdrgen.py',cdrgenpy)

cmd = u'schtasks /create /ru admin /rp password /sc hourly /tn "создание CDR файлов" /tr %s /st 12:00:00' % cdrgenpy
cmd = cmd.encode('cp1251')
os.system(cmd)
cmd = u'schtasks /create /ru admin /rp password /sc hourly /tn "передача CDR файлов" /tr %s /st 12:30:00' % execrunbatpy
cmd = cmd.encode('cp1251')
os.system(cmd)
