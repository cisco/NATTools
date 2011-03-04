#ifndef IPADDRESS_HELPER_H
#define IPADDRESS_HELPER_H


int getRemoteTurnServerIp(struct turn_info *turnInfo, char *fqdn);

int getLocalIPaddresses(struct turn_info *turnInfo, char *iface);

bool getLocalInterFaceAddrs(struct sockaddr *addr, char *iface, int ai_family);

int createLocalUDPSocket(int ai_family, struct sockaddr * localIp, uint16_t port);


#endif
