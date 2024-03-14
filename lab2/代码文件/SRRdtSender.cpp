#include "stdafx.h"
#include "Global.h"
#include "SRRdtSender.h"


SRRdtSender::SRRdtSender() :expectSequenceNumberSend(0), base(0), waitingState(false)
{
	memset(ACK, SR_seqnum, false);
}


SRRdtSender::~SRRdtSender()
{
}



bool SRRdtSender::getWaitingState() {
	return waitingState;
}




bool SRRdtSender::send(const Message& message) {
	if (this->waitingState) { //发送方处于等待确认状态
		return false;
	}
	ACK[expectSequenceNumberSend] = false;	//必须要写这个，不然在第一次发送结束后，ACK不知道为什么会被赋值
	this->packetWaitingAck[expectSequenceNumberSend].acknum = -1; //忽略该字段
	this->packetWaitingAck[expectSequenceNumberSend].seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck[expectSequenceNumberSend].checksum = 0;
	memcpy(this->packetWaitingAck[expectSequenceNumberSend].payload, message.data, sizeof(message.data));
	this->packetWaitingAck[expectSequenceNumberSend].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[expectSequenceNumberSend]);
	pUtils->printPacket("发送方发送报文", this->packetWaitingAck[expectSequenceNumberSend]);
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[expectSequenceNumberSend]);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方

	//SR协议需要对每一个报文都计时
	pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck[expectSequenceNumberSend].seqnum);			//启动发送方定时器
	printf("当前窗口为:[%d,%d]\n", base, (base + SR_Winsize - 1) % SR_Winsize);
	expectSequenceNumberSend = (expectSequenceNumberSend + 1) % SR_seqnum;

	if ((expectSequenceNumberSend - this->base + SR_seqnum) % SR_seqnum == SR_Winsize)
		this->waitingState = true;		//发送完窗口内所有数据之后，才能将waitingState置为true																			//进入等待状态
	return true;
}

void SRRdtSender::receive(const Packet& ackPkt) {
	//if (this->waitingState == true) {//如果发送方处于等待ack的状态，作如下处理；否则什么都不做
		//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	if (ackPkt.acknum < 0 || ackPkt.acknum > SR_seqnum - 1) {  //报文发生损坏，避免下面数组越界
		return;
	}
	//还未收到该分组的ack；校验和正确；acknum在[base,base + SR_Winszie - 1]之间
	if (!ACK[ackPkt.acknum] && checkSum == ackPkt.checksum && ((ackPkt.acknum >= this->base && ackPkt.acknum <= this->base + SR_Winsize - 1) || (ackPkt.acknum + 1 + SR_seqnum - this->base <= SR_Winsize))) {
		pns->stopTimer(SENDER, ackPkt.acknum);		//关闭定时器
		waitingState = false;
		ACK[ackPkt.acknum] = true;
		pUtils->printPacket("发送方正确收到确认！", ackPkt);
		int flag = 1;	//判断当前窗口内是否所有数据包都收到了ACK
		if (ackPkt.acknum == this->base) {	//滑动窗口（至base后面第一个未收到ack消息的报文处）
			for (int i = 0, j = (this->base + 1) % SR_seqnum; i < (expectSequenceNumberSend - this->base + SR_seqnum) % SR_seqnum - 1; i++, j = (j + 1) % SR_seqnum) {
				//注意窗口长度是(expectSequenceNumberSend - this->base + SR_seqnum) % SR_seqnum - 1
				//注意j的初始取值！！
				if (!ACK[j]) {
					this->base = j;
					flag = 0;
					break;
				}
			}
			if (flag)	this->base = expectSequenceNumberSend;
		}
		printf("SR发送方滑动窗口变化为：[");
		int j = this->base;
		for (int i = 0; i < SR_Winsize - 1; i++) {
			printf("%d，", j);
			j = (j + 1) % SR_seqnum;
		}
		printf("%d]\n", j);
	}
}
// 重传在以下函数中实现
void SRRdtSender::timeoutHandler(int seqNum) {
	//唯一一个定时器,无需考虑seqNum
	//printf("出现超时！重发报文！\n");
	pUtils->printPacket("发送方定时器时间到，重发上次发送的报文", this->packetWaitingAck[seqNum]);
	pns->stopTimer(SENDER, seqNum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//重新启动发送方定时器
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[seqNum]);			//重新发送数据包
}
