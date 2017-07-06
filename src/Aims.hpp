#ifndef AIDT_AIMS_HPP_
#define AIDT_AIMS_HPP_

#include <adbase/Utility.hpp>
#include <adbase/Logging.hpp>
#include <adbase/Kafka.hpp>
#include "AdbaseConfig.hpp"
#include "Aims/Kafka/ProducerIn.hpp"

#ifndef DECLARE_KAFKA_CONSUMER
#define DECLARE_KAFKA_CONSUMER(name) \
	adbase::kafka::Consumer* _kafkaConsumer##name = nullptr;\
	aims::kafka::Consumer##name* _kafkaConsumerCallback##name = nullptr;
#endif
#ifndef DECLARE_KAFKA_PRODUCER
#define DECLARE_KAFKA_PRODUCER(name) \
	adbase::kafka::Producer* _kafkaProducer##name = nullptr;\
	aims::kafka::Producer##name* _kafkaProducerCallback##name = nullptr;
#endif

class Aims {
public:
	Aims(AimsContext* context);
	~Aims();
	void run();
	void stop();
    adbase::kafka::Producer* getProducer();

private:
	/// 传输上下文指针
	AimsContext* _context; 
	AdbaseConfig* _configure;

	DECLARE_KAFKA_PRODUCER(In)
	void init();
	void initKafkaProducer();
};

#endif
