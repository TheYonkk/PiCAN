#!/bin/bash

# copy service description to system-wide services directory
sudo cp telem_server.service /etc/systemd/system/

# enable the services to start up on boot
sudo systemctl enable telem_server
sudo systemctl start telem_server


