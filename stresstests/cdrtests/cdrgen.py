#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os, errno, io, platform, re, sys, shutil, logging, socket, subprocess
import datetime, random

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
	for dn in dirnames:
		if(re.search(u'Входящие CDR файлы', dn)):
			indir = os.path.join(root, dn)
			print indir
			login = indir.split(u'\\')[2]
			counter1 = 3
			while True:
				fsize = random.randint(5*1024, 800*1024)
				datestring = datetime.datetime.strftime(datetime.datetime( \
					random.randint(2000, 2015), \
					random.randint(1, 12), \
					random.randint(1, 28), \
					random.randrange(23), \
					random.randrange(59), \
					random.randrange(59), \
					random.randrange(1000000)), '%Y%m%d%H%M%S')
				cdrfn = u'%s%s.ama' % (login.replace('k',''),datestring)
				filepath = os.path.join(indir,cdrfn)
				f = open(filepath, 'w')
				while True:
					dt = u'20170101010101'
					ac = u'%s%s%s%s%s' % (login[1],login[2],login[3],login[4],login[5])
					ph = u'41256'
					line = u'%s %s%s%s' % (dt,ac,ph,u'\r\n')
					f.write(line)
					fsize = fsize - 25
					if fsize < 0:
						break
				f.close()
				counter1 = counter1 - 1
				if counter1 <= 0:
					break
			counter1 = 3
			while True:
				fsize = random.randint(1*1024, 10*1024)
				datestring = datetime.datetime.strftime(datetime.datetime( \
					random.randint(2000, 2015), \
					random.randint(1, 12), \
					random.randint(1, 28), \
					random.randrange(23), \
					random.randrange(59), \
					random.randrange(59), \
					random.randrange(1000000)), '%Y%m%d%H%M%S')
				cdrfn = u'%s%s.ama' % (login.replace('k',''),datestring)
				filepath = os.path.join(indir,cdrfn)
				f = open(filepath, 'w')
				while True:
					dt = u'20170202020202'
					ac = u'%s%s%s%s%s' % (login[1],login[2],login[3],login[4],login[5])
					ph = u'41535'
					line = u'%s %s%s%s' % (dt,ac,ph,u'\r\n')
					f.write(line)
					fsize = fsize - 25
					if fsize < 0:
						break
				f.close()
				counter1 = counter1 - 1
				if counter1 <= 0:
					break