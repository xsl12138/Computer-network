#pragma once
#include "winsock2.h"
#include <stdio.h>
#include <iostream>
#include <fstream>	//读写文件用
using namespace std;

#pragma comment(lib,"ws2_32.lib")

#define setserver //1-1
#define output		//2-2
#define response //1-4  2-1  2-3  2-4
#define exception //2-5

void main() {
	WSADATA wsaData;
	/*
		select()机制中提供的fd_set的数据结构，实际上是long类型的数组
		每一个数组元素都能与一打开的文件句柄（不管是socket句柄，还是其他文件或命名管道或设备句柄）建立联系，建立联系的工作由程序员完成.//
		当调用select()时，由内核根据IO状态修改fd_set的内容，由此来通知执行了select()的进程哪个socket或文件句柄发生了可读或可写事件。
	*/
	fd_set rfds;				//用于检查socket是否有数据到来的的文件描述符，用于socket非阻塞模式下等待网络事件通知（有数据到来）
	fd_set wfds;				//用于检查socket是否可以发送的文件描述符，用于socket非阻塞模式下等待网络事件通知（可以发送数据）
	bool first_connetion = true;//是否为用户的第一次请求

	//1-1
#ifdef setserver
	cout << "请输入监听地址（四个数字，中间以空格隔开）：" << endl;
	int p1, p2, p3, p4;
	cin >> p1 >> p2 >> p3 >> p4;
	while (1) {
		if (p1 <= 255 && p1 >= 0 && p2 <= 255 && p2 >= 0 && p3 <= 255 && p3 >= 0 && p4 <= 255 && p4 >= 0) {
			cout << "格式正确！" << endl;
			break;
		}
		cout << "格式错误！请重新输入：";
		cin >> p1 >> p2 >> p3 >> p4;
	}
	int listen_addr = p1 << 24 + p2 << 16 + p3 << 8 + p4;	//设置的监听地址
	int port_number;	//设置的端口号
	cout << "请输入监听端口号：" << endl;
	cin >> port_number;
	while (1) {
		if (port_number <= 65535 && port_number >= 0) {
			cout << "格式正确！" << endl;
			break;
		}
		cout << "格式错误！请重新输入";
		cin >> port_number;
	}
	string s;
	cout << "请输入主目录：" << endl;
	cin >> s;
	char catalog[256] = { '\0' };
	for (int i = 0; i < s.length(); i++) {
		catalog[i] = s[i];
	}
	catalog[s.length()] = '\0';		//这里将主目录转换成字符数组形式是为了方便后续打开文件的操作
#endif

	//初始化Winsock环境（固定）
	int nRc = WSAStartup(0x0202, &wsaData);

	if (nRc) {
		printf("Winsock  startup failed with error!\n");
	}

	if (wsaData.wVersion != 0x0202) {
		printf("Winsock version is not correct!\n");
	}

	printf("Winsock  startup Ok!\n");
	//初始化结束

	SOCKET srvSocket;			//监听socket
	sockaddr_in addr, clientAddr;//服务器地址和客户端地址
	SOCKET sessionSocket;		//会话socket，负责和client进程通信（服务器实际有多个socket，对每个客户端，都会创建一个绘画socket）
	int addrLen;				//ip地址长度
	//create socket（固定）
	srvSocket = socket(AF_INET, SOCK_STREAM, 0);//第二个参数指定使用TCP
	if (srvSocket != INVALID_SOCKET)
		printf("Socket create Ok!\n");
	//创建监听Socket结束

	//设置服务器的端口和地址
	//1-1
#ifdef setserver
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port_number);//设置设定的端口号
	addr.sin_addr.S_un.S_addr = htonl(listen_addr);//设置设定的监听地址
	cout << "监听地址、端口号、主目录设置完毕！" << endl;
#else
	addr.sin_family = AF_INET;
	addr.sin_port = htons(5050);	//端口号5050
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//设置监听地址，这里是主机上任意一块网卡的地址，即INADDR_ANY
	cout << "监听地址、端口号、主目录设置完毕！" << endl;
#endif
	//设置服务器端口和地址结束

	//binding（绑定端口号和监听socket）（固定）
	int rtn = bind(srvSocket, (LPSOCKADDR)&addr, sizeof(addr));
	//2-5 支持一定的异常处理能力
#ifdef exception
	if (WSAGetLastError() == WSAEADDRINUSE) {	//检测端口号是否被占用（来源：csdn）
		printf("端口已被占用！\n");
		return;
	}
