#define _XOPEN_SOURCE 500
#include <ftw.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <libssh/libssh.h>
#include "ksparam.h"
#include "log.h"

#define CONFIG_FILE ".kspus/kspus.cfg"

GPtrArray *pParams; /* связанный список с параметрами прочитанными из kspus.cfg */

int checkcdun(ksparam *p, const char *cdun)
{
  if(strstr(p->indir, cdun) == NULL)
  {
    log_debug("%s %d %s indir:%s mismatch with %s",
              __FILE__, __LINE__, __func__, p->indir, p->cfg);
    return -1;
  }
  if(strstr(p->outdir, cdun) == NULL)
  {
    log_debug("%s %d %s outdir:%s mismatch with %s",
              __FILE__, __LINE__, __func__, p->outdir, p->cfg);
    return -1;
  }
  if(strstr(p->errdir, cdun) == NULL)
  {
    log_debug("%s %d %s errdir:%s mismatch with %s",
              __FILE__, __LINE__, __func__, p->errdir, p->cfg);
    return -1;
  }
  if(strstr(p->archivedir, cdun) == NULL)
  {
    log_debug("%s %d %s archivedir:%s mismatch with %s",
              __FILE__, __LINE__, __func__, p->archivedir, p->cfg);
    return -1;
  }
  if(strstr(p->locallogin, cdun) == NULL)
  {
    log_debug("%s %d %s locallogin:%s mismatch with %s",
              __FILE__, __LINE__, __func__, p->archivedir, p->cfg);
    return -1;
  }
  if(strstr(p->pubkeyfn, cdun) == NULL)
  {
    log_debug("%s %d %s pubkeyfn:%s mismatch with %s",
              __FILE__, __LINE__, __func__, p->pubkeyfn, p->cfg);
    return -1;
  }
  if(strstr(p->privkeyfn, cdun) == NULL)
  {
    log_debug("%s %d %s privkeyfn:%s mismatch with %s",
              __FILE__, __LINE__, __func__, p->privkeyfn, p->cfg);
    return -1;
  }
  if(strstr(p->remotelogin, cdun) == NULL)
  {
    log_debug("%s %d %s remotelogin:%s mismatch with %s",
              __FILE__, __LINE__, __func__, p->remotelogin, p->cfg);
    return -1;
  }
  if(strstr(p->filelog, cdun) == NULL)
  {
    log_debug("%s %d %s filelog:%s mismatch with %s",
              __FILE__, __LINE__, __func__, p->filelog, p->cfg);
    return -1;
  }
    
  return 0;
}

