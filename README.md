 # PiLogger <!-- omit in toc -->

- [Clone the repository](#clone-the-repository)
- [Hardware setup](#hardware-setup)
  - [Installing Ubuntu](#installing-ubuntu)
    - [Updating network information after installation](#updating-network-information-after-installation)
    - [Updating username and hostname after installation](#updating-username-and-hostname-after-installation)
  - [SSH keygen and key swap](#ssh-keygen-and-key-swap)
  - [i2c Setup](#i2c-setup)
    - [Sixfab Power API](#sixfab-power-api)
  - [Install Telegraf](#install-telegraf)
  - [Setup the Shutdown Button](#setup-the-shutdown-button)
  - [CAN Hat setup](#can-hat-setup)
  - [SocketCAN](#socketcan)
    - [Installing](#installing)
    - [Automatically bring up CAN interfaces](#automatically-bring-up-can-interfaces)
    - [Useful commands](#useful-commands)
      - [Bring up a virtual CAN interface](#bring-up-a-virtual-can-interface)
      - [Bring up a hardware CAN interface](#bring-up-a-hardware-can-interface)
      - [Sending and viewing CAN messages](#sending-and-viewing-can-messages)
  - [Install cron jobs](#install-cron-jobs)
  - [Set up modem manager for cellular connection](#set-up-modem-manager-for-cellular-connection)
- [Logger](#logger)
- [InfluxDB](#influxdb)
  - [Installation](#installation)
    - [Creating an InfluxDB service](#creating-an-influxdb-service)

# Clone the repository

**Very important!** This repository includes git submodules so that dependencies are less of a mess. The only downside is that the submodules do not clone automatically! When cloning the repository, it is imperitive that you use this command, otherwise you will have to initialize the submodules and clone them individually.

```
git clone --recurse-submodules https://github.com/TheYonkk/PiCAN.git
```

# Hardware setup

## Installing Ubuntu

There are many ways to succeed at installing Ubuntu Server. This application was built on Ubuntu Server 21.04, so check yourself with earlier versions. I don't need to explain how to do this. You can read [this tutorial](https://ubuntu.com/tutorials/how-to-install-ubuntu-on-your-raspberry-pi#1-overview) instead.

**NOTE:** After writing the ubuntu files to the sd card, I highly reccomend for you to skip to the [CAN Hat setup](#can-hat-setup) section of this readme. This is because the CAN hat requires you to modify files on the SD while the raspberry pi is offline. This process is very similar to the network setup in the ubuntu tutorial.

Once you're done install ubuntu and you've logged in for the first time, be sure to update the default packages: 
```
sudo apt-get update
sudo apt-get upgrade
sudo reboot
```

### Updating network information after installation

I updated the WiFi information after reading [his article](https://linuxconfig.org/ubuntu-20-04-connect-to-wifi-from-command-line). 

### Updating username and hostname after installation

Again, so easy. Google really is amazing. [This post](https://askubuntu.com/a/317008) showed me how to change the username from ubuntu to whatever I wanted, and [this article](https://www.cyberciti.biz/faq/ubuntu-change-hostname-command/) told me how to change the hostname. Please note, you must create a temporary user when changing the username (which is explained in the post I linked).

## SSH keygen and key swap

To log into the pi without the use of a password, you can exchange ssh keys. Do to so, make sure that you can an ssh key created by running `ssh-keygen`. You can hit enter to use all of the default options. This will create an ssh key. You should do this on both the server and your local PC (I think - it doesn't hurt).

Then, from your local Mac or Linux machine (not connected to the pi), you need to copy your local key to the server. To do this, you can type the command `ssh-copy-id <user>@<server>`. You should now be able to ssh without using a password.

On Windows, you must do this **in Windows PowerShell** instead to copy the ID: `type $env:USERPROFILE\.ssh\id_rsa.pub | ssh {IP-ADDRESS-OR-FQDN} "cat >> .ssh/authorized_keys"`. Of course, replace `{IP-ADDRESS-OR-FQDN}` as appropriate.

## i2c Setup

To interface with the Sixfab Power HAT, i2c is used. The tools used to communicate with it are diabled by default in Ubuntu. To enable it, run the following commands:
1. `sudo apt update`
2. `sudo apt upgrade -y`
3. `sudo apt install -y i2c-tools python3-pip`


By default, you must access i2c using sudo, otherwise your prompts will fail. I would reccomend adding yourself to the i2c user group as explained in [this wonderful post](https://askubuntu.com/a/1273900).

1. `sudo groupadd i2c` (group may exist already)
2. `sudo chown :i2c /dev/i2c-1` (or `i2c-0`)
3. `sudo chmod g+rw /dev/i2c-1`
4. `sudo usermod -aG i2c *INSERT YOUR USERNAME*`

Log out, then log back in. You should be able to run `i2cdetect -y 1` without sudo.

### Sixfab Power API

Install the api that interfaces (*sometimes, only if you're lucky*) with the battery UPS. `pip3 install power-api` Here's the [repo](https://github.com/sixfab/sixfab-power-python-api). The API is essentially a Python wrapper for their i2c communication protocol.

In order to get this module installed for all users (so that telegraf can use it), I would suggest installing power_api via the root user. `sudo su` and then `pip3 install power-api`. To log out of the root user, you can use the command `exit`.

## Install Telegraf

Telegraf is a program from the makers of InfluxDB that writes usage metrics to an influx instance. This is very useful to have on the raspberry pi. The installation instructs are on there website, [here](https://docs.influxdata.com/telegraf/v1.20/introduction/installation/), but you should just be able to run `sudo apt-get update && sudo apt-get install telegraf`.


After doing that, make sure that Telegraf runs during startup by enabling the service with `sudo systemctl enable telegraf`

What's nice about telegraf is that everything about how it operates is defined within a single configuration file located in the `/etc/telegraf` folder. You can edit the `telegraf.conf` as you please, but for a Raspberry Pi, I'd suggest copying the one in the `telegraf` folder of this repository. Review and edit the information in the influxdb configuration section, then copy it to `/etc/telegraf/telegraf.conf`. Make sure you have a valid token from influx that telegraf can use to write to the bucket specified in the config file. Then, after everything is in the config file, run `systemctl reload telegraf`.

## Setup the Shutdown Button

Be sure to enable the Pi to be able to be shutdown and start up by the press of a button. The instructions are located in the [shutdown folder](shutdown/). 


## CAN Hat setup

To setup the [PiCAN2 Duo](https://copperhilltech.com/pican2-duo-can-bus-board-for-raspberry-pi/) CAN hat on Ubuntu Server, you can follow the instructions on [CopperHill's website](https://copperhilltech.com/blog/pican2-pican3-and-picanm-driver-installation-for-raspberry-pi/) with the one exception that instead of adding the overlays in `/boot/config.txt` you should pop the SD card out, load it into another computer, and edit the `config.txt` file instead.

## SocketCAN

### Installing

```bash
sudo apt-get install can-utils
```

### Automatically bring up CAN interfaces

SocketCAN interfaces do not register upon system startup by default. There is a [cron job](#install-cron-jobs) that will bring these interfaces up upon boot.

### Useful commands


#### Bring up a virtual CAN interface

```bash
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```
Verify that the interface is up:

```bash
ifconfig
```

#### Bring up a hardware CAN interface

```bash
sudo /sbin/ip link set can0 up type can bitrate 1000000
sudo /sbin/ip link set can1 up type can bitrate 1000000
```


#### Sending and viewing CAN messages

Sending and viewing is easy. To send a message, use the format `cansend <interface> <ID>#<DATA>`. Note: The ID and data fields are in hexedecimal.

```bash
cansend vcan0 123#DEADBEEF
```

To view messages on the bus, use `candump <interface>`.

```bash
candump vcan0
```

If looking for a particular message on a busy bus, you can combine `candump` with `grep` to filter out any unwanted messages.

```bash
candump vcan0 | grep 123
```

## Install cron jobs
Cron is a service that allows you to run commands at a predetermined time interval. To edit the super user cron jobs, run `sudo crontab -e`. If it's your first time running the command, you'll be casked to choose your preferred text editor. After entering the editing window, paste in the following commands below:

```
# bring up CAN interfaces upon startup
@reboot /sbin/ip link set can0 up type can bitrate 1000000
@reboot /sbin/ip link set can1 up type can bitrate 1000000

# give all permissions for i2c communications
@reboot chmod 777 /dev/i2c-1
```

## Set up modem manager for cellular connection
Ubuntu gives a good tutorial [here](https://ubuntu.com/core/docs/networkmanager/configure-cellular-connections), but it doesn't all work for the PiLogger since our modem is reached through a USB interface.

First, install modem-manager:
```
sudo snap install modem-manager
sudo apt install network-manager
```

You can verify that the modem is plugged into your device if `cdc-wdm0` shows up when you run `sudo ls /dev/`. Now, add all discoverable modems with this command:

```
sudo nmcli c add type gsm ifname '*' con-name <name> apn <operator_apn>
```

For Mint mobile, the operator APN is `Wholesale`. You can name the con-name whatever you want. I named mine `i_love_internet`. Just be mindful that you need to use it in the next commands:

```
sudo nmcli c modify <name> connection.autoconnect yes
sudo nmcli r wwan on
```

You should be all set. You should be able to see a new interface after running `ifconfig`. Additionally, to make sure that you're actually getting service, you can run `ping -I <interface name in ifconfig menu> google.com`.

# Telemetry Client

To build and install the telemetry client application on a raspberry pi, cd into the `client` directory and read the [README.md](client/README.md) file.

# Telemetry Server

To build and install the telemetry server application on a raspberry pi, cd into the `server` directory and read the [README.md](server/README.md) file.

# InfluxDB

An Influx installation is only required for the server. If you are simply setting up a vehicle Pi, you can skip Influx installation.

[InfluxDB v2.0](https://influxdata.com) is the timeseries database used to store all data. It is vital that this runs, otherwise nothing will work

## Installation

Installation instructions are pretty straightforward [on their website](https://docs.influxdata.com/influxdb/v2.0/get-started/). The only addendum here is that we'd like for the database deamon to start up on boot, so we'll need to create a service for it after influx is installed

### Creating an InfluxDB service

These instructions are adapted from [this site](http://blog.lemminger.eu/run-influxdb-2-0-as-a-service/).

1. Create an influxdb user: `sudo useradd -rs /bin/false influxdb`
1. Create a user folder: `sudo mkdir /home/influxdb; chown influxdb /home/influxdb`
1. Install the services in the services directory of this repository by running the script `sudo ./install-services.sh`

The influxdb2 service will now start on boot, and all influx files will be stored in `/home/influxdb`. If you'd like to start it immediately after installing, run `sudo systemctl start influxdb2.service`


