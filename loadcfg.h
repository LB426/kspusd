#ifndef LOADCFG_H 
#define LOADCFG_H

int fparams(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf);
int reloadcfg(ksparam *p);

#endif
