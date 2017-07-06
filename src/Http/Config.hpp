#ifndef AIDT_HTTP_CONFIG_HPP_
#define AIDT_HTTP_CONFIG_HPP_

#include "HttpInterface.hpp"

namespace adserver {
namespace http {
class Config : HttpInterface {
public:
	Config(AdServerContext* context);
	void registerLocation(adbase::http::Server* http);
	void index(adbase::http::Request* request, adbase::http::Response* response, void*);
};
}
}

#endif