#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libssh/libssh.h>
#include <ctype.h>
#include "toregspus.h"
#include "log.h"

int verify_knownhost(ksparam *p){
  char *hexa;
  int state;
  char buf[10];
  unsigned char *hash = NULL;
  size_t hlen;
  ssh_key srv_pubkey;
  int rc;
  ssh_session session = p->my_ssh_session;

  state=ssh_is_server_known(session);

  rc = ssh_get_server_publickey(session, &srv_pubkey);
  if (rc < 0)
    return -1;

  rc = ssh_get_publickey_hash(srv_pubkey,
                              SSH_PUBLICKEY_HASH_SHA1,
                              &hash,
                              &hlen);
  ssh_key_free(srv_pubkey);
  if (rc < 0)
    return -1;

  switch(state){
    case SSH_SERVER_KNOWN_OK:
      break; /* ok */
    case SSH_SERVER_KNOWN_CHANGED:
      log_error(p,"Host key for server changed : server's one is now");
      ssh_print_hexa("Public key hash",hash, hlen);
      ssh_clean_pubkey_hash(&hash);
      log_error(p,"For security reason, connection will be stopped");
      return -1;
    case SSH_SERVER_FOUND_OTHER:
      log_error(p,"The host key for this server was not found but an other "
                  "type of key exists.");
      log_error(p,"An attacker might change the default server key to confuse "
                  "your client into thinking the key does not exist. "
                  "We advise you to rerun the client with -d or -r for more "
                  "safety.");
      return -1;
    case SSH_SERVER_FILE_NOT_FOUND:
      log_error(p,"Could not find known host file. If you accept the host "
                  "key here,");
      log_error(p,"the file will be automatically created.");
    case SSH_SERVER_NOT_KNOWN:
      hexa = ssh_get_hexa(hash, hlen);
      log_error(p,"The server is unknown. Do you trust the host key ?");
      log_error(p,"Public key hash: %s", hexa);
      ssh_string_free_char(hexa);
      if (fgets(buf, sizeof(buf), stdin) == NULL)
      {
	      ssh_clean_pubkey_hash(&hash);
        return -1;
      }
      if(strncasecmp(buf,"yes",3)!=0)
      {
	      ssh_clean_pubkey_hash(&hash);
        return -1;
      }
      log_error(p, "This new key will be written on disk for further usage. "
                   "do you agree ?");
      if (fgets(buf, sizeof(buf), stdin) == NULL)
      {
	      ssh_clean_pubkey_hash(&hash);
        return -1;
      }
      if(strncasecmp(buf,"yes",3)==0)
      {
        if (ssh_write_knownhost(session) < 0)
        {
          ssh_clean_pubkey_hash(&hash);
          log_error(p, "error %s\n", strerror(errno));
          return -1;
        }
      }

      break;
    case SSH_SERVER_ERROR:
      ssh_clean_pubkey_hash(&hash);
      log_error(p,"%s",ssh_get_error(session));
      return -1;
  }
  ssh_clean_pubkey_hash(&hash);
  return 0;
}

/* если всё хорошо то вернёт 0, иначе -1 
 * все ресурсы выделяемые тут освобождаются в disconnect_with_regspus() */
