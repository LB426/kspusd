#!/usr/bin/env python
# -*- coding: utf-8 -*-
###############################################################################
# FILENAME : tranfer.py
# DESCRIPTION : передача CDR файлов АТС ТФОП на FTP сервер
# NOTES : тестировано на Windows XP Python 2.7.14
# NOTES : этот скрипт передаёт все файлы из каталога srcdir имя которых 
# NOTES : соответствует шаблону cdrfnmtemplate
# NOTES : one prefix CDR file
# AUTHOR : Andrey K. 
# CHANGES :
# USAGE : python transfer.py 
# VERSION : 2.0.0
###############################################################################
from __future__ import unicode_literals
import sys
import re
import socket
import errno
import ftplib
import os
import shutil
import logging
import bz2
from datetime import datetime

try:
	slogin = os.environ['CDR_COLLECTOR_LOGIN']
	srcdir = os.environ['CDR_FILE_SOURCE_DIR']
	archdir = os.environ['CDR_FILE_ARCHIVE_DIR']
	sipmain = os.environ['CDR_COLLECTOR_MAIN_IP']
	siprsrv = os.environ['CDR_COLLECTOR_RSRV_IP']
	spasswd = os.environ['CDR_COLLECTOR_PASWD']
	fnprefix = os.environ['CDR_FILENAME_PREFIX']
	fnext = os.environ['CDR_FILENAME_EXT']
except KeyError:
	print u'проверьте переменные среды Windows:'
	print u'CDR_COLLECTOR_LOGIN - логин FTP сервера'
	print u'CDR_FILE_SOURCE_DIR - путь к директории с CDR файлами'
	print u'CDR_FILE_ARCHIVE_DIR - путь к директории где будет находиться архив'
	print u'CDR_COLLECTOR_MAIN_IP - ip адрес главного FTP сервера'
	print u'CDR_COLLECTOR_RSRV_IP - ip адрес резервного FTP сервера'
	print u'CDR_COLLECTOR_PASWD - пароль FTP сервера'
	print u'CDR_FILENAME_PREFIX - неизменяющиеся символы с начала имени файла'
	print u'CDR_FILENAME_EXT - расширение файла без точки'
	sys.exit(1)

logfile = "%s.log" % (slogin)              # имя лог файла
################# формирую шаблон имени файла ##################################
# если нет и префикса и суффикса
if fnprefix == u'*' and fnext == u'*':
	cdrfnmtemplate = "^.*$".format(fnprefix,fnext)
# если только суффикс
if fnprefix == u'*' and fnext != u'*':
	cdrfnmtemplate = "^(.*)(\.{0})$".format(fnext)
# если только префикс
if fnprefix != u'*' and fnext == u'*':
	cdrfnmtemplate = "^({0})(.*)$".format(fnprefix)
# если есть и префикс и суффикс
if fnprefix != u'*' and fnext != u'*':
	cdrfnmtemplate = "^({0})(.*)(\.{1})$".format(fnprefix,fnext)
################# конец шаблона ################################################
ftptimeout = 5

# кодирую в юникод
archdir = archdir.decode('cp1251')
srcdir = srcdir.decode('cp1251')

# настройка логгера
logfn = os.path.join(archdir, logfile)
if not os.path.exists(archdir):
	print u'не могу создать файл %s' % (logfn)
	sys.exit(1)
else:
	print u'лог файл %s' % (logfn)
