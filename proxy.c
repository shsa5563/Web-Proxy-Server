
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include "util.h"

long int _global_timeout = 0;
void sendRecvFile(int sockfd1, int newsockfd, char* fileRequest, char* temp, char* t2, char* t3, struct sockaddr_in host_addr);
void* connection_handler(void* param1);


void linkPrefetch(char* fileNameStr, char* host1);

void error(char* msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char* argv[])
{
    pid_t pid;
    struct sockaddr_in addr_in, cli_addr, serv_addr;
    struct hostent* host;
    int sockfd, newsockfd;

    if (argc < 2)
        error("Please provide the arguments like below:\n./proxy <port_no> <timeout(s)>");

    printf("\n***** PROXY SERVER *****\n");

    if (argc == 3) {
        _global_timeout = strToInteger(argv[2]);
    }
    bzero((char*)&serv_addr, sizeof(serv_addr));
    bzero((char*)&cli_addr, sizeof(cli_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
            (const void*)&optval, sizeof(int))
        < 0)
        return -1;
    if (sockfd < 0)
        error("Problem in initializing socket");

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        error("Error on binding");

    listen(sockfd, 50);
    int clilen = sizeof(cli_addr);

accepting:

    newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);

    if (newsockfd < 0)
        error("Problem in accepting connection");

    pid = fork();
    if (pid == 0) {
        struct sockaddr_in host_addr;
        struct in_addr** addr_list;
        int flag = 0, newsockfd1, n, port = 0, i, sockfd1;
        char buffer[510], t1[300], t2[300], t3[10];
        char* temp = NULL;
        char requestFileName[500];
        memset(requestFileName, '\0', 500);
        bzero((char*)buffer, 500);
        recv(newsockfd, buffer, 500, 0);

        sscanf(buffer, "%s %s %s", t1, t2, t3);

        if (((strncmp(t1, "GET", 3) == 0)) && ((strncmp(t3, "HTTP/1.1", 8) == 0) || (strncmp(t3, "HTTP/1.0", 8) == 0)) && (strncmp(t2, "http://", 7) == 0)) {

            //printf("\n*****************************\nThis is what I recvd :\n%s",buffer);
            strcpy(t1, t2);

            sprintf(requestFileName, "%s", t1);

            flag = 0;
            //printf("\n--------------------------------------hostName:%s\n", requestFileName);

            for (i = 7; i < strlen(t2); i++) {
                if (t2[i] == ':') {
                    flag = 1;
                    break;
                }
            }
            int fontFlag = 0;
            temp = strtok(t2, "//");
            if (flag == 0) {
                port = 80;
                temp = strtok(NULL, "/");
            }
            else {
                //printf("strstr(temp,css):%s --------------------- \nstrstr(temp,fonts): %s\n", strstr(requestFileName,"css"),strstr(requestFileName,"fonts"));
                if ((strstr(requestFileName, "css") != NULL) || (strstr(requestFileName, "fonts") != NULL)) {
                    port = 80;
                    temp = strtok(NULL, "/");
                }
                else {
                    temp = strtok(NULL, ":");
                    fontFlag = 1;
                }
            }

            sprintf(t2, "%s", temp);
            printf("host = %s", t2);

            if ((flag == 1) && (fontFlag == 1)) {
                temp = strtok(NULL, "/");
                port = atoi(temp);
            }

            strcat(t1, "^]");
            temp = strtok(t1, "//");
            temp = strtok(NULL, "/");
            if (temp != NULL)
                temp = strtok(NULL, "^]");
            printf("\npath = %s\nPort = %d\n", temp, port);

            bzero((char*)&host_addr, sizeof(host_addr));
            host_addr.sin_port = htons(port);
            host_addr.sin_family = AF_INET;

            FILE* dnsCheck = NULL;
            if (!((dnsCheck = fopen("dnsList.txt", "rb+")) != NULL)) {
                if ((dnsCheck = fopen("dnsList.txt", "wb+")) != NULL) {
                    printf("\nCreated the DNS-List file\n");
                    host = gethostbyname(t2);
                    char tempBuff[500];
                    memset(tempBuff, '\0', 500);
                    sprintf(tempBuff, "%s %s %d \n", t2, inet_ntoa(*(struct in_addr*)host->h_addr), host->h_length);
                    fwrite(tempBuff, sizeof(char), strlen(tempBuff), dnsCheck);
                    //printf("\nwriting to the file : %s\n", tempBuff);
                    fclose(dnsCheck);
                }
            }
            else {
                int dnsMatch = 0;

                char* tempBuf1 = NULL;
                size_t len = 0;
                int counter = 0;
                while ((getline(&tempBuf1, &len, dnsCheck)) != -1) {
                    char tempBuf2[500];
                    memset(tempBuf2, '\0', 500);
                    char tempBuf3[500];
                    memset(tempBuf3, '\0', 500);
                    long int tempLen = 0, tempBuf5 = 0;
                    sscanf(tempBuf1, "%s %s %ld", tempBuf2, tempBuf3, &tempLen);
                    if (strcmp(tempBuf2, t2) == 0) {
                        if (inet_pton(AF_INET, (char*)tempBuf3, &host_addr.sin_addr) != 1) {
                            printf("Invalid IP Address\n");
                            exit(-1);
                        }
                        dnsMatch = 1;
                        //printf("\nFound the Line in the file : %s tempBuf2 : %s\n", tempBuf1,tempBuf2 );
                        break;
                    }
                    //printf("\nLine in the file is : %s\n", tempBuf1);
                    memset(tempBuf1, '\0', strlen(tempBuf1));
                }

                if (!dnsMatch) {
                    host = gethostbyname(t2);
                    char tempBuff[500];
                    memset(tempBuff, '\0', 500);
                    sprintf(tempBuff, "%s %s %d \n", t2, inet_ntoa(*(struct in_addr*)host->h_addr), host->h_length);
                    fwrite(tempBuff, sizeof(char), strlen(tempBuff), dnsCheck);
                    //printf("\nwriting to the file : %s --  %s\n", tempBuff, inet_ntoa(*(struct in_addr *)host->h_addr));

                    bcopy((char*)host->h_addr, (char*)&host_addr.sin_addr.s_addr, host->h_length);
                }
                fclose(dnsCheck);
            }

            sockfd1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            int optval = 1;
            if (setsockopt(sockfd1, SOL_SOCKET, SO_REUSEADDR,
                    (const void*)&optval, sizeof(int))
                < 0)
                return -1;

            FILE* blockedWebsites = NULL;
            int blockedFlag = 0;
            if ((blockedWebsites = fopen("blocked.txt", "rb")) != NULL) {
                char* tempBuf1 = NULL;
                size_t len = 0;
                while ((getline(&tempBuf1, &len, blockedWebsites)) != -1) {
                    char tempBuf2[500];
                    memset(tempBuf2, '\0', 500);
                    sscanf(tempBuf1, "%s", tempBuf2);
                    if (strcmp(tempBuf2, t2) == 0) {
                        send(newsockfd, "400 : BAD REQUEST\nONLY HTTP REQUESTS ALLOWED", 18, 0);
                        blockedFlag = 1;
                        break;
                    }
                }
                fclose(blockedWebsites);
            }
            if (!blockedFlag) {
                sendRecvFile(sockfd1, newsockfd, requestFileName, temp, t2, t3, host_addr);
            }
        }
        else {
            send(newsockfd, "400 : BAD REQUEST\nONLY HTTP REQUESTS ALLOWED", 18, 0);
        }
        close(sockfd1);
        close(newsockfd);
        close(sockfd);
        exit(0);
    }
    else {
        close(newsockfd);
        goto accepting;
    }
    return 0;
}

