/******************************************************************************
 * FILENAME : main.c
 * DESCRIPTION : главный файл, содержит точку входа - функцию main()
 * PUBLIC FUNCTIONS :
 * NOTES :
 * AUTHOR : Andrey K. Seredin, odissey@rambler.ru
 * CHANGES :
 * USAGE: sudo ./kspusd /kspus
 *        имя программы + полный путь по каталогам к базовой директории
 *        kspusd нужны права root для работы
 * ***************************************************************************/

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/inotify.h>
#include <sys/syscall.h>
#include <poll.h>
#include <fcntl.h>
#include <ftw.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <glib.h>
#include <dirent.h>
#include <libssh/libssh.h> 
#include <libssh/callbacks.h>
#include "main.h"
#include "ksparam.h"
#include "myutils.h"
#include "md5calc.h"
#include "togz.h"
#include "log.h"
#include "inotifyevt.h"
#include "loadcfg.h"
/* #include "wkspuscfg.h" */

#define RUNNING_DIR "/"
#define PID_FILE "/var/run/kspusd.pid"

const volatile static char version[] = VERSION;
char basedir[128] = {0};      /* каталог в котором домашние каталоги пользователей в которых лежат in out и т.д */
pthread_t* threads;
/* связанный список с параметрами прочитанными из kspus.cfg, объявленв loadcfg.c */
extern GPtrArray *pParams;
int pidfd; /* fid to PID_FILE */
int inotify_watcher = 0;
pthread_mutex_t inotify_watcher_lock;
pthread_t cfgwthrd;

int processOutDir(ksparam *p);

/* обрабатывает файл который появился в in */
int processInDir(ksparam *p)
{
  if(moveToOut(p) == -1)
  {
    log_error( p, "processFile() moveToOut() move file %s to out unsuccess", p->infnm);
    return -1;
  }
  if(creGZfile(p) == -1)
  {
    log_error( p, "processFile() creGZfile() file %s unsuccess", p->gzfnm);
    return -1;
  }
  if(creMD5file(p) == -1)
  {
    log_error( p, "processFile() creMD5file() file %s unsuccess", p->md5fnm);
    return -1;
  }
  if(creTARfile(p) == -1)
  {
    log_error( p, "processFile() creTARfile() file %s unsuccess", p->tarfnm);
    return -1;
  }
  
  return 0;
}

int processOutDir(ksparam *p)
{
  if(checkAvailSrv(p) == -1)
  {
    log_error(p,"processFile checkAvailSrv() connect to %s unsuccess",p->availsrv);
    return -1;
  }
  if(sendTarToRegSpus(p) == -1)
  {
    log_error(p,"processFile sendTarToRegSpus file %s unsuccess",p->tarfnm);
    return -1;
  }
  if(checkTarOnRegSpus(p) == -1)
  {
    log_error(p,"processFile checkTarOnRegSpus file %s unsuccess",p->tarfnm);
    return -1;
  }
  if(moveTarToArchive(p) == -1)
  {
    log_error(p,"processFile moveTarToArchive file %s unsuccess",p->tarfnm);
    return -1;
  }

  return 0;
}

void* ErrorHandler(ksparam *p)
{
  int cerrfs;
  struct dirent *entry;
  DIR *dir;

  /* проверяем err файлы */
  if(checkAvailSrv(p) == -1)
  {
    log_error(p,"main server %s and backup server %s not respond",
                p->ipregspusmain, p->ipregspusbackup);
  }
  else
  {
    cerrfs = getErrFiles(p);
    if(cerrfs == -1)
    {
      log_error(p,"main.c ErrorHandler() getErrFiles() "
                  "can't count ERR file on %s@%s",
                  p->remotelogin, p->availsrv);
    }
    if(cerrfs > 0)
    {
      log_info(p,"exist %d err files on %s", cerrfs, p->availsrv);
      execLocalErrDir(p);
    }
    if(cerrfs == 0)
    {
      log_info(p,"no err files on %s", p->availsrv);
    }
  }
  /* проверяем директорию ~/in */
  dir = opendir(p->indir);
  if(dir == NULL)
  {
    log_error(p,"main.c ErrorHandler() opendir() %s errno: %s", p->indir,
                 strerror(errno));
  }
  else
  {
    while( (entry = readdir(dir)) != NULL )
    {
      if(entry->d_type != DT_DIR)
      {
        if(strstr(entry->d_name,".in.") == NULL)
        {
          log_info(p, "IN exist unsent file %s/%s", p->indir, entry->d_name);
          strcpy(p->infnm, entry->d_name);
          processInDir(p);
        }
      }
    }
    closedir(dir);
  }
  /* проверяем директорию ~/out */
  dir = opendir(p->outdir);
  if(dir == NULL)
  {
    log_error(p,"main.c ErrorHandler() opendir() %s errno: %s", p->outdir,
                 strerror(errno));
  }
  else
  {
    while( (entry = readdir(dir)) != NULL )
    {
      if(entry->d_type != DT_DIR)
      {
        log_info(p, "OUT exist unsent file %s/%s", p->outdir, entry->d_name);
        strcpy(p->tarfnm, entry->d_name);
        processOutDir(p);
      }
    }
    closedir(dir);
  }

  return NULL;
}

