#include <ftw.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stddef.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <libssh/libssh.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <iconv.h>
#include "togz.h"
#include "md5calc.h"
#include "totar.h"
#include "toregspus.h"
#include "mknewname.h"
#include "myutils.h"
#include "log.h"
#include "sendemail.h"

/* перемещает из in в out, при этом в out уже новое имя */
int moveToOut(ksparam *p)
{
  int rc = 0;
  char oldpath[256] = {0};
  char newpath[256] = {0};

  /* формируем новое имя файла */
  if( mknewnm(p) != 0 )
    return -1;
  
  strcpy(oldpath, p->indir);
  strcat(oldpath, "/");
  strcat(oldpath, p->infnm);
  
  strcpy(newpath, p->outdir);
  strcat(newpath, "/");
  strcat(newpath, p->outfnm);

  rc = rename(oldpath, newpath);
  if(rc == 0)
  {
    log_info( p, "success move from %s to %s", oldpath, newpath);
    chown_outfnm(p);
  }
  else
  {
    log_error(p, "myutils.c moveToOut() rename() oldpath:%s "
                 "newpath:%s", oldpath, newpath);
    return -1;
  }
  
  return 0;
}

/* создаёт .gz файл */
int creGZfile(ksparam * p)
{
  if( compressFileToGZ(p) == 0 )
    chown_gzfnm(p);  
  else
    return -1;

  return 0;
}

int creMD5file(ksparam * p)
{
  FILE *fp;
  char md5FileNameWithPath[256] = {0};

  /* создаём имя md5 файла */
  strcpy(p->md5fnm, p->gzfnm);
  strcat(p->md5fnm, ".md5");
  /* создаём имя md5 файла с путём */
  strcpy(md5FileNameWithPath, p->outdir);
  strcat(md5FileNameWithPath, "/");
  strcat(md5FileNameWithPath, p->md5fnm);

  fp = fopen(md5FileNameWithPath, "w");
  if( fp == NULL)
  {
    log_error(p, "creMD5file() fopen()=NULL %s",md5FileNameWithPath);
    return -1;
  }
  
  if( cdrmd5(p) == -1 )
  {
    log_error(p, "myutils.c creMD5file() cdrmd5()==-1");
    fclose(fp);
    return -1;
  }

  if( fprintf(fp, "%s\n", p->md5str ) < 0 )
  {
    log_error(p, "myutils.c creMD5file() fprintf() md5str:%s to md5file:%s unsuccess", 
              p->md5str, p->md5fnm);
    fclose(fp);
    return -1;
  }

  fclose(fp);
  chown_md5fnm(p);
  log_info(p, "success create md5 file %s md5sum: %s", 
           p->md5fnm, p->md5str);

  return 0;
}

int creTARfile(ksparam * p)
{
  strcpy(p->tarfnm, p->outfnm);
  strcat(p->tarfnm, ".tar");
  write_tar_archive(p);
  chown_tarfnm_outdir(p);
  return 0;
}

int checkAvailSrv(ksparam *p)
{
  int port = 22;
  int res = 0;
  int sock;
  struct sockaddr_in server;

  sock = socket(AF_INET , SOCK_STREAM , 0);
  if(sock == -1)
  {
    log_debug("%s %s %d %s could not create socket, errno = %s",
              p->locallogin,__FILE__, __LINE__, __func__, strerror(errno));
  }

  server.sin_addr.s_addr = inet_addr(p->ipregspusmain);
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  res = connect(sock, (struct sockaddr *)&server, sizeof(server));
  if(res == 0)
  {
    strcpy(p->availsrv, p->ipregspusmain);
    log_info(p,"available server main IP addr: %s",p->availsrv);
    close(sock);
    return 0;
  }
  else
  {
    log_error(p, "can't connect with main IP addr: %s errno: %s",
                 p->ipregspusmain, strerror(errno));
  }

  server.sin_addr.s_addr = inet_addr(p->ipregspusbackup);
  res = connect(sock, (struct sockaddr *)&server, sizeof(server));
  if(res == 0)
  {
    strcpy(p->availsrv, p->ipregspusbackup);
    log_warn(p,"available server backup IP addr: %s",p->availsrv);
    close(sock);
    return 0;
  }
  else
  {
    log_error(p, "can't connect with backup IP addr: %s errno: %s",
                 p->ipregspusbackup, strerror(errno));
  }

  close(sock);

  return -1;
}

