import sys
import time
import os
import ftplib
from Tkinter import *
from datetime import datetime

now=datetime.now()
datetime.strftime(datetime.now(), "%H:%M:%S")

root= Tk()
root.title ("Shedul")
root.geometry("200x100")
fra1= Frame(root,width=100,height=100,bg="darkred",bd=4)
fra1.pack()
scah = Spinbox(fra1,from_=0,to=24,width=2)
scam = Spinbox(fra1,from_=0,to=59,width=2)
scas = Spinbox(fra1,from_=0,to=59,width=2)
scah.grid(row=0,column=0)
scam.grid(row=0,column=2)
scas.grid(row=0,column=3)
fra2= Frame(fra1,width=10,height=15,bg="#000000",bd=4)
fra2.grid(row=0,column=4)
fra3= Frame(fra1,width=10,height=15,bg="#FFFFFF",bd=4)
fra3.grid(row=0,column=5)

def exi_me():
    now=datetime.now()
    datetime.strftime(datetime.now(), "%H:%M:%S")
    hh = scah.get()
    mm = scam.get()
    ss = scas.get()
    time_h = datetime.time(now)
    dd = hh+":"+mm+":"+ss
    time_xx = datetime.strptime(dd,"%H:%M:%S")
    time_x = datetime.time(time_xx)
    time_y = time_x < time_h
    print (time_y)
    print (time_x)
    print (time_h)

def sendonspusftp(fsrc, fdst):
    try:
        spusip = "10.152.46.122"
        spuslogin = "k86196001"
        spuspasswd = "mypassword"
        ftp = ftplib.FTP(spusip, spuslogin, spuspasswd)
        #print(ftp.getwelcome())
        stor = 'STOR ' + fdst
        #print("send from file " + fsrc + " to SPUS " + stor)
        ftp.storbinary(stor, open(fsrc,'rb'))
        ftp.quit()
        print("send file " + fsrc + " to ftp success.")
        return True
    except ftplib.all_errors, e:
        msg = "spus ftp error: %s" %e
        print(msg)
        return False

def exec1():
    srcdir = "C:/CDRFILE"
    print('start send file')
    for r,d,fs in os.walk(srcdir):
        for f in fs:
            fsrc = srcdir + "/" + f
            fdst = "/in/" + f
            sendonspusftp(fsrc, fdst)

def start_me():
    now=datetime.now()
    datetime.strftime(datetime.now(), "%H:%M:%S")
    hh = scah.get()
    mm = scam.get()
    ss = scas.get()
    i=0
    time_h = datetime.time(now)
    dd = hh+":"+mm+":"+ss
    time_xx = datetime.strptime(dd,"%H:%M:%S")
    time_x = datetime.time(time_xx)
    time_y = time_x < time_h
    print (time_y)
    print (time_x)
    print (time_h)
    while time_x > time_h:
            i=i+1
            sys.stdout.write('.')
            now=datetime.now()
            datetime.strftime(datetime.now(), "%H:%M:%S")
            time_h = datetime.time(now)
            time.sleep(1)
    print
    i=1
    while i <= 5:
        exec1()
        print "i=" + str(i)
        i=i+1
    
btn = Button (root,text="START1",comman=start_me)
btn.pack()
btn2 = Button (root,text="START2",comman=start_me)
btn2.pack()
btnq = Button (text="EXIT",command=exi_me)
btnq.pack()
root.mainloop()
