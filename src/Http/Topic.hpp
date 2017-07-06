#ifndef AIDT_HTTP_TOPIC_HPP_
#define AIDT_HTTP_TOPIC_HPP_

#include "HttpInterface.hpp"

namespace adserver {
namespace http {
class Topic : HttpInterface {
public:
	Topic(AdServerContext* context);
	void registerLocation(adbase::http::Server* http);
	void index(adbase::http::Request* request, adbase::http::Response* response, void*);
};
}
}

#endif