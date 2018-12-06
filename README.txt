Программа для передачи тарификационных файлов на региональный кластер СПУС.

установка библиотек необходимых для выполнения make:
продакшн сервер CentOS 7: yum install openssl openssl-libs openssl-devel libssh libssh-devel libarchive libarchive-devel glib2 glib2-devel libcurl-devel curl libcurl
девелоперская машина Linux Mint 18.2: sudo apt install libglib2.0-0 libglib2.0-dev libglib2.0-data libssh-4 libssh-dbg libssh-dev libssh-doc libarchive-dev libcurl4-openssl-dev

Удалить пользователя: userdel -f -r k86157020

Настройки тестовой среды.
1) Создание обычного пользователя системы СПУС на сервере AIX:
su - root
useradd --gid 1002 -m --skel /home/kspus k86157020
echo mypassword | passwd --stdin k86157020
usermod -G sshusers -a k86157020
mkdir /u03/KRD/861/57020
chown kspus:kspus /u03/KRD/861/57020
chmod 770 /u03/KRD/861/57020
su - k86157020
ln -s /u03/KRD/861/57020 ~/cdrin
mkdir ~/cdrin/err ~/cdrin/out ~/.ssh
chmod 700 ~/.ssh
exit

2) Создание обычного пользователя системы СПУС на сервере collector:
su - root
useradd -b /kspus --gid 1002 -m --skel /home/kspus k86157020
echo mypassword | passwd --stdin k86157020
su - k86157020
cd ~/.ssh
ssh-keygen -f $HOME/.ssh/id_rsa -t rsa -N ''
# сделаем обмен ключами
cat id_rsa.pub 
# копируем вывод - "открытый ключ пользователя k86157020" в буфер обмена
# и вставляем в файл authorized_keys на сервере AIX
# делаем так:
# заходим на сервер AIX под пользователем k86157020 
cat >> ~/.ssh/authorized_keys
# вставить ранее скопированный в буфер обмена ключ, нажать ENTER и нажать Ctrl+C
# на сервере AIX делаем так:
cat /etc/ssh/ssh_host_rsa_key.pub
# копируем вывод - "открытый ключ сервера AIX" в буфер и вставляем в файл 
# ~/.ssh/known_hosts пользователя k86157020 на сервере collector 
# таким образом чтобы получилась строка вида:
# <IP сервера> пробел <строка из /etc/ssh/ssh_host_rsa_key.pub сервера AIX>
# проверяем беспарольный вход для пользователя k86157020 с сервера
# collector на сервер AIX:
ssh k86157020@10.152.46.123
# если всё было сделано правильно то увидим приглашение коммандной строки сервера AIX
exit

3)Добавить в /root/.ssh/known_hosts строку вида:
10.144.200.148 ecdsa-sha2-nistp256 AAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAyNTYAAABBBJQTfc8DarWCLvn3vAWgyBrPf7eESpdcOZKHsF0ZKFDfXaUVzdw/ZCX2QUgCKtKD92w5sFRBT61SWAv/fG2DP2A=
как добавить её вручную неизвестно, автоматом она добавляется если зайти рутом и сделать ssh 10.144.200.148
если не проделать эту манипуляцию будет появляться ошибка авторизации для юзеров при передаче CDR файла

