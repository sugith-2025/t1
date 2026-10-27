#include <stdio.h>
#include <string.h>
#include<stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

void *handle_client(void *arg)
{
    printf("Thread started\n");
    fflush(stdout);
    // TAKE THE CLIENT SOCKET HERE
    int client_fd = *(int *)arg;

    // free the dynamically allocated memory
    free(arg);

    char roll[20] = {0};

    int n = read(client_fd, roll, sizeof(roll) - 1);
    roll[n] = '\0';

    printf("Received: %s\n", roll);
    fflush(stdout);
    
    // Fetching the details
    FILE *fp;
    char line[200];
    char response[500];
    
    int found =0;
    
    fp=fopen("students.csv","r");
    if (fp == NULL) {
        perror("fopen");
        close(client_fd);
        return NULL;
    }

    printf("CSV opened\n");
    fflush(stdout);
    
    fgets(line,sizeof(line),fp);
    
    while(fgets(line,sizeof(line),fp)){
        char *csv_roll;
        char *name;
        char *dept;
        char *semester;
        char *cgpa;


    // Split row using comma
        csv_roll = strtok(line, ",");

        name = strtok(NULL, ",");
        dept = strtok(NULL, ",");
        semester = strtok(NULL, ",");
        cgpa = strtok(NULL, ",");

        printf("Checking %s\n", csv_roll);
        fflush(stdout);

    // Compare requested roll number
       if(strcmp(csv_roll, roll) == 0)
       {
	    found = 1;

	    sprintf(response,
	        "Roll Number: %s\nName: %s\nDepartment: %s\nSemester: %s\nCGPA: %s",
	        csv_roll,
	        name,
	        dept,
	        semester,
	        cgpa);

	    break;
       }
    }
    
    fclose(fp);
    
    if(found==0)
    	strcpy(response,"Student Record Not Found");
    
    printf("Sending response\n");
    fflush(stdout);

    write(client_fd, response, strlen(response));

    close(client_fd);
    
    return NULL;
}

int main() {

   
    int server_fd, client_fd;
    struct sockaddr_in server_addr;
    char roll[20] = {0};
    
    // create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);                     


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

    // Listen
    if (listen(server_fd, 5) < 0)
{
    perror("listen");
    return 1;
}

    printf("Server waiting...\n");

    // Accepting the client
    while(1)
	{
        printf("Inside while loop\n");
	    client_fd = accept(server_fd,NULL,NULL);
        printf("Client connected\n");
        fflush(stdout);


	    pthread_t tid;

	    int *new_socket = malloc(sizeof(int));

	    *new_socket = client_fd;

	    pthread_create(
	        &tid,
	        NULL,
	        handle_client,
	        new_socket
	    );


	    pthread_detach(tid);
	}
    

    close(server_fd);

    return 0;
}
