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

#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#include <net/if.h>

/* IP */
#include <netinet/in.h>


#if defined(__APPLE__)
#include <netinet/in_var.h>
#endif


#include <sys/ioctl.h>
#include <unistd.h>



#include "sockaddr_util.h"

void sockaddr_reset(struct sockaddr_storage * sa)
{
    memset(sa, 0, sizeof *sa);
    sa->ss_family = AF_UNSPEC;
}

void sockaddr_initAsIPv4Any(struct sockaddr_in * sa)
{
    sa->sin_family = AF_INET;
    sa->sin_port = htons(0);
    sa->sin_addr.s_addr = INADDR_ANY;
}

void sockaddr_initAsIPv6Any(struct sockaddr_in6 * sa)
{
    struct in6_addr ipv6any = IN6ADDR_ANY_INIT;

    sa->sin6_family = AF_INET6;
    sa->sin6_port = htons(0);
    memcpy( &(sa->sin6_addr.s6_addr), &ipv6any,
            sizeof(struct in6_addr));
}


bool sockaddr_initFromIPv6String(struct sockaddr_in6 *sa,
                                 const char *addr_str)
{
    struct in6_addr ipv6;
    in_port_t port = 0;

    if (addr_str[0] == '[') {
        char tmp[64];
        const char * p = strchr(addr_str, ']');
        size_t len = p ? (size_t) (p - addr_str - 1): 0;

        if (!p || len >= sizeof(tmp)){
            return false;
        }

        memcpy(tmp, addr_str + 1, len);
        tmp[len] = '\0';

        if (!inet_pton(AF_INET6, tmp, &ipv6))
            return false;

        /* If there are more characters after ], it's supposed to
         * be :<port> */
        if (strlen(p) > 1) {
            unsigned int tmpport;
            if (sscanf(p, "]:%u", &tmpport) == 1 &&
                tmpport <= 65535)
                port = (in_port_t) tmpport;
            /* Ignore strange port values.  Use 0 in that case. */
        }
    } else if (!inet_pton(AF_INET6, addr_str, &ipv6)) {
        return false;
    }

    sa->sin6_family = AF_INET6;
    sa->sin6_port = htons(port);
    memcpy( &(sa->sin6_addr.s6_addr), &ipv6,
            sizeof(struct in6_addr));

    return true;
}



bool sockaddr_initFromIPv4String(struct sockaddr_in *sa,
                                 const char *addr_str)
{
    struct in_addr ipaddr;
    const char * p = strchr(addr_str, ':');
    char buf[16];
    in_port_t port = 0;

    if (!p) {
        if (!inet_pton(AF_INET, addr_str, &ipaddr))
            return false;
    } else {
        unsigned int tmpport;
        size_t len = (size_t) (p - addr_str);

        if (len >= sizeof(buf))
            return false;
        memcpy(buf, addr_str, len);
        buf[len] = '\0';

        if (!inet_pton(AF_INET, buf, &ipaddr))
            return false;
        if (sscanf(p, ":%u", &tmpport) == 1 &&
            tmpport <= 65535)
            port = tmpport;
        /* If the above test doesn't hit, the port is strange,
         * and we use 0.  This might not be the ideal solution,
         * but we keep it in for now to accommodate some users. */
    }

    sa->sin_family = AF_INET;
    sa->sin_port = htons(port);
    sa->sin_addr = ipaddr;

    return true;
}

bool sockaddr_initFromString( struct sockaddr *sa,
                              const char *addr_str)
{
    if (strlen(addr_str) == 0) {
        return false;
    }
    if (sockaddr_initFromIPv4String((struct sockaddr_in *)sa, addr_str))
        return true;
    if (sockaddr_initFromIPv6String((struct sockaddr_in6 *)sa, addr_str))
        return true;

    return false;
}


bool sockaddr_initFromIPv4Int(struct sockaddr_in *sin,
                              uint32_t ipaddr,
                              uint16_t port)
{
    sin->sin_family = AF_INET;
    sin->sin_port = port;
    sin->sin_addr.s_addr = ipaddr;

    return true;
}

bool sockaddr_initFromIPv6Int(struct sockaddr_in6 *sin,
                              const uint8_t ipaddr[16],
                              uint16_t port)
{
    sin->sin6_family = AF_INET6;
    sin->sin6_port = port;
    memcpy( &(sin->sin6_addr.s6_addr), ipaddr,
            sizeof(struct in6_addr) );

    return true;
}


bool sockaddr_sameAddr(const struct sockaddr * a,
                       const struct sockaddr * b)
{
    if( a->sa_family == b->sa_family ){
        if (a->sa_family == AF_INET) {
            const struct sockaddr_in *a4 = (const struct sockaddr_in *)a;
            const struct sockaddr_in *b4 = (const struct sockaddr_in *)b;

            if (a4->sin_addr.s_addr == b4->sin_addr.s_addr){
                return true;

            }
        }else if (a->sa_family == AF_INET6) {
            const struct sockaddr_in6 *a6 = (const struct sockaddr_in6 *)a;
            const struct sockaddr_in6 *b6 = (const struct sockaddr_in6 *)b;

            if ( (memcmp(&(a6->sin6_addr.s6_addr), &(b6->sin6_addr.s6_addr),
                         sizeof(struct in6_addr)) == 0))
            {
                return true;
            }
        }
    }
    return false;
}

