
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX 500
#define POLY "10011"

void divide(char *data, char *poly) {
    int i, j, n = strlen(data), p = strlen(poly);
    for (i = 0; i <= n-p; i++)
        if (data[i] == '1')
            for (j = 0; j < p; j++)
                data[i+j] = data[i+j] == poly[j] ? '0' : '1';
}

int receive_all(int c, void *buf, int size) {
    int got = 0, n;
    while (got < size) {
        n = recv(c, (char *)buf + got, size-got, 0);
        if (n <= 0) return 0;
        got += n;
    }
    return 1;
}

int receive_frame(int c, char *frame) {
    int i = 0;
    while (i < MAX-1) {
        if (!receive_all(c, &frame[i], 1)) return 0;
        if (frame[i++] == '\0') return 1;
    }
    return 0;
}

int main() {
    int s, c, seq, i, error;
    char frame[MAX], temp[MAX], ack[100];
    struct sockaddr_in server = {0};

    s = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind"); return 1;
    }
    listen(s, 5);
    printf("Receiver running...\n");
    c = accept(s, NULL, NULL);
    if (c < 0) { perror("accept"); return 1; }

    for (i = 0; i < 3; i++) {
        if (!receive_all(c, &seq, sizeof(seq)) ||
            !receive_frame(c, frame)) break;

        printf("\nSensor Sequence: %d\nReceived: %s\n", seq, frame);
        strcpy(temp, frame);
        divide(temp, POLY);

        error = 0;
        for (int j = 0; j < strlen(temp); j++)
            if (temp[j] == '1') error = 1;

        strcpy(ack, error ? "ERROR DETECTED" : "No error detected");
        printf("CRC Result: %s\n", ack);
        send(c, ack, strlen(ack)+1, 0);
    }

    close(c);
    close(s);
    return 0;
}
