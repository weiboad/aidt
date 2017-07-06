#ifndef AIDT_MESSAGE_HPP_
#define AIDT_MESSAGE_HPP_

#include <adbase/Utility.hpp> 
#include <adbase/Logging.hpp> 
#include <adbase/Metrics.hpp>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <limits.h>
#include "AdbaseConfig.hpp"

namespace app {
// message queue
typedef struct MessageItem {
    int partId;
    int tryNum;
    std::string topicName;
    adbase::Buffer message;
} MessageItem;
typedef adbase::Queue<MessageItem> MessageQueue;

class Message {
public:
	Message(AdbaseConfig* configure);
	~Message();
	bool send(std::string& topicName, int* partId, adbase::Buffer& message);
	void errorCallback(const std::string& topicName, int partId, const adbase::Buffer& message, const std::string& error);
	bool setMessage(const std::string& topicName, int partId, adbase::Buffer& message);
	void saveMessage();
	void loadMessage();

private:
	AdbaseConfig *_configure;
	MessageQueue _queue;
	adbase::metrics::Meters* _setRate    = nullptr;
	uint64_t _index; // no thread safe
	mutable std::mutex _mut;
	void serialize(adbase::Buffer& buffer, MessageItem& item);
};
}
#endif
