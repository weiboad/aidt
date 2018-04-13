#include "Reader.hpp"
#include "Watcher.hpp"
#include "Message.hpp"
#include <adbase/Logging.hpp>
#include "App.hpp"

namespace app {
// {{{ Reader::Reader()

Reader::Reader(std::shared_ptr<app::Config>& watcherConfig, App* app, AdbaseConfig *configure, std::shared_ptr<app::Message>& message) :
    _configure(configure),
    _watcherConfig(watcherConfig),
    _app(app),
    _message(message),
    _running(true) {
    _currentFdNum = 0;
    _dataStore = std::shared_ptr<DataStore>(new DataStore(nullptr));
}

// }}}
// {{{ Reader::~Reader()

Reader::~Reader() {
}

// }}}

// {{{ void Reader::read()

void Reader::read(const InotifyEvent& event, bool& isContinue) {
    WatcherFileInfo info;
    bool ret = _watcherConfig->findFile(const_cast<std::string&>(event.pathfile), info);
    if (!ret) {
        LOG_INFO << "Can't find pathFile " << event.pathfile << " in config manager";
        return;
    }

    if (info.isBinary) { // binary safe
        readBinary(event, info, isContinue);
    } else {
        readString(event, info, isContinue);
    }
}

// }}}
// {{{ void Reader::clear()

void Reader::clear(const InotifyEvent& event) {
    std::lock_guard<std::mutex> lk(_mut);
    LOG_INFO << "Delete file `" << event.origfile << "`, start.";
    for (auto &t : _fdMap) {
        LOG_INFO << "Map key: " << t.first << ", find: " << event.origfile;
    }

    if (_fdMap.find(event.origfile) != _fdMap.end()) {
        FdItem item = _fdMap[event.origfile];
        closeStream(item.fs);
        if (item.fs != nullptr) {
            delete item.fs;
        }
        _fdMap.erase(event.origfile);
        LOG_INFO << "Delete file `" << event.origfile << "`, offset map clear.";
    }

    // 删除 alias 数据
    std::vector<std::string> keys;
    for (auto &t : _aliasMap) {
        if (t.second == event.origfile) {
            keys.push_back(t.first);
        }
    }
    for (auto &t : keys) {
        _aliasMap.erase(t);
    }
}

// }}}
// {{{ void Reader::moveself()

void Reader::moveself(const InotifyEvent& event) {
    _watcherConfig->setMoveTime(const_cast<std::string&>(event.pathfile), static_cast<uint32_t>(time(nullptr)));
    {
        std::lock_guard<std::mutex> lk(_mut);
        if (_fdMap.find(event.origfile) != _fdMap.end()) {
            FdItem& item = _fdMap[event.origfile];
            item.moveProtected = true;
        }
    }
    LOG_INFO << "Set file move time:" << static_cast<uint32_t>(time(nullptr));
}

// }}}
// {{{ void Reader::moveto()

void Reader::moveto(const InotifyEvent& event) {
    std::lock_guard<std::mutex> lk(_mut);
    if (_fdMap.find(event.aliasfile) != _fdMap.end()) {
        _fdMap[event.origfile] = _fdMap[event.aliasfile];
        _fdMap[event.origfile].origfile = event.origfile;
        // 用来更正在 move 操作以前生成的队列数据
        _aliasMap[event.aliasfile] = event.origfile;
        _fdMap.erase(event.aliasfile);
    }
}

// }}}
// {{{ void Reader::save()

void Reader::save() {
    std::lock_guard<std::mutex> lk(_mut);
    std::string pathName = _configure->offsetFile;
    pathName.append(".swp");
    std::string bakPathName = _configure->offsetFile;
    bakPathName.append(".bak");
    std::ofstream ofs(pathName.c_str(), std::ofstream::trunc | std::ofstream::binary);
    for (auto &t : _fdMap) {
        WatcherFileInfo info;
        bool ret = _watcherConfig->findFile(const_cast<std::string&>(t.second.pathfile), info);
        if (!ret) {
            LOG_INFO << "Can't find pathFile " << t.second.pathfile << " in config manager";
            continue;
        }

        _dataStore->saveOffset(&ofs, t.second);
    }
    ofs.flush();
    ofs.close();
    if (0 != rename(_configure->offsetFile.c_str(), bakPathName.c_str())) {
        LOG_INFO << "Rename offset file to bak fail.";
    }
    if (0 != rename(pathName.c_str(), _configure->offsetFile.c_str())) {
        LOG_INFO << "Rename swp offset file to offset file fail.";
    }
    LOG_TRACE << "Save file offset file success.";
}

// }}}
// {{{ void Reader::load()

void Reader::load() {
    std::lock_guard<std::mutex> lk(_mut);
    _dataStore->loadOffsets(_configure->offsetFile, _fdMap);
}

// }}}
// {{{ void Reader::stop()

void Reader::stop() {
    _running = false;
}

// }}}

// {{{ void Reader::readString()

void Reader::readString(const InotifyEvent& event, const WatcherFileInfo& info, bool& isContinue) {
    isContinue = false;
    FdItem item;
    LOG_TRACE << "file " << event.origfile ;
    if (!initStream(event, item, info, isContinue)) {
        return;
    }

    std::filebuf* pbuf = item.fs->rdbuf();
    pbuf->pubseekpos(item.offset);
    LOG_TRACE << "file " << event.origfile << ", seek:" << item.offset;

    size_t readSize = 0;
    int lines = 0;
    char nextchar;
    adbase::Buffer buffer;
    while((nextchar = pbuf->sbumpc()) != EOF && _running) {
        readSize++;
        if (nextchar != '\n') {
            buffer.append(&nextchar, 1);
            if (static_cast<int>(buffer.readableBytes()) > _configure->maxLineBytes) {
                LOG_INFO << "File " << event.origfile << " single line > " << _configure->maxLineBytes;
                isContinue = true;
                break;
            }
            continue;
        }

        if (buffer.readableBytes()) {
            // 更新文件读取的位置
            if (!sendMessage(buffer, info)) {
                isContinue = true;
                break;
            }
            //LOG_INFO << "FILE:" << event.origfile << " size:" << readSize << "mess:" << std::string(buffer.peek(), buffer.readableBytes());
            setFileOffset(const_cast<std::string&>(event.origfile), readSize);
            readSize = 0;
            lines++;

            buffer.retrieveAll();
            if (lines > _configure->maxLines) { // 超过单次事件的行数
                isContinue = true;
                break;
            }
        }
    }
}

// }}}
// {{{ void Reader::readBinary()

void Reader::readBinary(const InotifyEvent& event, const WatcherFileInfo& info, bool& isContinue) {
    isContinue = false;
    FdItem item;
    LOG_INFO << "file " << event.origfile ;
    if (!initStream(event, item, info, isContinue)) {
        return;
    }

    int lines = 0;
    item.fs->seekg(item.offset, std::ios::beg);
    LOG_INFO << "file " << event.origfile << ", seek:" << item.offset;
    while (_running) {
        char lenBuf[sizeof(uint32_t)] = {0};
        uint32_t len = 0;
        item.fs->read(lenBuf, sizeof(uint32_t));
        if (item.fs->good() && item.fs->gcount() == sizeof(uint32_t)) {
            ::memcpy(&len, lenBuf, sizeof len);
            len = adbase::networkToHost32(len);
        } else {
            break;
        }

        std::unique_ptr<char[]> data(new char[len]);
        item.fs->read(data.get(), len);
        if (item.fs->good() && item.fs->gcount() == len) {
            adbase::Buffer buffer;
            buffer.append(data.get(), len);
            if (!sendMessage(buffer, info)) {
                isContinue = true;
                break;
            }
            item.offset += len + sizeof(uint32_t);
            setFileOffset(const_cast<std::string&>(event.origfile), len + sizeof(uint32_t));
            lines++;
        } else {
            break;
        }
        item.fs->seekg(item.offset, std::ios::beg);
        if (lines > _configure->maxLines) { // 超过单次事件的行数
            isContinue = true;
            break;
        }
    }
}

// }}}
// {{{ void Reader::sendMessage()

bool Reader::sendMessage(adbase::Buffer &msg, const WatcherFileInfo& info) {
    uint32_t partId = -1;
    adbase::Buffer message;
    if (info.isBinary) {
        LOG_TRACE << "Message: binary(" << msg.readableBytes() << "), topic:" << info.topicName
                  << ", partId: " << partId;
        message = msg;
    } else {
        //message = msg;
        uint32_t formatBufferSize = static_cast<uint32_t>(msg.readableBytes())
                                    + static_cast<uint32_t>(info.format.size()) - 1;
        std::unique_ptr<char[]> buffer(new char[formatBufferSize]);
        snprintf(buffer.get(), formatBufferSize, info.format.c_str(), msg.peek());
        // 去除尾部 snprintf 自动添加的 \0 字符
        message.append(buffer.get(), formatBufferSize - 1);
    }

    return _message->setMessage(info.topicName, partId, message);
}

// }}}
// {{{ bool Reader::initStream()

bool Reader::initStream(const InotifyEvent& event, FdItem& item, const WatcherFileInfo& info, bool& isContinue) {
    int currentTime = static_cast<uint32_t>(time(nullptr));
    isContinue = false;
    std::lock_guard<std::mutex> lk(_mut);

    InotifyEvent tmpEvent = event;
    if (_currentFdNum > _configure->maxfd) { // 如果已经到打开文件最大数，主动gc
        LOG_TRACE << "Current not has enough fd, gc fd, max fd set is " << _configure->maxfd;
        gcFstreamNonLock(1);
    }
    if (_currentFdNum > _configure->maxfd) {
        LOG_TRACE << "open file max limit, pathfile: " << tmpEvent.origfile;
        isContinue = true;
        return false;
    }


    if (_aliasMap.find(tmpEvent.origfile) != _aliasMap.end()) {
        tmpEvent.origfile = _aliasMap[tmpEvent.origfile];
    }
    if (_fdMap.find(tmpEvent.origfile) == _fdMap.end()) {
        LOG_INFO << "Can't find `" << tmpEvent.origfile << "` fd item, create new it";
        item.pathfile = tmpEvent.pathfile;
        item.origfile = tmpEvent.origfile;
        item.offset = 0;
        item.fs = nullptr;
    } else {
        item = _fdMap[tmpEvent.origfile];
        if (info.moveTime != 0 && (currentTime - info.moveTime) > info.timeWait) { // 需要重新重置 offset
            LOG_INFO << "File move timeout, need reset origfile info, timewait:" << info.timeWait;
            item.offset = 0;
            closeStream(item.fs);
            item.moveProtected = false;
            _watcherConfig->setMoveTime(const_cast<std::string&>(tmpEvent.origfile), 0);
        }
    }

    item.fs = openStream(tmpEvent.origfile, info, item.fs);
    item.lastTime = currentTime;
    _fdMap[tmpEvent.origfile] = item;
    if (item.fs != nullptr) {
        return true;
    }
    LOG_INFO << "open file fail, pathfile: " << tmpEvent.origfile;
    return false;
}

// }}}
// {{{ void Reader::setFileOffset()

void Reader::setFileOffset(std::string &pathFile, uint64_t offset) {
    std::lock_guard<std::mutex> lk(_mut);
    FdItem item;
    if (_fdMap.find(pathFile) == _fdMap.end()) {
        return;
    } else {
        item = _fdMap[pathFile];
        item.offset += offset;
    }

    _fdMap[pathFile] = item;
}

// }}}

// {{{ void Reader::gcFstreamNonLock()

void Reader::gcFstreamNonLock(int lifeTime) {
    int currentTime = static_cast<uint32_t>(time(nullptr));
    int count = 0;
    for (auto &t : _fdMap) {
        if (t.second.fs != nullptr && t.second.fs->is_open()) {
            count++;
        }
        if (!t.second.lastTime || (currentTime - t.second.lastTime) < lifeTime || t.second.moveProtected) {
            continue;
        }
        closeStream(t.second.fs);
    }
    _currentFdNum = count;
}

// }}}
// {{{ void Reader::closeStream()

void Reader::closeStream(std::fstream* fs) {
    if (fs != nullptr && fs->is_open()) {
        fs->close();
    }
}

// }}}
// {{{ std::fstream* Reader::openStream()

std::fstream* Reader::openStream(const std::string& file, const WatcherFileInfo& info, std::fstream* stream) {
    if (stream != nullptr && stream->is_open() && stream->good()) {
        return stream;
    }

    if (stream != nullptr && !stream->good() && stream->is_open()) {
        closeStream(stream);
    }

    if (stream == nullptr) {
        if (info.isBinary) {
            stream = new std::fstream(file, std::fstream::in | std::fstream::binary);
        } else {
            stream = new std::fstream(file, std::fstream::in);
        }
        LOG_INFO << "New open file:" << file;
    } else {
        if (info.isBinary) {
            stream->open(file, std::fstream::in | std::fstream::binary);
        } else {
            stream->open(file, std::fstream::in);
        }
        LOG_INFO << "Re open file:" << file;
    }
    _currentFdNum++;

    if (!stream->good()) {
        stream->close();
    }
    if (!stream->is_open()) {
        if (stream != nullptr) {
            delete stream;
        }
        stream = nullptr;
    }
    return stream;
}

// }}}

}
