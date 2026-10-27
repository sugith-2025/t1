
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdlib.h>

int c;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

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

void save(char *type, char *id, char *issue)
{
    pthread_mutex_lock(&lock);

    FILE *f = fopen("issues.csv", "a");
    if (f != NULL) {
        fprintf(f, "\"%s\",\"%s\",\"%s\"\n", type, id, issue);
        fclose(f);
    }

    pthread_mutex_unlock(&lock);
}

void *handle_client(void *arg)
{
    char m[100];

    while (read_line(c, m, sizeof(m))) {
        printf("\nClient: %s\n", m);
        save("CHAT", "CLIENT", m);

        if (strcmp(m, "exit") == 0) break;
    }

    return NULL;
}

int main()
{
    int s;
    struct sockaddr_in a = {0};
    pthread_t t;
    char id[50], type[50], issue[100], m[100];

    s = socket(AF_INET, SOCK_STREAM, 0);
    a.sin_family = AF_INET;
    a.sin_port = htons(8080);
    a.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&a, sizeof(a));
    listen(s, 1);

    printf("Waiting for client...\n");
    c = accept(s, NULL, NULL);
    printf("Client connected!\n");

    if (!read_line(c, id, sizeof(id)) ||
        !read_line(c, type, sizeof(type)) ||
        !read_line(c, issue, sizeof(issue))) {
        close(c);
        close(s);
        return 1;
    }

    printf("ID: %s\nType: %s\nIssue: %s\n", id, type, issue);
    save(id, type, issue);

    send(c, "Issue saved!\n", 13, 0);

    pthread_create(&t, NULL, handle_client, NULL);

    while (fgets(m, sizeof(m), stdin)) {
        m[strcspn(m, "\n")] = '\0';

        char out[102];
        snprintf(out, sizeof(out), "%s\n", m);

        if (send(c, out, strlen(out), 0) <= 0) break;

        save("CHAT", "SERVER", m);

        if (strcmp(m, "exit") == 0) break;

        printf("Server: %s\n", m);
    }

    shutdown(c, SHUT_RDWR);
    pthread_join(t, NULL);
    close(c);
    close(s);
    pthread_mutex_destroy(&lock);

    return 0;
}
