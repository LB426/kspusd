#!/bin/bash

###############################################################################
# скрипт создаёт пользователей AIX и генерирует им ключи
# исполняется на AIX
###############################################################################

USERNAMES="k86154001 k86157020 k86157023 k86157024 k86157025 k86193012 k86193013
k86193015 k86193016 k86193014 k86193017 k86193002 k86138001 k86142001 k86142007
k86142021 k86161001 k86168025 k86168008 k86168011 k86168001 k86145001 k86145003
k86145002 k86145004 k86149017 k86191001 k86191020 k86191003 k86191005 k86158001
k86158002 k86196001 k86196023 k86135034 k86135001 k86135030"
GIDKSPUS=`getent group kspus | awk -F":" '{print $3}'`

echo "Создаю пользователей на сервере collector"
for un in ${USERNAMES} 
do
  unexist=`cat /etc/passwd|grep $un`
  if [ -z "$unexist" ]
  then
    echo "create user $un"
    useradd --gid $GIDKSPUS -m --skel /home/kspus $un
    echo mypassword | passwd --stdin $un
    usermod -G sshusers -a $un
    CDRDIR="/u03/KRD/861/`echo $un | cut -c 5-`"
    mkdir $CDRDIR
    chown kspus:kspus $CDRDIR
    chmod 770 $CDRDIR
    su - $un -c "ln -s $CDRDIR ~/cdrin"
    su - $un -c "mkdir ~/cdrin/err ~/cdrin/out ~/.ssh"
    su - $un -c "chmod 700 ~/.ssh"
  else
    echo "USER EXIST: $un"
  fi
done
