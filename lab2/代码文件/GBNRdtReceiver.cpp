#include "stdafx.h"
#include "Global.h"
#include "GBNRdtReceiver.h"

char receive_gbn_ok[21] = "receive success!";
char receive_gbn_checksumerr[21] = "checksum error!";
char receive_gbn_seqnumerr[21] = "seqnum error!";

GBNRdtReceiver::GBNRdtReceiver() :expectSequenceNumberRcvd(0)
{
	lastAckPkt.acknum = -1; //��ʼ״̬�£��ϴη��͵�ȷ�ϰ���ȷ�����Ϊ-1��ʹ�õ���һ�����ܵ����ݰ�����ʱ��ȷ�ϱ��ĵ�ȷ�Ϻ�Ϊ-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//���Ը��ֶ�
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		lastAckPkt.payload[i] = receive_gbn_ok[i];
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}


GBNRdtReceiver::~GBNRdtReceiver()
{
}

void GBNRdtReceiver::receive(const Packet& packet) {
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(packet);

	//���У�����ȷ��ͬʱ�յ����ĵ���ŵ��ڽ��շ��ڴ��յ��ı������һ��
	if (checkSum == packet.checksum && this->expectSequenceNumberRcvd == packet.seqnum) {
		pUtils->printPacket("���շ���ȷ�յ����ͷ��ı���", packet);

		//ȡ��Message�����ϵݽ���Ӧ�ò�
		Message msg;
		memcpy(msg.data, packet.payload, sizeof(packet.payload));
		pns->delivertoAppLayer(RECEIVER, msg);

		lastAckPkt.acknum = packet.seqnum; //ȷ����ŵ����յ��ı������
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
			lastAckPkt.payload[i] = receive_gbn_ok[i];
		}
		pUtils->printPacket("���շ�����ȷ�ϱ���", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�

		this->expectSequenceNumberRcvd = (1 + this->expectSequenceNumberRcvd) % GBN_seqnum; //������һ�������յ������
	}
	else {
		if (checkSum != packet.checksum) {
			pUtils->printPacket("���շ�û����ȷ�յ����ͷ��ı���,����У�����", packet);
			for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
				lastAckPkt.payload[i] = receive_gbn_checksumerr[i];
			}
		}
		else {//checkSum��ȷ����������Ų�һ��
			pUtils->printPacket("���շ�û����ȷ�յ����ͷ��ı���,������Ų���", packet);
			for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
				lastAckPkt.payload[i] = receive_gbn_seqnumerr[i];
			}
		}
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);	//�������Ҫ������һ�飡����
		pUtils->printPacket("���շ����·����ϴε�ȷ�ϱ���", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢���ϴε�ȷ�ϱ���

	}
}