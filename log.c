#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <pthread.h>
#include <stdlib.h>
#include "ksparam.h"
#include "log.h"
#include "myutils.h"

#define MAINLOG "/var/log/kspusd.log"

void log_format(ksparam *p, const char* tag, const char* message, va_list args)
{
  char timestr[128] = {0};
  time_t now;
  struct tm *t;
	char msgstr[2048] = {0};
  char syslogmsg[2560] = {0};
  FILE *f;

  now = time(NULL);
  t = localtime(&now);
  strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", t);
 
	vsprintf(msgstr, message, args);

  sprintf(syslogmsg, "[%s] %s %s", tag, p->locallogin, msgstr);
  syslog(LOG_DAEMON||LOG_DEBUG, "%s", syslogmsg);

  f = fopen( MAINLOG, "a+" );
  if(f == NULL)
  {
    syslog(LOG_DAEMON||LOG_DEBUG, "[ERRR] can't open log file %s", MAINLOG);
  }
  else
  {
    fprintf( f, "%s [%s] %s %s\n", timestr, tag, p->locallogin, msgstr );
    fclose(f);
  } 

  f = fopen( p->filelog, "a+" );
  if(f == NULL)
  {
    syslog(LOG_DAEMON||LOG_DEBUG, "[ERRR] can't open log file %s", p->filelog);
  }
  else
  {
    fprintf( f, "%s [%s] %s %s\n", timestr, tag, p->locallogin, msgstr );
    fclose(f);
    chown_logfile(p);
    chmod(p->filelog, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
  } 
}

void log_info(ksparam *p, const char* message, ...)
{
  va_list args;
  va_start(args, message);
  log_format(p,"INFO", message, args);
  va_end(args);
}

void log_warn(ksparam *p, const char* message, ...)
{
  va_list args;
  va_start(args, message);
  log_format(p,"WARN", message, args);
  va_end(args);
}

void log_error(ksparam *p, const char* message, ...)
{
  va_list args;
  va_start(args, message);
  log_format(p,"ERRR", message, args);
  va_end(args);
}

/* для упрощения кода отладочный лог идёт в syslog и в /var/log/kspusd.log */
void log_debug(const char* message, ...)
{
	char msgstr[2048] = {0};
  va_list args;
  char timestr[128] = {0};
  time_t now;
  struct tm *t;
  FILE *f;

  now = time(NULL);
  t = localtime(&now);
  strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", t);

  va_start(args, message);
	vsprintf(msgstr, message, args);
  va_end(args);

  char * indx = index(msgstr, 10);
  if(indx != NULL)
  {
    *indx = 157;
  }
  
  syslog(LOG_DAEMON||LOG_DEBUG, "[DEBG] %s", msgstr);

  f = fopen( MAINLOG, "a+" );
  if(f == NULL)
  {
    syslog(LOG_DAEMON||LOG_ERR, "[ERRR] can't open log file %s", MAINLOG);
  }
  else
  {
    fprintf( f, "%s [DEBG] %s\n", timestr, msgstr );
    fclose(f);
  } 
}