/* если всё хорошо то вернёт 0, иначе -1 */
int sendTarToRegSpus(ksparam *p)
{
  if( connect_with_regspus(p) == -1)
  {
    log_error(p, "myutils.c sendTarToRegSpus connect_with_regspus==-1");
    disconnect_with_regspus(p);
    return -1;
  }
  
  if( scp_tar_to_regspus(p) == -1 )
  {
    log_error(p, "myutils.c sendTarToRegSpus scp_tar_to_regspus send file %s on "
                 "regional SPUS unsuccessfulled", p->tarfnm);
    disconnect_with_regspus(p);
    return -1;
  }

  log_info(p, "success send tar file %s on %s", p->tarfnm, p->availsrv);
  disconnect_with_regspus(p);

  return 0;
}

/* если всё хорошо то вернёт 0 */
int checkTarOnRegSpus(ksparam *p)
{
  if( connect_with_regspus(p) == -1)
  {
    log_error(p, "myutils.c checkTarOnRegSpus connect_with_regspus==-1");
    disconnect_with_regspus(p);
    return -1;
  }

  if( checkfile_on_regspus(p) == -1)
  {
    log_error(p, "sendTarToRegSpus(). checkfile_on_regspus(). check file %s "
                 "on regional SPUS unsuccessfulled.", p->tarfnm);
    disconnect_with_regspus(p);
    return -1;
  }

  disconnect_with_regspus(p);

  return 0;
}

int moveTarToArchive(ksparam *p)
{
  char tarFileNameInOutFullPath[256] = {0};
  char tarFileNameInArchiveFullPath[256] = {0};

  strcpy(tarFileNameInOutFullPath, p->outdir);
  strcat(tarFileNameInOutFullPath, "/");
  strcat(tarFileNameInOutFullPath, p->tarfnm);
 
  strcpy(tarFileNameInArchiveFullPath, p->archivedir);
  strcat(tarFileNameInArchiveFullPath, "/");
  strcat(tarFileNameInArchiveFullPath, p->tarfnm);

  if( rename(tarFileNameInOutFullPath, tarFileNameInArchiveFullPath) == 0 )
  {
    log_info(p, "success move tar to archive %s", tarFileNameInArchiveFullPath);
    chown_tarfnm_archivedir(p); 
  }
  else
  {
    log_error(p, "moveTarToArchive rename return nonzero "
                 "for file: %s", tarFileNameInOutFullPath);
    return -1;
  }

  return 0;
}

int getErrFiles(ksparam *p)
{
  int counter_err_files = 0;

  if( connect_with_regspus(p) == -1)
  {
    log_error(p, "myutils.c getErrFiles() connect_with_regspus==-1");
    disconnect_with_regspus(p);
    return -1;
  }

  counter_err_files = recvErrFile(p);
  if( counter_err_files == -1)
  {
    log_error(p, "myutils.c getErrFiles() recvErrFile==-1 ");
    disconnect_with_regspus(p);
    return -1;
  }

  disconnect_with_regspus(p);

  chown_err_files(p);

  return counter_err_files;
}

