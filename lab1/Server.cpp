#pragma once
#include "winsock2.h"
#include <stdio.h>
#include <iostream>
#include <fstream>	//��д�ļ���
using namespace std;

#pragma comment(lib,"ws2_32.lib")

#define setserver //1-1
#define output		//2-2
#define response //1-4  2-1  2-3  2-4
#define exception //2-5

void main() {
	WSADATA wsaData;
	/*
		select()�������ṩ��fd_set�����ݽṹ��ʵ������long���͵�����
		ÿһ������Ԫ�ض�����һ�򿪵��ļ������������socket��������������ļ��������ܵ����豸�����������ϵ��������ϵ�Ĺ����ɳ���Ա���.//
		������select()ʱ�����ں˸���IO״̬�޸�fd_set�����ݣ��ɴ���ִ֪ͨ����select()�Ľ����ĸ�socket���ļ���������˿ɶ����д�¼���
	*/
	fd_set rfds;				//���ڼ��socket�Ƿ������ݵ����ĵ��ļ�������������socket������ģʽ�µȴ������¼�֪ͨ�������ݵ�����
	fd_set wfds;				//���ڼ��socket�Ƿ���Է��͵��ļ�������������socket������ģʽ�µȴ������¼�֪ͨ�����Է������ݣ�
	bool first_connetion = true;//�Ƿ�Ϊ�û��ĵ�һ������

	//1-1
#ifdef setserver
	cout << "�����������ַ���ĸ����֣��м��Կո��������" << endl;
	int p1, p2, p3, p4;
	cin >> p1 >> p2 >> p3 >> p4;
	while (1) {
		if (p1 <= 255 && p1 >= 0 && p2 <= 255 && p2 >= 0 && p3 <= 255 && p3 >= 0 && p4 <= 255 && p4 >= 0) {
			cout << "��ʽ��ȷ��" << endl;
			break;
		}
		cout << "��ʽ�������������룺";
		cin >> p1 >> p2 >> p3 >> p4;
	}
	int listen_addr = p1 << 24 + p2 << 16 + p3 << 8 + p4;	//���õļ�����ַ
	int port_number;	//���õĶ˿ں�
	cout << "����������˿ںţ�" << endl;
	cin >> port_number;
	while (1) {
		if (port_number <= 65535 && port_number >= 0) {
			cout << "��ʽ��ȷ��" << endl;
			break;
		}
		cout << "��ʽ��������������";
		cin >> port_number;
	}
	string s;
	cout << "��������Ŀ¼��" << endl;
	cin >> s;
	char catalog[256] = { '\0' };
	for (int i = 0; i < s.length(); i++) {
		catalog[i] = s[i];
	}
	catalog[s.length()] = '\0';		//���ｫ��Ŀ¼ת�����ַ�������ʽ��Ϊ�˷���������ļ��Ĳ���
#endif

	//��ʼ��Winsock�������̶���
	int nRc = WSAStartup(0x0202, &wsaData);

	if (nRc) {
		printf("Winsock  startup failed with error!\n");
	}

	if (wsaData.wVersion != 0x0202) {
		printf("Winsock version is not correct!\n");
	}

	printf("Winsock  startup Ok!\n");
	//��ʼ������

	SOCKET srvSocket;			//����socket
	sockaddr_in addr, clientAddr;//��������ַ�Ϳͻ��˵�ַ
	SOCKET sessionSocket;		//�Ựsocket�������client����ͨ�ţ�������ʵ���ж��socket����ÿ���ͻ��ˣ����ᴴ��һ���滭socket��
	int addrLen;				//ip��ַ����
	//create socket���̶���
	srvSocket = socket(AF_INET, SOCK_STREAM, 0);//�ڶ�������ָ��ʹ��TCP
	if (srvSocket != INVALID_SOCKET)
		printf("Socket create Ok!\n");
	//��������Socket����

	//���÷������Ķ˿ں͵�ַ
	//1-1
#ifdef setserver
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port_number);//�����趨�Ķ˿ں�
	addr.sin_addr.S_un.S_addr = htonl(listen_addr);//�����趨�ļ�����ַ
	cout << "������ַ���˿ںš���Ŀ¼������ϣ�" << endl;
#else
	addr.sin_family = AF_INET;
	addr.sin_port = htons(5050);	//�˿ں�5050
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//���ü�����ַ������������������һ�������ĵ�ַ����INADDR_ANY
	cout << "������ַ���˿ںš���Ŀ¼������ϣ�" << endl;
#endif
	//���÷������˿ں͵�ַ����

	//binding���󶨶˿ںźͼ���socket�����̶���
	int rtn = bind(srvSocket, (LPSOCKADDR)&addr, sizeof(addr));
	//2-5 ֧��һ�����쳣��������
#ifdef exception
	if (WSAGetLastError() == WSAEADDRINUSE) {	//���˿ں��Ƿ�ռ�ã���Դ��csdn��
		printf("�˿��ѱ�ռ�ã�\n");
		return;
	}
