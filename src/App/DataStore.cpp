#include "DataStore.hpp"
#include <fstream>

namespace app {
// {{{ DataStore::DataStore()

DataStore::DataStore(MessageQueue* queue): _queue(queue) {
}

// }}}
// {{{ DataStore::~DataStore()

DataStore::~DataStore() {
}

// }}}
// {{{ void DataStore::saveMessage()

void DataStore::saveMessage(const std::string& messageFile) {
	std::ofstream ofs(messageFile.c_str(), std::ofstream::app | std::ofstream::binary);

	MessageItem item;
    uint32_t queueCount = 0;
	while (!_queue->empty()) { // 获取队列中的数据
		bool ret = _queue->tryPop(item);
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
// {{{ void DataStore::saveMessage()

void DataStore::saveMessage(const std::string& messageFile, MessageItem& item) {
	std::ofstream ofs(messageFile.c_str(), std::ofstream::app | std::ofstream::binary);
    adbase::Buffer dumpBuffer;
    if (ofs.is_open() && ofs.good()) {
        serialize(dumpBuffer, item);
        ofs.write(dumpBuffer.peek(), dumpBuffer.readableBytes());
    }
	ofs.flush();
	ofs.close();
}

// }}}
// {{{ void DataStore::loadMessage()

void DataStore::loadMessage(const std::string& messageFile) {
	std::ifstream ifs(messageFile.c_str(), std::ios_base::in | std::ios_base::binary);
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
		_queue->push(newItem);
	}

	ifs.close();
	std::string bakPathName = messageFile + ".bak";
	if (0 != rename(messageFile.c_str(), bakPathName.c_str())) {
		LOG_INFO << "Rename error file to bak fail.";
	}
}

// }}}
// {{{ void DataStore::serialize()

void DataStore::serialize(adbase::Buffer& buffer, MessageItem& item) {
	buffer.appendInt32(item.partId);
	buffer.appendInt32(item.tryNum);
	buffer.appendInt32(static_cast<uint32_t>(item.topicName.size()));
	buffer.append(item.topicName);

	size_t messageSize = item.message.readableBytes();
	buffer.appendInt32(static_cast<uint32_t>(messageSize));
	buffer.append(item.message.peek(), messageSize);
}

// }}}
// {{{ bool DataStore::saveOffset()

bool DataStore::saveOffset(std::ofstream* ofs, const FdItem& item) {
    struct stat fileStat;
    if (0 != stat(item.origfile.c_str(), &fileStat) || !S_ISREG(fileStat.st_mode)) {
        return false;
    }
    uint64_t inode = static_cast<uint64_t>(fileStat.st_ino);
    uint64_t filesize = static_cast<uint64_t>(fileStat.st_size);

    adbase::Buffer buffer;
    buffer.appendInt64(item.offset);
    buffer.appendInt64(inode);
    buffer.appendInt64(filesize);
    buffer.appendInt32(static_cast<uint32_t>(item.pathfile.size()));
    buffer.appendInt32(static_cast<uint32_t>(item.origfile.size()));
    buffer.append(item.pathfile);
    buffer.append(item.origfile);
    ofs->write(buffer.peek(), buffer.readableBytes());

    return true;
}

// }}}
// {{{ void DataStore::loadOffsets()

void DataStore::loadOffsets(const std::string& offsetFile, FdMap& maps) {
    std::ifstream ifs(offsetFile.c_str(), std::ios_base::in | std::ios_base::binary);
    if (!ifs.good() || !ifs.is_open()) {
        return;
    }
    while (true) {
        adbase::Buffer loadBuffer;
        uint32_t headerSize = 8 * static_cast<uint32_t>(sizeof(uint32_t));
        char lenBuf[headerSize];
        memset(lenBuf, 0, headerSize);
        ifs.read(lenBuf, headerSize);
        if (!ifs.good() || ifs.gcount() != headerSize) {
            break;
        }
        loadBuffer.append(lenBuf, headerSize);

        uint64_t offset = loadBuffer.readInt64();
        loadBuffer.readInt64(); // inode
        loadBuffer.readInt64(); // filesize
        uint32_t pathFileSize = loadBuffer.readInt32();
        uint32_t origFileSize = loadBuffer.readInt32();
        char pathFileBuf[pathFileSize];
        char origFileBuf[origFileSize];
        memset(pathFileBuf, 0, pathFileSize);
        memset(origFileBuf, 0, origFileSize);
        ifs.read(pathFileBuf, pathFileSize);
        if (!ifs.good() || ifs.gcount() != pathFileSize) {
            break;
        }
        std::string pathFile(pathFileBuf, pathFileSize);
        ifs.read(origFileBuf, origFileSize);
        if (!ifs.good() || ifs.gcount() != origFileSize) {
            break;
        }
        std::string origFile(origFileBuf, origFileSize);
        FdItem item;
        item.pathfile = pathFile;
        item.origfile = origFile;
        item.offset   = offset;
        item.fs       = nullptr;
        maps[origFile] = item;
        LOG_INFO << "Load origin file offset info, originFile: " << item.origfile
                 << ", pathFile: " << item.pathfile
                 << ", offset: "   << item.offset;
    }
    ifs.close();
}

// }}}
// {{{ void DataStore::saveOffsets()

void DataStore::saveOffsets(const std::string& offsetFile, const FdMap& maps) {
    std::ofstream ofs(offsetFile.c_str(), std::ofstream::trunc | std::ofstream::binary);
    for (auto &t : maps) {
        saveOffset(&ofs, t.second);
    }
    ofs.flush();
    ofs.close();
}

// }}}

}
