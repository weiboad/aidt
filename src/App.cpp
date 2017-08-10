#include <adbase/Utility.hpp>
#include <adbase/Logging.hpp>
#include "App.hpp"
#include "App/ConfigPaser.hpp"
#include "App/Reader.hpp"
#include "App/Watcher.hpp"

//{{{ macros

#define LOAD_KAFKA_CONSUMER_CONFIG(name, sectionName) do {\
    _configure->isNewConsumer##name = config.getOptionBool("kafkac_"#sectionName, "isNewConsumer"#name);\
    _configure->topicNameConsumer##name    = config.getOption("kafkac_"#sectionName, "topicName"#name);\
    _configure->groupId##name      = config.getOption("kafkac_"#sectionName, "groupId"#name);\
    _configure->brokerListConsumer##name   = config.getOption("kafkac_"#sectionName, "brokerList"#name);\
    _configure->kafkaDebug##name   = config.getOption("kafkac_"#sectionName, "kafkaDebug"#name);\
    _configure->offsetPath##name   = config.getOption("kafkac_"#sectionName, "offsetPath"#name);\
    _configure->statInterval##name = config.getOption("kafkac_"#sectionName, "statInterval"#name);\
} while(0)

#define LOAD_KAFKA_PRODUCER_CONFIG(name, sectionName) do {\
    _configure->topicNameProducer##name    = config.getOption("kafkap_"#sectionName, "topicName"#name);\
    _configure->brokerListProducer##name   = config.getOption("kafkap_"#sectionName, "brokerList"#name);\
    _configure->debug##name        = config.getOption("kafkap_"#sectionName, "debug"#name);\
    _configure->queueLength##name  = config.getOptionUint32("kafkap_"#sectionName, "queueLength"#name);\
} while(0)

#define LOAD_TIMER_CONFIG(name) do {\
	_configure->interval##name = config.getOptionUint32("timer", "interval"#name);\
} while(0)

//}}}
// {{{ App::App()

App::App(AdbaseConfig* config) :
	_configure(config) {
}

// }}}
// {{{ App::~App()

App::~App() {
}

// }}}
// {{{ void App::run()

void App::run() {
    _config = std::shared_ptr<app::Config>(new app::Config());
    _message = std::shared_ptr<app::Message>(new app::Message(_configure));

    app::ConfigPaser::paser(_configure->aidtConfig, _config.get());

    _reader = std::shared_ptr<app::Reader>(new app::Reader(_config, this, _configure, _message));
    _reader->load();

    _watcher = std::shared_ptr<app::Watcher>(new app::Watcher(_configure, _config));
    _watcher->setReadHandler(std::bind(&app::Reader::read, _reader, std::placeholders::_1, std::placeholders::_2));
    _watcher->setClearHandler(std::bind(&app::Reader::clear, _reader, std::placeholders::_1));
    _watcher->setMoveToHandler(std::bind(&app::Reader::moveto, _reader, std::placeholders::_1));
    _watcher->setMoveSelfHandler(std::bind(&app::Reader::moveself, _reader, std::placeholders::_1));

    _watcher->start();
    _message->loadMessage();
}

// }}}
// {{{ void App::checkConfig()

void App::checkConfig() {
    _config = std::shared_ptr<app::Config>(new app::Config());
    app::ConfigPaser::paser(_configure->aidtConfig, _config.get());
}

// }}}
// {{{ void App::reload()

void App::reload() {
    app::ConfigPaser::paser(_configure->aidtConfig, _config.get());
}

// }}}
// {{{ void App::stop()

void App::stop() {
    if (_message) {
        _message->saveMessage();
    }

    if (_watcher) {
        _watcher->stop();
        if (_reader) {
            _reader->stop();
        }
    }
    if (_reader) {
        _reader->save();
    }
}

// }}}
// {{{ void App::setAdServerContext()

void App::setAdServerContext(AdServerContext* context) {
	context->app = this;
    context->message = _message.get();
}

// }}}
// {{{ void App::setAimsContext()

void App::setAimsContext(AimsContext* context) {
	context->app = this;
    context->message = _message.get();
}

// }}}
// {{{ void App::setTimerContext()

void App::setTimerContext(TimerContext* context) {
	context->app = this;
    context->reader = _reader.get();
}

// }}}
// {{{ void App::setAims()

void App::setAims(std::shared_ptr<Aims>& aims) {
    _aims = aims;
}

// }}}
//{{{ void App::loadConfig()

void App::loadConfig(adbase::IniConfig& config) {
	LOAD_KAFKA_PRODUCER_CONFIG(In, in);
	
	LOAD_TIMER_CONFIG(SyncOffset);

    _configure->aidtConfig = config.getOption("system", "aidtConfig");
    _configure->mallocTrimInterval = config.getOptionUint32("system", "mallocTrimInterval");
    _configure->mallocTrimPad = config.getOptionUint32("system", "mallocTrimPad");
    _configure->messageQueueLimit = config.getOptionUint32("message", "queueLimit");
    _configure->messageSendError  = config.getOption("message", "sendError");

    _configure->maxLines		  = config.getOptionUint32("reader", "maxLines");
    _configure->maxfd			  = config.getOptionUint32("reader", "maxfd");
    _configure->maxLineBytes	  = config.getOptionUint32("reader", "maxLineBytes");
    _configure->offsetFile		  = config.getOption("reader", "offsetFile");
}

//}}}
//{{{ uint64_t App::getSeqId()

uint64_t App::getSeqId() {
	std::lock_guard<std::mutex> lk(_mut);
	adbase::Sequence seq;
	return seq.getSeqId(static_cast<uint16_t>(_configure->macid), static_cast<uint16_t>(_configure->appid));
}

//}}}
//{{{ bool App::checkValidTopicAndPartition()

bool App::checkValidTopicAndPartition(const std::string& topicName, int partId) {
	std::lock_guard<std::mutex> lk(_mut);
    if (_topics.find(topicName) == _topics.end()) {
        _topics = _aims->getProducer()->getTopics();
    }
    if (_topics.find(topicName) == _topics.end()) {
        return false;
    }

    if (partId == -1) { //不需要指定分区
        return true; 
    }

    for (auto &t : _topics[topicName]) {
        if (t == static_cast<uint32_t>(partId)) {
            return true;
        }
    }
    return false;
}

//}}}
