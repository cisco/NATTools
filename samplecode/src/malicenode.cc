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

static int Callback(nfq_q_handle *myQueue, struct nfgenmsg *msg,
                    nfq_data *pkt, void *cbData) {
  uint32_t id = 0;
  nfqnl_msg_packet_hdr *header;
  if ((header = nfq_get_msg_packet_hdr(pkt))) {
    id = ntohl(header->packet_id);
  }

  cout << "pkt recvd: " << id << endl;

  char *pktData;
  int len = nfq_get_payload(pkt, &pktData);
  int ip_header_size = (pktData[0] & 0xF) * 4;

  int udp_length = (pktData[ip_header_size + 4] << 8) + pktData[ip_header_size + 5];
  int udp_data_offset = ip_header_size + UDP_HEADER_SIZE;
  cout << "Offset to UDP data: " << udp_data_offset << " UDP data length: " << udp_length << endl;

  // Not pretty... Should really rewrite to pure c.
  unsigned char *payload = (unsigned char *)malloc((size_t)udp_length);

  memcpy(payload, &pktData[udp_data_offset], (size_t)udp_length);

  bool isMsStun;
  if (!stunlib_isStunMsg(payload, udp_length, &isMsStun)) {
    cout << "NOT a STUN packet." << endl;
    free(payload);
    return nfq_set_verdict(myQueue, id, NF_ACCEPT, 0, NULL);
  }
  cout << "Is a STUN packet." << endl;

  StunMessage stunPkt;

  if (!stunlib_DecodeMessage(payload, udp_length, &stunPkt, NULL, NULL, isMsStun)) {
    cout << "Something went wrong in decoding..." << endl;
    free(payload);
    return nfq_set_verdict(myQueue, id, NF_ACCEPT, 0, NULL);
  }
  cout << "Message decoded fine." << endl;

  // ---Exchange this with stunlib_isStunMsg
  // ---If not stunmsg, return ACCEPT verdict.
  // ---If it is, decode.
  // Find MD-AGENT and MD-RESP-UP/DN.
  // Print contents of all of them.
  // Do some change in whichever of RESP-UP/DN is outside the integrity.
  // Recalculate IP, UDP and STUN checksums/fingerprints.
  // Return verdict WITH changed packet.

  free(payload);
  return nfq_set_verdict(myQueue, id, NF_ACCEPT, 0, NULL);
}

int main(int argc, char **argv) {
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
