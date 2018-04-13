/// 该程序自动生成，禁止修改
#include <adbase/Logging.hpp>
#include <unistd.h>
#include <signal.h>
#include <fstream>
#include "BootStrapCli.hpp"
#include "Version.hpp"

// {{{ BootStrapCli::BootStrapCli()

BootStrapCli::BootStrapCli() {
}

// }}}
// {{{ BootStrapCli::~BootStrapCli()

BootStrapCli::~BootStrapCli() {
}

// }}}
// {{{ void BootStrapCli::init()

void BootStrapCli::init(int argc, char **argv) {
	// 解析指定的参数
	parseOption(argc, argv);
}

// }}}
// {{{ void BootStrapCli::run()

void BootStrapCli::run() {
    int ret = 0;
    if (_opt == "dumpmessage") {
        ret = dumpMessage();
    } else if (_opt == "dumpoffset") {
        ret = dumpOffset();
    } else if (_opt == "dynamicoffset") {
        ret = dynamicOffset();
    }

    exit(ret);
}

// }}}
// {{{ void BootStrapCli::stop()

void BootStrapCli::stop(const int sig) {
}

// }}}
// {{{ void BootStrapCli::usage()

void BootStrapCli::usage() {
	std::cout << "Usage: aidt -o [opt] [options...] <path>" << std::endl;
	std::cout << "\t-o: 操作命令" << std::endl;
	std::cout << "\t\tdumpmessage: 导出消息数据到文件" << std::endl;
	std::cout << "\t-f: 操作的文件" << std::endl;
	std::cout << "\t-h: 帮助" << std::endl;
	exit(0);
}

// }}}
// {{{ void BootStrapCli::printVersion()

void BootStrapCli::printVersion() {
	std::cout << "VERSION  :  " << VERSION << std::endl;
	std::cout << "GIT SHA1 :  " << GIT_SHA1 << std::endl;
	std::cout << "GIT DIRTY:  " << GIT_DIRTY << std::endl;
	std::cout << "BUILD ID :  " << BUILD_ID << std::endl;
	exit(0);
}

// }}}
// {{{ void BootStrapCli::parseOption() 

void BootStrapCli::parseOption(int argc, char **argv) {
	int ch;
	while((ch = getopt(argc, argv, "o:f:d:hv")) != -1) {
		if (ch == 'o') {
            _opt = optarg;
            if (_opt != "dumpmessage" && _opt != "dumpoffset" && _opt != "dynamicoffset") {
			    usage();
            }
		} else if (ch == 'f') {
            _optFileName = optarg;
		} else if (ch == 'd') {
            _dumpDir = optarg;
		} else if (ch == 'h') {
			usage();
		} else if (ch == 'v') {
			printVersion();
		}
	}
}

// }}}
// {{{ int BootStrapCli::dumpMessage()

int BootStrapCli::dumpMessage() {
    struct stat fileStat;
    if (stat(_optFileName.c_str(), &fileStat) != 0 || !S_ISREG(fileStat.st_mode)) {
        LOG_ERROR << "Opt file `" << _optFileName << "` is not file or not exists.";
        return 1;
    }
    if (stat(_dumpDir.c_str(), &fileStat) != 0 || !S_ISDIR(fileStat.st_mode)) {
        LOG_ERROR << "Dump directory `" << _dumpDir << "` is not dir or not exists.";
        return 2;
    }

    app::MessageQueue queue;
    app::DataStore data(&queue);
    data.loadMessage(_optFileName);
    if (!queue.empty()) {
        LOG_INFO << "Load message size:" << queue.getSize(); 
    }
	while (!queue.empty()) { // 获取队列中的数据
        app::MessageItem item;
		bool ret = queue.tryPop(item);
		if (!ret) {
            break;
		}
        dumpMessageFile(item);
	}
    return 0;
}

// }}}
// {{{ void BootStrapCli::dumpMessageFile()

void BootStrapCli::dumpMessageFile(app::MessageItem& item) {
    std::string topicName = item.topicName;
    std::string fileName = _dumpDir;
    std::string::size_type n = fileName.find_last_not_of("/");
    if (n != std::string::npos) { 
        fileName.append("/");
    }
    fileName.append(topicName);
    std::ofstream ofs(fileName.c_str(), std::ofstream::app);
    ofs << item.message.retrieveAllAsString() << '\n';
    ofs.flush();
    ofs.close();
}

// }}}
// {{{ int BootStrapCli::dumpOffset()

int BootStrapCli::dumpOffset() {
    struct stat fileStat;
    if (stat(_optFileName.c_str(), &fileStat) != 0 || !S_ISREG(fileStat.st_mode)) {
        LOG_ERROR << "Opt file `" << _optFileName << "` is not file or not exists.";
        return 1;
    }
    if (stat(_dumpDir.c_str(), &fileStat) != 0 || !S_ISDIR(fileStat.st_mode)) {
        LOG_ERROR << "Dump directory `" << _dumpDir << "` is not dir or not exists.";
        return 2;
    }

    app::FdMap maps;
    app::DataStore data(nullptr);
    data.loadOffsets(_optFileName, maps);

    if (!maps.empty()) {
        LOG_INFO << "Load offset size:" << maps.size(); 
    }

    std::string fileName = _dumpDir;
    std::string::size_type n = fileName.find_last_not_of("/");
    if (n != std::string::npos) { 
        fileName.append("/");
    }
    fileName.append("dump_offsets");

    std::ofstream ofs(fileName.c_str(), std::ofstream::app);

    for (auto &t : maps) {
        ofs << t.second.offset << '\t' << t.second.pathfile << '\t' << t.second.origfile << '\n';
    }
    ofs.flush();
    ofs.close();
    return 0;
}

// }}}
// {{{ int BootStrapCli::dynamicOffset()

int BootStrapCli::dynamicOffset() {
    struct stat fileStat;
    if (stat(_optFileName.c_str(), &fileStat) != 0 || !S_ISREG(fileStat.st_mode)) {
        LOG_ERROR << "Opt file `" << _optFileName << "` is not file or not exists.";
        return 1;
    }
    if (stat(_dumpDir.c_str(), &fileStat) != 0 || !S_ISDIR(fileStat.st_mode)) {
        LOG_ERROR << "Dump directory `" << _dumpDir << "` is not dir or not exists.";
        return 2;
    }

    app::FdMap maps;
    app::DataStore data(nullptr);

    std::ifstream ifs(_optFileName.c_str(), std::ios_base::in | std::ios_base::binary);
    if (!ifs.good() || !ifs.is_open()) {
        return 1;
    }

    std::string line;
    while(std::getline(ifs, line)) {
        std::vector<std::string> infos = adbase::explode(line, '\t', true); 
        if (static_cast<uint32_t>(infos.size()) != 3) {
            LOG_ERROR << "Offset format is invalid, offset item:" << line; 
            continue;
        }

        errno = 0;
        uint64_t offset = static_cast<uint64_t>(strtoull(infos[0].c_str(), nullptr, 10));
        if (errno != 0) {
            continue;
        }
        app::FdItem item;
        item.pathfile = infos[1];
        item.origfile = infos[2];
        item.offset = offset;
        maps[item.origfile] = item;
    }

    if (!maps.empty()) {
        LOG_INFO << "Load offset size:" << maps.size(); 
    }

    std::string fileName = _dumpDir;
    std::string::size_type n = fileName.find_last_not_of("/");
    if (n != std::string::npos) { 
        fileName.append("/");
    }
    fileName.append("dynamic_offsets");
    data.saveOffsets(fileName, maps);
    return 0;
}

// }}}
