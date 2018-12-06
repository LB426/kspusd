#!/usr/bin/python
import sys
import time
import datetime
import os
import ftplib

ip = "10.152.46.122"
login = "k86196001"
passwd = "mypassword"
srcdir = "/home/kspus/" + login

def sendonspusftp(fsrc, fdst):
    try:
        ftp = ftplib.FTP(ip, login, passwd)
        stor = 'STOR ' + fdst
        ftp.storbinary(stor, open(fsrc,'rb'))
        ftp.quit()
        now = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(now + " file " + fsrc + " send to ftp " + 
              login + "@" + ip + ":" + fdst + " success ")
        return True
    except ftplib.all_errors, e:
        msg = "spus ftp error: %s" %e
        print(msg)
        return False

def processcdrfile():
    for r,d,fs in os.walk(srcdir):
        for f in fs:
            fsrc = srcdir + "/" + f
            fdst = "/in/" + f
            sendonspusftp(fsrc, fdst)

counter = 1
while True:
    sf = open("/mnt/start.txt", "r")
    lines = sf.readlines()
    sf.close()
    flag = lines[0].replace("\n", "")
    if flag == "START":
        date_string = lines[1]
        date_string = date_string.replace("\n", "")
        time_xx = datetime.datetime(*time.strptime(date_string, '%Y-%m-%d %H:%M:%S')[:6])
        now = datetime.datetime.now()
        if time_xx < now:
            processcdrfile()
            print "sleep on " + lines[2].replace("\n", "") + " sec"
            time.sleep(int(lines[2].replace("\n", "")))
        else:
            sys.stdout.write(".")
            sys.stdout.flush()       
            time.sleep(1)
        print "counter: " + str(counter)
        counter = counter + 1
    else:
        print "flag: " + flag
        break