bool sockaddr_samePort(const struct sockaddr * a,
                       const struct sockaddr * b)
{
    if ( a->sa_family == b->sa_family ) {
        if (a->sa_family == AF_INET) {
            const struct sockaddr_in *a4 = (const struct sockaddr_in *)a;
            const struct sockaddr_in *b4 = (const struct sockaddr_in *)b;

            if (a4->sin_port == b4->sin_port){
                return true;

            }
        }else if (a->sa_family == AF_INET6) {
            const struct sockaddr_in6 *a6 = (const struct sockaddr_in6 *)a;
            const struct sockaddr_in6 *b6 = (const struct sockaddr_in6 *)b;

            if (a6->sin6_port == b6->sin6_port) {
                return true;
            }
        }
    }
    return false;
}

unsigned int
sockaddr_ipPort (const struct sockaddr * a)
{
    if ( a && a->sa_family) {
        if (a->sa_family == AF_INET) {
            const struct sockaddr_in *a4 = (const struct sockaddr_in *)a;
            return ntohs (a4->sin_port);
        }else if (a->sa_family == AF_INET6) {
            const struct sockaddr_in6 *a6 = (const struct sockaddr_in6 *)a;
            return ntohs (a6->sin6_port);
        }
    }
    return 0;
}

bool sockaddr_alike(const struct sockaddr * a,
                    const struct sockaddr * b)
{
    if (a && b && sockaddr_sameAddr(a,b) && sockaddr_samePort(a,b)){
        return true;
    }

    return false;
}

bool sockaddr_isSet(const struct sockaddr * sa)
{
    if(sa && (sa->sa_family == AF_INET || sa->sa_family == AF_INET6)) {
        return true;
    }

    return false;
}


bool sockaddr_isAddrAny(const struct sockaddr * sa)
{
    if (sa && (sa->sa_family == AF_INET)) {
        return ( ((struct sockaddr_in*)sa)->sin_addr.s_addr == htonl(INADDR_ANY) );

    }else if (sa && (sa->sa_family == AF_INET6)) {
        return IN6_IS_ADDR_UNSPECIFIED(&((struct sockaddr_in6*)sa)->sin6_addr);
    }

    return false;
}


bool sockaddr_isAddrLoopBack(const struct sockaddr * sa)
{
    if (sa->sa_family == AF_INET) {
        return ( ((struct sockaddr_in*)sa)->sin_addr.s_addr == htonl(INADDR_LOOPBACK) );
    }else if (sa->sa_family == AF_INET6) {
        return IN6_IS_ADDR_LOOPBACK(&((struct sockaddr_in6*)sa)->sin6_addr);
    }
    return false;
}

bool sockaddr_isAddrPrivate(const struct sockaddr * sa)
{
    /*192.168.0.0/16 network*/
    uint32_t private_192 = 0xC0A80000;
    uint32_t mask_16 = 0xFFFF0000;
    /*172.16.0.0/20 network*/
    uint32_t private_172 = 0xAC100000;
    uint32_t mask_12 = 0xFFF00000;
    /*10.0.0.0/24 network*/
    uint32_t private_10 = 0x0A000000;
    uint32_t mask_8 = 0xFF000000;

    if (sa->sa_family == AF_INET) {
        if( (htonl(((struct sockaddr_in*)sa)->sin_addr.s_addr) & mask_16) == private_192 ) {
            return true;
        }
        if( (htonl(((struct sockaddr_in*)sa)->sin_addr.s_addr) & mask_12) == private_172) {
            return true;
        }
        if( (htonl(((struct sockaddr_in*)sa)->sin_addr.s_addr) & mask_8) == private_10) {
            return true;
        }
        return false;

    }else if (sa->sa_family == AF_INET6) {
        return false;
    }
    return false;
}

bool sockaddr_isAddrLinkLocal(const struct sockaddr * sa)
{
    if (sa->sa_family == AF_INET) {
        return false;
    }else if (sa->sa_family == AF_INET6) {
        return IN6_IS_ADDR_LINKLOCAL(&((struct sockaddr_in6*)sa)->sin6_addr);
    }
    return false;
}

bool sockaddr_isAddrSiteLocal(const struct sockaddr * sa)
{
    if (sa->sa_family == AF_INET) {
        return false;
    }else if (sa->sa_family == AF_INET6) {
        return IN6_IS_ADDR_SITELOCAL(&((struct sockaddr_in6*)sa)->sin6_addr);
    }
    return false;
}

#define	SOCKADDR_IN6_IS_ADDR_ULA(a)	\
   (((a)->s6_addr[0] & 0xFE)  == 0xFC)


