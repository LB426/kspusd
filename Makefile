CC = gcc
CFLAGS = -std=c99 -std=gnu99 -g -Og -D_REENTRANT -Wall `pkg-config --cflags glib-2.0` -I/home/bliz/lib/include
LIBS = `pkg-config --libs glib-2.0` -L/home/bliz/lib/lib -lssh -lssh_threads -lcrypto -lz -larchive -lcurl -lpthread
CLEANFILES = core core.* *.core *.o temp.* typescript* \
	*.lc *.lh *.bsdi *.sparc *.uw
BUILD_DATE = $(shell date +"%Y-%m-%d_%H:%M:%S")
PROGS = kspusd

.PHONY: all clean install uninstall

all: ${PROGS} 

clean:
	rm -rf ${PROGS} ${CLEANFILES}

inotifyevt.o: inotifyevt.c
	${CC} ${CFLAGS} -c inotifyevt.c -o inotifyevt.o

log.o: log.c
	${CC} ${CFLAGS} -c log.c -o log.o

myutils.o: myutils.c
	${CC} ${CFLAGS} -c myutils.c -o myutils.o

loadcfg.o: loadcfg.c
	${CC} ${CFLAGS} -c loadcfg.c -o loadcfg.o

totar.o: totar.c
	${CC} ${CFLAGS} -c totar.c -o totar.o

togz.o: togz.c
	${CC} ${CFLAGS} -c togz.c -o togz.o

md5calc.o: md5calc.c
	${CC} ${CFLAGS} -c md5calc.c -o md5calc.o

toregspus.o: toregspus.c
	${CC} ${CFLAGS} -c toregspus.c -o toregspus.o

mknewname.o: mknewname.c
	${CC} ${CFLAGS} -c mknewname.c -o mknewname.o

sendemail.o: sendemail.c
	${CC} ${CFLAGS} -c sendemail.c -o sendemail.o

main.o: main.c
	${CC} ${CFLAGS} -DVERSION='"${BUILD_DATE}"' -c main.c -o main.o

kspusd: inotifyevt.o log.o myutils.o loadcfg.o totar.o togz.o md5calc.o \
        toregspus.o mknewname.o sendemail.o main.o
	${CC} ${CFLAGS} -o kspusd main.o log.o md5calc.o togz.o myutils.o totar.o \
	                   toregspus.o mknewname.o inotifyevt.o loadcfg.o \
										 sendemail.o ${LIBS}

install:
	install -d /usr/local/kspus
	install --backup=numbered ./init.d/$(PROGS) /etc/init.d/$(PROGS)
	install --backup=numbered ./$(PROGS) /usr/local/kspus/$(PROGS)

uninstall:
	rm -rf /usr/local/kspus/$(PROGS)
