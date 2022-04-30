/**
 * \file consumer.cpp
 *
 * \author Dave Yonkers
 */

#include <arpa/inet.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

// ugh fix this eventually. I can't get the CMake include to work
#include "../src/external/json/single_include/nlohmann/json.hpp"
using json = nlohmann::json;

#include <chrono>

#include "Message.hpp"
#include "ThreadableQueue.hpp"
#include "consumer.hpp"

using namespace std;

extern const bool LOGGER_DEBUG;
extern bool stop_logging;
extern sem_t stdout_sem;

#define BUFFER_LENGTH 250
#define NETDB_MAX_HOST_NAME_LENGTH 128
#define MAX_PORT_NUMBER_LENGTH 10
#define INITIAL_MESSAGE_SEND_LENGTH_BYTES 64
#define SOCKET_SEND_BUFFER_SIZE 64

/**
 * Helper functions go here!
 */
namespace csmr {

/**
 * Gets the current system unix time in milliseconds
 * \returns The time in milliseconds
 */
long int current_time() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

/**
 * Sends json data to the server. This code is nearly line-for-line of IBM's
 documentation:
 * \param host The hostname of the server
 * \param port The port of the server

 * \param data A reference to the JSON object to send
 * \returns nothing
 */
void send_json_to_server(string host, string port, json& data) {
  /***********************************************************************/
  /* Variable and structure definitions.                                 */
  /***********************************************************************/
  int sd = -1, rc, bytesReceived = 0;
  char buffer[BUFFER_LENGTH];
  char server[NETDB_MAX_HOST_NAME_LENGTH];
  char servport[MAX_PORT_NUMBER_LENGTH];
  struct in6_addr serveraddr;
  struct addrinfo hints, *res = NULL;

  memset(&hints, 0x00, sizeof(hints));
  hints.ai_flags = AI_NUMERICSERV;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  // copy strings into defined data structures
  strcpy(server, host.c_str());
  strcpy(servport, port.c_str());
  // cout << host << " " << port << endl;

  /********************************************************************/
  /* Check if we were provided the address of the server using        */
  /* inet_pton() to convert the text form of the address to binary    */
  /* form. If it is numeric then we want to prevent getaddrinfo()     */
  /* from doing any name resolution.                                  */
  /********************************************************************/
  rc = inet_pton(AF_INET, server, &serveraddr);
  if (rc == 1) /* valid IPv4 text address? */
  {
    hints.ai_family = AF_INET;
    hints.ai_flags |= AI_NUMERICHOST;
    cout << "using ipv4" << endl;
  } else {
    rc = inet_pton(AF_INET6, server, &serveraddr);
    if (rc == 1) /* valid IPv6 text address? */
    {
      hints.ai_family = AF_INET6;
      hints.ai_flags |= AI_NUMERICHOST;
      cout << "using ipv6" << endl;
    }
  }
  /********************************************************************/
  /* Get the address information for the server using getaddrinfo().  */
  /********************************************************************/
  rc = getaddrinfo(server, servport, &hints, &res);
  if (rc != 0) {
    printf("Host not found --> %s\n", gai_strerror(rc));
    if (rc == EAI_SYSTEM) perror("getaddrinfo() failed");
    exit(1);
  }

  if (LOGGER_DEBUG){
    cout << "Hostname resolution was successful." << endl;
  }

  /********************************************************************/
  /* The socket() function returns a socket descriptor, which represents   */
  /* an endpoint.  The statement also identifies the address family,  */
  /* socket type, and protocol using the information returned from    */
  /* getaddrinfo().                                                   */
  /********************************************************************/
  sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (sd < 0) {
    perror("socket() failed");
    exit(1);
  }

  if (LOGGER_DEBUG){
    cout << "Socket creation was successful." << endl;
  }

  /********************************************************************/
  /* Use the connect() function to establish a connection to the      */
  /* server.                                                          */
  /********************************************************************/
  rc = connect(sd, res->ai_addr, res->ai_addrlen);
  if (rc < 0) {
    /*****************************************************************/
    /* Note: the res is a linked list of addresses found for server. */
    /* If the connect() fails to the first one, subsequent addresses */
    /* (if any) in the list can be tried if required.               */
    /*****************************************************************/
    perror("connect() failed");
    exit(1);
  }

  if (LOGGER_DEBUG){
    cout << "Connection to the host was successful." << endl;
  }

  /********************************************************************/
  /* Send the JSON data to the server                              */
  /********************************************************************/

  // first, send the data length
  std::string json_string = data.dump();
  // copy the length of the string into a fixed-length buffer of a length that
  // the server already knows about
  string json_length = to_string(json_string.length());
  char length_buffer[INITIAL_MESSAGE_SEND_LENGTH_BYTES] = {0};
  strcpy(length_buffer, json_length.c_str());
  rc = send(sd, length_buffer, INITIAL_MESSAGE_SEND_LENGTH_BYTES, 0);
  if (rc < 0) {
    perror("send() of buffer length failed.");
    exit(1);
  }

  // ok now send the entire json body
  rc = send(sd, json_string.c_str(), json_string.length(), 0);
  if (rc < 0) {
    perror("send() of json string failed.");
    exit(1);
  }

  if (LOGGER_DEBUG){
    cout << "Sent " << rc << " bytes to the server." << endl;
  }


}

}  // namespace csmr

