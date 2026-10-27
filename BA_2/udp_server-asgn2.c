#include <stdio.h>
#include <string.h>
#include<stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>


void *handle_client(void *arg)
{
}

int main() {

   
    int server_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char roll[20] = {0};
    
    // create socket
    server_fd = socket(AF_INET, SOCK_DGRAM, 0);                     


    server_addr.sin_family = AF_INET;                                           
    server_addr.sin_addr.s_addr = INADDR_ANY;   

    server_addr.sin_port = htons(3001);

    // Bind IP + Port
    if (bind(server_fd, (struct sockaddr *)&server_addr,
         sizeof(server_addr)) < 0)
    {
        perror("bind");
        return 1;
    }

    
    printf("Server waiting...\n");

    // Accepting the client
    while (1)
    {
        char roll[20] = {0};

        int n = recvfrom(server_fd,
                        roll,
                        sizeof(roll) - 1,
                        0,
                        (struct sockaddr *)&client_addr,
                        &client_len);

        roll[n] = '\0';

        printf("Received: %s\n", roll);

        FILE *fp;
        char line[200];
        char response[500];

        int found = 0;

        fp = fopen("students.csv", "r");

        if (fp == NULL)
        {
            perror("fopen");
            continue;
        }

        fgets(line, sizeof(line), fp);

        while (fgets(line, sizeof(line), fp))
        {
            char *csv_roll;
            char *name;
            char *dept;
            char *semester;
            char *cgpa;

            csv_roll = strtok(line, ",");
            name = strtok(NULL, ",");
            dept = strtok(NULL, ",");
            semester = strtok(NULL, ",");
            cgpa = strtok(NULL, ",");

            if (strcmp(csv_roll, roll) == 0)
            {
                found = 1;

                sprintf(response,
                        "Roll Number: %s\n"
                        "Name: %s\n"
                        "Department: %s\n"
                        "Semester: %s\n"
                        "CGPA: %s",
                        csv_roll,
                        name,
                        dept,
                        semester,
                        cgpa);

                break;
            }
        }

        fclose(fp);

        if (!found)
            strcpy(response, "Student Record Not Found");

        sendto(server_fd,
            response,
            strlen(response),
            0,
            (struct sockaddr *)&client_addr,
            client_len);
    }
    

    close(server_fd);

    return 0;
}
