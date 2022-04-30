# Telemetry server

A Python application to accept raw JSON AN data from the telemetry client, decode it, and insert decoded data into an
influxdb database.

## Installation

Installation is easier than the client, as Python is generally considered more user-friendly than C++. That being said,
the installation files never leave this directory (that is, with the exception of the service file). The reason being
that the Python virtual environment (that you *really* should create) will be kept in this directory. Before continuing,
be sure that at least Python 3.8 is installed and can be accessed via the command `python3` in the console.

### Create the virtual environment

To keep python packages nice and tidy, we install them in a new virtual environment. We will first make sure that
virtualenv is installed, then, from within the `PiCAN/server` directory, we will create a virtual environment called
`venv`.

1. `pip3 install virtualenv`
2. `python3 -m virtualenv venv`

### Activate the virtual environment

We need to activate the virtual environment to get access to the packages installed from within it.

1. `source venv/bin/activate` should do the trick.

### Install required packages

Now we need to make sure that the extra packages are install from pip. Fear not, they are listed in `requirements.txt`.

1. `pip3 install -r requirements.txt`

### Verify settings in the `telem_server.conf` file

Verify that the settings in telem_server.conf are correct. Explanations are found within the file. This config file
has parameters regarding DBC paths as well as server connection parameters. The file is formatted as a YAML file.

### Install the service

You can use the `install.sh` command to install the service with systemctl. Once installed, you may find the following
commands useful:

* Start the service: `sudo systemctl start telem_server`
* Stop the service: `sudo systemctl stop telem_server`
* Restart the service: `sudo systemctl restart telem_server`
* Disable the service from auto-start and reload: `sudo systemctl disable telem_server`
* Enable the service for auto-start and reload: `sudo systemctl enable telem_server`