###############################################################################
# Конфигурационный файл kspus.cfg                                             #
###############################################################################
su - k86157020
# Для того чтобы ключи были доступны демону kspusd копируем их в каталог ~/.kspus/keys
cp ~/.ssh/id_rsa ~/.kspus/keys
cp ~/.ssh/id_rsa.pub ~/.kspus/keys
chmod 660 ~/.kspus/keys/*
# меняем пользователя на нашего:
sed -i -e 's/username/k86157020/g' ~/.kspus/kspus.cfg
# меняем и проверяем остальные параметры:
vim ~/.kspus/kspus.cfg

###############################################################################
# Образцы оригинальных имен файлов различных АТС:                             #
###############################################################################
AXE10   - TTFI9849
АЛС4096 - 20170807.TXT
SI2000  - i287020170807141560.ama
NEAX61  - cdr0148
КВАНТ-Е - 08-06-03
ISTOK   - 86158_1604300002

###############################################################################
# пример команды на получение .err файла из spcdb2 ~/cdrin/err    #
###############################################################################
ssh -n k86196001@10.144.200.148 'cd ~/cdrin/err; tar -cf - *.err' | tar -xf - 2> /dev/null
имя err файла будет иметь вид: 
SI2086160031_170814110512_00_i361620170814111589.gz.err
err файл является текстовым файлом в кодировке CP1251
в нём ошибка обработки фала на сервере.

2017.10.17
В настоящий момент существует проблема - как определить что файл передался со станционного АРМ полностью?
Для передачи даже самого маленького файла нужно время, пусть это миллисекунды но это время.
В Linux при записи файла в каталог (проверено на FTP и на локальной filesystem) подсистема наблюдения за файлами/каталогами
inotify выдаёт уведомления об изменении содержимого каталогов немедленно.
Иными словами как определить что файл передан полностью?
Если файл начал записываться но ещё до конца не записался - уведомление от inotify о его создании приходит сразу,
как только первый байт файла попал на диск. Поэтому я сделал вот такой финт ушами:
FTP сервер ProFTPd имеет параметр - HiddenStores, он даёт следующее:
пока файл до конца не передан в каталоге in его не видно, он имеет скрытое имя .in.чего-то-там, а когда передача 
завершена ProFTPd делает mv (перемещение) из этого скрытого файл в файл с именем без .in.
Таким образом нам нужно отловить событие inotify - IN_MOVED_TO, а ProFTPd гарантирует целостность принятого файла.
В /etc/proftpd.conf после ControlsLog ставим: HiddenStores        on
остальное не меняем

2017.10.25
Кластер из двух нод под управлением pacemaker.
pcs status
pcs cluster status
pcs cluster standby --all
pcs cluster unstandby --all
pcs cluster standby adgesp-1   # перейти на 2-ю ноду
pcs cluster unstandby adgesp-1 # перейти на 1-ю ноду
pcs cluster standby adgesp-2   # перейти на 1-ю ноду
pcs resource restart kspusd    # рестартовать демона kspusd
pcs resource show              # посмотреть ресурсы
pcs resource list              # посмотреть ресурсы, выполняется долго, выводит огромный список

очистить Failed Actions:
crm_mon -1
crm_resource -P

2017.11.09
Сделал работу kspusd в режиме демона и немного изменил инит скрипт.
На серверах adgesp-1 и adgesp-2 заменил программу и скрипт.
Теперь демон создаёт файл /var/run/kspusd.lock в который сам записывает свой PID.
Если kspusd убить командой kill `/var/run/kspusd.lock ` он сам удаляет /var/run/kspusd.lock
Когда даёшь "kill pid" это равносильно "kill -SIGTERM pid".

2017.11.21
Сделал останов демона с корректным завершением всех потоков.
Для этого надо сделать так из под root:
kill -SIGHUP `cat /var/run/kspusd.lock`
всепотоки отработают текущую задачу и завершаться, в syslog будет запись вида:
Nov 21 13:32:13 collector kspusd: WorkProcess END
Nov 21 13:32:13 collector kspusd: Exit with code 0.
Файл /var/run/kspusd.lock также удалиться.
Если процесс kspusd был убит системой то /var/run/kspusd.lock не удалиться и последующий запуск
демона init скриптом будет неуспешным. Необходимо удалить /var/run/kspusd.lock вручную.
Поскольку это нештатная ситуация то она требует выявления причин, не рекомендуется удалять
/var/run/kspusd.lock из init скрипта.

2017.11.29
Логирование.
При старте kspusd до тех пор пока не запустились потоки отслеживающие появление CDR файлов
сообщения от демона пишуться только в syslog.
Смотреть их можно так: tail -50f /var/log/messages | grep kspusd
После старта потоков каждый поток пишет свой лог в файл ~/kspus.log
Траблы:
Сегодня kspusd не мог стартовать - был split-brain.
Смотреть split-brain так: tail -500f /var/log/messages | grep split-brain

2017.12.05
В файле kspus.cfg добавился параметр
--
[MISC]
error_handler_timeout=15
--
Этот параметр устанавливает периодичность опроса коллектора 2-го уровня на наличие err файлов
и отправку файлов зависших в каталогах in и out по причине некорректного завершения процесса kspusd.
error_handler_timeout измеряется в минутах.
Получение err файлов и опрос каталогов in и out выполняются потоком параллельным 
основному потоку наблюдения через inotify. Защита каталогов от одновременного чтения файлов 
сделана через мьютекс ksparam->mtx_dir_lock

2017.12.06
описание секции [CDR] в kspus.cfg, соответствие полей на старой системе ЕСП
------------------------------------------
новый kspus.cfg   | старый kspus.ini     |
------------------------------------------
[CDR]             | нет соответствия     |
namelen=8         | нет соответствия     |
regcode=86196     | REGION_CODE="86196"  |
cdrprefix=AXE4    | ATC_TYPE="AXE4"      |
atsnum=001        | ATC_NUMBER="001"     |
zz=00             | ZZ_CODE_IN_NAME="0"  |
------------------------------------------
Другие поля файла kspus.cfg не используются в программе kspusd 
на момент создания этой записи.

2017.12.07
Увеличиваем максимальное количество файловых дескрипторов в системе:
# vi /etc/sysctl.conf
fs.file-max = 1000000
reboot
Смотрим: 
# sysctl fs.file-max
# cat /proc/sys/fs/file-max
# sysctl fs.file-nr
сколько всего открытых дескрипторов в системе: 
# lsof | wc -l
--
Узнаем сколько дескрипторов использует наш процесс:
# lsof -p `cat /var/run/kspusd.lock` | wc -l
# lsof -a -p `cat /var/run/kspusd.lock` | wc -l
сколько файловых дескрипторов: 
# ls -1 /proc/`cat /var/run/kspusd.lock`/fd | wc -l
Посмотреть лимиты процесса:
# cat /proc/`cat /var/run/kspusd.lock`/limits
# prlimit --pid `cat /var/run/kspusd.lock`
Установить лимит на количество открытых дескрипторов для запущенного процесса:
# prlimit --nofile=500000:800000 --pid `cat /var/run/kspusd.lock`
я буду устанавливать лимиты в коде программы, функция main.c setProcLimits()
--
Ограничения Inotify. Взято отсюда:
https://unix.stackexchange.com/questions/13751/kernel-inotify-watch-limit-reached
http://bosenko.info/2016/02/21/inotify-i-tail-slishkom-mnogo-otkrytyh-fajlov/
To check the max number of inotify watches: 
# cat /proc/sys/fs/inotify/max_user_watches
# sysctl fs.inotify.max_user_watches
# vi /etc/sysctl.conf
fs.inotify.max_user_watches = 524288
fs.inotify.max_user_instances = 1024
# reboot
To see what's using up inotify watches:
# find /proc/*/fd -lname anon_inode:inotify | cut -d/ -f3 | xargs -I '{}' -- ps --no-headers -o '%p %U %c' -p '{}' | uniq -c | sort -nr
--
В файле log.c сделал запись лога-дебага в файл /var/log/kspusd.log
также сделал запись всех логов в syslog, в /var/log/kspusd.log и в ~/kspus.log