void sendRecvFile(int sockfd1, int newsockfd, char* fileRequest, char* temp, char* t2, char* t3, struct sockaddr_in host_addr)
{
    char buffer[500];
    FILE* fileOpen = NULL;
    //printf("filename %s",fileRequest);fflush(stdout);
    char md5string[33];
    memset(md5string, '\0', 33);
    char fileNameStr[500];
    memset(md5string, '\0', 500);
    getMD5sum(fileRequest, md5string);
    //printf("md5string %s",md5string);fflush(stdout);
    sprintf(fileNameStr, "%s.txt", md5string);

    if ((checkFileTimeout(fileNameStr, _global_timeout)) == 0) {

        printf("\nThe proxy cache is either expired or not present\n");
        memset(buffer, '\0', 500);

        if ((connect(sockfd1, (struct sockaddr*)&host_addr, sizeof(struct sockaddr))) < 0) {
            perror("Error in connecting to remote server");
            printf("\nCould Not Connect to Server\n");
            exit(0);
        }
        sprintf(buffer, "\nConnected to %s  IP - %s\n", t2, inet_ntoa(host_addr.sin_addr));

        // printf("\n%s\n", buffer);
        //send(newsockfd,buffer,strlen(buffer),0);
        bzero((char*)buffer, sizeof(buffer));

        if (temp != NULL)
            sprintf(buffer, "GET /%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n", temp, t3, t2);
        else
            sprintf(buffer, "GET / %s\r\nHost: %s\r\nConnection: close\r\n\r\n", t3, t2);

        if ((send(sockfd1, buffer, strlen(buffer), 0)) < 0)
            error("Error writing to socket");
        else {

            // printf("\n%s\n", buffer);

            if ((fileOpen = fopen(fileNameStr, "wb+")) != NULL) {
                int n = 0;
                do {
                    memset(buffer, '\0', 500);
                    n = recv(sockfd1, buffer, 500, 0);
                    if (!(n <= 0)) {
                        send(newsockfd, buffer, n, 0);
                        fwrite(buffer, sizeof(char), sizeof(buffer), fileOpen);
                    }
                } while (n > 0);
                fclose(fileOpen);
		
    		linkPrefetch(fileNameStr, t2);
            }
            else {
                printf("\nCOuld NOt open the file in write mode: %s\n", fileNameStr);
            }
        }
    }
    else {
        printf("\n Retrieving data from Proxy-Cache\n");
        if ((fileOpen = fopen(fileNameStr, "rb+")) != NULL) {
            memset(buffer, '\0', 500);
            int readBytes = 0;
            while ((readBytes = fread(buffer, 1, 500, fileOpen)) > 0) {
                //Send the message back to client
                send(newsockfd, buffer, readBytes, 0);
                memset(buffer, '\0', 500);
            }
            fclose(fileOpen);
        }
        else {
            printf("\nCOuld NOt open the filein read mode: %s\n", fileNameStr);
        }
    }

    /*Link Prefetching*/
}