/**
 * CAN socket consuming thread main function.
 *
 * This thread pops a CAN frame from the decoding queue whenever it is
 * available, decodes it, then adds it to a batch write for the database.
 */
void* consumer(void* args) {
  // cast params pointer to a useable form (oooof, Dr. Owen would hate me)
  ConsumerParams params = *(ConsumerParams*)args;

  auto queue = params.queue;
  auto& interface_to_physical_name_map = params.bus_name_map;

  // initialize a variable that will hold the next time that we're forced to
  // write to the database.
  auto next_write_time = csmr::current_time() + params.max_write_delay;

  // variable to keep track of the number of measurements in the current batch
  int current_batch_size = 0;

  // variable to keep track of the json data that will be sent in the request to
  // the server
  json json_body;

  // decode messages until asked to stop -----
  while (!stop_logging) {
    // loop to read and package a batch write unti 1) the max time is up, 2) the
    // max write size has been reached, 3) someone is closing the thread
    while ((csmr::current_time() < next_write_time) &&
           (current_batch_size < params.max_write_size) && !stop_logging) {
      // if the json data is empty, write info data
      if (json_body.empty()) {
        json_body["key"] = params.key;
      }

      // pop a CMessage from the queue
      auto message = queue->Pop();
      auto frame = message->GetFrame();
      auto iface = message->GetBusName();
      auto bus_name = params.bus_name_map[iface];
      auto time = message->GetTime();

      // fill a json data structure with mesage data
      json msg_json;
      msg_json["id"] = frame->can_id;
      msg_json["t"] = time;
      msg_json["data"] = json::array();
      for (int i = 0; i < frame->can_dlc; i++) {
        msg_json["data"].push_back(frame->data[i]);
      }

      // add the message to the json body
      json_body["messages"][bus_name].push_back(msg_json);

      // print the frame data if in LOGGER_DEBUG mode
      if (false) {
        sem_wait(&stdout_sem);
        cout << iface_to_string(iface) << " consumer - ";
        cout << hex << setw(3) << setfill('0') << frame->can_id << ": ";
        for (int i = 0; i < frame->can_dlc; i++)
          printf("%02X ", frame->data[i]);
        cout << endl;
        sem_post(&stdout_sem);
      }
    }

    // send the data to the server
    csmr::send_json_to_server(params.host, params.port, json_body);

    // reset the JSON structure
    json_body = json();

    // set the next time to write a batch and reset the batch size count
    next_write_time = csmr::current_time() + params.max_write_delay;
    current_batch_size = 0;
  }

  return nullptr;
}
