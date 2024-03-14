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
	if (this->waitingState) { //���ͷ����ڵȴ�ȷ��״̬
		return false;
	}

	this->packetWaitingAck[expectSequenceNumberSend].acknum = -1; //���Ը��ֶ�
	this->packetWaitingAck[expectSequenceNumberSend].seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck[expectSequenceNumberSend].checksum = 0;
	memcpy(this->packetWaitingAck[expectSequenceNumberSend].payload, message.data, sizeof(message.data));
	this->packetWaitingAck[expectSequenceNumberSend].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[expectSequenceNumberSend]);
	pUtils->printPacket("���ͷ����ͱ���", this->packetWaitingAck[expectSequenceNumberSend]);
	//pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[expectSequenceNumberSend]);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
	//����ֻ��Ҫ���ǻ���ŵİ����Ƿ�ʱ����
	if(base == expectSequenceNumberSend) pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck[expectSequenceNumberSend].seqnum);			//�������ͷ���ʱ��
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[expectSequenceNumberSend]);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
	printf("��ǰ����Ϊ:[%d,%d]\n", base, base + GBN_Winsize - 1);
	//������һ���������
	//if(expectSequenceNumberSend < GBN_seqnum) expectSequenceNumberSend += 1;
	expectSequenceNumberSend = (expectSequenceNumberSend + 1) % GBN_seqnum;
	if((expectSequenceNumberSend - base + GBN_seqnum) % GBN_seqnum == GBN_Winsize)
		this->waitingState = true;		//�����괰������������֮�󣬲��ܽ�waitingState��Ϊtrue																			//����ȴ�״̬
	return true;
}

void GBNRdtSender::receive(const Packet& ackPkt) {
	//if (this->waitingState == true) {//������ͷ����ڵȴ�ack��״̬�������´�������ʲô������
		//���У����Ƿ���ȷ
		int checkSum = pUtils->calculateCheckSum(ackPkt);
		//printf("\n�ɹ�����GBNRdtSender:;receive����������������������������������\n");
		//pUtils->printPacket("��������", ackPkt);
		//���У�����ȷ������ȷ�����=���ͷ��ѷ��Ͳ��ȴ�ȷ�ϵ����ݰ����
		if (checkSum == ackPkt.checksum && ackPkt.acknum != this->packetWaitingAck[this->base].seqnum - 1) {
			//ֻ���ǽ��յ���ȷACK��������ɣ���Ϊ�������ڵ��ƶ�ֻ����յ���ȷACK�й�
			int lastbase = this->base;
			this->base = (ackPkt.acknum + 1) % GBN_seqnum;
			this->waitingState = false;
			pUtils->printPacket("���ͷ��յ�����ȷȷ�ϣ�", ackPkt);
			printf("GBN�������ڱ仯Ϊ��[");
			int j = this->base;
			for (int i = 0; i < GBN_Winsize - 1; i++) {
				printf("%d��", j);
				j = (j + 1) % GBN_seqnum;
			}
			printf("%d]\n", j);
			if (this->base == this->expectSequenceNumberSend) {	//�ô��ڷ����Ѿ�ȫ�����յ���
				pns->stopTimer(SENDER, this->packetWaitingAck[lastbase].seqnum);		//�رն�ʱ��
			}
			else {
				pns->stopTimer(SENDER, this->packetWaitingAck[lastbase].seqnum);		//�رն�ʱ��
				pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck[base].seqnum);
			}
		}
	//}
}
// �ش������º�����ʵ��
void GBNRdtSender::timeoutHandler(int seqNum) {
	//Ψһһ����ʱ��,���迼��seqNum
	printf("���ֳ�ʱ���ط������������ѷ��͵ı��ģ�\n");
	//pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط��ϴη��͵ı���", this->packetWaitingAck);
	pns->stopTimer(SENDER, seqNum);										//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//�����������ͷ���ʱ��
	
	for (int i = 1, j = base; i <= (expectSequenceNumberSend - base + GBN_seqnum) % GBN_seqnum; i++) {
		pUtils->printPacket("���ͷ��������·��ͱ���", this->packetWaitingAck[j]);
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[j]);			//���·������ݰ�
		j = (j + 1) % GBN_seqnum;
	}
	//pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);			//���·������ݰ�

}
