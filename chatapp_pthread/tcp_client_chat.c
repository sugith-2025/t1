
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

int s;

int read_line(int sock, char *m, int size)
{
    int i = 0, n;
    char ch;

    while (i < size - 1) {
        n = recv(sock, &ch, 1, 0);
        if (n <= 0) return 0;
        if (ch == '\n') break;
        m[i++] = ch;
    }

    m[i] = '\0';
    return 1;
}

void *receive(void *arg)
{
    char m[100];

    while (read_line(s, m, sizeof(m))) {
        printf("\nServer: %s\n", m);
        if (strcmp(m, "exit") == 0) break;
    }

    return NULL;
}

void send_line(char *m)
{
    char out[102];
    snprintf(out, sizeof(out), "%s\n", m);
    send(s, out, strlen(out), 0);
}

int main()
{
    struct sockaddr_in a = {0};
    pthread_t t;
    char id[50], type[50], issue[100], m[100];

    s = socket(AF_INET, SOCK_STREAM, 0);

    a.sin_family = AF_INET;
    a.sin_port = htons(8080);
    a.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(s, (struct sockaddr *)&a, sizeof(a)) < 0) {
        perror("connect");
        close(s);
        return 1;
    }

    printf("Enter User ID: ");
    fgets(id, sizeof(id), stdin);
    id[strcspn(id, "\n")] = '\0';

    printf("Enter Issue Type: ");
    fgets(type, sizeof(type), stdin);
    type[strcspn(type, "\n")] = '\0';

    printf("Enter Issue: ");
    fgets(issue, sizeof(issue), stdin);
    issue[strcspn(issue, "\n")] = '\0';

    send_line(id);
    send_line(type);
    send_line(issue);

    if (!read_line(s, m, sizeof(m))) {
        printf("Server disconnected.\n");
        close(s);
        return 1;
    }

    printf("Server: %s\n", m);

    pthread_create(&t, NULL, receive, NULL);

    while (1) {
        printf("You: ");

        if (fgets(m, sizeof(m), stdin) == NULL) break;

        m[strcspn(m, "\n")] = '\0';
        send_line(m);

        if (strcmp(m, "exit") == 0) break;
    }

    shutdown(s, SHUT_RDWR);
    pthread_join(t, NULL);
    close(s);

    return 0;
}
