#include <fcntl.h>
#include <sys/stat.h>
#include <archive.h>
#include <archive_entry.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "totar.h"
#include "log.h"

void write_tar_archive(ksparam * p)
{
  struct archive *a;
  struct archive_entry *entry;
  struct stat st;
  char buf[8192];
  int len;
  FILE *fp;
  char tarFileNameWithPath[256] = {0};
  char gzFileNameWithPath[256] = {0};
  char md5FileNameWithPath[256] = {0};
  
  strcpy(tarFileNameWithPath, p->outdir);
  strcat(tarFileNameWithPath, "/");
  strcat(tarFileNameWithPath, p->tarfnm);

  strcpy(gzFileNameWithPath, p->outdir);
  strcat(gzFileNameWithPath, "/");
  strcat(gzFileNameWithPath, p->gzfnm);

  strcpy(md5FileNameWithPath, p->outdir);
  strcat(md5FileNameWithPath, "/");
  strcat(md5FileNameWithPath, p->md5fnm);

  a = archive_write_new();
  archive_write_set_format_pax_restricted(a);
  archive_write_open_filename(a, tarFileNameWithPath);

  stat(gzFileNameWithPath, &st);
  strftime(buf, 20, "st_atime %Y-%m-%d %H:%M:%S", localtime(&st.st_atime));
  strftime(buf, 20, "st_mtime %Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
  strftime(buf, 20, "st_ctime %Y-%m-%d %H:%M:%S", localtime(&st.st_ctime));
  memset(buf, 0, sizeof(buf));

  entry = archive_entry_new();
  archive_entry_set_pathname(entry, p->gzfnm);
  archive_entry_set_size(entry, st.st_size);
  archive_entry_set_filetype(entry, AE_IFREG);
  archive_entry_set_perm(entry, 0644);
  archive_entry_set_mtime(entry, st.st_atime, 0);
  archive_write_header(a, entry);
  fp = fopen(gzFileNameWithPath, "r");
  len = fread(buf, 1, sizeof(buf), fp);
  while ( len > 0 ) 
  {
      archive_write_data(a, buf, len);
      len = fread(buf, 1, sizeof(buf), fp);
  }
  fclose(fp);
  archive_entry_free(entry);

  memset(buf, 0, sizeof(buf));
  stat(md5FileNameWithPath, &st);
  entry = archive_entry_new();
  archive_entry_set_pathname(entry, p->md5fnm);
  archive_entry_set_size(entry, st.st_size);
  archive_entry_set_filetype(entry, AE_IFREG);
  archive_entry_set_perm(entry, 0644);
  archive_entry_set_mtime(entry, st.st_atime, 0);
  archive_write_header(a, entry);
  fp = fopen(md5FileNameWithPath, "r");
  len = fread(buf, 1, sizeof(buf), fp);
  while ( len > 0 ) 
  {
      archive_write_data(a, buf, len);
      len = fread(buf, 1, sizeof(buf), fp);
  }
  fclose(fp);
  archive_entry_free(entry);

  archive_write_close(a);
  archive_write_free(a);

  remove(md5FileNameWithPath);
  remove(gzFileNameWithPath);

  log_info(p, "success create tar file %s", tarFileNameWithPath);
}
