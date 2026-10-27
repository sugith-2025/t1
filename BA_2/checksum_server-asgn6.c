#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// 1's complement addition of two 8-bit values
unsigned int ones_complement_add(unsigned int a, unsigned int b)
{
    unsigned int sum = a + b;

    // End-around carry
    if (sum > 255)
    {
        unsigned int carry = sum >> 8;
        sum = (sum & 0xFF) + carry;

        printf("    Carry = %u\n", carry);
        printf("    Sum after end-around carry = %02X\n", sum);
    }

    return sum;
}

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);

    unsigned char data[BUFFER_SIZE];
    unsigned char received_checksum;
    int data_len;

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    // Server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind
    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(server_fd);
        exit(1);
    }

    // Listen
    if (listen(server_fd, 5) < 0)
    {
        perror("Listen failed");
        close(server_fd);
        exit(1);
    }

    printf("Checksum Server waiting for connection...\n");

    // Accept client
    client_fd = accept(server_fd,
                       (struct sockaddr *)&client_addr,
                       &addr_size);

    if (client_fd < 0)
    {
        perror("Accept failed");
        close(server_fd);
        exit(1);
    }

    printf("Client connected.\n");

    // Receive data length
    recv(client_fd, &data_len, sizeof(data_len), 0);

    // Receive data
    recv(client_fd, data, data_len, 0);

    // Receive checksum
    recv(client_fd, &received_checksum,
         sizeof(received_checksum), 0);

    printf("\n--- RECEIVER ---\n");

    printf("Received Data: ");

    for (int i = 0; i < data_len; i++)
    {
        printf("%02X ", data[i]);
    }

    printf("\n");

    printf("Received Checksum: %02X\n",
           received_checksum);

    // Calculate sum
    unsigned int sum = 0;

    printf("\nIntermediate Addition:\n");

    for (int i = 0; i < data_len; i++)
    {
        printf("Adding %02X\n", data[i]);

        sum = ones_complement_add(sum, data[i]);

        printf("    Intermediate sum = %02X\n", sum);
    }

    // Add checksum
    printf("\nAdding received checksum %02X\n",
           received_checksum);

    sum = ones_complement_add(sum,
                              received_checksum);

    printf("Final sum = %02X\n", sum);

    // Complement
    unsigned char result = (~sum) & 0xFF;

    printf("1's complement of final sum = %02X\n",
           result);

    if (result == 0)
    {
        printf("\nRESULT: No error detected.\n");
        printf("Received data is ERROR-FREE.\n");
    }
    else
    {
        printf("\nRESULT: Error detected!\n");
    }

    close(client_fd);
    close(server_fd);

    return 0;
}