bool sockaddr_isAddrULA(const struct sockaddr * sa)
{
    if (sa->sa_family == AF_INET) {
        return false;
    }else if (sa->sa_family == AF_INET6) {
        return SOCKADDR_IN6_IS_ADDR_ULA(&((struct sockaddr_in6*)sa)->sin6_addr);
    }
    return false;
}

int sockaddr_getIPv6Flags(const struct sockaddr * sa, const char* ifa_name, int ifa_len)
{
#if defined(__APPLE__)
    struct sockaddr_in6 *sin;
    struct in6_ifreq ifr6;
    int s6;
    
    sin = (struct sockaddr_in6 *)sa;
    strncpy(ifr6.ifr_name, ifa_name, ifa_len);
    
    if ((s6 = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        return 0;
    }
    ifr6.ifr_addr = *sin;
    if (ioctl(s6, SIOCGIFAFLAG_IN6, &ifr6) < 0) {
        close(s6);
        return 0;
    }
    
    return ifr6.ifr_ifru.ifru_flags6;
#else
return 0;
#endif
}


bool sockaddr_isAddrTemporary(const struct sockaddr * sa, const char* ifa_name, int ifa_len)
{
#if defined(__APPLE__)
    int flags6;

    if (sa->sa_family == AF_INET) {
        return false;
    }else if (sa->sa_family == AF_INET6) {
        flags6 = sockaddr_getIPv6Flags(sa, ifa_name,ifa_len);
            
        if(flags6 == 0)
            return false;
        
        if ((flags6 & IN6_IFF_TEMPORARY) != 0){
            return true;
        }
    }
    return false;
#else
    return false;
#endif
}


bool sockaddr_isAddrDeprecated(const struct sockaddr * sa, const char* ifa_name, int ifa_len)
{
#if defined(__APPLE__)
    int flags6;

    if (sa->sa_family == AF_INET) {
        return false;
    }else if (sa->sa_family == AF_INET6) {
        flags6 = sockaddr_getIPv6Flags(sa, ifa_name,ifa_len);
            
        if(flags6 == 0)
            return false;
        
        if ((flags6 & IN6_IFF_DEPRECATED) != 0){
            return true;
        }
    }
    return false;
#else
    return false;
#endif
}




const char *sockaddr_toString( const struct sockaddr *sa,
                               char *dest,
                               size_t destlen,
                               bool addport)
{
    if (sa->sa_family == AF_INET) {
        if (destlen < INET_ADDRSTRLEN + 8) { /* 8 is enough for :port and termination */
                 dest[0] = '\0';
                 return dest;
        }
        else
        {
            const struct sockaddr_in *sa4 = (const struct sockaddr_in *)sa;

            inet_ntop(AF_INET, &(sa4->sin_addr), dest, destlen);
            if(addport){
                int r = strlen(dest);
                sprintf(dest + r, ":%d", ntohs(sa4->sin_port));
            }
            return dest;
        }
    }else if (sa->sa_family == AF_INET6) {
        if (destlen < SOCKADDR_MAX_STRLEN) {
            dest[0] = '\0';
            return dest;
        }
        else
        {
            int r;
            const struct sockaddr_in6 *sa6 = (const struct sockaddr_in6 *)sa;
            if (addport){
                dest[0] = '[';
                inet_ntop(AF_INET6, &(sa6->sin6_addr), dest+1, destlen);
            }else{
                inet_ntop(AF_INET6, &(sa6->sin6_addr), dest, destlen);
            }
            r = strlen(dest);

            if (addport)
                dest[r++] = ']';
            if (addport){
                sprintf(dest + r, ":%d", ntohs(sa6->sin6_port));
            }else {
                dest[r] = '\0';
            }
            return dest;
        }
    }

    return NULL;
}

void sockaddr_copy(struct sockaddr * dst,
                   const struct sockaddr * src)
{

    if (src) {
        if (src->sa_family == AF_INET) {
            struct sockaddr_in *dst4 = (struct sockaddr_in *)dst;
            const struct sockaddr_in *src4 = (const struct sockaddr_in *)src;

            dst4->sin_family = AF_INET;
            dst4->sin_port = src4->sin_port;
            dst4->sin_addr.s_addr = src4->sin_addr.s_addr;

        }else if (src->sa_family == AF_INET6) {
            struct sockaddr_in6 *dst6 = (struct sockaddr_in6 *)dst;
            const struct sockaddr_in6 *src6 = (const struct sockaddr_in6 *)src;

            dst6->sin6_family = AF_INET6;
            dst6->sin6_port = src6->sin6_port;

            memcpy(&(dst6->sin6_addr.s6_addr), &(src6->sin6_addr.s6_addr),
                   sizeof(struct in6_addr));
        }
    }
}

void sockaddr_setPort(struct sockaddr * sa, uint16_t port)
{
    if (sa->sa_family == AF_INET) {
        struct sockaddr_in *sa4 = (struct sockaddr_in *)sa;
        sa4->sin_port = htons(port);

    }else if (sa->sa_family == AF_INET6) {
        struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)sa;
        sa6->sin6_port = htons(port);
    }
}
