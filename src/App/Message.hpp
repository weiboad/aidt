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
#include "DataStore.hpp"

namespace app {
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
	mutable std::mutex _mut;
    std::shared_ptr<DataStore> _dataStore;
};
}
#endif
