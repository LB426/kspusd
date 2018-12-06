#!/bin/bash

###############################################################################
# скрипт копирует ключи пользователей на AIX
###############################################################################

USERNAMES="k86154001 k86157020 k86157023 k86157024 k86157025 k86193012 k86193013
k86193015 k86193016 k86193014 k86193017 k86193002 k86138001 k86142001 k86142007
k86142021 k86161001 k86168025 k86168008 k86168011 k86168001 k86145001 k86145003
k86145002 k86145004 k86149017 k86191001 k86191020 k86191003 k86191005 k86158001
k86158002 k86196001 k86196023 k86135034 k86135001 k86135030"

SENDKEYFN="/tmp/sendkey"

for un in ${USERNAMES}
do
  echo "spawn scp /kspus/$un/.ssh/id_rsa.pub $un@10.152.46.123:/home/$un/.ssh/id_rsa.pub" > $SENDKEYFN
  echo 'expect "password:"' >> $SENDKEYFN
  echo "send mypassword\n;" >> $SENDKEYFN
  echo "interact" >> $SENDKEYFN
  expect $SENDKEYFN
done

rm -rf $SENDKEYFN
