Template: pump/old-conf
Description: You may need to reconfigure pump
 pump will no longer run automatically until you add an entry like:
   auto eth0
   iface eth0 inet dhcp
 to /etc/network/interfaces.  Run "man interfaces" for more info.
Description-ru: Вам может понадобится перенастроить pump
 pump  больше  не запускается автоматически, пока вы не добавите записи
 типа:
   auto eth0
   iface eth0 inet dhcp
 в файл /etc/network/interfaces. Подробности в "man interfaces".

