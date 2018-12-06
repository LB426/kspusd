#include "ksparam.h"

#ifndef TOREGSPUS_H
#define TOREGSPUS_H


int verify_knownhost( ksparam *p );

int connect_with_regspus( ksparam *p );

int disconnect_with_regspus( ksparam *p );

int scp_tar_to_regspus( ksparam *p );

int checkfile_on_regspus( ksparam *p );

int recvErrFile( ksparam *p );

#endif
