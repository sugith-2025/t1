
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

int s;
struct sockaddr_in client;
socklen_t len = sizeof(client);

void *receive(void *arg)
{
    char msg[100];

    while (1)
    {
        recvfrom(s, msg, sizeof(msg), 0,
                 (struct sockaddr *)&client, &len);

        printf("\nClient: %s\n", msg);

        if (strcmp(msg, "exit") == 0)
            break;
    }

    return NULL;
}

int main()
{
    struct sockaddr_in server;
    pthread_t t;
    char id[50], type[50], issue[100], msg[100];

    s = socket(AF_INET, SOCK_DGRAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&server, sizeof(server));

    printf("Waiting for client...\n");

    recvfrom(s, id, sizeof(id), 0,
             (struct sockaddr *)&client, &len);
    recvfrom(s, type, sizeof(type), 0,
             (struct sockaddr *)&client, &len);
    recvfrom(s, issue, sizeof(issue), 0,
             (struct sockaddr *)&client, &len);

    printf("ID: %s\nType: %s\nIssue: %s\n",
           id, type, issue);

    sendto(s, "Issue received!", 16, 0,
           (struct sockaddr *)&client, len);

    pthread_create(&t, NULL, receive, NULL);

    while (1)
    {
        printf("Server: ");
        fgets(msg, sizeof(msg), stdin);
        msg[strcspn(msg, "\n")] = '\0';

        sendto(s, msg, strlen(msg) + 1, 0,
               (struct sockaddr *)&client, len);

        if (strcmp(msg, "exit") == 0)
            break;
    }

    pthread_cancel(t);
    pthread_join(t, NULL);
    close(s);

    return 0;
}
