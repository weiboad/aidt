#ifndef AIDT_APP_HPP_
#define AIDT_APP_HPP_

#include <adbase/Config.hpp>
#include "AdbaseConfig.hpp"
#include "App/Config.hpp"
#include "App/Message.hpp"
#include "App/Watcher.hpp"
#include "App/Reader.hpp"
#include "Aims.hpp"

class App {
public:
	App(AdbaseConfig* config);
	~App();
	void reload();
	void resend();
	void run();
	void stop();
	void setAdServerContext(AdServerContext* context);
	void setTimerContext(TimerContext* context);
	void setAimsContext(AimsContext* context);
	void loadConfig(adbase::IniConfig& config);
	void checkConfig();
	uint64_t getSeqId();
    void setAims(std::shared_ptr<Aims>& aims);
    bool checkValidTopicAndPartition(const std::string& topicName, int partId);

private:
	AdbaseConfig* _configure;
    std::shared_ptr<app::Config> _config;
    std::shared_ptr<app::Message> _message;
    std::shared_ptr<app::Reader> _reader;
    std::shared_ptr<app::Watcher> _watcher;
	std::shared_ptr<Aims> _aims;
	mutable std::mutex _mut;
    std::unordered_map<std::string, std::vector<uint32_t>> _topics;
};

#endif