/*Link Prefetching*/

void linkPrefetch(char* fileNameStr, char* host1)
{

    FILE* prefetch = NULL;
    char src[20] = "src=\'";
    char src1[20] = "src=\"";
    char href[20] = "href=\'";
    char href1[20] = "href=\"";
    char coma[20]; 

    if ((prefetch = fopen(fileNameStr, "rb+")) != NULL) {
        char* tempBuf1 = NULL;
        size_t len = 0;
        int counter = 0;
        char* param = (char*)malloc(1024);
        while ((getline(&tempBuf1, &len, prefetch)) != -1) {
            char* test1 = NULL;
			int flagHref =0;
			int flagSrc =0;
			int flagHref1 =0;
			int flagSrc1 =0;
			if((test1=strstr(tempBuf1, href))!=NULL)
			{
				flagHref =1;
			}
			else if((test1=strstr(tempBuf1, src))!=NULL)
			{
				flagSrc =1;
			}
			else if((test1=strstr(tempBuf1, href1))!=NULL)
			{
				flagHref1 =1;
			}
			else if((test1=strstr(tempBuf1, src1))!=NULL)
			{
				flagSrc1 =1;
			}
			if(flagHref||flagSrc || flagHref1 || flagSrc1){
				int len =0;
				memset(coma,'\0',20);
				if(flagSrc){
					len = strlen(src);
					strcpy(coma,"\'");
				}
				else if (flagSrc1){
					len = strlen(src1);
					strcpy(coma,"\"");}
				else if(flagHref1){
					len = strlen(href1);
					strcpy(coma,"\"");}
				else if (flagHref){
					len = strlen(href);
					strcpy(coma,"\'");}
				char *test2 = NULL;
				test1 = test1+len;
                if ((test2 = strstr(test1, coma)) != NULL) {
                    test1[abs(test1 - test2)] = '\0';
                    if ((!((strstr(test1, "#")) != NULL)) && (strlen(test1) > 1) && (!(strstr(test1, "youtube") != NULL)) && (!(strstr(test1, "twitter") != NULL))) {
                        printf("%s\n", test1);
                        if (fork() == 0) {
                            printf("\n------- Prefetching:: %s\n", test1);

                             char* temp = NULL;

		            struct sockaddr_in host_addr;
		            struct in_addr** addr_list;
			    struct hostent* host;
			    int sockfd1 =0;
			    int port =0;
                            memset(temp, '\0', 500);
                            char t1[500];
                            memset(t1, '\0', 500);
                            char t2[500];
                            memset(t2, '\0', 500);
			    char t3[500];
                            memset(t3, '\0', 500);
                            strcpy(t1, test1);
                            strcpy(t2, test1);
                            if (strncmp(test1, "http://", 7) == 0)
                                {

                                    int flag = 0;
                                    //printf("\n--------------------------------------hostName:%s\n", requestFileName);

                                    for (int i = 7; i < strlen(t2); i++) {
                                        if (t2[i] == ':') {
                                            flag = 1;
                                            break;
                                        }
                                    }
                                    int fontFlag = 0;
                                    temp = strtok(t2, "//");
                                    if (flag == 0) {
                                        port = 80;
                                        temp = strtok(NULL, "/");
                                    }
                                    else {
                                        //printf("strstr(temp,css):%s --------------------- \nstrstr(temp,fonts): %s\n", strstr(requestFileName,"css"),strstr(requestFileName,"fonts"));
                                        if ((strstr(test1, "css") != NULL) || (strstr(test1, "fonts") != NULL)) {
                                            port = 80;
                                            temp = strtok(NULL, "/");
                                        }
                                        else {
                                            temp = strtok(NULL, ":");
                                            fontFlag = 1;
                                        }
                                    }

                                    sprintf(t2, "%s", temp);
                                    //printf("host = %s", t2);

                                    if ((flag == 1) && (fontFlag == 1)) {
                                        temp = strtok(NULL, "/");
                                        port = atoi(temp);
                                    }

                                    strcat(t1, "^]");
                                    temp = strtok(t1, "//");
                                    temp = strtok(NULL, "/");
                                    if (temp != NULL)
                                        temp = strtok(NULL, "^]");
                                }
                            else {
                                strcpy(t2, host1); //host
                                port = 80; //port
                                strcpy(temp, test1); //path
                                strcpy(t3, "HTTP/1.1"); // http
                            }
                            //printf("\npath = %s\nPort = %d\n", temp, port);

                            bzero((char*)&host_addr, sizeof(host_addr));
                            host_addr.sin_port = htons(port);
                            host_addr.sin_family = AF_INET;

                            FILE* dnsCheck = NULL;
                            if (!((dnsCheck = fopen("dnsList.txt", "rb+")) != NULL)) {
                                if ((dnsCheck = fopen("dnsList.txt", "wb+")) != NULL) {
                                    //printf("\nCreated the DNS-List file\n");
                                    host = gethostbyname(t2);
                                    char tempBuff[500];
                                    memset(tempBuff, '\0', 500);
                                    sprintf(tempBuff, "%s %s %d \n", t2, inet_ntoa(*(struct in_addr*)host->h_addr), host->h_length);
                                    fwrite(tempBuff, sizeof(char), strlen(tempBuff), dnsCheck);
                                    //printf("\nwriting to the file : %s\n", tempBuff);
                                    fclose(dnsCheck);
                                }
                            }
                            else {
                                int dnsMatch = 0;

                                char* tempBuf1 = NULL;
                                size_t len = 0;
                                int counter = 0;
                                while ((getline(&tempBuf1, &len, dnsCheck)) != -1) {
                                    char tempBuf2[500];
                                    memset(tempBuf2, '\0', 500);
                                    char tempBuf3[500];
                                    memset(tempBuf3, '\0', 500);
                                    long int tempLen = 0, tempBuf5 = 0;
                                    sscanf(tempBuf1, "%s %s %ld", tempBuf2, tempBuf3, &tempLen);
                                    if (strcmp(tempBuf2, t2) == 0) {
                                        if (inet_pton(AF_INET, (char*)tempBuf3, &host_addr.sin_addr) != 1) {
                                            printf("Invalid IP Address\n");
                                            exit(-1);
                                        }
                                        dnsMatch = 1;
                                        //printf("\nFound the Line in the file : %s tempBuf2 : %s\n", tempBuf1,tempBuf2 );
                                        break;
                                    }
                                    //printf("\nLine in the file is : %s\n", tempBuf1);
                                    memset(tempBuf1, '\0', strlen(tempBuf1));
                                }

                                if (!dnsMatch) {
                                    host = gethostbyname(t2);
                                    char tempBuff[500];
                                    memset(tempBuff, '\0', 500);
                                    sprintf(tempBuff, "%s %s %d \n", t2, inet_ntoa(*(struct in_addr*)host->h_addr), host->h_length);
                                    fwrite(tempBuff, sizeof(char), strlen(tempBuff), dnsCheck);
                                    //printf("\nwriting to the file : %s --  %s\n", tempBuff, inet_ntoa(*(struct in_addr *)host->h_addr));

                                    bcopy((char*)host->h_addr, (char*)&host_addr.sin_addr.s_addr, host->h_length);
                                }
                                fclose(dnsCheck);
                            }

                            sockfd1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                            int optval = 1;
                            if (setsockopt(sockfd1, SOL_SOCKET, SO_REUSEADDR,
                                    (const void*)&optval, sizeof(int))
                                < 0)
                                printf("Error in SetSOcket\n");

                            FILE* blockedWebsites = NULL;
                            int blockedFlag = 0;
                            if ((blockedWebsites = fopen("blocked.txt", "rb")) != NULL) {
                                char* tempBuf1 = NULL;
                                size_t len = 0;
                                while ((getline(&tempBuf1, &len, blockedWebsites)) != -1) {
                                    char tempBuf2[500];
                                    memset(tempBuf2, '\0', 500);
                                    sscanf(tempBuf1, "%s", tempBuf2);
                                    if (strcmp(tempBuf2, t2) == 0) {
                                        blockedFlag = 1;
                                        break;
                                    }
                                }
                                fclose(blockedWebsites);
                            }
                            if (!blockedFlag) {
                                char buffer[500];
                                FILE* fileOpen = NULL;
                                //printf("filename %s",fileRequest);fflush(stdout);
                                char md5string[33];
                                memset(md5string, '\0', 33);
                                char fileNameStr[500];
                                memset(md5string, '\0', 500);
                                getMD5sum(test1, md5string);
                                //printf("md5string %s",md5string);fflush(stdout);
                                sprintf(fileNameStr, "%s.txt", md5string);

                                if ((checkFileTimeout(fileNameStr, _global_timeout)) == 0) {

                                    //printf("\nThe proxy cache is either expired or not present\n");
                                    memset(buffer, '\0', 500);

                                    if ((connect(sockfd1, (struct sockaddr*)&host_addr, sizeof(struct sockaddr))) < 0) {
                                        perror("Error in connecting to remote server");
                                        printf("\nCould Not Connect to Server\n");
                                        exit(0);
                                    }
                                    sprintf(buffer, "\nConnected to %s  IP - %s\n", t2, inet_ntoa(host_addr.sin_addr));

                                    // printf("\n%s\n", buffer);
                                    //send(newsockfd,buffer,strlen(buffer),0);
                                    bzero((char*)buffer, sizeof(buffer));

                                    if (temp != NULL)
                                        sprintf(buffer, "GET /%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n", temp, t3, t2);
                                    else
                                        sprintf(buffer, "GET / %s\r\nHost: %s\r\nConnection: close\r\n\r\n", t3, t2);

                                    if ((send(sockfd1, buffer, strlen(buffer), 0)) < 0)
                                        error("Error writing to socket");
                                    else {

                                        // printf("\n%s\n", buffer);

                                        if ((fileOpen = fopen(fileNameStr, "wb+")) != NULL) {
                                            int n = 0;
                                            do {
                                                memset(buffer, '\0', 500);
                                                n = recv(sockfd1, buffer, 500, 0);
                                                if (!(n <= 0)) {
                                                    fwrite(buffer, sizeof(char), sizeof(buffer), fileOpen);
                                                }
                                            } while (n > 0);
                                            fclose(fileOpen);
                                        }
                                        else {
                                            printf("\nCOuld NOt open the file in write mode: %s\n", fileNameStr);
                                        }
                                    }
                                }
                            }
                            exit(0);
                        }
                    }
                }
            }
            memset(tempBuf1, '\0', strlen(tempBuf1));
        }
        fclose(prefetch);
    }
}

