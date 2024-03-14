// StopWait.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "Global.h"
#include "RdtSender.h"
#include "RdtReceiver.h"
#include "GBNRdtReceiver.h"
#include "GBNRdtSender.h"
#include "StopWaitRdtSender.h"
#include "StopWaitRdtReceiver.h"
#include "SRRdtReceiver.h"
#include "SRRdtSender.h"
#include "TCPRdtReceiver.h"
#include "TCPRdtSender.h"
#include<fstream>

#define _GBN 0;
#define _SR 0; 
#define _TCP 1;

int main(int argc, char* argv[])
{
	ofstream cout("D:\\study_materials\\computer_network\\experiment\\lab2\\code\\Windows_VS2017\\message.txt");
	freopen("D:\\study_materials\\computer_network\\experiment\\lab2\\code\\Windows_VS2017\\message.txt", "w", stdout);	//������ض���Ϊ��������ļ�out.txt��
#if _GBN
	//cout << "222" << endl;
	RdtSender* ps = new GBNRdtSender();
	RdtReceiver* pr = new GBNRdtReceiver();
	//cout << "333" << endl;
#elif _SR
	RdtSender* ps = new SRRdtSender();
	RdtReceiver* pr = new SRRdtReceiver();
#elif _TCP
	RdtSender* ps = new TCPRdtSender();
	RdtReceiver* pr = new TCPRdtReceiver();
#else
	RdtSender* ps = new StopWaitRdtSender();
	RdtReceiver* pr = new StopWaitRdtReceiver();
#endif

	//	pns->setRunMode(0);  //VERBOSģʽ
	pns->setRunMode(1);  //����ģʽ
	pns->init();
	pns->setRtdSender(ps);
	pns->setRtdReceiver(pr);
	pns->setInputFile("D:\\study_materials\\computer_network\\experiment\\lab2\\code\\Windows_VS2017\\input.txt");
	pns->setOutputFile("D:\\study_materials\\computer_network\\experiment\\lab2\\code\\Windows_VS2017\\output.txt");
	pns->start();

	delete ps;
	delete pr;
	delete pUtils;									//ָ��Ψһ�Ĺ�����ʵ����ֻ��main��������ǰdelete
	delete pns;										//ָ��Ψһ��ģ�����绷����ʵ����ֻ��main��������ǰdelete

	return 0;
}

