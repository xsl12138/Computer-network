#include "stdafx.h"
#include "Global.h"
#include "GBNRdtSender.h"


GBNRdtSender::GBNRdtSender() :expectSequenceNumberSend(0), base(0), waitingState(false)
{
}


GBNRdtSender::~GBNRdtSender()
{
}



bool GBNRdtSender::getWaitingState() {
	return waitingState;
}




bool GBNRdtSender::send(const Message& message) {
	if (this->waitingState) { //发送方处于等待确认状态
		return false;
	}

	this->packetWaitingAck[expectSequenceNumberSend].acknum = -1; //忽略该字段
	this->packetWaitingAck[expectSequenceNumberSend].seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck[expectSequenceNumberSend].checksum = 0;
	memcpy(this->packetWaitingAck[expectSequenceNumberSend].payload, message.data, sizeof(message.data));
	this->packetWaitingAck[expectSequenceNumberSend].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[expectSequenceNumberSend]);
	pUtils->printPacket("发送方发送报文", this->packetWaitingAck[expectSequenceNumberSend]);
	//pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[expectSequenceNumberSend]);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
	//这里只需要考虑基序号的包裹是否超时即可
	if(base == expectSequenceNumberSend) pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck[expectSequenceNumberSend].seqnum);			//启动发送方定时器
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[expectSequenceNumberSend]);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
	printf("当前窗口为:[%d,%d]\n", base, base + GBN_Winsize - 1);
	//更新下一个发送序号
	//if(expectSequenceNumberSend < GBN_seqnum) expectSequenceNumberSend += 1;
	expectSequenceNumberSend = (expectSequenceNumberSend + 1) % GBN_seqnum;
	if((expectSequenceNumberSend - base + GBN_seqnum) % GBN_seqnum == GBN_Winsize)
		this->waitingState = true;		//发送完窗口内所有数据之后，才能将waitingState置为true																			//进入等待状态
	return true;
}

void GBNRdtSender::receive(const Packet& ackPkt) {
	//if (this->waitingState == true) {//如果发送方处于等待ack的状态，作如下处理；否则什么都不做
		//检查校验和是否正确
		int checkSum = pUtils->calculateCheckSum(ackPkt);
		//printf("\n成功进入GBNRdtSender:;receive函数！！！！！！！！！！！！！！！\n");
		//pUtils->printPacket("数据如下", ackPkt);
		//如果校验和正确，并且确认序号=发送方已发送并等待确认的数据包序号
		if (checkSum == ackPkt.checksum && ackPkt.acknum != this->packetWaitingAck[this->base].seqnum - 1) {
			//只考虑接收到正确ACK的情况即可，因为滑动窗口的移动只与接收到正确ACK有关
			int lastbase = this->base;
			this->base = (ackPkt.acknum + 1) % GBN_seqnum;
			this->waitingState = false;
			pUtils->printPacket("发送方收到了正确确认！", ackPkt);
			printf("GBN滑动窗口变化为：[");
			int j = this->base;
			for (int i = 0; i < GBN_Winsize - 1; i++) {
				printf("%d，", j);
				j = (j + 1) % GBN_seqnum;
			}
			printf("%d]\n", j);
			if (this->base == this->expectSequenceNumberSend) {	//该窗口分组已经全部接收到了
				pns->stopTimer(SENDER, this->packetWaitingAck[lastbase].seqnum);		//关闭定时器
			}
			else {
				pns->stopTimer(SENDER, this->packetWaitingAck[lastbase].seqnum);		//关闭定时器
				pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck[base].seqnum);
			}
		}
	//}
}
// 重传在以下函数中实现
void GBNRdtSender::timeoutHandler(int seqNum) {
	//唯一一个定时器,无需考虑seqNum
	printf("出现超时！重发滑动窗口中已发送的报文！\n");
	//pUtils->printPacket("发送方定时器时间到，重发上次发送的报文", this->packetWaitingAck);
	pns->stopTimer(SENDER, seqNum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//重新启动发送方定时器
	
	for (int i = 1, j = base; i <= (expectSequenceNumberSend - base + GBN_seqnum) % GBN_seqnum; i++) {
		pUtils->printPacket("发送方正在重新发送报文", this->packetWaitingAck[j]);
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[j]);			//重新发送数据包
		j = (j + 1) % GBN_seqnum;
	}
	//pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);			//重新发送数据包

}
