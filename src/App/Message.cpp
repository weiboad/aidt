#include "Message.hpp"
#include <fstream>

namespace app {
// {{{ Message::Message()

Message::Message(AdbaseConfig* configure):
	_configure(configure), 
	_index(0) {
	adbase::metrics::Metrics::buildGauges("message", "queue.size", 1, [this](){
		return _queue.getSize();
	});
	_setRate   = adbase::metrics::Metrics::buildMeters("message", "set.rate");
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
    if (topicName.empty() || message.readableBytes() == 0) {
        return; 
    }
    std::ofstream ofs(_configure->messageSendError.c_str(), std::ofstream::app | std::ofstream::binary);
    if (ofs.is_open() && ofs.good()) {
        MessageItem item;
        item.topicName = topicName;
        item.partId = partId;
        item.message = message;
        adbase::Buffer dumpBuffer;
        serialize(dumpBuffer, item);
        ofs.write(dumpBuffer.peek(), dumpBuffer.readableBytes());
    }
    ofs.flush();
    ofs.close();
}

// }}}
// {{{ void Message::saveMessage()

void Message::saveMessage() {
	std::lock_guard<std::mutex> lk(_mut);
	std::ofstream ofs(_configure->messageSendError.c_str(), std::ofstream::app | std::ofstream::binary);

	MessageItem item;
    uint32_t queueCount = 0;
	while (!_queue.empty()) { // 获取队列中的数据
		bool ret = _queue.tryPop(item);
		if (ret) {
            adbase::Buffer dumpBuffer;
            serialize(dumpBuffer, item);
            ofs.write(dumpBuffer.peek(), dumpBuffer.readableBytes());
            queueCount++;
		}
	}

	LOG_INFO << "Save message, count " << queueCount;
	ofs.flush();
	ofs.close();
}

// }}}
// {{{ void Message::loadMessage()

void Message::loadMessage() {
	std::lock_guard<std::mutex> lk(_mut);
	std::ifstream ifs(_configure->messageSendError.c_str(), std::ios_base::in | std::ios_base::binary);
	if (!ifs.good() || !ifs.is_open()) {
		return;	
	}

	while (true) {
		adbase::Buffer loadBuffer;
		uint32_t headerSize = 3 * static_cast<uint32_t>(sizeof(uint32_t));
		char lenBuf[headerSize];
		memset(lenBuf, 0, headerSize);
		ifs.read(lenBuf, headerSize);
		if (!ifs.good() || ifs.gcount() != headerSize) {
			break;
		}
		loadBuffer.append(lenBuf, headerSize);

		uint32_t partId = loadBuffer.readInt32();
		uint32_t tryNum = loadBuffer.readInt32();
		uint32_t topicNameSize = loadBuffer.readInt32(); 
		char topicNameBuf[topicNameSize];
		memset(topicNameBuf, 0, topicNameSize);
		ifs.read(topicNameBuf, topicNameSize);
		if (!ifs.good() || ifs.gcount() != topicNameSize) {
			break;
		}
		std::string topicName(topicNameBuf, topicNameSize);

		char messageSizeBuf[sizeof(uint32_t)] = {0};
		memset(messageSizeBuf, 0, sizeof(uint32_t));
		ifs.read(messageSizeBuf, sizeof(uint32_t));
		if (!ifs.good() || ifs.gcount() != sizeof(uint32_t)) {
			break;
		}
		uint32_t messageSize = 0;
		::memcpy(&messageSize, messageSizeBuf, sizeof messageSize);
		messageSize = adbase::networkToHost32(messageSize);

        std::unique_ptr<char[]> data(new char[messageSize]);
		ifs.read(data.get(), messageSize);
		if (!ifs.good() || ifs.gcount() != messageSize) {
			break;
		}
		adbase::Buffer message;
		message.append(data.get(), messageSize);
		
		MessageItem newItem;
		newItem.partId = partId;
		newItem.tryNum = tryNum;
		newItem.topicName = topicName;
		newItem.message = message;
		_queue.push(newItem);
	}

	ifs.close();
	std::string bakPathName = _configure->messageSendError + ".bak";
	if (0 != rename(_configure->messageSendError.c_str(), bakPathName.c_str())) {
		LOG_INFO << "Rename error file to bak fail.";
	}
}

// }}}
// {{{ void Message::serialize()

void Message::serialize(adbase::Buffer& buffer, MessageItem& item) {
	buffer.appendInt32(item.partId);
	buffer.appendInt32(item.tryNum);
	buffer.appendInt32(static_cast<uint32_t>(item.topicName.size()));
	buffer.append(item.topicName);

	size_t messageSize = item.message.readableBytes();
	buffer.appendInt32(static_cast<uint32_t>(messageSize));
	buffer.append(item.message.peek(), messageSize);
}

// }}}
}
