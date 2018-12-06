#!/bin/bash

###############################################################################
# скрипт генерирует файл kspus.cfg для всех пользователей kXXX_чегото-там
# файлы создаются в директории $HOME/.kspus/kspus.cfg
# на моём тестовом сервере домашние каталоги пользователей системы kspus
# лежат в /kspus например для пользователя k86196001 файл kspus.cfg будет 
# создан в каталоге /kspus/k86196001/.kspus/kspus.cfg
# исполняется на collector
###############################################################################

USERNAMES="k86154001 k86157020 k86157023 k86157024 k86157025 k86193012 k86193013
k86193015 k86193016 k86193014 k86193017 k86193002 k86138001 k86142001 k86142007
k86142021 k86161001 k86168025 k86168008 k86168011 k86168001 k86145001 k86145003
k86145002 k86145004 k86149017 k86191001 k86191020 k86191003 k86191005 k86158001
k86158002 k86196001 k86196023 k86135034 k86135001 k86135030"

for un in ${USERNAMES} 
do
#  if [ ! -d "/kspus/$un/.kspus" ]; then
#    mkdir /kspus/$un/.kspus
#    chown $un:kspus /kspus/$un/.kspus
#  fi
#  if [ ! -f "/kspus/$un/.kspus/kspus.cfg" ]; then
    cat > /kspus/$un/.kspus/kspus.cfg <<END
[DIRS]
in=/kspus/$un/in
out=/kspus/$un/out
err=/kspus/$un/err
archive=/kspus/$un/archive

[AUTH]
pubkey=/kspus/$un/.ssh/id_rsa.pub
privkey=/kspus/$un/.ssh/keys/id_rsa
locallogin=$un
localgroup=kspus

[REGIONALSPUS]
ip=10.152.46.123
remotelogin=$un
rmtindir=~/cdrin
rmtcmd=~/system/checkfile.sh ~/cdrin

[CDR]
regcode=`echo $un | cut -c 5-`
cdrprefix=
atstype=
atsnum=
zz=00

[LOGGING]
filelog=/kspus/$un/kspus.log
syslog=yes
stdoutlog=yes
END
    chown $un:kspus /kspus/$un/.kspus/kspus.cfg
    chmod 660 /kspus/$un/.kspus/kspus.cfg
#fi
done
