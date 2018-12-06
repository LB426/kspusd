#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <limits.h>
#include "togz.h"
#include "log.h"

int compressFileToGZ(ksparam *p)
{
  char buf[BUFSIZ] = {0};
  size_t bytes_read = 0;
  FILE *fp;
  char srcFileNameWithPath[256] = {0};
  char gzFileNameWithPath[256] = {0};

  /* делаем имя файла c путём из которого будет gz файл */
  strcpy(srcFileNameWithPath, p->outdir);
  strcat(srcFileNameWithPath, "/");
  strcat(srcFileNameWithPath, p->outfnm);
  
  /* делаем имя gz файла */
  strcpy(p->gzfnm, p->outfnm);
  strcat(p->gzfnm, ".gz"); 

  /* делаем имя файла с путём для gz файла */
  strcpy(gzFileNameWithPath, p->outdir);
  strcat(gzFileNameWithPath, "/");
  strcat(gzFileNameWithPath, p->gzfnm);
  
  fp = fopen(srcFileNameWithPath, "r");
  if(fp)
  {
    gzFile out = gzopen(gzFileNameWithPath, "wb");
    if(!out)
    {
      log_error(p, "togz.c compressFileToGZ() Unable to open %s for writing", 
                   gzFileNameWithPath);
      return -1;
    }
    bytes_read = fread(buf, 1, BUFSIZ, fp);
    while (bytes_read > 0)
    {
      int bytes_written = gzwrite(out, buf, bytes_read);
      if(bytes_written == 0)
      {
        int err_no = 0;
        log_error(p, "togz.c compressFileToGZ() gzwrite() Error during compression: %s",
                     gzerror(out, &err_no));
        gzclose(out);
        fclose(fp);
        return -1;
      }
      bytes_read = fread(buf, 1, BUFSIZ, fp);
    }
    gzclose(out);
    fclose(fp);
    remove(srcFileNameWithPath);
  }
  else
  {
    log_error(p, "togz.c compressFileToGZ() fopen() Can't open file %s", 
               srcFileNameWithPath);
    return -1;
  }

  log_info(p, "success create gz file %s", gzFileNameWithPath);

  return 0;
}

