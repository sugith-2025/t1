#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9090
#define BUFFER_SIZE 1024

// DNS Database Structure
struct DNSRecord
{
    char domain[100];
    char ip[50];
};

int main()
{
    int sockfd;

    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    struct sockaddr_in server_addr, client_addr;

    socklen_t client_len;

    // DNS Table
    struct DNSRecord dns_table[] =
    {
        {"google.com", "142.250.195.14"},
        {"youtube.com", "142.250.77.206"},
        {"facebook.com", "157.240.22.35"},
        {"amazon.com", "205.251.242.103"},
        {"ssn.edu.in", "192.168.1.10"}
    };

    int number_of_records =
        sizeof(dns_table) / sizeof(dns_table[0]);

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    // Server address details
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(sockfd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(sockfd);
        exit(1);
    }

    printf("====================================\n");
    printf("       UDP DNS SERVER STARTED\n");
    printf("====================================\n");

    printf("Listening on port %d...\n\n", PORT);

    while (1)
    {
        client_len = sizeof(client_addr);

        memset(buffer, 0, BUFFER_SIZE);
        memset(response, 0, BUFFER_SIZE);

        // Receive domain name from client
        int bytes_received = recvfrom(
            sockfd,
            buffer,
            BUFFER_SIZE - 1,
            0,
            (struct sockaddr *)&client_addr,
            &client_len
        );

        if (bytes_received < 0)
        {
            perror("Receive failed");
            continue;
        }

        buffer[bytes_received] = '\0';

        printf("Client requested: %s\n", buffer);

        int found = 0;

        // Search domain in DNS table
        for (int i = 0; i < number_of_records; i++)
        {
            if (strcmp(buffer, dns_table[i].domain) == 0)
            {
                strcpy(response, dns_table[i].ip);

                found = 1;

                break;
            }
        }

        // Domain not found
        if (!found)
        {
            strcpy(response, "NXDOMAIN");
        }

        // Send response to client
        if (sendto(
                sockfd,
                response,
                strlen(response),
                0,
                (struct sockaddr *)&client_addr,
                client_len
            ) < 0)
        {
            perror("Send failed");
            continue;
        }

        printf("Response sent: %s\n\n", response);
    }

    close(sockfd);

    return 0;
}
