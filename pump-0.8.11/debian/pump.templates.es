Template: pump/old-conf
Description: You may need to reconfigure pump
 pump will no longer run automatically until you add an
 entry like:
   auto eth0
   iface eth0 inet dhcp
 to /etc/network/interfaces.  Run "man interfaces" for more info.
Description-es: Es posible que necesite volver a configurar pump
 pump no funcionará automáticamente hasta que añada en 
 /etc/network/interfaces lo siguiente:
   auto eth0
   iface eth0 inet dhcp
 Puede encontrar más información mediante "man interfaces".

