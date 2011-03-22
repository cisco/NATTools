#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>


#include "sockaddr_util.h"




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
                              uint8_t ipaddr[16],
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

bool sockaddr_alike(const struct sockaddr * a,
                    const struct sockaddr * b)
{
    if (sockaddr_sameAddr(a,b) && sockaddr_samePort(a,b)){
        return true;
    }

    return false;
}

bool sockaddr_isSet(const struct sockaddr * sa)
{
    if(sa->sa_family == AF_INET || sa->sa_family == AF_INET6){
        return true;
    }

    return false;
}


bool sockaddr_isAddrAny(const struct sockaddr * sa)
{
    if (sa->sa_family == AF_INET) {
        return ( ((struct sockaddr_in*)sa)->sin_addr.s_addr == htonl(INADDR_ANY) );

    }else if (sa->sa_family == AF_INET6) {
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

bool sockaddr_isAddrLinkLocal(const struct sockaddr * sa)
{
    if (sa->sa_family == AF_INET) {
        return false;
    }else if (sa->sa_family == AF_INET6) {
        return IN6_IS_ADDR_LINKLOCAL(&((struct sockaddr_in6*)sa)->sin6_addr);
    }
    return false;
}


const char *sockaddr_toString( const struct sockaddr *sa,
                               char *dest,
                               size_t destlen,
                               bool addport)
{
    if (destlen < SOCKADDR_MAX_STRLEN) {
        dest[0] = '\0';
        return dest;
    }

    if (sa->sa_family == AF_INET) {
        const struct sockaddr_in *sa4 = (const struct sockaddr_in *)sa;

        inet_ntop(AF_INET, &(sa4->sin_addr), dest, destlen);
        if(addport){
            int r = strlen(dest);
            sprintf(dest + r, ":%d", ntohs(sa4->sin_port));
        }
        return dest;

    }else if (sa->sa_family == AF_INET6) {
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

    return NULL;
}

void sockaddr_copy(struct sockaddr * dst,
                   const struct sockaddr * src)
{
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
