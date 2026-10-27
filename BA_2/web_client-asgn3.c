#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<string.h>
#include <netdb.h>
#include<time.h>

void main(){
	int sock;

	
	struct sockaddr_in server_addr;
	char url[100]={0};
	int port=80;
	char hostport[256];
	char hostname[256]={0};
	char path[256]={0};
	
	printf("Enter the URL: ");
	scanf("%s",url);
	
	printf("Web Server URL: %s\n",url);
	
	int parsed = sscanf(url, "%*[^:]://%255[^/]%255s", hostport, path);
	if (parsed != 2)
	    strcpy(path, "/");

	
	if (parsed >= 1) {

		char *colon = strchr(hostport, ':');

		if (colon)
		{
			*colon = '\0';
			strcpy(hostname, hostport);
			port = atoi(colon + 1);
		}
		else
		{
			strcpy(hostname, hostport);
		}

		printf("Hostname: %s\n", hostname);
		printf("Port:     %d\n", port);
		printf("Path:     %s\n", path);
	} else {
		printf("Failed to parse URL.\n");
	}
	
	struct hostent *server;

	server = gethostbyname(hostname);

	if(server == NULL)
	{
	    printf("Host not found\n");
	    exit(1);
	}

	memcpy(&server_addr.sin_addr,
	       server->h_addr,
	       server->h_length);
	
	server_addr.sin_port=htons(port);
	server_addr.sin_family = AF_INET;
	
	// Create TCP Socket 
	sock=socket(AF_INET,SOCK_STREAM,0);
	if(sock<0) {
		perror("Socket creation");
		exit(1);
	}
	
	struct timespec start_connect, end_connect;
	clock_gettime(CLOCK_MONOTONIC, &start_connect);
	if(connect(sock,(struct sockaddr *)&server_addr,sizeof(server_addr))<0){
		perror("Connect");
		exit(1);
	}
	clock_gettime(CLOCK_MONOTONIC, &end_connect);

	double connection_time = (end_connect.tv_sec - start_connect.tv_sec) + (end_connect.tv_nsec - start_connect.tv_nsec) / 1e9;
	
	char request[512];

	snprintf(request,sizeof(request),
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Connection: close\r\n"
		"\r\n",
		path,
		hostname);
		
	printf("%s", request);
	
	struct timespec send_time,first_receive_time,end_download_time;

	clock_gettime(CLOCK_MONOTONIC, &send_time);

	if(send(sock, request, strlen(request), 0) < 0)
	{
	    perror("Send");
	    exit(1);
	}
	
	char buffer[1024];
	int bytes;
	char filename[100];

	char *p = strrchr(path,'/');

	if(p && strlen(p+1)>0)
		strcpy(filename,p+1);
	else
		strcpy(filename,"downloaded_file");

	FILE *fp = fopen(filename,"wb");
	int header_done = 0;
	if(fp==NULL) {
		perror("File");
		exit(1);
	}
	int first_packet=1,total_bytes=0;
	while ((bytes = recv(sock, buffer, sizeof(buffer), 0)) > 0)
	{
		if(first_packet)
		{
			clock_gettime(CLOCK_MONOTONIC, &first_receive_time);
			first_packet = 0;
		}

		if(!header_done)
		{
			char *body = strstr(buffer, "\r\n\r\n");

			if(body)
			{
				header_done = 1;
				body += 4;

				int body_size = bytes - (body - buffer);

				fwrite(body, 1, body_size, fp);
				total_bytes += body_size;
			}
		}
		else
		{
			fwrite(buffer, 1, bytes, fp);
			total_bytes += bytes;
		}
	}
	if(bytes < 0)
	{
	    perror("recv");
	}
	clock_gettime(CLOCK_MONOTONIC, &end_download_time);
	double response_time =(first_receive_time.tv_sec - send_time.tv_sec) + (first_receive_time.tv_nsec - send_time.tv_nsec)/ 1e9;
	double download_time =(end_download_time.tv_sec - first_receive_time.tv_sec) +(end_download_time.tv_nsec - first_receive_time.tv_nsec)/1e9;
	double throughput = total_bytes / download_time;
	fclose(fp);
	close(sock);
	
	printf("Response downloaded in %s\n", filename);
	printf("\n------ Performance ------\n");

	printf("Connection Time : %.6f sec\n", connection_time);

	printf("Response Time   : %.6f sec\n", response_time);

	printf("Download Time   : %.6f sec\n", download_time);

	printf("Downloaded Size : %d bytes\n", total_bytes);

	printf("Throughput      : %.2f KB/s\n",
	       throughput/1024.0);

	
}

