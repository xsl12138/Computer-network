#pragma once
#ifndef SR_RDT_SENDER_H
#define SR_RDT_SENDER_H
#include "RdtSender.h"
class SRRdtSender :public RdtSender
{
private:
	int expectSequenceNumberSend;	// ��һ��������� 
	int base;						//�����
	bool waitingState;				// �Ƿ��ڵȴ�Ack��״̬
	Packet packetWaitingAck[SR_seqnum];		//�ѷ��Ͳ��ȴ�Ack�����ݰ�
	bool ACK[SR_seqnum];		//�������յ���ACK�ź�

public:

	bool getWaitingState();
	bool send(const Message& message);						//����Ӧ�ò�������Message����NetworkServiceSimulator����,������ͷ��ɹ��ؽ�Message���͵�����㣬����true;�����Ϊ���ͷ����ڵȴ���ȷȷ��״̬���ܾ�����Message���򷵻�false
	void receive(const Packet& ackPkt);						//����ȷ��Ack������NetworkServiceSimulator����	
	void timeoutHandler(int seqNum);					//Timeout handler������NetworkServiceSimulator����

public:
	SRRdtSender();
	virtual ~SRRdtSender();
};

#endif