void execLocalErrDir(ksparam *p)
{
  DIR *d;
  struct dirent *dir;
  FILE *fp;
  struct stat sb;
  void *file_buffer;
  int fd;

  d = opendir(p->errdir);
  if(d)
  {
    while((dir = readdir(d)) != NULL)
    {
      if(strstr(dir->d_name, "err") != NULL)
      {
        char errfn[256] = {0};
        sprintf(errfn, "%s/%s", p->errdir, dir->d_name);
        
        if(stat(errfn, &sb) == -1)
        {
          log_error(p, "myutils.c execLocalErrDir() can't stat %s", errfn);
          break;
        }
        
        fp = fopen(errfn, "r");
        if(fp == NULL)
        {
          log_error(p, "myutils.c execLocalErrDir() can't open err file %s", errfn);
          break;
        }
        
        fd = fileno(fp);
        if(fd == -1)
        {
          log_error(p,"myutils.c execLocalErrDir() fileno detects that its argument "
                      "is not a valid stream. file:", errfn);
          fclose(fp);
          break;
        }

        file_buffer = mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
        if((long long *)file_buffer == MAP_FAILED)
        {
          log_error(p,"myutils.c execLocalErrDir() mmap ret MAP_FAILED");
          fclose(fp);
          return;
        }

        size_t sizeCP1251buf = sb.st_size;
        size_t sizeUTF8buf = sizeCP1251buf * 2 + strlen(NOTICEADDMSG) * 2;
        p->errmail.email[5] = (char*)calloc(sizeUTF8buf, sizeof(char));
        char *pIn  = (char*)file_buffer;
        char *pOut = p->errmail.email[5];

        iconv_t conv = iconv_open("UTF-8","CP1251");
        iconv(conv, &pIn, &sizeCP1251buf, &pOut, &sizeUTF8buf);
        strcat(p->errmail.email[5], NOTICEADDMSG);
        sendemail(p);

        iconv_close(conv);
        free(p->errmail.email[5]);
        munmap(file_buffer, sb.st_size);
        fclose(fp);
        log_info(p, "was send email with content err file: %s", errfn);
      }
    }
    closedir(d);
  }
}

void chown_logfile(ksparam *p ) 
{
  uid_t          uid;
  gid_t          gid;
  struct passwd *pwd;
  struct group  *grp;

  pwd = getpwnam(p->locallogin);
  if (pwd == NULL) 
    log_error(p, "chown_logfile(). Failed to get uid: %s", p->locallogin);
  uid = pwd->pw_uid;

  grp = getgrnam(p->localgroup);
  if (grp == NULL)
    log_error(p, "chown_logfile(). Failed to get gid: %s", p->localgroup);
  gid = grp->gr_gid;

  if (chown(p->filelog, uid, gid) == -1) 
  {
    log_error(p, "do_chown(). chown fail.");
  }
}

void chown_outfnm(ksparam *p ) 
{
  uid_t          uid;
  gid_t          gid;
  struct passwd *pwd;
  struct group  *grp;
  char outfnmFullPath[256] = {0};

  strcpy(outfnmFullPath, p->outdir);
  strcat(outfnmFullPath, "/");
  strcat(outfnmFullPath, p->outfnm);

  pwd = getpwnam(p->locallogin);
  if (pwd == NULL) 
    log_error(p, "chown_outfnm(). Failed to get uid: %s", p->locallogin);
  uid = pwd->pw_uid;

  grp = getgrnam(p->localgroup);
  if (grp == NULL)
    log_error(p, "chown_outfnm(). Failed to get gid: %s", p->localgroup);
  gid = grp->gr_gid;

  if (chown(outfnmFullPath, uid, gid) == -1) 
  {
    log_error(p, "chown_outfnm(). chown fail.");
  }
}

void chown_gzfnm(ksparam *p ) 
{
  uid_t          uid;
  gid_t          gid;
  struct passwd *pwd;
  struct group  *grp;
  char gzfnmFullPath[256] = {0};

  strcpy(gzfnmFullPath, p->outdir);
  strcat(gzfnmFullPath, "/");
  strcat(gzfnmFullPath, p->gzfnm);

  pwd = getpwnam(p->locallogin);
  if (pwd == NULL) 
    log_error(p, "chown_gzfnm(). Failed to get uid: %s", p->locallogin);
  uid = pwd->pw_uid;

  grp = getgrnam(p->localgroup);
  if (grp == NULL)
    log_error(p, "chown_gzfnm(). Failed to get gid: %s", p->localgroup);
  gid = grp->gr_gid;

  if (chown(gzfnmFullPath, uid, gid) == -1) 
  {
    log_error(p, "chown_gzfnm(). chown fail.");
  }
}

