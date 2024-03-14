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
	if (this->waitingState) { //���ͷ����ڵȴ�ȷ��״̬
		return false;
	}
	ACK[expectSequenceNumberSend] = false;	//����Ҫд�������Ȼ�ڵ�һ�η��ͽ�����ACK��֪��Ϊʲô�ᱻ��ֵ
	this->packetWaitingAck[expectSequenceNumberSend].acknum = -1; //���Ը��ֶ�
	this->packetWaitingAck[expectSequenceNumberSend].seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck[expectSequenceNumberSend].checksum = 0;
	memcpy(this->packetWaitingAck[expectSequenceNumberSend].payload, message.data, sizeof(message.data));
	this->packetWaitingAck[expectSequenceNumberSend].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[expectSequenceNumberSend]);
	pUtils->printPacket("���ͷ����ͱ���", this->packetWaitingAck[expectSequenceNumberSend]);
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[expectSequenceNumberSend]);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�

	//SRЭ����Ҫ��ÿһ�����Ķ���ʱ
	pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck[expectSequenceNumberSend].seqnum);			//�������ͷ���ʱ��
	printf("��ǰ����Ϊ:[%d,%d]\n", base, (base + SR_Winsize - 1) % SR_Winsize);
	expectSequenceNumberSend = (expectSequenceNumberSend + 1) % SR_seqnum;

	if ((expectSequenceNumberSend - this->base + SR_seqnum) % SR_seqnum == SR_Winsize)
		this->waitingState = true;		//�����괰������������֮�󣬲��ܽ�waitingState��Ϊtrue																			//����ȴ�״̬
	return true;
}

void SRRdtSender::receive(const Packet& ackPkt) {
	//if (this->waitingState == true) {//������ͷ����ڵȴ�ack��״̬�������´�������ʲô������
		//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	if (ackPkt.acknum < 0 || ackPkt.acknum > SR_seqnum - 1) {  //���ķ����𻵣�������������Խ��
		return;
	}
	//��δ�յ��÷����ack��У�����ȷ��acknum��[base,base + SR_Winszie - 1]֮��
	if (!ACK[ackPkt.acknum] && checkSum == ackPkt.checksum && ((ackPkt.acknum >= this->base && ackPkt.acknum <= this->base + SR_Winsize - 1) || (ackPkt.acknum + 1 + SR_seqnum - this->base <= SR_Winsize))) {
		pns->stopTimer(SENDER, ackPkt.acknum);		//�رն�ʱ��
		waitingState = false;
		ACK[ackPkt.acknum] = true;
		pUtils->printPacket("���ͷ���ȷ�յ�ȷ�ϣ�", ackPkt);
		int flag = 1;	//�жϵ�ǰ�������Ƿ��������ݰ����յ���ACK
		if (ackPkt.acknum == this->base) {	//�������ڣ���base�����һ��δ�յ�ack��Ϣ�ı��Ĵ���
			for (int i = 0, j = (this->base + 1) % SR_seqnum; i < (expectSequenceNumberSend - this->base + SR_seqnum) % SR_seqnum - 1; i++, j = (j + 1) % SR_seqnum) {
				//ע�ⴰ�ڳ�����(expectSequenceNumberSend - this->base + SR_seqnum) % SR_seqnum - 1
				//ע��j�ĳ�ʼȡֵ����
				if (!ACK[j]) {
					this->base = j;
					flag = 0;
					break;
				}
			}
			if (flag)	this->base = expectSequenceNumberSend;
		}
		printf("SR���ͷ��������ڱ仯Ϊ��[");
		int j = this->base;
		for (int i = 0; i < SR_Winsize - 1; i++) {
			printf("%d��", j);
			j = (j + 1) % SR_seqnum;
		}
		printf("%d]\n", j);
	}
}
// �ش������º�����ʵ��
void SRRdtSender::timeoutHandler(int seqNum) {
	//Ψһһ����ʱ��,���迼��seqNum
	//printf("���ֳ�ʱ���ط����ģ�\n");
	pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط��ϴη��͵ı���", this->packetWaitingAck[seqNum]);
	pns->stopTimer(SENDER, seqNum);										//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//�����������ͷ���ʱ��
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[seqNum]);			//���·������ݰ�
}
