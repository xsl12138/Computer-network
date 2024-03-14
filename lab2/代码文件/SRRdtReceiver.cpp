#include "stdafx.h"
#include "Global.h"
#include "SRRdtReceiver.h"

char receive_sr_ok[21] = "receive success!";
char receive_sr_checksumerr[21] = "checksum error!";
char receive_sr_seqnumerr[21] = "seqnum error!";

SRRdtReceiver::SRRdtReceiver() :expectSequenceNumberRcvd(0)
{
	memset(rcvseq, SR_seqnum, false);
	lastAckPkt.acknum = -1; //��ʼ״̬�£��ϴη��͵�ȷ�ϰ���ȷ�����Ϊ-1��ʹ�õ���һ�����ܵ����ݰ�����ʱ��ȷ�ϱ��ĵ�ȷ�Ϻ�Ϊ-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//���Ը��ֶ�
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		lastAckPkt.payload[i] = receive_sr_ok[i];
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}


SRRdtReceiver::~SRRdtReceiver()
{
}

void SRRdtReceiver::receive(const Packet& packet) {
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(packet);
	if (packet.seqnum < 0 || packet.seqnum > SR_seqnum - 1) {  //���ķ����𻵣�������������Խ��
		return;
	}
	//printf("\n");
	//У�����ȷ����������ڻ������ڷ�Χ��
	if (checkSum == packet.checksum && (packet.seqnum - expectSequenceNumberRcvd >= 0 && packet.seqnum - expectSequenceNumberRcvd <= SR_Winsize - 1) || (packet.seqnum + 1 + SR_seqnum - expectSequenceNumberRcvd <= SR_Winsize)) {
		//if ((packet.seqnum - expectSequenceNumberRcvd >= 0 && packet.seqnum - expectSequenceNumberRcvd <= SR_Winsize - 1) || (packet.seqnum + 1 + SR_seqnum - expectSequenceNumberRcvd <= SR_Winsize)) {
			pUtils->printPacket("���շ���ȷ�յ����ͷ��ı���", packet);
			if (packet.seqnum == expectSequenceNumberRcvd) {	//������������б��ģ���������
				Message msg;
				memcpy(msg.data, packet.payload, sizeof(packet.payload));
				int flag = expectSequenceNumberRcvd;  //[rvcbase,flag]Ϊ����Ӧ�ò�ı����������
				//�������Ƿ񻺴���������ŵı���
				for (int i = (expectSequenceNumberRcvd + 1) % SR_seqnum, j = 1; j < SR_Winsize; j++, i = (i + 1) % SR_seqnum) {
					if (rcvseq[i] == true) {
						flag = i;
					}
					else
						break;
				}
				if (flag == expectSequenceNumberRcvd) {
					printf("���շ�û�л���������ı��ģ�ֱ�ӽ��������Ϊ%d�ı��ĵݽ���Ӧ�ò�\n", expectSequenceNumberRcvd);
					pns->delivertoAppLayer(RECEIVER, msg);
				}
				else {
					printf("���շ�������� %d ~ %d �ı��ĵݽ���Ӧ�ò�\n", expectSequenceNumberRcvd, flag);
					pns->delivertoAppLayer(RECEIVER, msg);
					for (int i = (expectSequenceNumberRcvd + 1) % SR_seqnum, j = 0; j < (flag - expectSequenceNumberRcvd + SR_seqnum) % SR_seqnum; j++, i = (i + 1) % SR_seqnum) {
						pns->delivertoAppLayer(RECEIVER, rcvmsg[i]);
						rcvseq[i] = false;
					}
				}
				expectSequenceNumberRcvd = (flag + 1) % SR_seqnum;	//���½��շ���������
				printf("�ݽ����ĺ󣬽��շ���ǰ���ڣ�[%d,%d]\n", expectSequenceNumberRcvd, (expectSequenceNumberRcvd - 1 + SR_Winsize) % SR_seqnum);
			}
			else {	//���汨��
				memcpy(rcvmsg[packet.seqnum].data, packet.payload, sizeof(packet.payload));
				rcvseq[packet.seqnum] = true;
				printf("���Ĳ����������ܷ��������Ϊ%d�ı��ģ����շ���ǰ���ڣ�[%d,%d]\n", packet.seqnum, expectSequenceNumberRcvd, (expectSequenceNumberRcvd - 1 + SR_Winsize) % SR_seqnum);
			}
			lastAckPkt.acknum = packet.seqnum; //ȷ����ŵ����յ��ı������
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) { //���¸�ֵ
				lastAckPkt.payload[i] = receive_sr_ok[i];
			}
			pUtils->printPacket("���շ�����ȷ�ϱ���", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�
		//}
	}
	else {	
		if (checkSum != packet.checksum) { //checkSum����
			pUtils->printPacket("���շ�û����ȷ�յ����ͷ��ı���,����У�����", packet);
			for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
				lastAckPkt.payload[i] = receive_sr_checksumerr[i];
			}
		}
		else {	//checkSum��ȷ����������Ų���
			pUtils->printPacket("���շ�û����ȷ�յ����ͷ��ı���,������Ų���", packet);
			//һ�㶼������ڽ��շ������ǰ������
			//������һ������A�����շ��ɹ����գ����ͷ����͹�����A��ACK
			//��֮����շ����յ����µ���ȷ����B������B��ACK����A��ACK�п�����Ϊ����ԭ��ʧ��
			//�����ʱ�����ͷ���ʱ�ٴη��ͱ���A������շ����뷵���Ѿ��յ�����A����Ϣ�������յ�����һ�����ĵ���Ϣ��
			//�����ͷ�����Զ�ղ�������A��ACK���Ӷ�������ѭ��
			lastAckPkt.acknum = packet.seqnum;
			for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
				lastAckPkt.payload[i] = receive_sr_seqnumerr[i];
			}
		}
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);	//�������Ҫ������һ�飡����
		pUtils->printPacket("���շ����·����ϴε�ȷ�ϱ���", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢���ϴε�ȷ�ϱ���
	}
	//printf("\n");
}