2017.12.11
Переделал способ запуска. Теперь принимает параметры:
-d /имя/каталога/с_полным_путём
-v печатает дату компиляции main.c
Также изменился init скрипт. Теперь путь к базовому каталогу нужно задавать в файле
/etc/sysconfig/kspusd в виде переменной OPTIONS, например так:
vi /etc/sysconfig/kspusd
OPTIONS="-d /r0/kspus"
Файл /etc/sysconfig/kspusd нужно создать вручную

2017.12.14
настройка формирования core-dump файлов для Centos7:
# vi /etc/sysctl.conf
kernel.core_pattern = core.%e.%t.%p
kernel.core_pipe_limit = 0
kernel.core_uses_pid = 0
fs.suid_dumpable = 1
# vi /etc/security/limits.conf
*    soft     core      unlimited
--
PID файл стал называться /var/run/kspusd.pid

2017.12.19
Потребление памяти.
первые 10 самых жрущих память процессов:
ps -eo user,pcpu,pmem,pid,cmd | sort -r -k3 | head -10
или top и нажать Shift+M то он будет сортировать процессы по использованию памяти

2017.12.21
Параметр error_handler_timeout отсчитывает время от последнего срабатывания потока
ожидающего появления файла в IN, либо от времени старта потока (если в IN ещё ничего не было).