int connect_with_regspus( ksparam *p )
{
  int port = 22;
  int rc = 0;

  p->my_ssh_session = ssh_new();
  if (p->my_ssh_session == NULL)
  {
    log_error(p,"toregspus.c connect_with_regspus() ssh_new() ret NULL");
    return -1;
  }
  
  ssh_options_set(p->my_ssh_session, SSH_OPTIONS_HOST, p->availsrv);
  ssh_options_set(p->my_ssh_session, SSH_OPTIONS_PORT, &port);
  ssh_options_set(p->my_ssh_session, SSH_OPTIONS_USER, p->remotelogin);

  rc = ssh_connect(p->my_ssh_session);
  if (rc != SSH_OK)
  {
    log_error(p,"%s %d %s IP addr: %s, %s", __FILE__, __LINE__, __func__,
                p->availsrv, ssh_get_error(p->my_ssh_session) );
    return -1;
  }

  /* Verify the server's identity */
  if ( verify_knownhost(p) < 0 )
  {
    log_error(p,"toregspus.c connect_with_regspus() verify_knownhost() < 0");
    return -1;
  }

  /* импортируем публичный ключ */
  /*
  rc = ssh_pki_import_pubkey_file( p->pubkeyfn, &p->pubkey );
  if (rc != SSH_OK)
  {
    if (rc == SSH_EOF)
      log_error(p, "%s %d %s sh_pki_import_pubkey_file() file doesn't exist or "
                "permission denied %s",__FILE__, __LINE__, __func__,
                p->pubkeyfn);
    if (rc == SSH_ERROR)
      log_error(p, "%s %d %s SSH_ERROR %s %s", __FILE__, __LINE__, __func__,
                p->pubkeyfn, ssh_get_error(p->my_ssh_session));
    rc = -1;
  }
  */
  /* конец импорта публичного ключа */
  /*log_info(p,"connect_with_regspus(). ssh_pki_import_pubkey_file() SSH_OK");*/

  /* Authenticate ourselves */
  rc = ssh_userauth_try_publickey(p->my_ssh_session, NULL, p->pubkey);
  if (rc != SSH_AUTH_INFO )
  {
    if (rc == SSH_AUTH_ERROR )
      log_error(p, "connect_with_regspus(). ssh_userauth_try_publickey(). "
                   "SSH_AUTH_ERROR: A serious error happened.");
    if (rc == SSH_AUTH_DENIED )
      log_error(p, "connect_with_regspus(). ssh_userauth_try_publickey(). "
                   "SSH_AUTH_DENIED: The server doesn't accept that public "
                   "key as an authentication token. Try another key or "
                   "another method.");
    if (rc == SSH_AUTH_PARTIAL)
      log_error(p, "connect_with_regspus(). ssh_userauth_try_publickey(). "
                   "Error string: You've been partially authenticated, you "
                   "still have to use another method.");
    rc = -1;
  }
  /* log_info(p, "connect_with_regspus(). ssh_userauth_try_publickey(). "
                "SSH_AUTH_INFO"); */

  /* импорт закрытого ключа */
  /*
  rc = ssh_pki_import_privkey_file( p->privkeyfn, NULL, NULL, NULL,
                                   &p->privkey);
  if (rc != SSH_OK)
  {
    if (rc == SSH_EOF)
      log_debug("%s %d %s file doesn't exist or permission denied %s",
                __FILE__, __LINE__, __func__, p->privkeyfn);
    if (rc == SSH_ERROR)
      log_debug("%s %d %s SSH_ERROR %s %s",__FILE__, __LINE__, __func__,
                 p->privkeyfn, ssh_get_error(p->my_ssh_session));
    rc = -1;
  }
  */
  /* конец импорта закрытого ключа */
  /* log_info(p, "connect_with_regspus(). ssh_pki_import_privkey_file() "
                "SSH_OK"); */

  rc = ssh_userauth_publickey(p->my_ssh_session, NULL, p->privkey);
  if (rc != SSH_AUTH_SUCCESS )
  {
    if (rc == SSH_AUTH_ERROR )
      log_error(p, "toregspus.c connect_with_regspus ssh_userauth_publickey "
                   "SSH_AUTH_ERROR: A serious error happened.");
    if (rc == SSH_AUTH_DENIED )
      log_error(p, "toregspus.c connect_with_regspus ssh_userauth_publickey "
                   "SSH_AUTH_DENIED: The server doesn't accept that "
                   "public key as an authentication token. Try another "
                   "key or another method.");
    if (rc == SSH_AUTH_PARTIAL)
      log_error(p, "toregspus.c connect_with_regspus ssh_userauth_publickey "
                   "You've been partially authenticated, you still have "
                   "to use another method.");
    if (rc == SSH_AUTH_AGAIN)
      log_error(p,"toregspus.c connect_with_regspus ssh_userauth_publickey "
                  "In nonblocking mode, you've got to call this again later");
    rc = -1;
  }
  /* log_info(p, "connect_with_regspus ssh_userauth_publickey "
                "SSH_AUTH_SUCCESS"); */

  return rc;
}

int disconnect_with_regspus( ksparam *p )
{
  if( ssh_is_connected(p->my_ssh_session) == 1 )
    ssh_disconnect(p->my_ssh_session);
  ssh_free(p->my_ssh_session);
/* ssh_key_free(p->pubkey);
  ssh_key_free(p->privkey); 
  ssh_free(p->my_ssh_session); */
  return 0;
}

