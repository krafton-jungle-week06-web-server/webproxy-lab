#include "csapp.h"

int main(int argc, char **argv){
    int clientfd;   //소켓식별자
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if(argc!=3){
        fprintf(stderr, "uage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1];     //서버의 IP주소
    port = argv[2];     // 서버의 포트

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }
    Close(clientfd);
    exit(0);
}
