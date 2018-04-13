### 由 0.4.2.1 版本升级到 1.0.x 方法

首先确认旧节点所有传输的配置路劲

cat agent-10023.offset|awk -F `echo -e '\x1a'` '{print $1}'    |sort -nr |uniq

转化 offset 为升级的中间文本

cat agent-10023.offset|awk -F `echo -e '\x1a'` '{print $3"\t"$1"\t"$2}' > tmp_agent_offset

如果修改传输路劲可以修改编辑

:%s/\/data1\/www\//\/usr\/local\/adinf\/logs\//g
:%s/\/data1\/run\//\/usr\/local\/adinf\/logs\//g

将新节点的 agent.offset  转化为中间升级文本 

./aidt-cli -o dumpoffset -f agent.offset -d ./

合并新旧的 offset 中间文本

cat tmp_agent\_offset > dump\_offsets

通过中间文本生成最终的 agent.offset

./aidt-cli  -o dynamicoffset  -f dump_offsets -d ./