logger = logging.getLogger('cdr_transfer_script')
logger.setLevel(logging.DEBUG)
logfh = logging.FileHandler(logfn)
logfh.setLevel(logging.DEBUG)
logch = logging.StreamHandler()
logch.setLevel(logging.DEBUG)
formatter = logging.Formatter(fmt='%(asctime)s %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
logfh.setFormatter(formatter)
logch.setFormatter(formatter)
logger.addHandler(logfh)
logger.addHandler(logch)

def compress(fsrc, fdst):
	f_in = open(fsrc, 'rb')
	bz2file = fdst + '.bz2'
	f_out = bz2.BZ2File(bz2file, 'wb', compresslevel=9)
	f_out.writelines(f_in)
	f_out.close()
	f_in.close()
	os.remove(fsrc)
	logger.info(u'Файл %s сжат в файл %s' % (fsrc, bz2file))

def getreadyftp(ip1,ip2):
	readyip = ""
	flag = 0
	ftp = ftplib.FTP()
	try:
		ftp.connect(ip1, timeout=ftptimeout)
		readyip = ip1
		ftp.quit()
	except ftplib.all_errors, e:
		logger.info(u"Не могу соединиться с главным FTP %s Error: %s" %
					(ip1, e))	
		flag = 1
	if(flag == 1):
		try:
			ftp.connect(ip2, timeout=ftptimeout)
			readyip = ip2
			ftp.quit()
		except ftplib.all_errors, e:
			logger.info(u"Не могу соединиться с резервным FTP %s Error: %s" %
						(ip2, e))
	return readyip

# проверяю существует ли каталог источник CDR файлов
if not os.path.exists(srcdir):
	logger.info(u'каталог с CDR файлами не существует. Exit.')
	sys.exit(1)
# создание архивного каталога для локального архива
if not os.path.exists(archdir):
	os.makedirs(archdir)
# проверяю не заняты ли файлы другой программой
for f in os.listdir(srcdir):
	src = os.path.join(srcdir, f)
	if os.path.isfile(src):
		try:
			os.rename(src, src)
		except OSError as e:
			logger.info(u'файл %s занят другим приложением, err: %d %s' % (src, 
			            e.errno, unicode(e.strerror,'cp1251')) )
			sys.exit(1)
	
logger.info(u'НАЧАЛО ПРОГРАММЫ')
###############################################################################
# создание временного каталога, если каталог существует он переименовывается
# для того чтобы избежать потери CDR файлов
# переименованные tmpdir нужно периодически удалять
###############################################################################
tmpdir = os.path.join(archdir, u'tmp')
if not os.path.exists(tmpdir):
	os.makedirs(tmpdir)
	logger.info(u'временный каталог создан')
else:
	logger.info(u"директория %s существует" % (tmpdir))
	tmpbkp = os.path.join(archdir, datetime.now().strftime("tmp%Y-%m-%d_%H-%M-%S"))
	shutil.move(tmpdir, tmpbkp)
	os.makedirs(tmpdir)
	logger.info(u'переместил %s в %s' % (tmpdir,tmpbkp))
###############################################################################
# начало блока создания списка имён готовых к передаче файлов
###############################################################################
ReadyList = []
############### АТС зависимый кусок ###########################################
for f in os.listdir(srcdir):
	if(re.search(cdrfnmtemplate,f)):
		ReadyList.append(f)
################# Конец АТС зависимого куска ##################################

if(len(ReadyList)>0):
	logger.info(u'есть %d файлов для передачи' % (len(ReadyList)))
	for f in ReadyList:
		src = os.path.join(srcdir, f)
		dst = os.path.join(tmpdir, f)
		shutil.copy(src,dst)
	logger.info(u'файлы скопированны во временный каталог')
else:
	logger.info(u'нет файлов для передачи')
	shutil.rmtree(tmpdir)
	logger.info(u'временный каталог удалён')
	logger.info(u'Exit.')
	sys.exit(0)
###############################################################################
# Конец блока создания списка имён файлов готовых к передаче файлов
###############################################################################

###############################################################################
# Начало блока отправки на FTP сервер
###############################################################################
try:
	readyip = getreadyftp(sipmain,siprsrv)
	if readyip == '':
		shutil.rmtree(tmpdir)
		logger.info(u'временный каталог удалён %s' % (tmpdir))
		logger.info(u'Не смог передать фалы на FTP. Выход из програмы.')
		sys.exit(1)
	ftp = ftplib.FTP(readyip,slogin,spasswd)
	banner = ftp.getwelcome()
	logger.info('Соединился с FTP %s %s' % (readyip, banner))	
	for f in ReadyList:
		src = os.path.join(srcdir, f)
		dst = 'STOR /in/' + f
		ftp.storbinary(dst, open(src,'rb'))
		logger.info(u'Файл отправлен успешно: %s' % (src))
	ftp.quit()
except ftplib.all_errors, e:
	logger.info(u'ошибки FTP в процессе отправки:%s %s' % (readyip,e))
	shutil.rmtree(tmpdir)
	logger.info(u'временный каталог удалён')
	logger.info(u'Преждевременное завершение работы. Exit.')
	sys.exit(1)
###############################################################################
# Конец блока отправки на FTP сервер
###############################################################################

###############################################################################
# Начало блока архивации файлов в локальный архив
###############################################################################
year = datetime.now().strftime("%Y")
month = datetime.now().strftime("%m_%B")
archdir2 = os.path.join(archdir, year, month)
if not os.path.exists(archdir2):
	os.makedirs(archdir2)
for f in ReadyList:
	src = os.path.join(tmpdir, f)
	dst = os.path.join(archdir2, f)
	compress(src,dst)
###############################################################################
# Конец блока архивации файлов в локальный архив
###############################################################################

###############################################################################
# Начало блока удаления исходных файлов
# Если программа добралась сюда значит можно говорить об успешной передаче
# на FTP и смело сжимать файлы в архив
###############################################################################
############### АТС зависимый кусок ###########################################
for f in ReadyList:
	src = os.path.join(srcdir, f)
	os.remove(src)
	srcok = os.path.join(src, u'ok') # сделано для AXE10
	if os.path.exists(srcok):        # которая формирует файл .ok
		os.remove(srcok)             # рядом с основным CDR
################# Конец АТС зависимого куска ##################################
logger.info(u'файлы источники удалены')
###############################################################################
# Конец блока удаления исходных файлов
###############################################################################

# удаление временного каталога
shutil.rmtree(tmpdir)
logger.info(u'временный каталог удалён')

logger.info(u'КОНЕЦ ПРОГРАММЫ.')
sys.exit(0)
