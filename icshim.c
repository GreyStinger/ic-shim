#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFER_LEN 4096
#define IS_COMMENT(buffer) ((buffer[0] == '/') && (buffer[1] == '/'))
#define IS_BLANK(buffer) (buffer[0] == '\0')

int filterhost(char* host)
{   
    FILE* fp;
    const int bufferLength = 255;
    char buffer[bufferLength];

    fp = fopen("./regs.dat", "r");

    if (fp == NULL)
    {
        printf("ERROR: Couldn't find regs.dat file\n");
        fclose(fp);

        return 0;
    }

    while(fgets(buffer, bufferLength, fp))
    {
        printf("Checking %s", buffer);
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (IS_COMMENT(buffer) || IS_BLANK(buffer)) continue;

        printf("Checking %s\n", buffer);

        if (strstr(host, buffer))
        {
            fclose(fp);

            printf("Blocking connection to: %s with key: %s\n", host, buffer);

            return 1;
        }
    }
    
    return 0;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int (*original_connect)(int, const struct sockaddr*, socklen_t) = NULL;
    original_connect = dlsym(RTLD_NEXT, "connect");

    struct sockaddr_in *myaddr = (struct sockaddr_in*)addr;
    struct sockaddr_in6 *myaddr6 = (struct sockaddr_in6*)addr;

    char *fam = "OTHER";

    if (addr->sa_family == AF_INET)
        fam = "AF_INET";
    if (addr->sa_family == AF_INET6)
        fam = "AF_INET6";
    
    char addr_str[BUFFER_LEN] = {0};

    if (addr->sa_family == AF_INET)
    {
        inet_ntop(addr->sa_family, &(myaddr->sin_addr.s_addr), addr_str, BUFFER_LEN);
    }
    else if (addr->sa_family == AF_INET6)
    {
        inet_ntop(addr->sa_family, &(myaddr6->sin6_addr.s6_addr), addr_str, BUFFER_LEN);
    }

    char host[BUFFER_LEN] = {0};
    char server[BUFFER_LEN] = {0};

    getnameinfo(addr, addrlen, host, BUFFER_LEN, server, BUFFER_LEN, 0);

    // Pass call without printing to exclude all uncared-for hosts
    if (strstr(host, "sylvie") || strstr(host, "_gateway"))
        return original_connect(sockfd, addr, addrlen);

    if (filterhost(host))
    {
        errno = ECONNREFUSED;
        return -1;
    }

    printf("%s (%s) %s\n", host, fam, addr_str);
 
    return original_connect(sockfd, addr, addrlen);
}