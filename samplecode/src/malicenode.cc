#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstdio>

#include <time.h>
#include <string.h>

#include <netinet/in.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>

#include <stunlib.h>

#define UDP_HEADER_SIZE 8

using namespace std;

uint16_t udp_checksum(char *buff, size_t len, uint16_t *src_addr, uint16_t *dest_addr);

static int Callback(nfq_q_handle *myQueue, struct nfgenmsg *msg,
                    nfq_data *pkt, void *cbData)
{
  uint32_t id = 0;
  nfqnl_msg_packet_hdr *header;
  if ((header = nfq_get_msg_packet_hdr(pkt))) {
    id = ntohl(header->packet_id);
  }

  cout << "pkt recvd: " << id << endl;

  char *pktData;
  int packet_size = nfq_get_payload(pkt, &pktData);
  int ip_header_size = (pktData[0] & 0xF) * 4;

  int udp_length = (pktData[ip_header_size + 4] << 8) + pktData[ip_header_size + 5];
  int udp_payload_length = udp_length - 8;
  int udp_data_offset = ip_header_size + UDP_HEADER_SIZE;
  cout << "Offset to UDP data: " << udp_data_offset << " UDP data length: " << udp_length << endl;

  // Not pretty... Should really rewrite to pure c.
  unsigned char *payload = (unsigned char *)malloc((size_t)(udp_payload_length));

  memcpy(payload, &pktData[udp_data_offset], (size_t)(udp_payload_length));

  bool isMsStun;
  if (!stunlib_isStunMsg(payload, udp_payload_length, &isMsStun)) {
    cout << "NOT a STUN packet." << endl;
    free(payload);
    return nfq_set_verdict(myQueue, id, NF_ACCEPT, 0, NULL);
  }
  cout << "Is a STUN packet." << endl;

  StunMessage stunPkt;

  if (!stunlib_DecodeMessage(payload, udp_payload_length, &stunPkt, NULL, NULL, isMsStun)) {
    cout << "Something went wrong in decoding..." << endl;
    free(payload);
    return nfq_set_verdict(myQueue, id, NF_ACCEPT, 0, NULL);
  }
  cout << "Message decoded fine." << endl;

  if ((stunPkt.msgHdr.msgType == STUN_MSG_BindRequestMsg
      || stunPkt.msgHdr.msgType == STUN_MSG_RefreshRequestMsg)
      && stunPkt.hasMaliceMetadata && stunPkt.maliceMetadata.hasMDRespUP) {
    if (stunPkt.maliceMetadata.mdRespUP.hasFlowdataResp) {
      stunPkt.maliceMetadata.mdRespUP.flowdataResp.DT = 5;
      stunPkt.maliceMetadata.mdRespUP.flowdataResp.LT = 5;
      stunPkt.maliceMetadata.mdRespUP.flowdataResp.JT = 5;
      stunPkt.maliceMetadata.mdRespUP.flowdataResp.minBW = 42;
      stunPkt.maliceMetadata.mdRespUP.flowdataResp.maxBW = 31337;
      cout << "Changing MD-RESP-UP to some sweet, sweet bandwith and stuff." << endl;
    }
  } else if ((stunPkt.msgHdr.msgType == STUN_MSG_BindResponseMsg
             || stunPkt.msgHdr.msgType == STUN_MSG_RefreshResponseMsg)
             && stunPkt.hasMaliceMetadata && stunPkt.maliceMetadata.hasMDRespDN) {
    
    if (stunPkt.maliceMetadata.mdRespDN.hasFlowdataResp) {
      stunPkt.maliceMetadata.mdRespDN.flowdataResp.DT = 1;
      stunPkt.maliceMetadata.mdRespDN.flowdataResp.LT = 1;
      stunPkt.maliceMetadata.mdRespDN.flowdataResp.JT = 1;
      stunPkt.maliceMetadata.mdRespDN.flowdataResp.minBW = 0;
      stunPkt.maliceMetadata.mdRespDN.flowdataResp.maxBW = 1814;
      cout << "Changing MD-RESP-DN to some crappy bandwith and stuff." << endl;
    }
  }

  static const char password[] = "VOkJxbRl1RmTxUk/WvJxBt";
  int msg_len = stunlib_encodeMessage(&stunPkt, payload, udp_payload_length, (unsigned char*)password, strlen(password), NULL, false);
  cout << "Reencoded message, " << msg_len << " bytes. UDP data: " << udp_payload_length << endl;
  memcpy(&pktData[udp_data_offset], payload, (size_t)udp_payload_length);

  // UDP Checksum recalculation.
  pktData[ip_header_size + 6] = 0x00; 
  pktData[ip_header_size + 7] = 0x00; 
  uint16_t *ip_src = (uint16_t *) (pktData + 3 * 4), *ip_dst = (uint16_t *) (pktData + 4 * 4);
  uint16_t checksum = htonl(udp_checksum(&pktData[ip_header_size], udp_length, ip_src, ip_dst));
  pktData[ip_header_size + 6] = (checksum >> 8) & 0xFF;
  pktData[ip_header_size + 7] = checksum & 0xFF;

  free(payload);
  return nfq_set_verdict(myQueue, id, NF_ACCEPT, packet_size, (unsigned char*)pktData);
}

int main(int argc, char **argv)
{
  struct nfq_handle *nfqHandle;

  struct nfq_q_handle *myQueue;
  struct nfnl_handle *netlinkHandle;

  int fd, res;
  char buf[4096];

  if (!(nfqHandle = nfq_open())) {
    perror("nfq_open");
    exit(1);
  }

  if (nfq_unbind_pf(nfqHandle, AF_INET) < 0) {
    perror("nfq_unbind_pf");
    exit(1);
  }

  if (nfq_bind_pf(nfqHandle, AF_INET) < 0) {
    perror("nfq_bind_pf");
    exit(1);
  }

  if (!(myQueue = nfq_create_queue(nfqHandle,  0, &Callback, NULL))) {
    perror("nfq_create_queue");
    exit(1);
  }

  if (nfq_set_mode(myQueue, NFQNL_COPY_PACKET, 0xffff) < 0) {
    perror("nfq_set_mode");
    exit(1);
  }

  netlinkHandle = nfq_nfnlh(nfqHandle);
  fd = nfnl_fd(netlinkHandle);

  cout << "Up and running, waiting for packets...\n\n";
  while ((res = recv(fd, buf, sizeof(buf), 0)) && res >= 0) {
    nfq_handle_packet(nfqHandle, buf, res);
  }

  nfq_destroy_queue(myQueue);

  nfq_close(nfqHandle);

  return 0;
}

uint16_t udp_checksum(char *buff, size_t len, uint16_t *ip_src, uint16_t *ip_dst)
{
  uint16_t *buf = (uint16_t *)buff;
  uint32_t sum;
  size_t length = len;

  // Calculate the sum
  sum = 0;
  while (len > 1)
  {
    sum += *buf++;
    if (sum & 0x80000000)
           sum = (sum & 0xFFFF) + (sum >> 16);
    len -= 2;
  }

  if ( len & 1 )
    // Add the padding if the packet length
    sum += *((uint8_t *)buf);

  // Add the pseudo-header
  sum += *(ip_src++);
  sum += *ip_src;

  sum += *(ip_dst++);
  sum += *ip_dst;

  sum += htons(IPPROTO_UDP);
  sum += htons(length);

  // Add the carries
  while (sum >> 16)
    sum = (sum & 0xFFFF) + (sum >> 16);

  // Return the one's complement of sum
  return (uint16_t)(~sum);
}
