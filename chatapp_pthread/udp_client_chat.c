
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

int s;
struct sockaddr_in server;
socklen_t len = sizeof(server);

void *receive(void *arg)
{
    char msg[100];

    while (1)
    {
        recvfrom(s, msg, sizeof(msg), 0,
                 (struct sockaddr *)&server, &len);

        printf("\nServer: %s\n", msg);

        if (strcmp(msg, "exit") == 0)
            break;
    }

    return NULL;
}

int main()
{
    pthread_t t;
    char id[50], type[50], issue[100], msg[100];

    s = socket(AF_INET, SOCK_DGRAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Enter User ID: ");
    fgets(id, sizeof(id), stdin);
    id[strcspn(id, "\n")] = '\0';

    printf("Enter Issue Type: ");
    fgets(type, sizeof(type), stdin);
    type[strcspn(type, "\n")] = '\0';

    printf("Enter Issue: ");
    fgets(issue, sizeof(issue), stdin);
    issue[strcspn(issue, "\n")] = '\0';

    sendto(s, id, strlen(id) + 1, 0,
           (struct sockaddr *)&server, len);
    sendto(s, type, strlen(type) + 1, 0,
           (struct sockaddr *)&server, len);
    sendto(s, issue, strlen(issue) + 1, 0,
           (struct sockaddr *)&server, len);

    recvfrom(s, msg, sizeof(msg), 0,
             (struct sockaddr *)&server, &len);

    printf("Server: %s\n", msg);

    pthread_create(&t, NULL, receive, NULL);

    while (1)
    {
        printf("Client: ");
        fgets(msg, sizeof(msg), stdin);
        msg[strcspn(msg, "\n")] = '\0';

        sendto(s, msg, strlen(msg) + 1, 0,
               (struct sockaddr *)&server, len);

        if (strcmp(msg, "exit") == 0)
            break;
    }

    pthread_cancel(t);
    pthread_join(t, NULL);
    close(s);

    return 0;
}
