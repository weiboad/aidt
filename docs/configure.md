Aidt Configuration
==================

### system

| Property | Default | Range  | Desc |
| --  | -- | -- |  -- |
| appid | 0 | 0-255  | 用来计算 baseid ，可忽略 | 
| macid | 0 | 0-255  | 用来计算 baseid ，可忽略 | 
| daemon | no | no/yes  | 是否将程序以守护进程方式运行 | 
| pidFile | ./aidt.pid | -  | 程序运行的 pid 文件路径 | 
| aidtConfig | ../conf/aidt.yml | -  | aidt 文件传输配置文件路径 | 

### logging

| Property | Default | Range  | Desc |
| --  | -- | -- |  -- |
| logsDir | logs/ | -  | 程序日志写入目录 | 
| logRollSize | 52428800 | - | 日志文件分卷大小 bytes | 
| logLevel | 1 | [1,2,3] | 1: LOG_TRACE 2:LOG_DEBUG 3:LOG_INFO| 
| isAsync | no | no/yes | 是否开启异步写日志的方式, 默认不开启会写入到 stdout 上 | 

### adserver

| Property | Default | Range  | Desc |
| --  | -- | -- |  -- |
| mc | yes | yes/no  | 是否开启 memcache 代理服务 | 
| http | yes | yes  | http 默认开启，请勿修改 | 
| head | no | no  | head 在 aidt 中没有用到该协议，启动无效 | 

### mc

| Property | Default | Range  | Desc |
| --  | -- | -- |  -- |
| host | 0.0.0.0 | ip  | 监听 mc server ip 地址 | 
| port | 40011 | port  | 监听 mc server port | 
| threadNum | 4 | >0  | mc server work 线程数 | 
| serverName | adinf-adserver | - | mc server name | 

### http

| Property | Default | Range  | Desc |
| --  | -- | -- |  -- |
| host | 0.0.0.0 | ip  | 监听 http server ip 地址 | 
| port | 40010 | port  | 监听 http server port | 
| timeout | 3 | >0  | http server 读写超时，单位秒 | 
| threadNum | 4 | >0  | http server work 线程数 | 
| serverName | adinf-adserver | - | http server name | 
| accessLogDir | logs/ | - | http server 访问日志目录路劲 | 
| accesslogRollSize | 52428800 | >0 | http server 访问日志分卷大小 | 
| defaultController | index | - | 请勿修改 | 
| defaultAction | index | - | 请勿修改 | 

### kafkap_in

| Property | Default | Range  | Desc |
| --  | -- | -- |  -- |
| topicNameIn | test | -  | 无效参数 | 
| brokerListIn | 127.0.0.1:9192 | -  | kafka 集群地址, 注意此处不是 zookeeper 地址 | 
| debugIn | none | -  | kafka 调试级别 | 
| queueLengthIn | 10000 | >0  | 代理缓存队列大小 | 

### message

| Property | Default | Range  | Desc |
| --  | -- | -- |  -- |
| sendError | error | -  | 如果发送失败，消息落地到本地文件的路劲  | 

### reader

| Property | Default | Range  | Desc |
| --  | -- | -- |  -- |
| maxLines | 1000 | -  | 单次事件读取最大行数 | 
| maxfd | 512 | -  | 最大打开文件个数 | 
| maxLineBytes | 1048576 | -  | 单行最大字节数，建议和kafka 最大消息保持一致 | 
| offsetFile | ./agent.offset | -  | 保存读取文件位置的文件路劲 | 
