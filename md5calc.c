#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>
#include "myutils.h"
#include "md5calc.h"
#include "main.h"
#include "log.h"

/* вычисляет md5 сумму gz файла */
int cdrmd5(ksparam *p)
{
  FILE *fp;
  int fd;
  void *file_buffer;
  unsigned char result[MD5_DIGEST_LENGTH];
  char gzFileNameWithPath[256] = {0};
  struct stat sb;
  
  strcpy(gzFileNameWithPath, p->outdir);
  strcat(gzFileNameWithPath, "/");
  strcat(gzFileNameWithPath, p->gzfnm);

  if(stat(gzFileNameWithPath, &sb) == -1)
  {
    log_error(p, "md5calc.c cdrmd5 can't stat %s", gzFileNameWithPath);
    return -1;
  }

  fp = fopen(gzFileNameWithPath, "r");
  if(fp == NULL)
  {
    log_error(p, "md5calc.c cdrmd5 can't open %s", gzFileNameWithPath);
    return -1;
  }
  
  fd = fileno(fp);
  if(fd == -1)
  {
    log_error(p,"md5calc.c cdrmd5 fileno detects that its argument "
                "is not a valid stream. file:", gzFileNameWithPath);
    return -1;
  }

  file_buffer = mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if((long long *)file_buffer == MAP_FAILED)
  {
    log_error(p,"md5calc.c cdrmd5 mmap ret MAP_FAILED");
    return -1;
  }

  MD5((unsigned char*) file_buffer, sb.st_size, result);
  munmap(file_buffer, sb.st_size);
  fclose(fp);

  memset(p->md5str, 0, 256);
  for(int i=0; i < MD5_DIGEST_LENGTH; i++)
  {
    char buf[8] = {0};
    snprintf(buf, 4, "%02x", result[i]);
    strcat(p->md5str, buf);
  }
  
  return 0;
}
