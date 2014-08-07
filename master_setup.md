master_setup
-
This describes the OS config of the master to operate as a standalone access point. Devices can then join this WiFi network and open a web page on the master to load the client javascript page and control the network of lights.

This config uses a rtl8192cu based USB WiFi adaptor and special build of hostapd from adafruit.

slave ---\
slave ----- nRF24 --- spi --- master -- usb wifi -- client
slave ---/

installing required packages::
    sudo aptitude install wireless-tools hostapd bridge-utils isc-dhcp-server dnsmasq
    wget http://www.adafruit.com/downloads/adafruit_hostapd.zip 
    unzip adafruit_hostapd.zip 
    sudo mv /usr/sbin/hostapd /usr/sbin/hostapd.ORIG
    sudo mv hostapd /usr/sbin
    sudo chmod 755 /usr/sbin/hostapd

/etc/network/interfaces::
    auto lo eth0 wlan0
    iface lo inet loopback
    iface eth0 inet dhcp
    iface wlan0 inet static
        address 192.168.10.1
        netmask 255.255.255.0

/etc/default/hostapd.conf::
    DAEMON_CONF="/etc/hostapd/hostapd.conf"

/etc/hostapd/hostapd.conf::
    interface=wlan0
    driver=rtl871xdrv
    ssid=dark_hollow
    hw_mode=g
    channel=12
    macaddr_acl=0
    auth_algs=1
    ignore_broadcast_ssid=0
    wpa=2
    wpa_passphrase=plain_text_passphrase
    wpa_key_mgmt=WPA-PSK
    wpa_pairwise=TKIP
    rsn_pairwise=CCMP

/etc/dhcp/dhcpd.conf::
    ddns-update-style none;
    default-lease-time 600;
    max-lease-time 7200;
    authoritative;
    log-facility local7;

    subnet 192.168.10.0 netmask 255.255.255.0 {
    range 192.168.10.2 192.168.10.127;
    option broadcast-address 192.168.10.255;
    option routers 192.168.10.1;
    default-lease-time 600;
    max-lease-time 7200;
    option domain-name "dark-hollow";
    option domain-name-servers 192.168.10.1;
    }

/etc/default/isc-dhcp-server::
    INTERFACES="wlan0"

/etc/hosts::
    192.168.10.1    master master.dark_hollow

/etc/dnsmasq.conf::
    interface=wlan0
    no-dhcp-interface=wlan0
    #The following will enter a wildcard that will resolve all addresses to 192.168.10.1
    address=/#/192.168.10.1

/etc/default/dnsmasq::
    IGNORE_RESOLVCONF=yes
    no-resolve
    domain-needed

now activate this config::
    sudo service isc-dhcpd-server restart
    sudo service dnsmasq restart
    sudo /usr/sbin/hostapd /etc/hostapd/hostapd.conf

With this config, the server.sh script can be run to start the master_server process to talk to the slaves and the js/server.js process to serve up the client web page to control the ensemble of slaves.


