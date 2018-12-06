Сколько файлов в каталогах:
for f in `find /kspus -name "in" -print`; do X=`ls -1 $f | wc -l` ; echo "$X $f"; done
for f in `find /kspus -name "out" -print`; do X=`ls -1 $f | wc -l` ; echo "$X $f"; done
for f in `find /kspus -name "archive" -print`; do X=`ls -1 $f | wc -l` ; echo "$X $f"; done

Показать kspus.cfg:
for f in `find /kspus -name "kspus.cfg" -print`; do echo $f; done

есть ли kspus.log:
for f in `find /kspus -name "kspus.log" -print`; do echo $f; done

Показать файлы на AIX:
find -L /home -regextype posix-egrep -regex ".*\.(tar|gz|OK)$"

Очистить файлы на collector:
for f in `find /kspus -name "archive" -print`; do rm -rf $f/*; done
for f in `find /kspus -name "in" -print`; do rm -rf $f/*; done
for f in `find /kspus -name "out" -print`; do rm -rf $f/*; done
for f in `find /kspus -name "kspus.log" -print`; do rm -rf $f; done

Очистить файлы на AIX:
find -L /home -regextype posix-egrep -regex ".*\.(tar|gz|OK)$" -exec rm -f {} \;

Вывод лога на экран:
отключить:
for f in `find /kspus -name "kspus.cfg" -print`; do sed -i 's/stdoutlog=yes/stdoutlog=no/g' $f; done
включить:
for f in `find /kspus -name "kspus.cfg" -print`; do sed -i 's/stdoutlog=no/stdoutlog=yes/g' $f; done

Установить время на СПУС кластерах:
sudo sed -i 's/ntp1.vniiftri.ru/10.152.8.2/g' /etc/ntp.conf ; sudo service ntpd stop ; sudo ntpdate 10.152.8.2 ; sudo service ntpd start ; date

Добавить namelen в файл kspus.cfg
for f in `find /kspus -name "kspus.cfg" -print`; do sed -i 's/zz=00/zz=00\nnamelen=/g' $f; done

Подсчитать количество файлов во всех директориях out:
Y=0 ; for f in `find /r0/kspus -name "out" -print`; do X=`ls -1 $f | wc -l` ; Y=`echo $Y+$X|bc` ; echo "dir: $f fs: $X"; done ; echo "files in all dirs out: $Y"

Очистить swap: swapoff -a && swapon -a

Посмотреть сколько памяти занимет процесс: ps_mem -p `cat /var/run/kspusd.pid`
Посмотреть сколько памяти занимют все процессы: ps_mem

Переместить все файлы из одного каталога в другой совпадающие по маске:
for f in `find ./archive -name "*_2007*" -print`;do echo "mv $f ./archive2007/`echo $f|awk -F'/' '{print $3}'`"; mv $f ./archive2007/`echo $f|awk -F'/' '{print $3}'`; done
