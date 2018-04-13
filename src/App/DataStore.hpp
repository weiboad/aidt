#ifndef AIDT_DATASTORE_HPP_
#define AIDT_DATASTORE_HPP_

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

typedef struct FdItem {
	std::string pathfile;
	std::string origfile;
	uint64_t offset;
	int lastTime;
	bool moveProtected; // 是否处于保护期，如果处于文件保护状态在gc过程中保留
    std::fstream *fs;
} FdItem;
typedef std::unordered_map<std::string, FdItem> FdMap;

class DataStore {
public:
	DataStore(MessageQueue* queue);
	~DataStore();
	void saveMessage(const std::string& messageFile);
	void loadMessage(const std::string& messageFile);
	void saveMessage(const std::string& messageFile, MessageItem& item);
	bool saveOffset(std::ofstream* ofs, const FdItem& item);
	void saveOffsets(const std::string& offsetFile, const FdMap& maps);
	void loadOffsets(const std::string& offsetFile, FdMap& maps);
private:
	MessageQueue* _queue;
	void serialize(adbase::Buffer& buffer, MessageItem& item);
};
}
#endif
