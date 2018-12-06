#include "ksparam.h"

#ifndef MYUTILS_H
#define MYUTILS_H

#define NOTICEADDMSG "---\r\nЕсли Вам надоело получать это сообщение устраните ошибки связанные с err файлами.\r\nЗатем удалите все файлы из каталога err на региональном коллекторе и на своём сервере.\r\n"

int moveToOut(ksparam *p);
int creGZfile(ksparam *p);
int creMD5file(ksparam *p);
int creTARfile(ksparam *p);
int checkAvailSrv(ksparam *p);
int sendTarToRegSpus(ksparam *p);
int checkTarOnRegSpus(ksparam *p);
int moveTarToArchive(ksparam *p);
int getErrFiles(ksparam *p);
void execLocalErrDir(ksparam *p);
void chown_logfile(ksparam *p);
void chown_outfnm(ksparam *p);
void chown_gzfnm(ksparam *p);
void chown_md5fnm(ksparam *p); 
void chown_tarfnm_outdir(ksparam *p);
void chown_tarfnm_archivedir(ksparam *p);
void chown_err_files(ksparam *p);

#endif
