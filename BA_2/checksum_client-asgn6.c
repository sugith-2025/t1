#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// 1's complement addition
unsigned int ones_complement_add(unsigned int a,
                                 unsigned int b)
{
    unsigned int sum = a + b;

    if (sum > 255)
    {
        unsigned int carry = sum >> 8;

        printf("    Carry = %u\n", carry);

        sum = (sum & 0xFF) + carry;

        printf("    Sum after end-around carry = %02X\n",
               sum);
    }

    return sum;
}

int main()
{
    int sock;
    struct sockaddr_in server_addr;

    unsigned char data[BUFFER_SIZE];
    unsigned char checksum;

    int data_len;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    // Server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Localhost
    server_addr.sin_addr.s_addr =
        inet_addr("127.0.0.1");

    // Connect
    if (connect(sock,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        close(sock);
        exit(1);
    }

    printf("Connected to checksum server.\n");

    // Input
    printf("Enter data: ");

    char input[BUFFER_SIZE];

    fgets(input, BUFFER_SIZE, stdin);

    input[strcspn(input, "\n")] = '\0';

    data_len = strlen(input);

    // Convert each character to 8-bit word
    for (int i = 0; i < data_len; i++)
    {
        data[i] = (unsigned char)input[i];
    }

    printf("\n--- SENDER ---\n");

    printf("8-bit words:\n");

    for (int i = 0; i < data_len; i++)
    {
        printf("%02X ", data[i]);
    }

    printf("\n");

    // Calculate checksum
    unsigned int sum = 0;

    printf("\nIntermediate Addition:\n");

    for (int i = 0; i < data_len; i++)
    {
        printf("Adding %02X\n", data[i]);

        sum = ones_complement_add(sum, data[i]);

        printf("    Intermediate sum = %02X\n",
               sum);
    }

    // Generate checksum
    checksum = (~sum) & 0xFF;

    printf("\nFinal Sum = %02X\n", sum);

    printf("Checksum = %02X\n", checksum);

    // Send data length
    send(sock, &data_len, sizeof(data_len), 0);

    // Send data
    send(sock, data, data_len, 0);

    // Send checksum
    send(sock, &checksum,
         sizeof(checksum), 0);

    printf("\nData and checksum transmitted successfully.\n");

    close(sock);

    return 0;
}
