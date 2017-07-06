#include "ProducerIn.hpp"
#include "App/Message.hpp"
#include "App.hpp"

namespace aims {
namespace kafka {
// {{{	ProducerIn::ProducerIn()

ProducerIn::ProducerIn(AimsContext* context) :
	_context(context) {
}

// }}}
// {{{ ProducerIn::~ProducerIn()

ProducerIn::~ProducerIn() {
}

// }}}
// {{{ bool ProducerIn::send()

bool ProducerIn::send(std::string& topicName, int* partId, adbase::Buffer& data) {
    if (_context->message == nullptr) {
        return false;
    }
    if (_context->app == nullptr) {
        return false;
    }

    bool ret = _context->message->send(topicName, partId, data);
    if (!_context->app->checkValidTopicAndPartition(topicName, *partId)) {
        _context->message->errorCallback(topicName, *partId, data, "Topic or parttion is invalid.");
        return false; 
    }
    return ret;
}

// }}}
// {{{ void ProducerIn::errorCallback()

void ProducerIn::errorCallback(const std::string& topicName, int partId, const adbase::Buffer& message, const std::string& error) {
    if (_context->message == nullptr) {
        return;
    }
    _context->message->errorCallback(topicName, partId, message, error);
}

// }}}
}
}
