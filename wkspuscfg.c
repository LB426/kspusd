#include <sys/inotify.h>
#include <errno.h>
#include <string.h>
#include <glib.h>
#include "inotifyevt.h"
#include "ksparam.h"
#include "log.h"
#include "wkspuscfg.h"

extern pthread_t cfgwthrd; /* объявлено в main.c */
extern GPtrArray *pParams; /* объявлено в loadcfg.c */

void* cfgWatchingThrd(void* thread_data)
{
  int inotifyFd;
  ssize_t numRead;
  char buf[16384];

  log_debug("Thread watching change kspus.cfg started");
  inotifyFd = inotify_init();
  if (inotifyFd == -1)
  {
    log_debug("%s %d %s notify_init() failed errno: %s", 
              __FILE__, __LINE__, __func__, strerror(errno));
    return NULL;
  }
  for(int i = 0; i < pParams->len; i++)
  {
    ksparam *kp = g_ptr_array_index(pParams,i);
    kp->wd = inotify_add_watch(inotifyFd, kp->cfg, IN_ALL_EVENTS);
    if (kp->wd == -1)
    {
      log_debug("%s %d %s inotify_add_watch() failed file:%s errno: %s",
                __FILE__, __LINE__, __func__, kp->cfg, strerror(errno));
      return NULL;
    }
    log_debug("Add watcher %s", kp->cfg);
  }
  for(;;)
  {
    char *p;
    struct inotify_event *event;
    numRead = read(inotifyFd, buf, 16384);
    if(numRead <= 0)
      log_debug("%s %d %s read(inotifyFd) failed errno: %s", 
                __FILE__, __LINE__, __func__, strerror(errno));
    for(p = buf; p < buf + numRead; )
    {
      event = (struct inotify_event *) p;
      displayInotifyEvent(event); 
      if(event->mask & IN_OPEN)
      {
        if (event->len > 0)
        {
          log_debug("inotify event: IN_OPEN file: %s", event->name);
        }
        else
          log_debug("inotify event: IN_OPEN event->len=%d",event->len);
      }
      if(event->mask & IN_CLOSE_NOWRITE)
      {
        if (event->len > 0)
        {
          log_debug("inotify event: IN_CLOSE_NOWRITE file: %s", event->name);
        }
        else
          log_debug("inotify event: IN_CLOSE_NOWRITE event->len=%d",event->len);
      }
      if(event->mask & IN_MOVE_SELF)
      {
        if (event->len > 0)
        {
          log_debug("inotify event: IN_MOVE_SELF file: %s", event->name);
        }
        else
          log_debug("inotify event: IN_MOVE_SELF event->len=%d",event->len);
      }
      if(event->mask & IN_ATTRIB)
      {
        if (event->len > 0)
        {
          log_debug("inotify event: IN_ATTRIB file: %s", event->name);
        }
        else
          log_debug("inotify event: IN_ATTRIB event->len=%d",event->len);
      }
      if(event->mask & IN_DELETE_SELF)
      {
        for(int i = 0; i < pParams->len; i++)
        {
          ksparam *kp = g_ptr_array_index(pParams,i);
          if(event->wd == kp->wd)
          {
            log_debug("inotify event: IN_DELETE_SELF file: %s",kp->cfg);
          }
        }
      }
      if(event->mask & IN_IGNORED)
      {
        for(int i = 0; i < pParams->len; i++)
        {
          ksparam *kp = g_ptr_array_index(pParams,i);
          if(event->wd == kp->wd)
          {
            log_debug("inotify event: IN_IGNORED file: %s",kp->cfg);
            kp->wd = inotify_add_watch(inotifyFd, kp->cfg, IN_ALL_EVENTS);
            if (kp->wd == -1)
            {
              log_debug("%s %d %s inotify_add_watch() failed file:%s errno: %s",
                        __FILE__, __LINE__, __func__, kp->cfg, strerror(errno));
              return NULL;
            }
            log_debug("Add watcher %s", kp->cfg);
          }
        }
      }
      p += sizeof(struct inotify_event) + event->len;
    }
  }
  log_debug("Thread watching change kspus.cfg stopped");
  return NULL;
}
