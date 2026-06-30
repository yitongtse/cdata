#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main()
{
    struct hostent *host = gethostbyname("www.cisco.com");
    if (!host) {
        herror("gethostbyname");
        return 1;
    }
    printf("Host name : %s\n", host->h_name);

    int i;
    uint32_t ip_addr;
    struct in_addr **addr = (struct in_addr **)host->h_addr_list;
    for (i=0; addr[i]; i++) {
        inet_pton(AF_INET, inet_ntoa(*addr[i]), &ip_addr);
        printf("IP address: %s (%08x)\n", inet_ntoa(*addr[i]), htonl(ip_addr));
        return 0;
    }
    return 1;
}
