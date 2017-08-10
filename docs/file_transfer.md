# 文件目录数据传输

aidt 采用 inotify 机制来监听文件目录数据的变更，将变更的数据实时的传输到 kafka, 目前该模块支持普通文件、目录、以及递归目录的方式传输。并且也支持二进制安全的方式传输文件内容

### 配置

aidt 在文件目录传输配置上采用yaml 配置文件格式，在 `transfers` 段下配置一个或多个文件传输配置，配置项如下：

```yaml
transfers
    -
     path: /tmp/test1
     is_dir: true
     is_binary: false
     is_recursive: false
     format: '%s'
     topic_name: test
    -
     path: /tmp/test2
     is_dir: true
     is_binary: false
     is_recursive: false
     format: '%s'
     topic_name: test
```

如上例所示配置了传输 `/tmp/test1` 和 `/tmp/test2` 两个目录下所有文件内容到 topic 为 `test` 的kafka 集群

#### 配置项格式

| Property | Range | Default | Desc |
| --  | -- | -- | -- |
| path | string |  None | 要监听传输的文件或目录 | 
| is_dir | true/false | false | 监听的路劲是否是目录 |
| is_recursive | true/false | false | 是否递归目录方式读取数据，只有在监听路劲为目录时生效 |
| is_binary | true/false | false | 传输的数据是否按照二进制方式传输，默认是按行传输 |
| format | string | '%s' | 自定义格式化消息, 只有在传输普通行消息时生效 |
| topic_name | string | None | 消息传输的 topic |

### 重新热加载配置

aidt 支持热修改传输配置，通过发送 `-USR1` 信号进行配置 reload, 在重新加载时一定要检查配置文件语法， 检查语法的方式：

```
./aidt -c ../config/system.ini -t
```