/* читает параметры из файлов kspus.cfg и заносит их в связанный список */
int fparams(const char *fpath, const struct stat *sb,
          int tflag, struct FTW *ftwbuf)
{
  if( strstr(fpath, CONFIG_FILE) != NULL )
  {
    char curdirusername[256] = {0};
    GKeyFile* gkf;
    GError *err = NULL;
    ksparam *p2 = NULL;

    /* нужно сделать проверку совпадает ли locallogin и остальные пути
     * с логином в fpath */
    strcpy(curdirusername, fpath);
    char *p1 = strstr(curdirusername, CONFIG_FILE)-1;
    *p1 = 0;
    char *cdun = rindex(curdirusername, '/')+1; /* имя пользователя выдранное из пути к текущему kspus.cfg */

    gkf = g_key_file_new();
    if( !g_key_file_load_from_file(gkf, fpath, G_KEY_FILE_NONE, &err) )
    {
      log_debug("%s %d %s g_key_file_load_from_file() file %s GError = %s",
                __FILE__, __LINE__, __func__, fpath, err->message);
      g_error_free (err);
      return 1;
    }
    else
    {
      log_debug("loadcfg.c fparams() Starting reading params from file: %s", fpath);
    }
   
    p2 = (ksparam*) malloc (sizeof(ksparam));

    p2->cfg = (char*)calloc(256, sizeof(char));
    if(p2->cfg == NULL)
    {
      log_debug("loadcfg.c fparams() calloc for tarfnm return NULL.");
      return 1;
    }
    else
    {
      strcpy(p2->cfg,fpath);
    }

    err = NULL;
    p2->indir = g_key_file_get_string(gkf, "DIRS", "in", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(DIRS->in). %s", err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->indir) == 0)
    {
      log_debug("loadcfg.c fparams() DIRS->in not found in %s", fpath);
      return 1;
    }
    
    err = NULL;
    p2->outdir = g_key_file_get_string(gkf, "DIRS", "out", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(DIRS->out). %s", err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->outdir) == 0)
    {
      log_debug("loadcfg.c fparams() DIRS->outdir not found in %s", fpath);
      return 1;
    }
    
    err = NULL;
    p2->errdir = g_key_file_get_string(gkf, "DIRS", "err", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(DIRS->err). %s", err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->errdir) == 0)
    {
      log_debug("loadcfg.c fparams() DIRS->errdir not found in %s", fpath);
      return 1;
    }

    err = NULL;
    p2->archivedir = g_key_file_get_string(gkf, "DIRS", "archive", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(DIRS->archive). %s", err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->archivedir) == 0)
    {
      log_debug("loadcfg.c fparams() DIRS->archivedir not found in %s", fpath);
      return 1;
    }

    err = NULL;
    p2->remotelogin = g_key_file_get_string(gkf, "REGIONALSPUS", "remotelogin", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(REGIONALSPUS->remotelogin). %s",
                err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->remotelogin) == 0)
    {
      log_debug("fparams. REGIONALSPUS->remotelogin not found in %s", fpath);
      return 1;
    }

    err = NULL;
    p2->ipregspusmain = g_key_file_get_string(gkf, "REGIONALSPUS", "ipmain", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(REGIONALSPUS->ipmain). %s",
                err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->ipregspusmain) == 0)
    {
      log_debug("loadcfg.c fparams() REGIONALSPUS->ipregspusmain not found in %s", fpath);
      return 1;
    }
    
    err = NULL;
    p2->ipregspusbackup = g_key_file_get_string(gkf, "REGIONALSPUS", "ipbackup", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(REGIONALSPUS->ipbackup). %s",
                err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->ipregspusbackup) == 0)
    {
      log_debug("loadcfg.c fparams() REGIONALSPUS->ipregspusbackup not found in %s", fpath);
      return 1;
    }

    /* загружаем имя файла публичного ключа */
    err = NULL;
    p2->pubkeyfn = g_key_file_get_string(gkf, "AUTH", "pubkey", &err);
    if(err != NULL)
    {
      log_debug("%s %d %s AUTH->pubkey %s %s",
                __FILE__, __LINE__, __func__, fpath, err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->pubkeyfn) == 0)
    {
      log_debug("%s %d %s AUTH->pubkey not found in %s",
                __FILE__, __LINE__, __func__, fpath);
      return 1;
    }
    
    /* загружаем имя файла закрытого ключа */
    err = NULL;
    p2->privkeyfn = g_key_file_get_string(gkf, "AUTH", "privkey", &err);
    if(err != NULL)
    {
      log_debug("%s %d %s AUTH->privkey %s %s",
                __FILE__, __LINE__, __func__, fpath, err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->privkeyfn) == 0)
    {
      log_debug("%s %d %s AUTH->privkeyfn not found in %s",
                __FILE__, __LINE__, __func__, fpath);
      return 1;
    }

    err = NULL;
    p2->locallogin = g_key_file_get_string(gkf, "AUTH", "locallogin", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(AUTH->locallogin)."
                " %s", err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->locallogin) == 0)
    {
      log_debug("loadcfg.c fparams() AUTH->locallogin not found in %s", fpath);
      return 1;
    }

    err = NULL;
    p2->localgroup = g_key_file_get_string(gkf, "AUTH", "localgroup", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(AUTH->localgroup). %s", 
                     err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->localgroup) == 0)
    {
      log_debug("loadcfg.c fparams() AUTH->localgroup not found in %s", fpath);
      return 1;
    }
    
    err = NULL;
    p2->regcode = g_key_file_get_string(gkf, "CDR", "regcode", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(CDR->regcode). %s", err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->regcode) == 0)
    {
      log_debug("loadcfg.c fparams() CDR->regcode not found in %s", fpath);
      return 1;
    }
    
    err = NULL;
    p2->cdrprefix = g_key_file_get_string(gkf, "CDR", "cdrprefix", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(CDR->cdrprefix). %s", err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->cdrprefix) == 0)
    {
      log_debug("loadcfg.c fparams() CDR->cdrprefix not found in %s", fpath);
      return 1;
    }
    
    err = NULL;
    p2->atsnum = g_key_file_get_string(gkf, "CDR", "atsnum", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(CDR->atsnum). %s", err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->atsnum) == 0)
    {
      log_debug("loadcfg.c fparams() CDR->atsnum not found in %s", fpath);
      return 1;
    }
    
    err = NULL;
    p2->zz = g_key_file_get_string(gkf, "CDR", "zz", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(CDR->zz). %s", err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->zz) == 0)
    {
      log_debug("loadcfg.c fparams() CDR->zz not found in %s", fpath);
      return 1;
    }

    err = NULL;
    p2->namelen = g_key_file_get_integer(gkf, "CDR", "namelen", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(CDR->namelen). %s", err->message);
      g_error_free (err);
      return 1;
    }

    err = NULL;
    p2->rmtindir = g_key_file_get_string(gkf, "REGIONALSPUS", "rmtindir", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(REGIONALSPUS->rmtindir). %s", 
                     err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->rmtindir) == 0)
    {
      log_debug("loadcfg.c fparams() REGIONALSPUS->rmtindir not found in %s", fpath);
      return 1;
    }

    err = NULL;
    p2->rmtcmd = g_key_file_get_string(gkf, "REGIONALSPUS", "rmtcmd", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(REGIONALSPUS->rmtcmd). %s", 
                     err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->rmtcmd) == 0)
    {
      log_debug("loadcfg.c fparams() REGIONALSPUS->rmtcmd not found in %s", fpath);
      return 1;
    }

    err = NULL;
    p2->filelog = g_key_file_get_string(gkf, "LOGGING", "filelog", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(LOGGING->filelog). %s", 
                     err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->filelog) == 0)
    {
      log_debug("loadcfg.c fparams() LOGGING->filelog not found in %s", fpath);
      return 1;
    }

    err = NULL;
    p2->syslog = g_key_file_get_string(gkf, "LOGGING", "syslog", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(LOGGING->syslog). %s", 
                     err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->syslog) == 0)
    {
      log_debug("loadcfg.c fparams() LOGGING->syslog not found in %s", fpath);
      return 1;
    }

    err = NULL;
    p2->stdoutlog = g_key_file_get_string(gkf, "LOGGING", "stdoutlog", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_string(LOGGING->stdoutlog). %s", 
                    err->message);
      g_error_free (err);
      return 1;
    }
    if(strlen(p2->stdoutlog) == 0)
    {
      log_debug("loadcfg.c fparams() LOGGING->stdoutlog not found in %s", fpath);
      return 1;
    }
    
    p2->availsrv = (char*)calloc(256, sizeof(char));
    if(p2->availsrv == NULL)
      log_debug("loadcfg.c fparams() calloc for availsrv return NULL.");  
    
    p2->infnm = (char*)calloc(256, sizeof(char));
    if(p2->infnm == NULL)
      log_debug("loadcfg.c fparams() calloc for infnm return NULL.");

    p2->outfnm = (char*)calloc(256, sizeof(char));
    if(p2->outfnm == NULL)
      log_debug("loadcfg.c fparams() calloc for outfnm return NULL.");

    p2->gzfnm = (char*)calloc(256, sizeof(char));
    if(p2->gzfnm == NULL)
      log_debug("loadcfg.c fparams() calloc for gzfnm return NULL.");

    p2->md5fnm = (char*)calloc(256, sizeof(char));
    if(p2->md5fnm == NULL)
      log_debug("loadcfg.c fparams() calloc for md5fnm return NULL.");

    p2->tarfnm = (char*)calloc(256, sizeof(char));
    if(p2->tarfnm == NULL)
      log_debug("loadcfg.c fparams() calloc for tarfnm return NULL.");

    p2->md5str = (char*)calloc(256, sizeof(char));
    if(p2->md5str == NULL)
      log_debug("loadcfg.c fparams() calloc for md5str return NULL.");

    err = NULL;
    p2->errmail.addr1 = g_key_file_get_string(gkf, "NOTICE", "email1", &err);
    if(err != NULL)
    {
      log_debug("%s %d %s g_key_file_get_string(NOTICE->email1). %s",
                __FILE__, __LINE__, __func__, err->message);
      g_error_free (err);
      return 1;
    }
    else
    {
      if(strlen(p2->errmail.addr1) == 0)
      {
        log_debug("loadcfg.c fparams. NOTICE->email1 has blank value %s", fpath);
        return 1;
      }
    }

    err = NULL;
    p2->errmail.addr2 = g_key_file_get_string(gkf, "NOTICE", "email2", &err);
    if(err != NULL)
    {
      log_debug("%s %d %s g_key_file_get_string(NOTICE->email2). %s",
                __FILE__, __LINE__, __func__, err->message);
      g_error_free (err);
      p2->errmail.addr2 = NULL;
    }
    else
    {
      if(strlen(p2->errmail.addr2) == 0)
      {
        log_debug("loadcfg.c fparams. NOTICE->email2 has blank value %s", fpath);
        p2->errmail.addr2 = NULL;
      }
    }

    err = NULL;
    p2->errmail.addr3 = g_key_file_get_string(gkf, "NOTICE", "email3", &err);
    if(err != NULL)
    {
      log_debug("%s %d %s g_key_file_get_string(NOTICE->email3). %s",
                __FILE__, __LINE__, __func__, err->message);
      g_error_free (err);
      p2->errmail.addr3 = NULL;
    }
    else
    {
      if(strlen(p2->errmail.addr3) == 0)
      {
        log_debug("loadcfg.c fparams. NOTICE->email3 has blank value %s", fpath);
        p2->errmail.addr3 = NULL;
      }
    }

    err = NULL;
    p2->error_handler_timeout = g_key_file_get_integer(gkf, "MISC", "error_handler_timeout", &err);
    if(err != NULL)
    {
      log_debug("loadcfg.c fparams() g_key_file_get_integer(MISC->error_handler_timeout). %s %s",
                 p2->locallogin, err->message);
      g_error_free (err);
      p2->error_handler_timeout = 15;
      log_debug("loadcfg.c fparams() error_handler_timeout set to 15 minut for %s", p2->locallogin);
    }

    if(pthread_mutex_init(&(p2->mtx_dir_lock), NULL) != 0)
    {
      log_debug("loadcfg.c fparams() pthread_mutex_init(). %s Errno: %s",
                 p2->locallogin, strerror(errno));
      return 1;
    }

    if( checkcdun(p2,cdun) == -1 )
      return -1;

    g_ptr_array_add(pParams, (gpointer) p2); 
    g_key_file_free(gkf);
    
    log_debug("loadcfg.c fparams() Parameters successfully loaded from file: %s", fpath);
  }

  return 0;
}

int reloadcfg(ksparam *p)
{
  GKeyFile* gkf;
  GError *err = NULL;

  gkf = g_key_file_new();
  if( !g_key_file_load_from_file(gkf, p->cfg, G_KEY_FILE_NONE, &err) )
  {
    log_error(p,"%s %d %s file %s GError = %s",
              __FILE__, __LINE__, __func__, p->cfg, err->message);
    g_error_free (err);
    return 1;
  }
  else
  {
    log_info(p,"start reload params from file: %s", p->cfg);
  }

  g_key_file_free(gkf);

  return 0;
}
