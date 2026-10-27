#include <stdio.h>
#include <string.h>
#include<stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int sock;
    struct sockaddr_in server_addr;

    char roll[20];
    char buffer[500] = {0};


    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);


    // Server details
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(3001);


    // Server IP
    inet_pton(AF_INET, "127.0.0.1",
              &server_addr.sin_addr);


    // Connect to server
    if (connect(sock,
            (struct sockaddr *)&server_addr,
            sizeof(server_addr)) < 0)
{
    perror("connect");
    return 1;
}

printf("Connected to server!\n");


    // Get roll number from user
    printf("Enter Roll Number: ");
    scanf("%s", roll);


    // Send roll number
    write(sock, roll, strlen(roll));


    // Receive response
    read(sock, buffer, sizeof(buffer));


    // Display result
    printf("\nStudent Details:\n");
    printf("%s\n", buffer);


    close(sock);


    return 0;
}