/* поток который ждёт уведомления от inotify о появлении файла в in */
void* thread(void* thread_data)
{
  ksparam *kp = (ksparam*) thread_data;
  int inotifyFd, wd, pid, rc;
  unsigned long tid;
  int stopflag = 0;

  errno = 0;
  rc = 0;
  /* импортируем публичный ключ */
  rc = ssh_pki_import_pubkey_file( kp->pubkeyfn, &kp->pubkey );
  if (rc != SSH_OK)
  {
    if (rc == SSH_EOF)
      log_error(kp,"%s %d %s public key file doesn't exist or "
                "permission denied %s",__FILE__, __LINE__, __func__,
                kp->pubkeyfn);
    if (rc == SSH_ERROR)
      log_error(kp,"%s %d %s SSH_ERROR %s %s", __FILE__, __LINE__, __func__,
                kp->pubkeyfn, ssh_get_error(kp->my_ssh_session));
    return NULL;
  }
  log_info(kp,"public key import success from file %s", kp->pubkeyfn);
  /* импортируем закрытый ключ */
  rc = ssh_pki_import_privkey_file( kp->privkeyfn, NULL, NULL, NULL,
                                   &kp->privkey);
  if (rc != SSH_OK)
  {
    if (rc == SSH_EOF)
      log_error(kp,"%s %d %s private key file doesn't exist or "
                "permission denied %s",__FILE__, __LINE__, __func__,
                kp->privkeyfn);
    if (rc == SSH_ERROR)
      log_error(kp,"%s %d %s SSH_ERROR %s %s",__FILE__, __LINE__, __func__,
                kp->privkeyfn, ssh_get_error(kp->my_ssh_session));
    return NULL;
  }
  /* конец импорта закрытого ключа */
  log_info(kp,"private key import success from file %s", kp->privkeyfn);

  /* inotifyFd = inotify_init(); */
  inotifyFd = inotify_init1(IN_NONBLOCK);
  if (inotifyFd == -1)
  {
    log_debug("%s main.c thread() inotify_init() Stop thread. Errno: %s",
                kp->locallogin, strerror(errno));
    return NULL;
  }
  errno = 0;
  wd = inotify_add_watch(inotifyFd, kp->indir, IN_CREATE|IN_MOVE);
  if (wd == -1)
  {
    log_debug("%s main.c thread() inotify_add_watch(). Stop thread. Errno: %s",
                kp->locallogin, strerror(errno));
    return NULL;
  }

  /* увеличиваем счётчик запущенных потоков */
  pthread_mutex_lock( &inotify_watcher_lock );
  inotify_watcher = inotify_watcher + 1;
  log_debug("%s %s %s inotify_watcher counter: %d", kp->locallogin, __FILE__,
             __func__, inotify_watcher);
  pthread_mutex_unlock( &inotify_watcher_lock );
  /* конец увеличения счётчика */
  
  pid = getpid();
  tid = syscall(SYS_gettid);
  log_debug("%s PID:%d tidself:%lu tid:%lu watch dir %s use inotify fid: %d",
            kp->locallogin, pid, pthread_self(), tid, kp->indir, inotifyFd);
  
  ssize_t numRead;
  char buf[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
  struct pollfd plst[1];
  int retval;
  int timeout = kp->error_handler_timeout * 60 * 1000 ;
  plst[0].fd = inotifyFd;
  plst[0].events = POLLIN;
  char *ptr;
  const struct inotify_event *event;
  for (;;)
  {
    log_debug("%s poll wait inotify event inotifyFd: %d",kp->locallogin,inotifyFd);
    retval = poll(plst,(unsigned long)1, timeout );
    if(retval == -1)
    {
      log_debug("%s %s %d poll error: %s",kp->locallogin,__FILE__,__LINE__, strerror(errno));
      continue;
    }
    if(retval == 0)
    {
      log_debug("%s poll timed out, no file descriptors were ready", kp->locallogin);
      ErrorHandler(kp);
      continue;
    }
    if(retval > 0)
    {
      log_debug("%s poll inotify event exist retval: %d inotifyFd: %d", kp->locallogin, retval, inotifyFd);
      numRead = read(inotifyFd, buf, sizeof(buf));
      if(numRead == 0)
      {
        log_debug("%s %s %d read from inotifyFd ret 0: end of file",kp->locallogin,__FILE__,__LINE__);
        break;
      }
      if(numRead == -1)
      {
        log_debug("%s %s %d read error: %s",kp->locallogin,__FILE__,__LINE__,strerror(errno));
        continue;
      }
      if(numRead > 0)
      {
        log_debug("%s %s read ret %d",kp->locallogin,__FILE__,(int)numRead);
        for(ptr = buf; ptr < buf + numRead; ptr += sizeof(struct inotify_event) + event->len)
        {
          event = (const struct inotify_event *) ptr;
          /* displayInotifyEvent(event); */
          if(event->mask & IN_MOVED_TO)
          {
            if (event->len > 0)
            {
              log_info(kp, "inotify event: IN_MOVED_TO file: %s", event->name);
              strcpy(kp->infnm, event->name);
              processInDir(kp);
              processOutDir(kp);
            }
          }
          if(event->mask & IN_CREATE)
          {
            /* log_debug("%s inotify event: IN_CREATE %s", kp->locallogin, event->name); */
            if(strstr(event->name,"stopwork") != NULL)
            {
              char fn[256] = {0};
              sprintf(fn, "%s/%s", kp->indir, event->name);
              log_info(kp, "inotify event: IN_CREATE file %s", fn);
              if(unlink(fn) == -1)
                log_debug("%s main.c thread() can't unlink stopfile %s errno: %s", 
                          kp->locallogin, fn, strerror(errno));
              stopflag = 1;
            }
            if(strstr(event->name,"reloadcfg") != NULL)
            {
              /* reloadcfg(kp); */
            }
          }
        }
        if(stopflag == 1) break;
      }
    }
  }

  ssh_key_free(kp->pubkey);
  ssh_key_free(kp->privkey); 

  log_debug("%s the thread that watches IN is stopped", kp->locallogin);

  return NULL;
}

/* запускает наши потоки ожидания уведомлений от inotify */
int runthreads()
{
  int status;
  log_debug( "main.c runthreads() start threads to wait for files "
             "in the directorys IN");
  /* создаю потоки наблюдения за директориями IN */
  threads = (pthread_t*) malloc( pParams->len * sizeof(pthread_t) );
  for(int i = 0; i < pParams->len; i++)
  {
    ksparam *kp = g_ptr_array_index(pParams,i);
    status = pthread_create(&(threads[i]), NULL, thread, kp);
    if(status != 0)
    {
      log_debug("main.c runthreads() pthread_create() can't create "
                "thread watching for %s, errno = %s", kp->indir, strerror(errno));
      return -1;
    }
  }

  log_debug("%s %d %s watching threads started: %d", 
            __FILE__, __LINE__, __func__, pParams->len);

  /* создаю поток наблюдающий за изменениями в файлах kspus.cfg */
  /*
  status = pthread_create(&cfgwthrd, NULL, cfgWatchingThrd, NULL);
  if(status != 0)
  {
    log_debug("%s %d %s can't create cfgWatchingThrd errno = %s",
              __FILE__, __LINE__, __func__, strerror(errno));
    return -1;
  }
  status = pthread_join(cfgwthrd, NULL);
  if(status != 0)
  {
    log_debug("%s %d %s pthread_join(cfgwthrd) errno = %s",
              __FILE__, __LINE__, __func__, strerror(errno));
  }
  else
  {
    log_debug("stop thread cfgWatchingThrd()");
  }
  */

  /* делаю pthread_join чтобы главный поток main
   * ожидал завершения всех потоков наблюдающих за IN
   * главный поток останавливает выполнение уже на i=0 */
  for(int i = 0; i < pParams->len; i++)
  {
    status = pthread_join(threads[i], NULL);
    if(status != 0)
    {
      log_debug("main.c runthreads() pthread_join() errno: %s", 
                strerror(errno));
    }
    else
    {
      ksparam *kp = (ksparam*) g_ptr_array_index(pParams,i);
      log_debug("stop work thread for watching dir: %s", kp->indir);
    }
  }
  free(threads);

  log_debug("main.c runthreads() end function.");
  
  return 0;
}

void clearstopfiles()
{
  char fn[256] = {0};
  struct stat sb;
  int res = 0;

  for(int i = 0; i < pParams->len; i++)
  {
    ksparam *kp = (ksparam*) g_ptr_array_index(pParams,i);
    strcpy(fn,kp->indir);
    strcat(fn,"/");
    strcat(fn,"stopwork");
    if( stat(fn, &sb) == -1 )
    {
      if(errno == ENOENT)
      {
        //log_debug("main. clearstopfiles(). %s Errno: %s", fn, strerror(errno));
      }
      continue;
    }
    else
    {
      log_debug("main.c clearstopfiles() exist: %s", fn);
      res = remove(fn);
      if(res == -1)
        log_debug("main.c clearstopfiles() remove(%s) errno: %s", fn, strerror(errno));
    }
  }
}

int setProcLimits()
{
  int espusrs_amount = pParams->len; 
  struct rlimit rlim;
  int thrds_per_espusr = 2;
  int intfys_per_espusr = 2;
  int logs_per_epusr = 2;
  int rlim_cur_orig = 0;
  int rlim_max_orig = 0;

  getrlimit(RLIMIT_NOFILE, &rlim);
  rlim_cur_orig = rlim.rlim_cur;
  rlim_max_orig = rlim.rlim_max;
  log_debug("main.c setProcLimits() current RLIMIT_NOFILE "
            "rlim_cur: %d rlim_max: %d", rlim_cur_orig, rlim_max_orig);
  rlim.rlim_cur = (rlim_cur_orig + espusrs_amount * 
                   (thrds_per_espusr + intfys_per_espusr + logs_per_epusr)) * 3;
  rlim.rlim_max = rlim.rlim_cur * 4;
  if(setrlimit(RLIMIT_NOFILE, &rlim) == -1)
  {
    log_debug("main.c setProcLimits() errno: %s", strerror(errno));
    return -1;
  }
  else
  {
    log_debug("main.c set process limits for RLIMIT_NOFILE "
              "rlim_cur: %d rlim_max: %d", rlim.rlim_cur, rlim.rlim_max);
  }
  return 0;
}

int workproc()
{
  DIR* dir = opendir(basedir);
  if(dir == NULL)
  {
    log_debug("main.c error open basedir %s errno: %s", basedir,
              strerror(errno));
    return -1;
  }
  closedir(dir);

  /* загружаем параметры в связанный список pParams */
  pParams = g_ptr_array_new ();
  int res = nftw(basedir, fparams, 20, 0);
  if(res == -1) 
  {
    log_debug("main.c workproc() nftw() ret -1 basedir: %s", basedir);
    return -1;
  }
  if(res == 1) 
  {
    log_debug("main.c workproc() nftw() ret 1");
    return -1;
  }
  if(pParams->len == 0)
  {
    log_debug("main.c loaded %d param structure from kspus.cfg files",
              pParams->len);
    return -1;
  }

  /* устанавливаем лимиты процесса */
  if(setProcLimits() == -1)
  {
    log_debug("main.c setProcLimits() == -1");
    return -1;
  }

  clearstopfiles();

  res = pthread_mutex_init( &inotify_watcher_lock, NULL );
  if(res != 0)
  {
    log_debug("main.c workproc() pthread_mutex_init(inotify_watcher_lock). "
              "res=%d errno: %s", res, strerror(errno));
    return -1;
  }

  res = runthreads();
  if( res == -1 )
  {
    log_debug("main.c workproc() runthreads() ret %d", res);
  }

  pthread_mutex_destroy( &inotify_watcher_lock );
  
  g_ptr_array_free(pParams, TRUE);
  
  log_debug("main.c workproc() end function");

  return 0;
}

void signal_handler(sig)
int sig;
{
  switch(sig)
  {
  case SIGHUP:
    log_debug("main.c signal_handler() hangup signal catched SIGHUP");
    for(int i = 0; i < pParams->len; i++)
    {
      ksparam *kp = (ksparam*) g_ptr_array_index(pParams,i);
      FILE *fp;
      char fn[256] = {0};
      strcpy(fn,kp->indir);
      strcat(fn,"/");
      strcat(fn,"stopwork");
      fp = fopen(fn,"w");
      if(fp == NULL)
      {
        log_debug("main() signal_handler() error create: %s errno: %s",
                  fn, strerror(errno));
      }
      else
      {
        log_debug("main.c signal_handler() file created: %s",fn);
        fclose(fp);
      }
    }
    break;
  case SIGTERM:
    log_debug("main.c terminate signal catched SIGTERM");
    if(unlink(PID_FILE) == -1)
    {
      log_debug("main.c signal_handler() can't unlink lock file, errno: %s",
                 strerror(errno));
    }
    log_debug("main.c exit(1) killed with SIGTERM.");
    exit(1);
    break;
  }
}

/******************************************************************
 * принимает один параметр - полный путь к базовому каталогу 
 * в котором домашние каталоги юзеров в которых находятся
 * директории in out error archive 
 * каталог с настройками ~/.kspusd
 * в котором должен быть настроечный файл пользователя kspus.cfg
 ******************************************************************/
int main(int argc, char **argv)
{
  int pid, stdiofd, opt;
  char str[10];

  /* получаем параметры командной строки */
  while ((opt = getopt(argc, argv, "vd:")) != -1)
  {
    switch (opt)
    {
    case 'v':
      printf("build date: %s\n",version);
      printf("libssh version: %s\n", ssh_version(0));
      exit(0);
      break;
    case 'd':
      strcpy(basedir, optarg); 
      break;
    case '?':
    default:
      fprintf(stderr, "Usage: %s [-d basedir] [-v]\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  }
  if(strlen(basedir) == 0)
  {
    printf("Basedir not defined.\nUsage: %s [-d basedir] [-v]\n", argv[0]);
    log_debug("Basedir not defined. Usage: %s [-d basedir] [-v]\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  printf("basedir: %s\n", basedir);
  log_debug("%s %d %s basedir: %s", __FILE__, __LINE__, __func__, basedir);

  /* демонизируем процесс */
  if( getppid() == 1 ) 
    return 0;

  pid = fork();
  if(pid == -1)
  {
    log_debug("fork error. errno: %s", strerror(errno));
    exit(1);
  }
  if( pid > 0 )
  {
    log_debug("main.c daemon process created with PID %d",pid);
    exit(0);
  }

  /* остальной код выполняется уже в дочернем процессе */
  setsid();                                    /* obtain a new process group */
  for(int i=getdtablesize(); i>=0; --i) 
    close(i);                                  /* close all descriptors */
  stdiofd = open("/dev/null", O_RDWR);         /* handle standart I/O stdin */ 
  if(dup(stdiofd) == -1)
    log_debug("stdout dup ret -1");             /* handle standart I/O stdout */
  if(dup(stdiofd) == -1)
    log_debug("stderr dup ret -1");             /* handle standart I/O stderr */
  umask(0);
  if(chdir(RUNNING_DIR) == -1)                  /* change running directory */
    log_debug("chdir to RUNNING_DIR error. errno: %s", strerror(errno));

  /* записываем PID процесса в pid-файл */
  pidfd = open(PID_FILE, O_RDWR|O_CREAT, 0640);
  if( pidfd == -1 )
  {
    log_debug("can't create pid file. errno: %s", strerror(errno));
    exit(1);
  }
  sprintf(str, "%u\n", getpid());
  if(write(pidfd, str, strlen(str)) == -1)
  {
    log_debug("can't write PID to pid file. errno: %s", strerror(errno));
    exit(1);
  }
  signal(SIGCHLD,SIG_IGN); /* ignore child */
  signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGHUP,signal_handler); /* catch hangup signal */
  signal(SIGTERM,signal_handler); /* catch kill signal */

  /* выполняю инициализацию libssh для работы в многопоточном 
   * приложении взято тут: 
   * http://api.libssh.org/master/libssh_tutor_threads.html */
  ssh_threads_set_callbacks(ssh_threads_get_pthread());
  ssh_init();

  if(workproc() == -1)
  {
    log_debug("main.c workproc() ret -1");
  }
  
  close(pidfd);
  if(unlink(PID_FILE) == -1)
    log_debug("can't unlink pid file, errno: %s",strerror(errno));

  log_debug("main.c Exit with code 0.");

  return 0;
}
