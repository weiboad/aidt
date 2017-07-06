#include "Http.hpp"

// {{{ Http::Http()

Http::Http(AdServerContext* context, adbase::http::Server* http) :
	_context(context),
	_http(http) {
}

// }}}
// {{{ Http::~Http()

Http::~Http() {
	ADSERVER_HTTP_STOP(Server);
	ADSERVER_HTTP_STOP(Index);
	ADSERVER_HTTP_STOP(Config);
	ADSERVER_HTTP_STOP(Topic);
	ADSERVER_HTTP_STOP(Message);
}

// }}}
// {{{ void Http::addController()

void Http::addController() {
	ADSERVER_HTTP_ADD_CONTROLLER(Server);
	ADSERVER_HTTP_ADD_CONTROLLER(Index);
	ADSERVER_HTTP_ADD_CONTROLLER(Config);
	ADSERVER_HTTP_ADD_CONTROLLER(Topic);
	ADSERVER_HTTP_ADD_CONTROLLER(Message);
}

// }}}
// {{{ std::unordered_map<std::string, std::string> Http::rewrite()

std::unordered_map<std::string, std::string> Http::rewrite() {
	std::unordered_map<std::string, std::string> urls;
	return urls;
}

// }}}