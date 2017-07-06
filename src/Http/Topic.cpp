#include "Topic.hpp"

namespace adserver {
namespace http {
// {{{ Topic::Topic()

Topic::Topic(AdServerContext* context) :
	HttpInterface(context) {
}

// }}}
// {{{ void Topic::registerLocation()

void Topic::registerLocation(adbase::http::Server* http) {
	ADSERVER_HTTP_ADD_API(http, Topic, index)
}

// }}}
// {{{ void Topic::index()

void Topic::index(adbase::http::Request* request, adbase::http::Response* response, void*) {
	(void)request;
	responseJson(response, "{\"msg\": \"hello adinf\"}", 0, "");
}

// }}}
}
}