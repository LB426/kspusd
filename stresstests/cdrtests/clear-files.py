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

for root, dirnames, filenames in os.walk(basedir):
	for fn in filenames:
		if(re.search("^.*\.bz2$", fn)):
			file_path = os.path.join(root, fn)
			os.remove(file_path)
			print u'removed %s' % file_path
		if(re.search("^.*\.ama$", fn)):
			file_path = os.path.join(root, fn)
			os.remove(file_path)
			print u'removed %s' % file_path

for root, dirnames, filenames in os.walk(basedir):
	for dn in dirnames:
		if(re.search("^tmp.*$", dn)):
			dnf = os.path.join(root, dn)
			shutil.rmtree(dnf.encode(sys.getfilesystemencoding(), "r"))
