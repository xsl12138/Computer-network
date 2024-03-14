// StopWait.cpp : 定义控制台应用程序的入口点。
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
	freopen("D:\\study_materials\\computer_network\\experiment\\lab2\\code\\Windows_VS2017\\message.txt", "w", stdout);	//将输出重定向为从输出到文件out.txt中
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

	//	pns->setRunMode(0);  //VERBOS模式
	pns->setRunMode(1);  //安静模式
	pns->init();
	pns->setRtdSender(ps);
	pns->setRtdReceiver(pr);
	pns->setInputFile("D:\\study_materials\\computer_network\\experiment\\lab2\\code\\Windows_VS2017\\input.txt");
	pns->setOutputFile("D:\\study_materials\\computer_network\\experiment\\lab2\\code\\Windows_VS2017\\output.txt");
	pns->start();

	delete ps;
	delete pr;
	delete pUtils;									//指向唯一的工具类实例，只在main函数结束前delete
	delete pns;										//指向唯一的模拟网络环境类实例，只在main函数结束前delete

	return 0;
}