#endif
	if (rtn != SOCKET_ERROR)
		printf("Socket bind Ok!\n");


	//1-2��������
	rtn = listen(srvSocket, 5);
	if (rtn != SOCKET_ERROR)
		printf("Socket listen Ok!\n");

	clientAddr.sin_family = AF_INET;
	addrLen = sizeof(clientAddr);

	char recvBuf[4096];			//���ý��ջ�����

	u_long blockMode = 1;//��srvSock��Ϊ������ģʽ�Լ����ͻ���������
	/*
		����ioctlsocket����srvSocket��Ϊ������ģʽ���ĳɷ������fd_setԪ�ص�״̬
		��ÿ��Ԫ�ض�Ӧ�ľ���Ƿ�ɶ����д��Ҳ���ǰ�����ģʽ��Ϊ��ѯģʽ��
	*/
	if ((rtn = ioctlsocket(srvSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) {
		//FIONBIO��������ֹ�׽ӿ�s�ķ�����ģʽ��
		cout << "ioctlsocket() failed with error!\n";
		return;
	}
	cout << "ioctlsocket() for server socket ok!	Waiting for client connection and data\n";

	//���read,write����������rfds��wfds�����˳�ʼ����������FD_ZERO����գ��������FD_SET
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	//���õȴ��ͻ���������
	FD_SET(srvSocket, &rfds);

	while (true) {
		//���read,write������
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		//���õȴ��ͻ���������
		FD_SET(srvSocket, &rfds);/*
									��srvSocket����rfds����
									�������ͻ�������������ʱ��rfds������srvSocket��Ӧ�ĵ�״̬Ϊ�ɶ�
									��������������þ��ǣ����õȴ��ͻ���������
								 */
		//���first_connetionΪtrue��sessionSocket��û�в���
		if (!first_connetion) {
			if (sessionSocket != INVALID_SOCKET) {		//��������жϣ�����ʲô���������ᵼ�·�����ֱ�ӹر�
				//���õȴ��ỰSOKCET�ɽ������ݻ�ɷ�������
				FD_SET(sessionSocket, &rfds);
				FD_SET(sessionSocket, &wfds);
				/*
					���sessionSocket����Ч�ģ���sessionSocket����rfds�����wfds����
					���ͻ��˷������ݹ���ʱ��rfds������sessionSocket�Ķ�Ӧ��״̬Ϊ�ɶ���
					�����Է������ݵ��ͻ���ʱ��wfds������sessionSocket�Ķ�Ӧ��״̬Ϊ��д��
					�����������������þ��ǣ����õȴ��ỰSOKCET�ɽ������ݻ�ɷ�������
				*/
			}
		}

		/*
			select����ԭ������Ҫ�������ļ����������ϣ��ɶ�����д�����쳣����ʼ������select��������״̬��
			���пɶ�д�¼����������õĵȴ�ʱ��timeout���˾ͻ᷵�أ�����֮ǰ�Զ�ȥ�����������¼��������ļ�������������ʱ�������¼��������ļ����������ϡ�
			��select�����ļ��ϲ�û�и����û������а����ļ����������ļ�����������Ҫ�û��������б�������(ͨ��FD_ISSET���ÿ���ļ���������״̬)��
		*/
		//��ʼ�ȴ����ȴ�rfds���Ƿ��������¼���wfds���Ƿ��п�д�¼��������ܹ����Զ���д���ļ�������������
		int nTotal = select(0, &rfds, &wfds, NULL, NULL);

		//���srvSock�յ��������󣬽��ܿͻ���������
		if (FD_ISSET(srvSocket, &rfds)) {
			nTotal--;//��Ϊ�ͻ���������Ҳ��ɶ��¼������-1��ʣ�µľ��������пɶ��¼��ľ�����������ж��ٸ�socket�յ������ݣ�

			//�����ỰSOCKET 1-3//
			sessionSocket = accept(srvSocket, (LPSOCKADDR)&clientAddr, &addrLen);
			if (sessionSocket != INVALID_SOCKET)
				printf("Socket listen one client request!\n");
#ifdef output
			cout << "�����IP��ַΪ��" << clientAddr.sin_addr.S_un.S_addr << endl;
			cout << "����Ķ˿ں�Ϊ��" << clientAddr.sin_port << endl;
#endif
			//�ѻỰSOCKET��Ϊ������ģʽ
			if ((rtn = ioctlsocket(sessionSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) { //FIONBIO��������ֹ�׽ӿ�s�ķ�����ģʽ��
				cout << "ioctlsocket() failed with error!\n";
				return;
			}
			cout << "ioctlsocket() for session socket ok!	Waiting for client connection and data\n";

			//���õȴ��ỰSOKCET�ɽ������ݻ�ɷ�������
			FD_SET(sessionSocket, &rfds);
			FD_SET(sessionSocket, &wfds);

			first_connetion = false;//�Ѿ������˻Ựsocket�������Ϊfalse

		}

		//���ỰSOCKET�Ƿ������ݵ���
		if (nTotal >= 0) {
			//cout << "nTotal:" << nTotal << endl;
			//����ỰSOCKET�����ݵ���������ܿͻ�������
			if (FD_ISSET(sessionSocket, &rfds)) {
				cout << "�����ݲ�����" << endl;
				//receiving data from client
				memset(recvBuf, '\0', 4096);
				rtn = recv(sessionSocket, recvBuf, 2560, 0);
				if (rtn > 0) {
					//printf("Received %d bytes from client: %s\n", rtn, recvBuf);
#ifdef output
					printf("����������Ϊ��%s\n��%d���ֽ�", recvBuf, rtn);
#endif
#ifdef response
					char method[1000] = { '\0' }, destination[1000] = { '\0' };
					//method�������󷽷�����POST��GET�ȣ�destination����URL����ǰ���catalog��Ͽɵ��ļ�·��
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
					//char* filename;	//�ļ�·������catalog��destination��ϼ��ɣ�
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
					if (method[0] == 'G') { //��GET����
						FILE* fp;
						fp = fopen(filename, "rb");
						if (!fp) {	//���ļ�ʧ�ܣ��޷��ɹ���λ�ļ���
							cout << filename << "�ļ���ʧ��" << endl;
							cout << "����404 not found" << endl;	//2-3 ���������Ĵ�����
							string message = "http/1.1 404 NOTFOUND\r\n";	//��ʽ���汾�� ״̬�� ԭ�����		//ע�ⱨ�ĸ�ʽ����ʽ��ȷ���ܱ��ͻ�����Ӧ
							rtn = send(sessionSocket, message.c_str(), message.length(), 0);
							message = "Content-Type: text/html\r\n";
							rtn = send(sessionSocket, message.c_str(), message.length(), 0);
							message = "\r\n";
							rtn = send(sessionSocket, message.c_str(), message.length(), 0);
							message = "<html><body><h1>404</h1><p> FILE Not Found!</p></body></html>";
							rtn = send(sessionSocket, message.c_str(), message.length(), 0);
							if (rtn == SOCKET_ERROR) std::cout << "��������Ӧ���ķ���ʧ�ܣ�" << endl;
							else cout << "��Ӧ�����ѷ��ͣ�" << endl;
						}
						else {//���ļ��ɹ�
							cout << filename << "�ļ��򿪳ɹ�" << endl;
							cout << "���û�����200 OK" << endl;//���������Ĵ�����
							string message;
							message = "http/1.1 200 OK\r\n";
							rtn = send(sessionSocket, message.c_str(), message.length(), 0);
							int messagelength;
							//��ȡ�����ܳ���
							fseek(fp, 0, SEEK_END);	//fseek���ù��λ�ã���ĩβ����SEEK_END��
							messagelength = ftell(fp);	//ftell��ȡ������ļ��е�λ��
							rewind(fp);		//���ļ�ָ�루��꣩�ص��ļ���ʼλ��
							//�������ݳ��ȷ����ڴ�buffer
							char* Buffer = (char*)malloc((messagelength + 1) * sizeof(char));
							memset(Buffer, 0, messagelength + 1);
							//�����ݶ���buffer
							fread(Buffer, 1, messagelength, fp);
							fclose(fp);

							if (filename[length - 2] == 'n')//png
							{
								message = "Content-Type: application/octet-stream\r\n";
								rtn = send(sessionSocket, message.c_str(), message.length(), 0);
								message = "\r\n";
								rtn = send(sessionSocket, message.c_str(), message.length(), 0);
								rtn = send(sessionSocket, Buffer, messagelength, 0);
								if (rtn == SOCKET_ERROR) std::cout << "��������Ӧ���ķ���ʧ�ܣ�" << endl;
								else cout << "���ݱ����ѷ��ͣ�" << endl;
							}
							else if (filename[length - 2] == 'p')//jpg
							{
								message = "Content-Type: image/jpeg\r\n";
								rtn = send(sessionSocket, message.c_str(), message.length(), 0);
								message = "\r\n";
								rtn = send(sessionSocket, message.c_str(), message.length(), 0);
								rtn = send(sessionSocket, Buffer, messagelength, 0);
								if (rtn == SOCKET_ERROR) std::cout << "��������Ӧ���ķ���ʧ�ܣ�" << endl;
								else cout << "���ݱ����ѷ��ͣ�" << endl;
							}
							else//txt/html
							{
								message = "Content-Type: text/html;charset=utf-8\r\n";
								rtn = send(sessionSocket, message.c_str(), message.length(), 0);
								message = "\r\n";
								rtn = send(sessionSocket, message.c_str(), message.length(), 0);
								rtn = send(sessionSocket, Buffer, messagelength, 0);
								if (rtn == SOCKET_ERROR) std::cout << "��������Ӧ���ķ���ʧ�ܣ�" << endl;
								else cout << "���ݱ����ѷ��ͣ�" << endl;
							}
							fclose(fp);
						}
					}

#endif
				}
				else {//�������յ��˿ͻ��˶Ͽ����ӵ�����Ҳ��ɶ��¼��������������rtn=0
					printf("Client leaving ...\n");
					closesocket(sessionSocket);  //��Ȼclient�뿪�ˣ��͹ر�sessionSocket
					nTotal--;					//��Ϊ�ͻ����뿪Ҳ���ڿɶ��¼���������Ҫ-1
					sessionSocket = INVALID_SOCKET;
				}

			}
		}
	}

}