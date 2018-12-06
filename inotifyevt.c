#include <stdio.h>
#include <sys/inotify.h>
#include "ksparam.h"
#include "log.h"

/* Display information from inotify_event structure */
void displayInotifyEvent(struct inotify_event *i)
{
	log_debug("wd =%2d; ", i->wd);
	if (i->cookie > 0)
		 log_debug("cookie =%4d; ", i->cookie);

	log_debug("mask = ");
	if (i->mask & IN_ACCESS)        log_debug("IN_ACCESS ");
	if (i->mask & IN_ATTRIB)        log_debug("IN_ATTRIB ");
	if (i->mask & IN_CLOSE_NOWRITE) log_debug("IN_CLOSE_NOWRITE ");
	if (i->mask & IN_CLOSE_WRITE)   log_debug("IN_CLOSE_WRITE ");
	if (i->mask & IN_CREATE)        log_debug("IN_CREATE ");
	if (i->mask & IN_DELETE)        log_debug("IN_DELETE ");
	if (i->mask & IN_DELETE_SELF)   log_debug("IN_DELETE_SELF ");
	if (i->mask & IN_IGNORED)       log_debug("IN_IGNORED ");
	if (i->mask & IN_ISDIR)         log_debug("IN_ISDIR ");
	if (i->mask & IN_MODIFY)        log_debug("IN_MODIFY ");
	if (i->mask & IN_MOVE_SELF)     log_debug("IN_MOVE_SELF ");
	if (i->mask & IN_MOVED_FROM)    log_debug("IN_MOVED_FROM ");
	if (i->mask & IN_MOVED_TO)      log_debug("IN_MOVED_TO ");
	if (i->mask & IN_OPEN)          log_debug("IN_OPEN ");
	if (i->mask & IN_Q_OVERFLOW)    log_debug("IN_Q_OVERFLOW ");
	if (i->mask & IN_UNMOUNT)       log_debug("IN_UNMOUNT ");

	if (i->len > 0)
		 log_debug("name = %s", i->name);
}