/* если всё хорошо то вернёт 0, иначе -1 */
int scp_tar_to_regspus( ksparam *p )
{
  ssh_scp scp;
  int rc = 0;
  int length;
  FILE *fp;
  int len;
  unsigned char buf[1024];
  struct stat st;
  char tarFileNameWithPath[256] = {0};
  
  strcpy(tarFileNameWithPath, p->outdir);
  strcat(tarFileNameWithPath, "/");
  strcat(tarFileNameWithPath, p->tarfnm);

  scp = ssh_scp_new(p->my_ssh_session, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, p->rmtindir);
  if (scp == NULL)
  {
    log_error(p, "scp_tar_to_regspus(). ssh_scp_new(). Allocating scp session: %s",
              ssh_get_error(p->my_ssh_session));
    return -1;
  }

  rc = ssh_scp_init(scp);
  if (rc != SSH_OK)
  {
    log_error(p, "scp_tar_to_regspus(). ssh_scp_init(). initializing scp session: %s",
              ssh_get_error(p->my_ssh_session));
    ssh_scp_free(scp);
    return -1;
  }

  stat(tarFileNameWithPath, &st);
  length = st.st_size;
  rc = ssh_scp_push_file(scp, tarFileNameWithPath, length, S_IRUSR | S_IWUSR);
  if (rc != SSH_OK)
  {
    log_error(p, "scp_tar_to_regspus(). ssh_scp_push_file(). can't open remote file: %s",
              ssh_get_error(p->my_ssh_session));
    ssh_scp_close(scp);
    ssh_scp_free(scp);
    return -1;
  }

  fp = fopen(tarFileNameWithPath, "r");
  if( fp == NULL)
  {
    log_error(p, "toregspus.c scp_tar_to_regspus fopen=NULL %s",tarFileNameWithPath);
    return -1;
  }
  len = fread(buf, 1, sizeof(buf), fp);
  while ( len > 0 )
  {
    if( ssh_scp_write(scp, buf, len) != SSH_OK )
    {
      log_error(p,"toregspus.c scp_tar_to_regspus ssh_scp_write can't write to remote "
                  "file:%s errstr:%s", p->tarfnm, ssh_get_error(p->my_ssh_session));
      fclose(fp);
      ssh_scp_close(scp);
      ssh_scp_free(scp);
      return -1;
    }
    len = fread(buf, 1, sizeof(buf), fp);
  }
  fclose(fp);

  ssh_scp_close(scp);
  ssh_scp_free(scp);

  return 0;
}

