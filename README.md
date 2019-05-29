# networking

## 相关说明

OS: ubuntu 18.04  
apt安装mininet（采用源码安装一直报错  
作业题目：作业2  
如何下载本工程:   
虚拟机上安装git `sudo apt-get install git`  
用git clone 下载 `sudo git clone https://github.com/9kalikali/networking.git`  
tips: 建议直接su切到root用户下,就不用每次都输sudo了,看个人习惯  

## 主要文件
- arp.c, arpcache.c 负责处理arp请求和应答, arp缓存操作(查询和更新)
- icmp.c 负责icmp转发
- ip.c, ip_forwarding.c 负责ip地址查找和ip数据包转发
- router_topo.py 建立网络拓扑

## 实验流程
1. 编译路由器程序  
   + 进入到networking/router目录下, 输入`make`指令进行编译, 此时会报一些warning, 可以不用理会<img src="https://github.com/9kalikali/networking/blob/master/imgs/before_compile.png">  
   + 编译成功后, 目录下会多出一个叫`router`的可执行文件<img src="https://github.com/9kalikali/networking/blob/master/imgs/compiled.png">
2. 运行router_topo.py, 此时会自动进入mininet交互界面, 此时可以通过`nodes`和`net`命令检查下拓扑是否正确  
3. 执行`xterm r1`命令, 会自动新开启一个的终端, 可以理解为进入router1的系统, 在此窗口执行./router, 路由器启动完成  
4. 执行`xterm h1`, 与上一步相同会开起新终端进入host1, ping相应的地址
<img src="https://github.com/9kalikali/networking/blob/master/imgs/route_running.png">

## TODO
todo1. 接下来需要自行编写一个拓扑（参照router_topo.py）运行该拓扑并进行测试  
todo2. 截图,撰写报告

## 作业进度

- [x] 提交组队信息
- [x] 上传例程
- [x] 编写程序
- [x] 测试
- [x] 实验内容1
- [x] 实验内容2

- [x] 撰写报告
- [ ] 作业提交
