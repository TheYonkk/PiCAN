#!/bin/bash

# make a folder in the etc directory to store the configuration file
sudo mkdir /etc/telem_client

# disable the client if it exists
sudo systemctl stop telem_client

# move the config file to the etc directory and move the binary file to the bin directory
sudo cp src/build/client.conf /etc/telem_client/
sudo cp src/build/telem_client /bin/

# copy service description to system-wide services directory
sudo cp telem_client.service /etc/systemd/system/

# enable the services to start up on boot
sudo systemctl enable telem_client
sudo systemctl start telem_client


