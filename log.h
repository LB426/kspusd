#ifndef LOG_H 
#define LOG_H

void log_error ( ksparam *p, const char* message, ... );
void log_info  ( ksparam *p, const char* message, ... );
void log_warn  ( ksparam *p, const char* message, ... );
void log_debug ( const char* message, ... );

#endif