/* если всё хорошо то вернёт 0, иначе -1 */
int checkfile_on_regspus( ksparam *p )
{
  ssh_channel channel;
  int rc = 0;
  char buffer[256];
  int nbytes;
  char cmd[2048] = {0};
  char rescmd[8192] = {0};
  char * pResCmd = rescmd;
  int cntr = sizeof(rescmd) - 1;
  int cnbytes = 0;

  channel = ssh_channel_new(p->my_ssh_session);
  if (channel == NULL)
  {
    log_error(p, "checkfile_on_regspus(). ssh_channel_new(). Error string: %s",
              ssh_get_error(p->my_ssh_session));
    return -1;
  }

 /* когда нет ключа в authorized_keys ошибка выпадает тут примерно через 5 минут
  * а до этого просто зависон */
  rc = ssh_channel_open_session(channel);
  if (rc != SSH_OK)
  {
    ssh_channel_free(channel);
    log_error(p, "checkfile_on_regspus(). ssh_channel_open_session(). Error string: %s",
              ssh_get_error(p->my_ssh_session));
    return -1;
  }
  
  strcpy(cmd, p->rmtcmd);
  strcat(cmd, " ");
  strcat(cmd, p->tarfnm);
  strcat(cmd, " ; echo $?");
  log_info(p,"attemt exec on ESP2 cmd: '%s'", cmd);

  rc = ssh_channel_request_exec(channel, cmd);
  if (rc != SSH_OK)
  {
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    log_error(p, "checkfile_on_regspus(). ssh_channel_request_exec(). Error string: %s",
              ssh_get_error(p->my_ssh_session));
    return -1;
  }

  memset(buffer, 0, sizeof(buffer));
  memset(rescmd, 0, sizeof(rescmd));
  nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
  while (nbytes > 0)
  {
    pResCmd = (char*)mempcpy((void*)pResCmd, (void*)buffer, nbytes);
    memset(buffer, 0, sizeof(buffer));
    cntr = cntr - nbytes;
    if( cntr <= 0 )
    {
      log_error(p,"toregspus.c checkfile_on_regspus() buffer rescmd overflow");
      break;
    }
    cnbytes = cnbytes + nbytes; /* считаем сколько байт прилетело */
    /* log_info(p, "nbytes=%d cnbytes=%d rescmd='%s'", nbytes, cnbytes, rescmd); */
    if (nbytes == SSH_ERROR)
    {
      log_error(p, "while() checkfile_on_regspus ssh_channel_read ret SSH_ERROR. Error string: %s",
                   ssh_get_error(p->my_ssh_session));
      ssh_channel_send_eof(channel);
      ssh_channel_close(channel);
      ssh_channel_free(channel);
      return -1;
    }
    nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
  }

  /* log_debug("nbytes=%d cnbytes=%d strlen=%d rescmd='%s'", nbytes, cnbytes, strlen(rescmd), rescmd); */
 
  if (nbytes == SSH_ERROR)
  {
    log_error(p, "checkfile_on_regspus ssh_channel_read ret SSH_ERROR. Error string: %s",
                 ssh_get_error(p->my_ssh_session));
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    return -1;
  }

  ssh_channel_send_eof(channel);
  ssh_channel_close(channel);
  ssh_channel_free(channel);

  /* заменяю символ \n новой строки в конце прилетевших данных на 0 */
  rescmd[cnbytes-1] = 0;

  /* код ошибки будет в конце прилетевшей строки
   * и он состоит из 1 или 2-х цифровых символов
   * минимальное число байт возвращаемое сервером - 2
   * один это цифра 0 и второй это символ перевода строки
   * */

  /* индекс последнего символа в полученной строке */
  int last = cnbytes-2;

  /* проверяем что сервер вернул 0 */
  if( rescmd[last] == '0' )
  {
    log_info(p, "success checking transferred file: %s OK",
             p->tarfnm);
    return 0;
  }

  /* если сервер не вернул 0 */
  /* индекс предпоследнего символа в полученной строке */
  int penult = last-1;

  /* проверяем что сервер вернул 11 */
  if( (rescmd[penult] == '1') && (rescmd[last] == '1') )
  {
    log_error(p, "toregspus.c checkfile_on_regspus(). retcode 11. unable to open file %s. "
                 "Need to transfer the file again!", p->tarfnm);
    return 0;
  }
  /* проверяем что сервер вернул 12 */
  if( (rescmd[penult] == '1') && (rescmd[last] == '2') )
  {
    log_error(p, "toregspus.c checkfile_on_regspus(). retcode 12. unable to open file %s. "
                 "Need to transfer the file again!", p->tarfnm);
    return 0;
  }
  /* проверяем что сервер вернул 13 */
  if( (rescmd[penult] == '1') && (rescmd[last] == '3') )
  {
    log_error(p, "toregspus.c checkfile_on_regspus(). retcode 13. unable to open file %s. "
                 "Need to transfer the file again!", p->tarfnm);
    return 0;
  }
  /* проверяем что сервер вернул 14 */
  if( (rescmd[penult] == '1') && (rescmd[last] == '4') )
  {
    log_error(p,"toregspus.c checkfile_on_regspus(). retcode 14. %s "
                "md5-file is not changed to ok-file. CDR file is not "
                "adopted system for further processing. "
                "The file will be transfer again the next time.", p->tarfnm);
    return 0;
  }
  
  log_error(p,"toregspus.c checkfile_on_regspus(). retcode unknown. "
              "rescmd='%s'",rescmd);

  return 0;
}

