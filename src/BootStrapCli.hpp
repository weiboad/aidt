/// 该程序自动生成，禁止修改
#ifndef AIDT_BOOTSTRAPCLI_HPP_
#define AIDT_BOOTSTRAPCLI_HPP_

#include <signal.h>
#include <thread>
#include <adbase/Utility.hpp>
#include <adbase/Logging.hpp>
#include <adbase/Net.hpp>
#include "AdbaseConfig.hpp"
#include "App/DataStore.hpp"

class App;
class Timer;
class AdServer;
class Aims;
class BootStrapCli {
public:
	BootStrapCli();
	~BootStrapCli();
	void init(int argc, char **argv);
	void run();
	void stop(const int sig);

private:
    std::string _optFileName;
    std::string _opt;
    std::string _dumpDir;
	void parseOption(int argc, char **argv);
	void usage();
	void printVersion();
	int dumpMessage();
	void dumpMessageFile(app::MessageItem& item);
	int dumpOffset();
	int dynamicOffset();
};

#endif