void chown_md5fnm(ksparam *p ) 
{
  uid_t          uid;
  gid_t          gid;
  struct passwd *pwd;
  struct group  *grp;
  char md5fnmFullPath[256] = {0};

  strcpy(md5fnmFullPath, p->outdir);
  strcat(md5fnmFullPath, "/");
  strcat(md5fnmFullPath, p->md5fnm);

  pwd = getpwnam(p->locallogin);
  if (pwd == NULL) 
    log_error(p, "chown_md5fnm(). Failed to get uid: %s", p->locallogin);
  uid = pwd->pw_uid;

  grp = getgrnam(p->localgroup);
  if (grp == NULL)
    log_error(p, "chown_md5fnm(). Failed to get gid: %s", p->localgroup);
  gid = grp->gr_gid;

  if (chown(md5fnmFullPath, uid, gid) == -1) 
  {
    log_error(p, "chown_md5fnm(). chown fail.");
  }
}

void chown_tarfnm_outdir(ksparam *p ) 
{
  uid_t          uid;
  gid_t          gid;
  struct passwd *pwd;
  struct group  *grp;
  char tarfnmOutDirFullPath[256] = {0};

  strcpy(tarfnmOutDirFullPath, p->outdir);
  strcat(tarfnmOutDirFullPath, "/");
  strcat(tarfnmOutDirFullPath, p->tarfnm);

  pwd = getpwnam(p->locallogin);
  if (pwd == NULL) 
    log_error(p, "chown_tarfnm_outdir(). Failed to get uid: %s", p->locallogin);
  uid = pwd->pw_uid;

  grp = getgrnam(p->localgroup);
  if (grp == NULL)
    log_error(p, "chown_tarfnm_outdir(). Failed to get gid: %s", p->localgroup);
  gid = grp->gr_gid;

  if (chown(tarfnmOutDirFullPath, uid, gid) == -1) 
  {
    log_error(p, "chown_tarfnm_outdir(). chown fail.");
  }
}

void chown_tarfnm_archivedir(ksparam *p ) 
{
  uid_t          uid;
  gid_t          gid;
  struct passwd *pwd;
  struct group  *grp;
  char tarfnmArchiveDirFullPath[256] = {0};

  strcpy(tarfnmArchiveDirFullPath, p->archivedir);
  strcat(tarfnmArchiveDirFullPath, "/");
  strcat(tarfnmArchiveDirFullPath, p->tarfnm);

  pwd = getpwnam(p->locallogin);
  if (pwd == NULL) 
    log_error(p, "chown_tarfnm_archivedir(). Failed to get uid: %s", p->locallogin);
  uid = pwd->pw_uid;

  grp = getgrnam(p->localgroup);
  if (grp == NULL)
    log_error(p, "chown_tarfnm_archivedir(). Failed to get gid: %s", p->localgroup);
  gid = grp->gr_gid;

  if (chown(tarfnmArchiveDirFullPath, uid, gid) == -1) 
  {
    log_error(p, "chown_tarfnm_archivedir(). chown fail.");
  }
}

void chown_err_files(ksparam *p)
{
  DIR *d;
  struct dirent *dir;
  uid_t uid;
  gid_t gid;
  struct passwd *pwd;
  struct group  *grp;
  char errFileNameFullPath[256] = {0};
  struct stat sb;
 
  pwd = getpwnam(p->locallogin);
  if (pwd == NULL) 
    log_error(p, "chown_tarfnm_archivedir(). Failed to get uid: %s", p->locallogin);
  uid = pwd->pw_uid;

  grp = getgrnam(p->localgroup);
  if (grp == NULL)
    log_error(p, "chown_tarfnm_archivedir(). Failed to get gid: %s", p->localgroup);
  gid = grp->gr_gid;
 
  d = opendir(p->errdir);
  if(d)
  {
    while ((dir = readdir(d)) != NULL)
    {
      strcpy(errFileNameFullPath, p->errdir);
      strcat(errFileNameFullPath, "/");
      strcat(errFileNameFullPath, dir->d_name);
      if( stat(errFileNameFullPath, &sb) == -1 ) 
      {
        log_error(p,"myutils.c chown_err_files() stat==-1 %s", errFileNameFullPath);
      }
      else
      {
        if( (sb.st_mode & S_IFMT) == S_IFREG )
          if (chown(errFileNameFullPath, uid, gid) == -1)
            log_error(p, "myutils.c chown_err_files() chown fail: %s", errFileNameFullPath);
      }
    }
    closedir(d);
  }
}