#endif
	if (rtn != SOCKET_ERROR)
		printf("Socket bind Ok!\n");


	//1-2（监听）
	rtn = listen(srvSocket, 5);
	if (rtn != SOCKET_ERROR)
		printf("Socket listen Ok!\n");

	clientAddr.sin_family = AF_INET;
	addrLen = sizeof(clientAddr);

	char recvBuf[4096];			//设置接收缓冲区

	u_long blockMode = 1;//将srvSock设为非阻塞模式以监听客户连接请求
	/*
		调用ioctlsocket，将srvSocket改为非阻塞模式，改成反复检查fd_set元素的状态
		看每个元素对应的句柄是否可读或可写。也就是把阻塞模式改为轮询模式。
	*/
	if ((rtn = ioctlsocket(srvSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) {
		//FIONBIO：允许或禁止套接口s的非阻塞模式。
		cout << "ioctlsocket() failed with error!\n";
		return;
	}
	cout << "ioctlsocket() for server socket ok!	Waiting for client connection and data\n";

	//清空read,write描述符，对rfds和wfds进行了初始化，必须用FD_ZERO先清空，下面才能FD_SET
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	//设置等待客户连接请求
	FD_SET(srvSocket, &rfds);

	while (true) {
		//清空read,write描述符
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		//设置等待客户连接请求
		FD_SET(srvSocket, &rfds);/*
									将srvSocket加入rfds数组
									即：当客户端连接请求到来时，rfds数组里srvSocket对应的的状态为可读
									因此这条语句的作用就是：设置等待客户连接请求
								 */
		//如果first_connetion为true，sessionSocket还没有产生
		if (!first_connetion) {
			if (sessionSocket != INVALID_SOCKET) {		//不加这个判断，无论什么请求到来都会导致服务器直接关闭
				//设置等待会话SOKCET可接受数据或可发送数据
				FD_SET(sessionSocket, &rfds);
				FD_SET(sessionSocket, &wfds);
				/*
					如果sessionSocket是有效的，将sessionSocket加入rfds数组和wfds数组
					当客户端发送数据过来时，rfds数组里sessionSocket的对应的状态为可读；
					当可以发送数据到客户端时，wfds数组里sessionSocket的对应的状态为可写。
					因此上面二条语句的作用就是：设置等待会话SOKCET可接受数据或可发送数据
				*/
			}
		}

		/*
			select工作原理：传入要监听的文件描述符集合（可读、可写，有异常）开始监听，select处于阻塞状态。
			当有可读写事件发生或设置的等待时间timeout到了就会返回，返回之前自动去除集合中无事件发生的文件描述符，返回时传出有事件发生的文件描述符集合。
			但select传出的集合并没有告诉用户集合中包括哪几个就绪的文件描述符，需要用户后续进行遍历操作(通过FD_ISSET检查每个文件描述符的状态)。
		*/
		//开始等待（等待rfds里是否有输入事件，wfds里是否有可写事件，返回总共可以读或写的文件描述符个数）
		int nTotal = select(0, &rfds, &wfds, NULL, NULL);

		//如果srvSock收到连接请求，接受客户连接请求
		if (FD_ISSET(srvSocket, &rfds)) {
			nTotal--;//因为客户端请求到来也算可读事件，因此-1，剩下的就是真正有可读事件的句柄个数（即有多少个socket收到了数据）

			//产生会话SOCKET 1-3//
			sessionSocket = accept(srvSocket, (LPSOCKADDR)&clientAddr, &addrLen);
			if (sessionSocket != INVALID_SOCKET)
				printf("Socket listen one client request!\n");
#ifdef output
			cout << "请求的IP地址为：" << clientAddr.sin_addr.S_un.S_addr << endl;
			cout << "请求的端口号为：" << clientAddr.sin_port << endl;
#endif
			//把会话SOCKET设为非阻塞模式
			if ((rtn = ioctlsocket(sessionSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) { //FIONBIO：允许或禁止套接口s的非阻塞模式。
				cout << "ioctlsocket() failed with error!\n";
				return;
			}
			cout << "ioctlsocket() for session socket ok!	Waiting for client connection and data\n";

			//设置等待会话SOKCET可接受数据或可发送数据
			FD_SET(sessionSocket, &rfds);
			FD_SET(sessionSocket, &wfds);

			first_connetion = false;//已经产生了会话socket，因此设为false

		}

		//检查会话SOCKET是否有数据到来
		if (nTotal >= 0) {
			//cout << "nTotal:" << nTotal << endl;
			//如果会话SOCKET有数据到来，则接受客户的数据
			if (FD_ISSET(sessionSocket, &rfds)) {
				cout << "有数据产生！" << endl;
				//receiving data from client
				memset(recvBuf, '\0', 4096);
				rtn = recv(sessionSocket, recvBuf, 2560, 0);
				if (rtn > 0) {
					//printf("Received %d bytes from client: %s\n", rtn, recvBuf);
#ifdef output
					printf("请求命令行为：%s\n共%d个字节", recvBuf, rtn);
#endif
#ifdef response
					char method[1000] = { '\0' }, destination[1000] = { '\0' };
					//method保存请求方法，如POST，GET等；destination保存URL，与前面的catalog组合可得文件路径
					int i = 0;
					for (i = 0; recvBuf[i] != ' '; i++) {
						method[i] = recvBuf[i];
					}
					method[i] = '\0';
					i += 1;
					int cur = i;
					for (; recvBuf[i] != ' '; i++) {
						destination[i - cur] = recvBuf[i];
					}
					destination[i - cur] = '\0';
					//char* filename;	//文件路径（将catalog和destination结合即可）
					//filename = strcat(catalog, destination);
					char filename[265];
					int length = 0;
					for (length = 0; catalog[length] != '\0'; length++) {
						filename[length] = catalog[length];
					}
					cur = length;
					for (; destination[length - cur] != '\0'; length++) {
						filename[length] = destination[length - cur];
					}
					filename[length] = '\0';
					if (method[0] == 'G') { //是GET方法
						FILE* fp;
						fp = fopen(filename, "rb");
						if (!fp) {	//读文件失败（无法成功定位文件）
							cout << filename << "文件打开失败" << endl;
							cout << "返回404 not found" << endl;	//2-3 输出对请求的处理结果
							string message = "http/1.1 404 NOTFOUND\r\n";	//格式：版本号 状态码 原因短语		//注意报文格式，格式正确才能被客户端相应
							rtn = send(sessionSocket, message.c_str(), message.length(), 0);
							message = "Content-Type: text/html\r\n";
							rtn = send(sessionSocket, message.c_str(), message.length(), 0);
							message = "\r\n";
							rtn = send(sessionSocket, message.c_str(), message.length(), 0);
							message = "<html><body><h1>404</h1><p> FILE Not Found!</p></body></html>";
							rtn = send(sessionSocket, message.c_str(), message.length(), 0);
							if (rtn == SOCKET_ERROR) std::cout << "服务器响应报文发送失败！" << endl;
							else cout << "响应报文已发送！" << endl;
						}
						else {//读文件成功
							cout << filename << "文件打开成功" << endl;
							cout << "向用户返回200 OK" << endl;//输出对请求的处理结果
							string message;
							message = "http/1.1 200 OK\r\n";
							rtn = send(sessionSocket, message.c_str(), message.length(), 0);
							int messagelength;
							//获取数据总长度
							fseek(fp, 0, SEEK_END);	//fseek设置光标位置（到末尾，即SEEK_END）
							messagelength = ftell(fp);	//ftell读取光标在文件中的位置
							rewind(fp);		//让文件指针（光标）回到文件起始位置
							//根据数据长度分配内存buffer
							char* Buffer = (char*)malloc((messagelength + 1) * sizeof(char));
							memset(Buffer, 0, messagelength + 1);
							//将数据读入buffer
							fread(Buffer, 1, messagelength, fp);
							fclose(fp);

							if (filename[length - 2] == 'n')//png
							{
								message = "Content-Type: application/octet-stream\r\n";
								rtn = send(sessionSocket, message.c_str(), message.length(), 0);
								message = "\r\n";
								rtn = send(sessionSocket, message.c_str(), message.length(), 0);
								rtn = send(sessionSocket, Buffer, messagelength, 0);
								if (rtn == SOCKET_ERROR) std::cout << "服务器响应报文发送失败！" << endl;
								else cout << "数据报文已发送！" << endl;
							}
							else if (filename[length - 2] == 'p')//jpg
							{
								message = "Content-Type: image/jpeg\r\n";
								rtn = send(sessionSocket, message.c_str(), message.length(), 0);
								message = "\r\n";
								rtn = send(sessionSocket, message.c_str(), message.length(), 0);
								rtn = send(sessionSocket, Buffer, messagelength, 0);
								if (rtn == SOCKET_ERROR) std::cout << "服务器响应报文发送失败！" << endl;
								else cout << "数据报文已发送！" << endl;
							}
							else//txt/html
							{
								message = "Content-Type: text/html;charset=utf-8\r\n";
								rtn = send(sessionSocket, message.c_str(), message.length(), 0);
								message = "\r\n";
								rtn = send(sessionSocket, message.c_str(), message.length(), 0);
								rtn = send(sessionSocket, Buffer, messagelength, 0);
								if (rtn == SOCKET_ERROR) std::cout << "服务器响应报文发送失败！" << endl;
								else cout << "数据报文已发送！" << endl;
							}
							fclose(fp);
						}
					}

#endif
				}
				else {//否则是收到了客户端断开连接的请求，也算可读事件。但这种情况下rtn=0
					printf("Client leaving ...\n");
					closesocket(sessionSocket);  //既然client离开了，就关闭sessionSocket
					nTotal--;					//因为客户端离开也属于可读事件，所以需要-1
					sessionSocket = INVALID_SOCKET;
				}

			}
		}
	}

}