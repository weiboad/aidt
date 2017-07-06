#include "Message.hpp"
#include "App.hpp"

namespace adserver {
namespace http {
// {{{ Message::Message()

Message::Message(AdServerContext* context) :
	HttpInterface(context) {
}

// }}}
// {{{ void Message::registerLocation()

void Message::registerLocation(adbase::http::Server* http) {
	ADSERVER_HTTP_ADD_API(http, Message, index)
}

// }}}
// {{{ void Message::index()

void Message::index(adbase::http::Request* request, adbase::http::Response* response, void*) {
	if (_context->app == nullptr || _context->message == nullptr) {
		responseJson(response, "", 1000, "System is starting.", true);
		return;	
	}

	std::string topicName = request->getQuery("topic_name");
	if (topicName == "") {
		responseJson(response, "", 1000, "Must defined `topic_name`.", true);
		return;
	}
	std::string partIdStr = request->getQuery("part_id");
	errno = 0;
	uint32_t partId = static_cast<uint32_t>(strtoul(partIdStr.c_str(), nullptr, 10));
	if (errno != 0 || !partId) {
        partId = -1;
	}

	std::string data = request->getPost("data");
	if (data == "") {
		responseJson(response, "", 1000, "Must defined `data`.", true);
		return;
	}

	if (!_context->app->checkValidTopicAndPartition(topicName, partId)) {
		responseJson(response, "", 1000, "Topic or partId is invalid.", true);
		return;
	}

	adbase::Buffer buffer;	
	buffer.append(data);
	bool ret = _context->message->setMessage(topicName, partId, buffer);
	if (!ret) {
		responseJson(response, "", 1000, "Message queue fully.", true);
		return;
	}
	responseJson(response, "", 0, "Message send success", true);
}

// }}}
}
}