2018.01.29
В связи с частыми падениями программы (раз в 4-5 дней) принято решение перейти на более новую версию
библиотеки libssh. Для этого делаею следующее:
1) Клонируем репозитарий: git clone git://git.libssh.org/projects/libssh.git libssh
2) здесь нужно как-то переключиться на нужную ветку: git checkout tags/libssh-0.7.5
3) чтобы скомпилялось нужно: sudo yum --enablerepo=epel install libcmocka libcmocka-devel
3) компилируем и устанавливаем libssh из этой ветки, я ставил в ~/lib , инстукция в INSTALL
4) чтобы настроить Makefile использовал: https://gcc.gnu.org/onlinedocs/cpp/Search-Path.html
   и https://gcc.gnu.org/onlinedocs/cpp/Invocation.html#Invocation
   В Makefile меняем CFLAGS и LIBS. Для CFLAGS в конец добавляю -I/home/bliz/lib/include
   для LIBS перед -lssh -lssh_threads добавляю -L/home/bliz/lib/lib , чтобы получилось так
   -L/home/bliz/lib/lib -lssh -lssh_threads
5) Для чистоты эксперимента удаляю системный libssh: sudo yum remove libssh-devel libssh libssh-debuginfo
6) компилирую kspusd
7) Чтобы нормально запустилось нужно сделать так: export LD_LIBRARY_PATH=/home/bliz/lib/lib
   для запуска из под root так: sudo LD_LIBRARY_PATH=/home/bliz/lib/lib ./kspusd -v

Для отладки программы нужны отладочные символы, чтобы их сделать и проверить делаем core.dump файл так:
kill -3 pid
наш дамп будет в корне.

2018.03.14
лог работы с файлами ошибок на старой системе СПУС:
-----------------
-bash-3.2$ cat kspus.log | grep ews186164001_180313225144_00_AMA.130318.gz
2018-03-13 21:30:04 86164001-1: compressing ews186164001_180313225144_00_AMA.130318 to ews186164001_180313225144_00_AMA.130318.gz .....OK
2018-03-13 21:30:04 86164001-1: md5summ of file: "/r0/in/ews186164001_180313225144_00_AMA.130318.gz": 4bbfab49efc06adbf2d449a1629457a5
2018-03-13 21:30:04 86164001-1: archiving files ews186164001_180313225144_00_AMA.130318.gz and ews186164001_180313225144_00_AMA.130318.gz.md5 in ews186164001_180313225144_00_AMA.130318.tar .....OK
2018-03-13 21:30:04 86164001-1: source files (ews186164001_180313225144_00_AMA.130318 ews186164001_180313225144_00_AMA.130318.gz ews186164001_180313225144_00_AMA.130318.gz.md5) removed
2018-03-13 22:30:07 86164001-1: downloading err-file ews186164001_180313225144_00_AMA.130318.gz.err from remote server ....OK
2018-03-13 22:30:07 86164001-1: decompressing file "ews186164001_180313225144_00_AMA.130318.gz" from "/r0/archive/ews186164001_180313225144_00_AMA.130318.tar" ....OK
2018-03-13 22:30:07 86164001-1: ungziping file /r0/error/ews186164001_180313225144_00_AMA.130318.gz ....OK
2018-03-13 22:30:07 86164001-1: deleting remote file "~/cdrin/err/ews186164001_180313225144_00_AMA.130318.gz.err" on server "10.144.200.148" ....OK
2018-03-13 22:30:07 86164001-1: CDR-file was succesfull restored to "AMA.13031800" form the file "ews186164001_180313225144_00_AMA.130318.gz", which is not processed on the regional server
 -----------------
