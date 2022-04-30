/**
 * \file consumer.hpp
 *
 * \author Dave Yonkers
 * 
 * This thread will consume the CAN messages
 */

#include <string>
#include <unordered_map>

#include <linux/can.h>
#include <linux/can/raw.h>

#include <memory>

#include "Message.hpp"
#include "ThreadableQueue.hpp"
#include "interfaces.hpp"



/**
 * This struct should be populated and passed to the monitor thread as a void*
 * object. It will then be downcast (oof) to obtain the parameters
 */
struct ConsumerParams {
    std::unordered_map<Interface, std::string> bus_name_map;
    CThreadableMsgQueue<std::shared_ptr<CMessage>> *queue;
    int max_write_size;
    int max_write_delay;
    std::string host;
    std::string port;
    std::string key;
};

void* consumer(void*);

