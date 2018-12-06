#include <libssh/libssh.h>

#ifndef KSPARAM_H
#define KSPARAM_H

struct _email
{
  char *addr1;
  char *addr2;
  char *addr3;
  int lines_read;
  char *email[8];
};

/* описание массива указателей *email[8] из структуры _email 
 * смотри в файле sendemail.c */

typedef struct _email email;

struct _ksparam
{
  char *cfg; /* полние имя файла kspus.cfg */
  int wd; /* inotify дескриптор наблюдателя за kspus.cfg */
  /* под эти указатели память выделяется библиотекой glib key-value file parser */
  char *indir;
  char *outdir;
  char *errdir;
  char *archivedir;
  char *locallogin;
  char *localgroup;
  char *remotelogin;
  char *ipregspusmain;
  char *ipregspusbackup;
  char *pubkeyfn;
  char *privkeyfn;
  char *atstype;
  char *regcode;
  char *cdrprefix;
  char *atsnum;
  char *logfn;
  char *zz;
  char *rmtindir;
  char *rmtcmd;
  char *filelog;
  char *syslog;
  char *stdoutlog;
  /* под следующие указатели выделяется память в loadcfg.c fparams() */
  char *availsrv;
  char *infnm;  /* имя файла в in без пути */
  char *outfnm; /* имя файла переименованного по стандарту */
  char *gzfnm;  /* имя файла сжатого в GZ */
  char *md5fnm; /* имя файла содержащего MD5sum gzfnm */
  char *tarfnm; /* имя файла TAR */
  char *md5str; /* MD5 сумма файла */
  ssh_session my_ssh_session;
  ssh_key pubkey;
  ssh_key privkey;
  int namelen;
  email errmail;
  int error_handler_timeout;
  pthread_mutex_t mtx_dir_lock;
};

typedef struct _ksparam ksparam;

#endif
