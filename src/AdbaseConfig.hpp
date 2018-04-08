#ifndef AIDT_ADBASE_CONFIG_HPP_
#define AIDT_ADBASE_CONFIG_HPP_

#include <string>
#include <adbase/Net.hpp>
#include <adbase/Metrics.hpp>

// {{{ macros

#ifndef DECLARE_KAFKA_CONSUMER_CONFIG
#define DECLARE_KAFKA_CONSUMER_CONFIG(name) \
	std::string topicNameConsumer##name;\
	std::string groupId##name;\
	std::string brokerListConsumer##name;\
	std::string kafkaDebug##name;\
	std::string offsetPath##name;\
	std::string statInterval##name;\
	bool isNewConsumer##name;
#endif
#ifndef DECLARE_KAFKA_PRODUCER_CONFIG
#define DECLARE_KAFKA_PRODUCER_CONFIG(name) \
	std::string brokerListProducer##name;\
	std::string debug##name;\
	int queueLength##name; \
    std::string securityProtocol;\
    std::string saslMechanisms;\
    std::string kerberosServiceName;\
    std::string kerberosPrincipal;\
    std::string kerberosCmd;\
    std::string kerberosKeytab;\
    std::string kerberosMinTime;\
    std::string saslUsername;\
    std::string saslPassword;
#endif
#ifndef DECLARE_TIMER_CONFIG
#define DECLARE_TIMER_CONFIG(name) \
	int interval##name;
#endif

// }}}

typedef struct adbaseConfig {
	bool daemon;
	std::string pidFile;
	int appid;
	int macid;
    int mallocTrimPad;
    int mallocTrimInterval;
	
	// logging config
	std::string logsDir;
	size_t logRollSize;
	int logLevel;
	bool isAsync;
	
	bool isStartMc;	
	bool isStartHead;	
	bool isStartHttp;	

	std::string httpHost;
	int httpPort;
	int httpTimeout;
	int httpThreadNum;
	std::string httpDefaultController;
	std::string httpDefaultAction;
	std::string httpServerName;
	std::string httpAccessLogDir;
	int httpAccesslogRollSize;

	std::string headHost;
	int headPort;
	std::string headServerName;
	int headThreadNum;

	std::string mcHost;
	int mcPort;
	std::string mcServerName;
	int mcThreadNum;
	DECLARE_KAFKA_PRODUCER_CONFIG(In);
	DECLARE_TIMER_CONFIG(SyncOffset);

    std::string aidtConfig;

    // producer
    int messageQueueLimit;
    std::string messageSendError;

    // file reader
    int maxLines;
    int maxfd;
    int maxLineBytes;
    std::string offsetFile;
} AdbaseConfig;

class App;
namespace app {
    class Message;
    class Reader;
}
typedef struct adserverContext {
	AdbaseConfig* config;
	adbase::EventBasePtr mainEventBase;	
	App* app;
	adbase::metrics::Metrics* metrics;
	// 前后端交互数据指针添加到下面
    app::Message* message;
} AdServerContext;

typedef struct aimsContext {
	AdbaseConfig* config;
	App* app;
	// 消息队列交互上下文
    app::Message* message;
} AimsContext;
typedef struct timerContext {
	AdbaseConfig* config;
	adbase::EventBasePtr mainEventBase;	
	App* app;
	// 定时器交互上下文
    app::Reader* reader;
} TimerContext;

#endif
