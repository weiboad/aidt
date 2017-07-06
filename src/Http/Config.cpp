#include "Config.hpp"

namespace adserver {
namespace http {
// {{{ Config::Config()

Config::Config(AdServerContext* context) :
	HttpInterface(context) {
}

// }}}
// {{{ void Config::registerLocation()

void Config::registerLocation(adbase::http::Server* http) {
	ADSERVER_HTTP_ADD_API(http, Config, index)
}

// }}}
// {{{ void Config::index()

void Config::index(adbase::http::Request* request, adbase::http::Response* response, void*) {
	(void)request;
	responseJson(response, "{\"msg\": \"hello adinf\"}", 0, "");
}

// }}}
}
}