#include "stdafx.h"
#include "Global.h"
#include "SRRdtReceiver.h"

char receive_sr_ok[21] = "receive success!";
char receive_sr_checksumerr[21] = "checksum error!";
char receive_sr_seqnumerr[21] = "seqnum error!";

SRRdtReceiver::SRRdtReceiver() :expectSequenceNumberRcvd(0)
{
	memset(rcvseq, SR_seqnum, false);
	lastAckPkt.acknum = -1; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//忽略该字段
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		lastAckPkt.payload[i] = receive_sr_ok[i];
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}


SRRdtReceiver::~SRRdtReceiver()
{
}

void SRRdtReceiver::receive(const Packet& packet) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(packet);
	if (packet.seqnum < 0 || packet.seqnum > SR_seqnum - 1) {  //报文发生损坏，避免下面数组越界
		return;
	}
	//printf("\n");
	//校验和正确，报文序号在滑动窗口范围内
	if (checkSum == packet.checksum && (packet.seqnum - expectSequenceNumberRcvd >= 0 && packet.seqnum - expectSequenceNumberRcvd <= SR_Winsize - 1) || (packet.seqnum + 1 + SR_seqnum - expectSequenceNumberRcvd <= SR_Winsize)) {
		//if ((packet.seqnum - expectSequenceNumberRcvd >= 0 && packet.seqnum - expectSequenceNumberRcvd <= SR_Winsize - 1) || (packet.seqnum + 1 + SR_seqnum - expectSequenceNumberRcvd <= SR_Winsize)) {
			pUtils->printPacket("接收方正确收到发送方的报文", packet);
			if (packet.seqnum == expectSequenceNumberRcvd) {	//交付缓存的所有报文，滑动窗口
				Message msg;
				memcpy(msg.data, packet.payload, sizeof(packet.payload));
				int flag = expectSequenceNumberRcvd;  //[rvcbase,flag]为传给应用层的报文序号区间
				//下面检查是否缓存了连续序号的报文
				for (int i = (expectSequenceNumberRcvd + 1) % SR_seqnum, j = 1; j < SR_Winsize; j++, i = (i + 1) % SR_seqnum) {
					if (rcvseq[i] == true) {
						flag = i;
					}
					else
						break;
				}
				if (flag == expectSequenceNumberRcvd) {
					printf("接收方没有缓存的连续的报文，直接将报文序号为%d的报文递交给应用层\n", expectSequenceNumberRcvd);
					pns->delivertoAppLayer(RECEIVER, msg);
				}
				else {
					printf("接收方将缓存的 %d ~ %d 的报文递交给应用层\n", expectSequenceNumberRcvd, flag);
					pns->delivertoAppLayer(RECEIVER, msg);
					for (int i = (expectSequenceNumberRcvd + 1) % SR_seqnum, j = 0; j < (flag - expectSequenceNumberRcvd + SR_seqnum) % SR_seqnum; j++, i = (i + 1) % SR_seqnum) {
						pns->delivertoAppLayer(RECEIVER, rcvmsg[i]);
						rcvseq[i] = false;
					}
				}
				expectSequenceNumberRcvd = (flag + 1) % SR_seqnum;	//更新接收方滑动窗口
				printf("递交报文后，接收方当前窗口：[%d,%d]\n", expectSequenceNumberRcvd, (expectSequenceNumberRcvd - 1 + SR_Winsize) % SR_seqnum);
			}
			else {	//缓存报文
				memcpy(rcvmsg[packet.seqnum].data, packet.payload, sizeof(packet.payload));
				rcvseq[packet.seqnum] = true;
				printf("报文不连续！接受方缓存序号为%d的报文，接收方当前窗口：[%d,%d]\n", packet.seqnum, expectSequenceNumberRcvd, (expectSequenceNumberRcvd - 1 + SR_Winsize) % SR_seqnum);
			}
			lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) { //重新赋值
				lastAckPkt.payload[i] = receive_sr_ok[i];
			}
			pUtils->printPacket("接收方发送确认报文", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
		//}
	}
	else {	
		if (checkSum != packet.checksum) { //checkSum不对
			pUtils->printPacket("接收方没有正确收到发送方的报文,数据校验错误", packet);
			for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
				lastAckPkt.payload[i] = receive_sr_checksumerr[i];
			}
		}
		else {	//checkSum正确，但报文序号不对
			pUtils->printPacket("接收方没有正确收到发送方的报文,报文序号不对", packet);
			//一般都是序号在接收方基序号前面的情况
			//假如有一个报文A，接收方成功接收，向发送方发送过报文A的ACK
			//但之后接收方又收到了新的正确报文B，发送B的ACK，而A的ACK有可能因为网络原因丢失了
			//如果这时，发送方因超时再次发送报文A，则接收方必须返回已经收到报文A的消息（而非收到的上一个报文的消息）
			//否则发送方将永远收不到报文A的ACK，从而陷入死循环
			lastAckPkt.acknum = packet.seqnum;
			for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
				lastAckPkt.payload[i] = receive_sr_seqnumerr[i];
			}
		}
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);	//这里必须要重新算一遍！！！
		pUtils->printPacket("接收方重新发送上次的确认报文", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送上次的确认报文
	}
	//printf("\n");
}