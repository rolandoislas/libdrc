Network configuration for Wii U GamePad <-> PC
==============================================

The Wii U GamePad communicates with other devices via the use of a slightly
obfuscated version of Wi-Fi 802.11n and WPA2 AES-CCMP. Luckily, the required
modifications for support of this obfuscated Wi-Fi protocol can be applied in
userland without any kernel modification.

Using libdrc to communicate with a Wii U GamePad however also requires the host
computer to get some information from its Wi-Fi interface which is not exported
by default on Linux. For this purpose, a kernel module needs to be built and
loaded on the machine to provide the required information.

Required hardware
-----------------

Connecting a Wii U GamePad to a computer requires compatible Wi-Fi hardware.
Any Wi-Fi NIC that can create access points on 5GHz 802.11 channels could
potentially work. In practice, the following drivers/NICs were tested:

* rt2800usb: works with Linux >= 3.11
* ath9k/carl9170: works with some caveats (communication might desync more
  often because of a TSF drifting issue)

Pairing the Wii U GamePad with a computer
-----------------------------------------

TODO: instructions for hostapd and WPS (most of it already on drc.delroth.net,
needs to be re-tested and ReSTed).

Setting up the Wi-Fi access point
---------------------------------

First of all, build the patched hostapd version available from
``memahaxx/drc-hostap``::

    git clone https://bitbucket.org/memahaxx/drc-hostap
    cd drc-hostap
    cp conf/hostapd.config hostapd/.config
    cd hostapd
    make -j4

Adapt the example configuration file from ``conf/wiiu_ap_normal.conf``:

* Change the network interface name
* Change the network SSID
* Change the PSK used for the network WPA2 security

Stop anything that might conflict with hostapd (for example, NetworkManager is
a known offender unless configured specifically to avoid this). Then start the
access point::

    sudo ./hostapd -dd ../conf/wiiu_ap_normal.conf

If everything worked well, hostapd should be waiting for devices to connect
instead of exiting immediately.

Make sure that no network interface is using the 192.168.1.0/24 network (it is
used for communication with the GamePad), then configure the interface::

    sudo ip a a 192.168.1.10/24 dev $IFACE
    sudo ip l set mtu 1800 dev $IFACE

Starting a DHCP server
----------------------

The GamePad uses DHCP to get an IP address from the Wii U or the PC it is
connected to. This IP address should always be 192.168.1.11. Any simple DHCP
server should work, but we recommend using netboot_, a very simple,
self-contained DHCP server.

.. _netboot: https://github.com/ITikhonov/netboot

Using netboot, the following command line should work (with the propre MAC
address of the GamePad)::

    ./netboot 192.168.1.255 192.168.1.10 192.168.1.11 aa-bb-cc-dd-ee-ff

From there, when powering on the GamePad, it should get an IP from netboot and
start sending packets to the computer. Using libdrc demos should work.
