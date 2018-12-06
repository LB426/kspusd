#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>
#include "ksparam.h"
#include "log.h"

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
  ksparam *p = (ksparam*)userp;
  const char *data;
 
  if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1))
  {
    return 0;
  }
 
  data = p->errmail.email[p->errmail.lines_read];
 
  /* printf("p->errmail.lines_read = %d\n", p->errmail.lines_read); 
  printf("datalen = %d data = '%s'\n", (int)strlen(data), data); */
  if(data) 
  {
    size_t len = strlen(data);
    memcpy(ptr, data, len);
    p->errmail.lines_read++;
 
    return len;
  }
 
  return 0;
}

void sendemail(ksparam *p)
{
  CURL *curl;
  CURLcode res = CURLE_OK;
  struct curl_slist *recipients = NULL;
  size_t len;
  char hostname[64];
  char To[128];
  char From[128];
  char Subject[256];
  char ConType[64];
  char emptyline[3] = { "\r\n\0" };
  char sender[64];

  if(p->errmail.addr1 == NULL)
  {
    log_info(p, "End of send notice. Email1 not set in kspus.cfg");
    return;
  }

  gethostname(hostname, 64);
  sprintf(sender, "%s@%s", p->locallogin, hostname);

  p->errmail.lines_read = 0;
  
  sprintf(To,      "To: %s\r\n", p->errmail.addr1);
  sprintf(From,    "From: %s\r\n", sender);
  sprintf(Subject, "Subject: err file %s\r\n", sender);
  sprintf(ConType, "Content-Type: text/plain; charset=utf-8\r\n");

  len = strlen(To);
  p->errmail.email[0] = (char*)calloc(len+1, sizeof(char)); /* TO */
  memcpy(p->errmail.email[0], To, len);

  len = strlen(From);
  p->errmail.email[1] = (char*)calloc(len+1, sizeof(char)); /* FROM */
  memcpy(p->errmail.email[1], From, len);

  len = strlen(Subject);
  p->errmail.email[2] = (char*)calloc(len+1, sizeof(char)); /* SUBJECT */
  memcpy(p->errmail.email[2], Subject, len);

  len = strlen(ConType);
  p->errmail.email[3] = (char*)calloc(len+1, sizeof(char)); /* Content-Type */
  memcpy(p->errmail.email[3], ConType, len);

  p->errmail.email[4] = (char*)calloc(3, sizeof(char));     /* empty line to divide headers from body, see RFC5322 */
  memcpy(p->errmail.email[4], emptyline, 3);

  /* p->errmail.email[5] это BODY письма. 
   * заполняется в myutils.c функция execLocalErrDir() */

  p->errmail.email[6] = (char*)calloc(3, sizeof(char));
  memcpy(p->errmail.email[6], emptyline, 3);

  p->errmail.email[7] = NULL;
 
  curl = curl_easy_init();
  if(curl)
  {
    curl_easy_setopt(curl, CURLOPT_URL, "smtp://localhost");
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, sender);
    recipients = curl_slist_append(recipients, p->errmail.addr1);
    if(p->errmail.addr2 != NULL)
      recipients = curl_slist_append(recipients, p->errmail.addr2);
    if(p->errmail.addr3 != NULL)
      recipients = curl_slist_append(recipients, p->errmail.addr3);
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt(curl, CURLOPT_READDATA, p);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
      log_error(p, "sendemail.c sendemail() curl_easy_perform() failed: %s",
                       curl_easy_strerror(res));
    }
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
  }

  free(p->errmail.email[0]);
  free(p->errmail.email[1]);
  free(p->errmail.email[2]);
  free(p->errmail.email[3]);
  free(p->errmail.email[4]);
  free(p->errmail.email[6]);
}
