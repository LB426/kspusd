#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "mknewname.h"
#include "log.h"

int mknewnm(ksparam * p)
{
  struct stat st;
  char dtfin[32] = {0};
  int rc;
  char infnmFullPath[256] = {0};

  strcpy(infnmFullPath, p->indir);
  strcat(infnmFullPath, "/");
  strcat(infnmFullPath, p->infnm);

  rc = stat(infnmFullPath, &st); /* non Thread safety ? */
  if ( rc == -1)
  {
     log_error(p, "mknewnm(). stat(). Msg: rc == -1 , infnmFullPath: %s", infnmFullPath);
     return -1;
  }

  strftime(dtfin, 20, "%y%m%d%H%M%S", localtime(&st.st_mtime)); /* non Thread safety ? */
 
  /* чтобы сделать переименование независимым от оригинального имени файла
   * нужно в файле настроек указать количество символов оригинального имени
   * от начала имени, которые будкт копироваться в новое имя
   * */ 
  strcpy(p->outfnm, p->cdrprefix);
  strcat(p->outfnm, p->regcode);
  strcat(p->outfnm, p->atsnum);
  strcat(p->outfnm, "_");
  strcat(p->outfnm, dtfin);
  strcat(p->outfnm, "_");
  strcat(p->outfnm, p->zz);
  strcat(p->outfnm, "_");
  if( strlen(p->infnm) >= p->namelen )
  {
    strncat(p->outfnm, p->infnm, p->namelen);
  }
  else
  {
    strcat(p->outfnm, p->infnm);
    log_error(p, "mknewnm. Real filename length for file %s less than specified in kspus.cfg %d", p->infnm, p->namelen);
    return -1;
  }

  return 0;
}
