#ifndef AIDT_AIMS_KAFKA_PRODUER_INHPP_
#define AIDT_AIMS_KAFKA_PRODUER_INHPP_

#include <adbase/Kafka.hpp>
#include <adbase/Logging.hpp>
#include "AdbaseConfig.hpp"

namespace aims {
namespace kafka {
class ProducerIn {
public:
	ProducerIn(AimsContext* context);
	~ProducerIn();
	bool send(std::string& topicName, int* partId, adbase::Buffer& message);
    void errorCallback(const std::string& topicName, int partId, const adbase::Buffer& message, const std::string& error);
private:
	AimsContext* _context;
};

}
}
#endif
