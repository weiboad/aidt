/// 该程序自动生成，禁止修改
#include "BootStrapCli.hpp"
// {{{ global

// 引导程序
BootStrapCli* bootMain;

// }}}
// {{{ static void killSignal()

static void killSignal(const int sig) {
	if (bootMain != nullptr) {
		bootMain->stop(sig);
        delete bootMain;
	}
}

// }}}
// {{{ static void registerSignal()

static void registerSignal() {
	/* 忽略Broken Pipe信号 */
	signal(SIGPIPE, SIG_IGN);
	/* 处理kill信号 */
	signal(SIGINT,  killSignal);
	signal(SIGKILL, killSignal);
	signal(SIGQUIT, killSignal);
	signal(SIGTERM, killSignal);
	signal(SIGHUP,  killSignal);
	signal(SIGSEGV, killSignal);
}

// }}}
// {{{ int main()

int main(int argc, char **argv) {
	try {
		// 初始化 BootStrap
		bootMain = new BootStrapCli();
		bootMain->init(argc, argv);
		// 注册信号处理
		registerSignal();
		// 启动运行
		bootMain->run();
	} catch (std::exception &e) {
		LOG_SYSFATAL << "Main exception: " << e.what();
	} catch (...) {
		LOG_SYSFATAL << "System exception.";
	}

	return 0;
}

// }}}
