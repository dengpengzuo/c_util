# c_util

从redis\twemcach\libevent中抽取的常用的C工具库。包含有内存分配、网络事件。
欢迎各位大牛们的意见或者代码 :), 请邮至 mailto:cqzuodp@163.com.

##  编译
    mkdir build && cd build && cmake .. && make

    ./bin/test_s > ./log/test_s.log 2>&1 < /dev/null &

##  下一步计划
    * 提供类似于netty的bytebuf的工具.
    * 提供一个基于socket event的io框架.
