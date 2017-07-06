#ifndef AIDT_HTTP_MESSAGE_HPP_
#define AIDT_HTTP_MESSAGE_HPP_

#include "HttpInterface.hpp"

namespace adserver {
namespace http {
class Message : HttpInterface {
public:
	Message(AdServerContext* context);
	void registerLocation(adbase::http::Server* http);
	void index(adbase::http::Request* request, adbase::http::Response* response, void*);
};
}
}

#endif