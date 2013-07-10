#include <iostream>
#include <iomanip>

#include <cstdlib>
#include <cstdio>

#include <time.h>

#include <netinet/in.h>

extern "C" {
  #include <linux/netfilter.h>
  #include <libnetfilter_queue/libnetfilter_queue.h>
}

using namespace std;

static int Callback(nfq_q_handle *myQueue, struct nfgenmsg *msg,
                    nfq_data *pkt, void *cbData) {
  uint32_t id = 0;
  nfqnl_msg_packet_hdr *header;

  cout << "pkt recvd: ";
  if ((header = nfq_get_msg_packet_hdr(pkt))) {
    id = ntohl(header->packet_id);
    cout << "id " << id << "; hw_protocol " << setfill('0') << setw(4) <<
      hex << ntohs(header->hw_protocol) << "; hook " << ('0'+header->hook)
         << " ; ";
  }

  nfqnl_msg_packet_hw *macAddr = nfq_get_packet_hw(pkt);
  if (macAddr) {
    cout << "mac len " << ntohs(macAddr->hw_addrlen) << " addr ";
    for (int i = 0; i < 8; i++) {
      cout << setfill('0') << setw(2) << hex << macAddr->hw_addr;
    }
  } else {
    cout << "no MAC addr";
  }

  timeval tv;
  if (!nfq_get_timestamp(pkt, &tv)) {
    cout << "; tstamp " << tv.tv_sec << "." << tv.tv_usec;
  } else {
    cout << "; no tstamp";
  }

  cout << "; mark " << nfq_get_nfmark(pkt);

  cout << "; indev " << nfq_get_indev(pkt);
  cout << "; outdev " << nfq_get_outdev(pkt);

  cout << endl;

  char *pktData;
  int len = nfq_get_payload(pkt, &pktData);
  int ip_header_size = (pktData[0] & 0xF) * 4;

  cout << "\n Headersize: " << dec << ip_header_size << endl;
  if (len) {
    cout << "data[" << len << "]: '";
    for (int i = 0; i < len; i++) {
      if (isprint(pktData[i]))
        cout << pktData[i];
      else cout << " ";
    }
    cout << "'" << endl;
  }

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
