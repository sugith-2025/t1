#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8081
#define BUFFER_SIZE 2048

// CRC division
void crc_division(char data[], char generator[],
                  char remainder[])
{
    int data_len = strlen(data);
    int gen_len = strlen(generator);

    char temp[BUFFER_SIZE];

    strcpy(temp, data);

    for (int i = 0; i <= data_len - gen_len; i++)
    {
        if (temp[i] == '1')
        {
            for (int j = 0; j < gen_len; j++)
            {
                temp[i + j] =
                    (temp[i + j] == generator[j])
                    ? '0'
                    : '1';
            }
        }
    }

    // Last gen_len - 1 bits are the remainder
    strncpy(remainder,
            temp + data_len - gen_len + 1,
            gen_len - 1);

    remainder[gen_len - 1] = '\0';
}

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;

    socklen_t addr_size =
        sizeof(client_addr);

    char frame[BUFFER_SIZE];
    char generator[BUFFER_SIZE];

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

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
    listen(server_fd, 5);

    printf("CRC Server waiting for connection...\n");

    // Accept
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

    // Receive generator
    recv(client_fd, generator,
         sizeof(generator), 0);

    // Receive frame
    recv(client_fd, frame,
         sizeof(frame), 0);

    printf("\n--- RECEIVER ---\n");

    printf("Generator: %s\n", generator);

    printf("Received Frame: %s\n", frame);

    // Ask whether to introduce error
    int choice;

    printf("\nDo you want to introduce a bit error?\n");
    printf("1. Yes\n");
    printf("2. No\n");
    printf("Enter choice: ");

    scanf("%d", &choice);

    if (choice == 1)
    {
        int position;

        printf("\nEnter bit position to flip ");
        printf("(0 to %lu): ",
               strlen(frame) - 1);

        scanf("%d", &position);

        if (position >= 0 &&
            position < strlen(frame))
        {
            if (frame[position] == '0')
                frame[position] = '1';
            else
                frame[position] = '0';

            printf("Frame after introducing error:\n");
            printf("%s\n", frame);
        }
        else
        {
            printf("Invalid position.\n");
        }
    }

    // Calculate CRC
    char remainder[BUFFER_SIZE];

    crc_division(frame,
                 generator,
                 remainder);

    printf("\nCRC Remainder = %s\n",
           remainder);

    // Check remainder
    int error = 0;

    for (int i = 0; i < strlen(remainder); i++)
    {
        if (remainder[i] != '0')
        {
            error = 1;
            break;
        }
    }

    if (error)
    {
        printf("\nRESULT: ERROR DETECTED!\n");
        printf("Received frame contains errors.\n");
    }
    else
    {
        printf("\nRESULT: NO ERROR DETECTED.\n");
        printf("Received frame is ERROR-FREE.\n");
    }

    close(client_fd);
    close(server_fd);

    return 0;
}
