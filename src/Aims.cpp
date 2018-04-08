#include "Aims.hpp"

// {{{ Aims::Aims()

Aims::Aims(AimsContext* context) :
	_context(context) {
	_configure = _context->config;
}

// }}}
// {{{ Aims::~Aims()

Aims::~Aims() {
}

// }}}
// {{{ void Aims::run()

void Aims::run() {
	// 初始化 server
	init();
	_kafkaProducerIn->start();
}

// }}}
// {{{ void Aims::init()

void Aims::init() {
	initKafkaProducer();
}

// }}}
// {{{ void Aims::stop()

void Aims::stop() {
	if (_kafkaProducerIn != nullptr) {
		_kafkaProducerIn->stop();
	}
	if (_kafkaProducerCallbackIn != nullptr) {
		delete _kafkaProducerCallbackIn;
		_kafkaProducerCallbackIn = nullptr;
	}
}

// }}}
// {{{ adbase::kafka::Producer* Aims::getProducer()

adbase::kafka::Producer* Aims::getProducer() {
    return _kafkaProducerIn;
}

// }}}
// {{{ void Aims::initKafkaProducer()

void Aims::initKafkaProducer() {
	_kafkaProducerCallbackIn = new aims::kafka::ProducerIn(_context);
    std::unordered_map<std::string, std::string> configs;

    configs["metadata.broker.list"] = _configure->brokerListProducerIn;
    configs["debug"] = _configure->debugIn;
    configs["security.protocol"] = _configure->securityProtocol;
    configs["sasl.mechanisms"] = _configure->saslMechanisms;
    configs["sasl.kerberos.service.name"] = _configure->kerberosServiceName;
    configs["sasl.kerberos.principal"] = _configure->kerberosPrincipal;
    configs["sasl.kerberos.kinit.cmd"] = _configure->kerberosCmd;
    configs["sasl.kerberos.keytab"] = _configure->kerberosKeytab;
    configs["sasl.kerberos.min.time.before.relogin"] = _configure->kerberosMinTime;
    configs["sasl.username"] = _configure->saslUsername;
    configs["sasl.password"] = _configure->saslPassword;

	_kafkaProducerIn = new adbase::kafka::Producer(configs, _configure->queueLengthIn);
	_kafkaProducerIn->setSendHandler(std::bind(&aims::kafka::ProducerIn::send,
											   _kafkaProducerCallbackIn,
											   std::placeholders::_1, std::placeholders::_2,
											   std::placeholders::_3));
	_kafkaProducerIn->setErrorHandler(std::bind(&aims::kafka::ProducerIn::errorCallback,
											   _kafkaProducerCallbackIn, 
											   std::placeholders::_1, std::placeholders::_2,
											   std::placeholders::_3, std::placeholders::_4));
}

// }}}
