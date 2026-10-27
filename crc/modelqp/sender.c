
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX 500

void divide(char *data, char *poly) {
    int i, j, n = strlen(data), p = strlen(poly);
    for (i = 0; i <= n-p; i++)
        if (data[i] == '1')
            for (j = 0; j < p; j++)
                data[i+j] = data[i+j] == poly[j] ? '0' : '1';
}

int main() {
    int s, i, j, seq, choice, err;
    char data[100], bits[MAX], temp[MAX], frame[MAX], ack[100];
    char *poly[] = {"1011", "1101", "11001", "1001"};
    struct sockaddr_in server;

    s = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect"); return 1;
    }

    printf("Choose polynomial (1-4): ");
    scanf("%d", &choice);
    if (choice < 1 || choice > 4) return 1;

    for (seq = 1; seq <= 3; seq++) {
        printf("Sensor %d data: ", seq);
        scanf("%99s", data);
        bits[0] = '\0';

        for (i = 0; data[i]; i++)
            for (j = 7; j >= 0; j--) {
                int n = strlen(bits);
                bits[n] = ((unsigned char)data[i] >> j & 1) + '0';
                bits[n+1] = '\0';
            }

        strcpy(temp, bits);
        for (i = 0; i < strlen(poly[choice-1])-1; i++)
            strcat(temp, "0");
        divide(temp, poly[choice-1]);

        strcpy(frame, bits);
        strncat(frame, temp+strlen(bits), strlen(poly[choice-1])-1);

        printf("Sequence %d | Sent frame: %s\n", seq, frame);
        printf("Inject error? (1=yes, 0=no): ");
        scanf("%d", &err);
        if (err) frame[0] = frame[0] == '0' ? '1' : '0';

        send(s, &seq, sizeof(seq), 0);
        send(s, frame, strlen(frame)+1, 0);
        if (recv(s, ack, sizeof(ack)-1, 0) > 0)
            printf("Receiver: %s\n", ack);
    }
    close(s);
    return 0;
}
