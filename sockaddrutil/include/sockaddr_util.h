/*
Copyright 2014 Cisco. All rights reserved. 

Redistribution and use in source and binary forms, with or without modification, are 
permitted provided that the following conditions are met: 

   1. Redistributions of source code must retain the above copyright notice, this list of 
      conditions and the following disclaimer. 

   2. Redistributions in binary form must reproduce the above copyright notice, this list 
      of conditions and the following disclaimer in the documentation and/or other materials 
      provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY CISCO ''AS IS'' AND ANY EXPRESS OR IMPLIED 
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
DAMAGE. 

The views and conclusions contained in the software and documentation are those of the 
authors and should not be interpreted as representing official policies, either expressed 
or implied, of Cisco.
*/



#ifndef SOCKADDR_UTIL_H
#define SOCKADDR_UTIL_H

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif


/*static const uint32_t SOCKADDR_MAX_STRLEN = INET6_ADDRSTRLEN + 8; //port, :, [];*/
#define SOCKADDR_MAX_STRLEN  INET6_ADDRSTRLEN + 8 


void sockaddr_reset(struct sockaddr_storage * sa);

/*
 *  Initialize a sockaddr_in (IPv4) as any ("0.0.0.0:0").
 *  Remember to put aside enough memory. (sockaddr_storage)
 */
void sockaddr_initAsIPv4Any(struct sockaddr_in * sa);



void sockaddr_initAsIPv6Any(struct sockaddr_in6 * sa);

/*
 *  Initialize a sockaddr from string.
 *  Remember to put aside enough memory. (sockaddr_storage)
 */
bool sockaddr_initFromString(struct sockaddr *sa,
                             const char *addr_str);

/*
 * Initialize a sockaddr from a IPv4 string
 */
bool sockaddr_initFromIPv4String(struct sockaddr_in *sa,
                                 const char *addr_str);

/*
 * Initialize a sockaddr from a IPv6 string
 */
bool sockaddr_initFromIPv6String(struct sockaddr_in6 *sa,
                                 const char *addr_str);


/*
 * Initialize IPv4 sockaddr from a int addr and a int port.
 * (Use htons and htonl if your data is stored in host format)
 *
 */
bool sockaddr_initFromIPv4Int(struct sockaddr_in *sa,
                              uint32_t addr,
                              uint16_t port);


/*
 * Initialize IPv6 sockaddr from a int addr and a int port.
 * (Use htons and htonl if your data is stored in host format)
 *
 */
bool sockaddr_initFromIPv6Int(struct sockaddr_in6 *sin,
                              const uint8_t ipaddr[16],
                              uint16_t port);


/*
 *  Checks if the address part is the same.
 *  No checking of ports or transport protocol
 */

bool sockaddr_sameAddr(const struct sockaddr * a,
                       const struct sockaddr * b);


/*
 *  Check if the port is the same.
 *
 */
bool sockaddr_samePort(const struct sockaddr * a,
                       const struct sockaddr * b);

/*
 *  Get port from address
 *
 */
unsigned int
sockaddr_ipPort (const struct sockaddr * a);

/*
 * Check if two sockaddr are alike
 * (IP proto, port and address)
 *
 */
bool sockaddr_alike(const struct sockaddr * a,
                    const struct sockaddr * b);

/*
 *   Checks if a sockaddr is reasonably initialized
 *
 */
bool sockaddr_isSet(const struct sockaddr * sa);

/*
 * Checks if a sockaddr has the address of 'any'
 *
 */
bool sockaddr_isAddrAny(const struct sockaddr * sa);


/*
 * Checks if a sockaddr loop-back
 *
 */
bool sockaddr_isAddrLoopBack(const struct sockaddr * sa);


/*
 * Checks if a sockaddr is private (RFC 1918)
 *
 */
bool sockaddr_isAddrPrivate(const struct sockaddr * sa);


/*
 * Checks if a sockaddr is a IPv6 link local address
 * Will return false if it is a IPv4 addr
 */
bool sockaddr_isAddrLinkLocal(const struct sockaddr * sa);

/*
 * Checks if a sockaddr is a IPv6 site local address
 * Will return false if it is a IPv4 addr
 */

bool sockaddr_isAddrSiteLocal(const struct sockaddr * sa);

/*
 * Checks if a sockaddr is a IPv6 ULA address
 * Will return false if it is a IPv4 addr
 */
bool sockaddr_isAddrULA(const struct sockaddr * sa);

/*
 * Get IPv6 Flags. 
 * Will return 0 in case if failure or IPv4 addr
 */
int sockaddr_getIPv6Flags(const struct sockaddr * sa, const char* ifa_name, int ifa_len);

/*
 * Checks if a sockaddr is a IPv6 temporary address
 * Will return false if it is a IPv4 addr
 */
bool sockaddr_isAddrTemporary(const struct sockaddr * sa, const char* ifa_name, int ifa_len);


/*
 * Checks if a sockaddr is a IPv6 deprecated address
 * Will return false if it is a IPv4 addr
 */
bool sockaddr_isAddrDeprecated(const struct sockaddr * sa, const char* ifa_name, int ifa_len);

/*
 * Converts a sockaddr to string
 * If add port is true the IPv6 string will contain [],
 * if not the IPv6 address is printed without[]
 */
const char *sockaddr_toString( const struct sockaddr *sa,
                               char *dest,
                               size_t destlen,
                               bool addport);

/*
 *  Copy a sockaddr from src to dst
 */
void sockaddr_copy(struct sockaddr * dst,
                   const struct sockaddr * src);


/*
 *   Set the port portion of a sockaddr
 */
void sockaddr_setPort(struct sockaddr * sa,
                      uint16_t port);

#ifdef __cplusplus
}
#endif

#endif
