#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define PORT 9090
#define BUFFER_SIZE 1024
#define CACHE_SIZE 10
#define TIMEOUT_SECONDS 5


// Structure for DNS Cache
struct Cache
{
    char domain[100];
    char ip[50];
};


// DNS Cache
struct Cache cache[CACHE_SIZE];

int cache_count = 0;


// Check domain format
int isValidDomain(char *domain)
{
    int length = strlen(domain);

    // Empty domain
    if (length == 0)
    {
        return 0;
    }

    // Domain must contain '.'
    if (strchr(domain, '.') == NULL)
    {
        return 0;
    }

    // Domain should not start or end with '.'
    if (domain[0] == '.' ||
        domain[length - 1] == '.')
    {
        return 0;
    }

    return 1;
}


// Check DNS cache
char *checkCache(char *domain)
{
    for (int i = 0; i < cache_count; i++)
    {
        if (strcmp(cache[i].domain, domain) == 0)
        {
            return cache[i].ip;
        }
    }

    return NULL;
}


// Add result to cache
void addToCache(char *domain, char *ip)
{
    if (cache_count < CACHE_SIZE)
    {
        strcpy(cache[cache_count].domain, domain);

        strcpy(cache[cache_count].ip, ip);

        cache_count++;
    }
}


int main()
{
    int sockfd;

    char domain[100];
    char buffer[BUFFER_SIZE];

    struct sockaddr_in server_addr;


    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        perror("Socket creation failed");

        exit(1);
    }


    // Server details
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;

    server_addr.sin_port = htons(PORT);

    server_addr.sin_addr.s_addr =
        inet_addr("127.0.0.1");


    while (1)
    {
        printf("\n====================================\n");
        printf("       UDP DNS CLIENT\n");
        printf("====================================\n");

        printf("Enter domain name (or exit): ");

        scanf("%99s", domain);


        // Exit condition
        if (strcmp(domain, "exit") == 0)
        {
            break;
        }


        // Validate domain
        if (!isValidDomain(domain))
        {
            printf("\nERROR: Invalid domain format!\n");

            printf("Example: google.com\n");

            continue;
        }


        // Check cache
        char *cached_ip = checkCache(domain);

        if (cached_ip != NULL)
        {
            printf("\nCACHE HIT!\n");

            printf("Domain     : %s\n", domain);

            printf("IP Address : %s\n", cached_ip);

            continue;
        }


        printf("\nCACHE MISS!\n");

        printf("Sending request to DNS server...\n");


        // Send domain to server
        if (sendto(
                sockfd,
                domain,
                strlen(domain),
                0,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)
            ) < 0)
        {
            perror("Send failed");

            continue;
        }


        // Setup timeout
        fd_set readfds;

        FD_ZERO(&readfds);

        FD_SET(sockfd, &readfds);


        struct timeval timeout;

        timeout.tv_sec = TIMEOUT_SECONDS;

        timeout.tv_usec = 0;


        // Wait for server response
        int result = select(
            sockfd + 1,
            &readfds,
            NULL,
            NULL,
            &timeout
        );


        // Timeout occurred
        if (result == 0)
        {
            printf("\nERROR: DNS request timed out!\n");

            continue;
        }


        // Select error
        if (result < 0)
        {
            perror("Select error");

            continue;
        }


        // Receive response
        socklen_t server_len =
            sizeof(server_addr);


        int bytes_received = recvfrom(
            sockfd,
            buffer,
            BUFFER_SIZE - 1,
            0,
            (struct sockaddr *)&server_addr,
            &server_len
        );


        if (bytes_received < 0)
        {
            perror("Receive failed");

            continue;
        }


        buffer[bytes_received] = '\0';


        // Check if domain exists
        if (strcmp(buffer, "NXDOMAIN") == 0)
        {
            printf("\nERROR: NXDOMAIN\n");

            printf("Domain '%s' not found in DNS server.\n",
                   domain);
        }
        else
        {
            printf("\nDNS Resolution Successful!\n");

            printf("Domain Name : %s\n", domain);

            printf("IP Address  : %s\n", buffer);


            // Store successful lookup in cache
            addToCache(domain, buffer);

            printf("\nResult added to cache.\n");
        }
    }


    close(sockfd);

    printf("\nDNS Client closed.\n");

    return 0;
}
