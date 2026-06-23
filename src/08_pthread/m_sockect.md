# 网络基础协议

## 1.1 协议

概念: 协议事先约定好；双方共同遵守一组 “数据传输/数据解析”规则
A->B # A 发送数据给 B
header: (0,0,0,0) -> 先发送数据头 -> 可以获取数据长度、用户地址、一些配置信息

body: (0,0,0,0) -> 再发送数据体

注意: 协议和具体平台无关。
常见的协议：
TCP 传输控制协议
UDP 用户数据报协议
HTTP 超文本传输协议
FTP 文件传输协议
IP 因特尔互联网协议
ARP 协议；通过已知道 IP，寻找主机MAC地址
RARP 通过MAC地址；确认ip

# OSI 七层模型

## 2.1 分层模型

## 1 物理层: 双绞线、光纤(传输介子)，将模拟信号转为数字信号。（属于硬件层面）

     通信介质、双绞线、光纤、调制解调器 modemn(模数转换和数模转换)

## 2 数据链路层: 数据校验；定义了网络传输的基本单位-帧

    ARP 协议，RARP 协议

## 3 网络层: 定义网络、两台机器之间的传输路径选择点到点的传输 (IP)

## 4 传输层: 阐述数据 TCP，UDP，端到端的传输

## 5 会话层: 通过传输层建立数据传输通道

## 6 表示层: 解码、翻译工作

## 7 应用层: 为客户提供各种应用服务:,email、fpt、ssh、http 服务

### 2.2 名词解释

点到点 -> A 传输数据到B经历过的字节(比如到各个路由器、网关各个节点)
端到端 -> A 电脑 到B电脑

## 2.3 TCP/IP 模型

TCP/IP ------------> ISO模型

网络接口层: (物理层、数据链路层)
网络层: (网络层)
传输层: (传输层)
应用层: (会话层、表示层、应用程)

## 2.4 传输过程

传输方: 层层打包
数据
应用层->数据 (传输的一些软件之类的、大小)
传输层->应用层->数据 (规定协议)
网络层->传输层->应用层->数据 (确定目标)
网络接口层->网络层->传输层->应用层->数据 (交给物理层面)

接收方: 层层解包
网络接口层->网络层->传输层->应用层->数据
网络层->传输层->应用层->数据
传输层->应用层->数据
应用层->数据
数据 (保存数据)

# 网络应用模式

## 3.1 C/S 模式

客户端/服务端
优点: 可以安装在本地；更多的缓存空间；协议选择灵活。
缺点: 客户端工具需要程序员开发，开发周期长；工作量大。
需要本地安装；对客户的隐私、安全有点影响。

## 3.2 B/S 模式

浏览器/web服务端
优点: 浏览器不用开发，开发周期相对来说比较小。
缺点: 协议只能选择 http，不能缓存大多数据；影响客户使用。

# 3、以太网帧格式

以太网帧格式；是包装在网络接口层(数据链路层)的协议

|目的地址(MAC地址,6字节)| 源地址(MAC地址,6字节) | 类型(2字节) | 数据(46-1500) 字节 | CRC(4字节) |

类型: 0800(2) -> IP 数据报(46-1500)
类型: 0806(2) -> ARP请求/应答(28) -> PAD(28)

ARP协议: 通过对方的IP地址获取MAC地址。
RPRA协议: 通过对方的MAC地址获取IP地址。

IP 段格式 8位一个字节，1为位表示 255个状态

UDP 数据格式
| 8字节(64位) 头部数据 | （数据） |

通过IP确定确定网络中唯一主机
通过端口确定唯一应用程序

IP + 端口 = 唯一主机中的唯一程序

TCP 数据格式流
ACK 请求建立链接
SYN 请求链接
FIN 4次挥手（关闭链接）

# 4、Socket 网络编程

sfd: 发送端 -> cfd: 接收端
cfd: 发送端 -> sfd: 接收端

## 4.1 Sokect 编程预备知识

网络字节序:
大端: 低位地址存放高位数据；高位地址存放低位数据。 (网络字节序)
小端: 低位地址存放低位数据；高位地址存放高位数据。

htonl -> 本机转网络
ntohl -> 网络传本机

inet_pton(...) ip字符串转为int (网络字节序)
inet_ntop(...) int 转为ip 字符串

## 4.2 Sokect struct sockaddr

struct sockadrr
struct sockaddr_in
struct sockaddr_un
struct sockaaddr_in6

## 4.3 Sokect Api 库函数

domain: 协议版本
type: 协议类型
SOCK_STREAM = TCP 协议
SOCK_DGRAM 报式; UDP 协议
protocol
默认位 0 表示使用默认类型

返回值:
返回一个大于0的文件描述符
失败；返回-1；设置 perror 描述
int socket(int domain,int type,int protocol);

sockfd 通过 socked 创建的文件描述
addr: 本地服务器ip和port
struct sockaddr_in serv
serv.sin_family = AF_INTE;
serv.sin_port = htons(8888)
serv.sin_addr.s_addr = htonl(INADDR_ANY) // 获取任意一个
inte_pton(AF_INET,"127.0.0.1",&serv.sin_addr.s_addr)
addrlen: 变量类型的内存大小
成功返回 0

// 将 ip+port 与文件描述符绑定
int bind(int sockfd,const struct sockaddr\*addr,socklen_t addrlen)

// 将套接字由主动转为被动
sockfd 文件描述
backlog 同时请求链接最大数的个数
已链接队列
请求链接列队
int listen(int sockfd,int backlog)

获得一样链接；若当前没有链接就会阻塞等待
int accpet(int sockfd,struct sockaddr*addr,socklen_t*addrlen)