int recvErrFile( ksparam *p )
{
  ssh_channel channel;
  int rc = 0;
  char buffer[256];
  int nbytes;
  char cmd[2048] = {0};
  char rescmd[8192] = {0};
  char * pResCmd = rescmd;
  int cntr = sizeof(rescmd) - 1;
  int bc = 0;
  ssh_scp scp;
  int len = 0;
  int counter_err_files = 0;

  channel = ssh_channel_new(p->my_ssh_session);
  if (channel == NULL)
  {
    log_error(p, "toregspus.c recvErrFile ssh_channel_new(). Error string: %s",
              ssh_get_error(p->my_ssh_session));
    return -1;
  }

  rc = ssh_channel_open_session(channel);
  if (rc != SSH_OK)
  {
    ssh_channel_free(channel);
    log_error(p, "toregspus.c recvErrFile ssh_channel_open_session(). Error string: %s",
              ssh_get_error(p->my_ssh_session));
    return -1;
  }
  
  strcpy(cmd, "ls -1 ~/cdrin/err/*.err");
  log_debug("%s toregspus.c recvErrFile() attempt exec command: '%s'",
            p->locallogin, cmd);

  rc = ssh_channel_request_exec(channel, cmd);
  if (rc != SSH_OK)
  {
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    log_error(p, "toregspus.c recvErrFile ssh_channel_request_exec() error string: %s",
              ssh_get_error(p->my_ssh_session));
    return -1;
  }

  nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
  while (nbytes > 0)
  {
    pResCmd = (char*)mempcpy((void*)pResCmd, (void*)buffer, nbytes);
    cntr = cntr - nbytes;
    if( cntr <= 0 )
    {
      log_warn(p, "toregspus.c recvErrFile buffer rescmd overflow");
      break;
    }
    bc = bc + nbytes;
    nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
  }
 
  if (nbytes == SSH_ERROR)
  {
    log_error(p, "toregspus.c recvErrFile ssh_channel_read ret SSH_ERROR. Error string: %s",
                 ssh_get_error(p->my_ssh_session));
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    return -1;
  }
  ssh_channel_send_eof(channel);
  ssh_channel_close(channel);
  ssh_channel_free(channel);

  /* подсчитываем сколько имён файлов пришло. 
   * имя файла заканчивается \n - 10 десятичной */
  for(int i=0; i < bc; i++)
  {
    if( rescmd[i] == 10 )
      counter_err_files = counter_err_files + 1;
  }
  if(counter_err_files > 0)
    log_info(p, "exist %d err files on %s", counter_err_files, p->availsrv);
  /* поскольку заранее неизвестно сколько err файлов будет
   * начинается этот геморрой с указателями на указатель !!! 
   * выделяем память под хранение указателей на имена файлов err
   * меняться не будет */
  char **pArrayRemoteErrFileNames = malloc( sizeof(char*) * counter_err_files );
  if(pArrayRemoteErrFileNames == NULL)
  {
    log_error(p, "toregspus.c recvErrFile malloc==NULL. "
                 "No enought memory space.");
    return -1;
  }
  /* указатель на текущую ячейку, будет меняться */
  char **errfnm = pArrayRemoteErrFileNames;
  pResCmd = rescmd;
  /* пробегаем буфер с ls от сервера, все CR меняем на 0
   * запоминаем адреса начала строк с именами err файлов в памяти по
   * указателю pArrayRemoteErrFileNames */
  for(int i=0; i < bc; i++)
  {
    if( rescmd[i] == 10 )
    {
      rescmd[i] = 0;
      int j = i;
      for(;;)
      {
        j=j-1;
        if(j < 0)
        {
          *errfnm = &rescmd[j+1];
          log_error(p, "toregspus.c recvErrFile problem with name err file."
                       " j<=0 . Output command 'ls -1 ~/cdrin/err/*.err' do not has slash '/' ");
          break;
        }
        if(rescmd[j] == '/')
        {
          *errfnm = &rescmd[j+1];
          log_info(p, "recvErrFile. Name err file: %s", *errfnm);
          break;
        }
      }
      /* *errfnm = pResCmd; */
      /* printf("loop1 errfnm: %p  *errfnm: %p  %s  pResCmd:%p\n", errfnm, *errfnm, *errfnm, pResCmd); */
      pResCmd = pResCmd + i + 1;
      errfnm++;
    }
  }
  /* cтавим указатель на текущую ячейку на начало */
  errfnm = pArrayRemoteErrFileNames;
  /* начинаем получать файлы */
  for(int i=0; i < counter_err_files; i++)
  {
    char fnerr_remote[256] = {0};
    char fnerr_local[256] = {0};
    FILE *fp = NULL;
    strcpy(fnerr_remote, "~/cdrin/err/"); 
    strcat(fnerr_remote, *errfnm);
    strcpy(fnerr_local, p->errdir);
    strcat(fnerr_local, "/");
    strcat(fnerr_local, *errfnm);
    errfnm++;

    /* открываю SCP сессию для получения файла */
    scp = ssh_scp_new(p->my_ssh_session, SSH_SCP_READ, fnerr_remote);
    if (scp == NULL)
    {
      log_error(p, "toregspus.c recvErrFile Allocating scp session: %s",
                ssh_get_error(p->my_ssh_session));
      free(pArrayRemoteErrFileNames);
      return -1;
    }
    rc = ssh_scp_init(scp);
    if (rc != SSH_OK)
    {
      log_error(p,"toregspus.c recvErrFile ssh_scp_init(). "
                  "initializing scp session: %s",
                  ssh_get_error(p->my_ssh_session));
      ssh_scp_free(scp);
      free(pArrayRemoteErrFileNames);
      return -1;
    }

    /* получаю информацию о файле. здесь я её не использую */
    rc = ssh_scp_pull_request(scp);
    if (rc != SSH_SCP_REQUEST_NEWFILE)
    {
      if( rc == SSH_SCP_REQUEST_WARNING)
        log_error(p,"toregspus.c recvErrFile SSH_SCP_REQUEST_WARNING: "
                    "The other side sent us a warning");
      else if( rc == SSH_ERROR)
        log_error(p,"toregspus.c recvErrFile SSH_ERROR: "
                    "Some error happened");
      else
        log_error(p,"toregspus.c recvErrFile Error receiving information about "
                    "file: %s\n",ssh_get_error(p->my_ssh_session));
      free(pArrayRemoteErrFileNames);
      ssh_scp_close(scp);
      ssh_scp_free(scp);
      return -1;
    }
    /* end */
    
    fp = fopen(fnerr_local, "w");
    if( fp == NULL)
    {
      log_error(p, "toregspus.c recvErrFile fopen()=NULL %s",fnerr_local);
      free(pArrayRemoteErrFileNames);
      ssh_scp_close(scp);
      ssh_scp_free(scp);
      return -1;
    }
    
    /* начинаю приём файла */
    ssh_scp_accept_request(scp); /* зачем это нужно я не понял, но так есть в примере */
    /* начинаю чтение данных файла которые уже переданы */
    len = ssh_scp_read(scp, buffer, sizeof(buffer));
    if (len == SSH_ERROR)
    {
      log_error(p, "toregspus.c recvErrFile Error receiving file data. "
                   "Filename %s. Error: %s", fnerr_remote, ssh_get_error(p->my_ssh_session));
      fclose(fp);
      free(pArrayRemoteErrFileNames);
      ssh_scp_close(scp);
      ssh_scp_free(scp);
      return -1;
    }
    while(1)
    {
      /* пишем в локальный файл */
      rc = fwrite(buffer,1,len,fp);
      if( (rc < len) || (rc == 0) )
      {
        log_error(p,"toregspus.c recvErrFile fwrite error, file: %s",
                    fnerr_local);
        fclose(fp);
        free(pArrayRemoteErrFileNames);
        ssh_scp_close(scp);
        ssh_scp_free(scp);
        return -1;
      }

      /* проверяю конец файла */
      rc = ssh_scp_pull_request(scp);
      if (rc == SSH_SCP_REQUEST_EOF)
        break;

      ssh_scp_accept_request(scp);
      /* начинаю чтение данных файла которые уже переданы */
      len = ssh_scp_read(scp, buffer, sizeof(buffer));
      if (len == SSH_ERROR)
      {
        log_error(p, "toregspus.c recvErrFile Error receiving file data. "
                     "Filename %s. Error: %s", fnerr_local, ssh_get_error(p->my_ssh_session));
        fclose(fp);
        free(pArrayRemoteErrFileNames);
        ssh_scp_close(scp);
        ssh_scp_free(scp);
        return -1;
      }
    }
    /* конец приёма файла */

    fclose(fp);
    ssh_scp_close(scp);
    ssh_scp_free(scp);

    log_info(p, "Recived err file: %s", fnerr_local);
  }
  free(pArrayRemoteErrFileNames);
  
  return counter_err_files;
}
