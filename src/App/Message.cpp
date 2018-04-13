#include "Message.hpp"
#include <fstream>

namespace app {
// {{{ Message::Message()

Message::Message(AdbaseConfig* configure):
	_configure(configure) {
	adbase::metrics::Metrics::buildGauges("message", "queue.size", 1, [this](){
		return _queue.getSize();
	});
	_setRate   = adbase::metrics::Metrics::buildMeters("message", "set.rate");
    _dataStore = std::shared_ptr<DataStore>(new DataStore(&_queue));
}

// }}}
// {{{ Message::~Message()

Message::~Message() {
}

// }}}
// {{{ bool Message::send()

bool Message::send(std::string& topicName, int* partId, adbase::Buffer& message) {
	MessageItem item;
	bool ret = _queue.tryPop(item);
	if (!ret) {
		return false;
	}

	*partId = item.partId;
	message = item.message;
	topicName = item.topicName;
	return true;
}

// }}}
// {{{ bool Message::setMessage()

bool Message::setMessage(const std::string& topicName, int partId, adbase::Buffer& message) {
	// 防止队列堆积
    if (static_cast<int>(_queue.getSize()) > _configure->messageQueueLimit) {
        return false;
    }
	MessageItem item;
	item.partId    = partId;
	item.message   = message;
	item.topicName = topicName;
	item.tryNum    = 0;
	_queue.push(item);
	if (_setRate != nullptr) {
		_setRate->mark();
	}
	return true;
}

// }}}
// {{{ void Message::errorCallback()

void Message::errorCallback(const std::string& topicName, int partId, const adbase::Buffer& message, const std::string&) {
	std::lock_guard<std::mutex> lk(_mut);
    if (topicName.empty() || message.readableBytes() == 0) {
        return; 
    }

    LOG_ERROR << "Send message fail, message size:" << message.readableBytes() << " topic name:" << topicName << " partId:" << partId;
    MessageItem item;
    item.topicName = topicName;
    item.partId = partId;
    item.message = message;
    _dataStore->saveMessage(_configure->messageSendError, item);
}

// }}}
// {{{ void Message::saveMessage()

void Message::saveMessage() {
	std::lock_guard<std::mutex> lk(_mut);
    _dataStore->saveMessage(_configure->messageSendError);
}

// }}}
// {{{ void Message::loadMessage()

void Message::loadMessage() {
	std::lock_guard<std::mutex> lk(_mut);
    _dataStore->loadMessage(_configure->messageSendError);
}

// }}}
}
