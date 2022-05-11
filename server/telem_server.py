#!/usr/bin/env python3

import socket
import argparse

from yaml import safe_load
import json

from influxdb_client import InfluxDBClient, Point, WriteOptions

import cantools

# import thread module
from _thread import *

# the length of the first message for every json recieve. This sets up the rest of the transaction.
INITIAL_MESSAGE_SEND_LENGTH_BYTES = 64


def success_cb(details, data):
    url, token, org = details
    print(url, token, org)
    data = data.decode('utf-8').split('\n')
    print('Total Rows Inserted:', len(data))


def error_cb(details, data, exception):
    print(exception)


def retry_cb(details, data, exception):
    print('Retrying because of an exception:', exception)


def main(config_file):
    with open(config_file, "r") as fp:
        config = safe_load(fp)

    # load the DBCs from the config
    vehicle_info_dict = {}
    for vehicle in config["vehicles"].keys():
        vehicle_info_dict[vehicle] = {}  # another empty dict to hold all config params

        # copy database bucket over
        vehicle_info_dict[vehicle]["db_bucket"] = config["vehicles"][vehicle]["db_bucket"]

        # load CAN dbcs
        for bus, dbc_path in config["vehicles"][vehicle]["busses"].items():
            vehicle_info_dict[vehicle][bus] = cantools.database.load_file(dbc_path)

    # verify that IPv6 and IPv4 dual-stack is possible
    if socket.has_dualstack_ipv6():
        family = socket.AF_INET6
        dualstack_ipv6 = True

    else:
        # if not dual stack, revert to IPv4, since *most* platforms support IPv4
        family = socket.AF_INET
        dualstack_ipv6 = False

    address = (config["host"], config["port"])

    with socket.create_server(address, family=family, dualstack_ipv6=dualstack_ipv6) as sock:

        while True:
            conn, addr = sock.accept()  # accepts conn like regular socket

            # start a new thread to handle this connection
            params = (conn, addr, vehicle_info_dict, config["db_server"])
            start_new_thread(accept_connection, params)


def accept_connection(conn, addr, vehicles_dict, db_info_dict):
    """
    @param conn - the connection object
    @param addr - the address of the connecting party
    This function is called everytime a new thread is created. The function
    "becomes" the thread... think of it like another main function.
    """

    with conn:  # automatically closes socket when done
        print("Accepted")

        data = conn.recv(INITIAL_MESSAGE_SEND_LENGTH_BYTES)

        json_length = int(data.decode('ascii').rstrip('\x00'))

        print(json_length)

        # keep reading the socket until all of the bytes are recieved.
        bytes_recieved = 0
        json_string = ""
        while bytes_recieved < json_length:
            data = conn.recv(4096)

            # decode the bytes into a python string and append it to the json
            data_string = data.decode('ascii')
            json_string += data_string

            # add to the total bytes recieved
            bytes_recieved += len(data)

        # parse the json string
        json_decoded = json.loads(json_string)

        # vehicle key name from client
        vehicle_key = json_decoded["key"]

        # filter information by this key
        try:
            vehicle_info_dict = vehicles_dict[vehicle_key]
        except KeyError:
            print(f"ERROR: No configuration loaded for vehicle with key {vehicle_key}! Skipping all data.")
            return

        # create a write api instance from the client. write API is in batching mode by default. The internal buffer is
        # flushed after the `with` statement is flushed
        client = InfluxDBClient(url=db_info_dict["url"], token=db_info_dict["token"], org=db_info_dict["org"])
        # write_api = client.write_api(write_options=SYNCHRONOUS)
        write_api = client.write_api(success_callback=success_cb, error_callback=error_cb,
                                     retry_callback=retry_cb)  # , write_options=SYNCHRONOUS)

        for bus, messages in json_decoded["messages"].items():

            try:
                db = vehicle_info_dict[bus]
            except KeyError:
                print(f"ERROR: No DBC loaded for {bus}! Skipping all {bus} messages...")
                continue

            for msg in messages:

                # get the message definition from the dbc
                msg_def = db.get_message_by_frame_id(msg["id"])

                # decode the message into its signals
                decoded_msg = msg_def.decode(msg["data"])

                # don't continue if decoded nothing
                if len(decoded_msg) == 0:
                    continue

                # create the datapoint for db ingestion
                # p = Point(msg_def.name)
                p = Point(msg_def.name).tag("bus", bus)

                # add each signal and the value. Everything is casted to a float or string to avoid write failures.
                # If you try to write to the DB and there's no output and no error, there's a good chance that there's a
                # type confict!!! This fix avoids that, since datatypes maybe be cast to an int depending on how they
                # are decoded from the DBC.
                at_least_one_val_added = False
                for sig_name, sig_val in decoded_msg.items():
                    try:
                        p = p.field(sig_name, float(sig_val))
                        at_least_one_val_added = True
                    except ValueError:
                        try:
                            p = p.field(sig_name, str(sig_val))
                            at_least_one_val_added = True
                        except ValueError:  # can't even cast to a string -> something's messed up here
                            continue

                # don't continue with the write if no signals were valid. If invalid, they will destroy the entire write,
                # so let's just forget about them.
                if not at_least_one_val_added:
                    continue

                # add timestamp
                p = p.time(msg["t"])

                # adds message to the write queue/buffer
                write_api.write(bucket=vehicle_info_dict["db_bucket"], record=p)

        # flushes the write buffer
        write_api.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Create a server that listens for incoming CAN data.')
    parser.add_argument('-c', '--config', type=str, default="telem_server.conf",
                        help='The path to the YAML configuration file.')
    args = parser.parse_args()

    main(args.config)
