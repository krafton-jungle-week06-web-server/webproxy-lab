#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static char method[MAXLINE];
static char uri[MAXLINE];

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *server_name, char *server_port, char *uri, char *filename, char *cgiargs);
void serve_static(char *method, int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(char *method, int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);
void proxy_to_tiny(char *server_name, char *server_port, char *uri);


/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
  printf("%s", user_agent_hdr);

  int listenfd, proxy_connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    proxy_connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(proxy_connfd);   // line:netp:tiny:doit

    Close(proxy_connfd);  // line:netp:tiny:close
  }


  return 0;
}

void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE], server_name[MAXLINE], server_port[MAXLINE];
  rio_t rio;

  // request line과 header를 읽는다.
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s",buf);
  sscanf(buf, "%s %s %s", method, uri, version);
  if ((strcasecmp(method, "GET") !=0) && (strcasecmp(method, "HEAD") !=0)) { // method가 HEAD나 GET이 아닐 때
    clienterror(fd, method, "501", "Not implemented", "Proxy does not implement this method");
    return;
  }
  read_requesthdrs(&rio);

  // GET 요청으로부터 URI 분할하기
  // GET tiny:9999/cgi-bin/adder?123&456 HTTP/1.1
  parse_uri(server_name, server_port, uri, filename, cgiargs);

  proxy_to_tiny(server_name, server_port, uri);
  /*
  if (stat(filename, &sbuf)<0){ // 파일이 없으면? 인 듯
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  if (is_static){ // 정적 컨텐츠 serve하기
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){ // 파일이 있어도 접근할 수 없다면? 인 듯
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(method, fd, filename, sbuf.st_size);
  }
  else{ // 동적 컨텐츠 serve하기
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){ // 파일이 있어도 접근할 수 없다면? 인 듯
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(method, fd, filename, cgiargs);
  }
  */

}


void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  // HTTP response body 빌드하기
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffffff""\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  // HTTP response 출력하기
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp) // tiny는 요청 헤더 내의 어떤 정보도 사용하지 않는다. 헤더를 읽고 무시하는 함수.
{ // rp는 request header pointer 인듯
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")){ // 버퍼의 끝까지 읽기만 하기
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}



int parse_uri(char *server_name, char *server_port, char *uri, char *filename, char *cgiargs)
{
    char uri2[100];
    // ':'를 구분자로 tiny를 구한다
    strcpy(uri2,uri);
    server_name = strtok(uri2, ":");
    // '/'를 구분자로 9999를 구한다
    // printf("%s\n",server_name);
    server_port = strtok(NULL, "/");
    // printf("%s\n",server_port);

    if (uri[strlen(uri)-1] == '/'){
        strcpy(uri,"/");
        printf("%s\n",uri);
    }
    else{
        char uri_with_slash[100];
        uri_with_slash[0] = '/'; // '/' 문자 추가
        uri_with_slash[1] = '\0'; // 문자열 끝을 표시
        // 남은 부분을 그대로 uri2에 저장한다
        // printf("%s\n",server_port);
        // printf("%s\n",uri);
        char *uri_no_slash = strtok(NULL, "");
        // 기존 문자열을 새로운 문자열에 이어붙임
        strcat(uri_with_slash, uri_no_slash);    // 결과 출력
        // printf("uri_with_slash: %s\n", uri_with_slash);
        strcpy(uri,uri_with_slash);
    }
    return 0;
}

void proxy_to_tiny(char *server_name, char *server_port, char *uri){

    int clientfd;   //소켓식별자
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    // if(argc!=3){
    //     fprintf(stderr, "uage: %s <host> <port>\n", argv[0]);
    //     exit(0);
    // }
    host = server_name;     // 서버의 IP주소
    port = server_port;     // 서버의 포트

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);


    typedef struct {

    } files;

    while (Fgets(buf, MAXLINE, stdin) != NULL) {

        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }
    Close(clientfd);
    exit(0);
}

// file name으로부터 file type을 얻는다.
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html")){
    strcpy(filetype, "text/html");
  }
  else if (strstr(filename, ".gif")){
    strcpy(filetype, "image/gif");
  }
  else if (strstr(filename, ".png")){
    strcpy(filetype, "image/png");
  }
  else if (strstr(filename, ".jpg")){
    strcpy(filetype, "image/jpeg");
  }
  else if (strstr(filename, ".mp4")){
    strcpy(filetype, "video/mp4");
  }
  else{
    strcpy(filetype, "text/plain");
  }
}
