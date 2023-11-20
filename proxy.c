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



int parse_uri(char *server_name, char *server_port, char *uri_constant, char *filename, char *cgiargs)
{
  char *ptr;
  char uri_arr[MAXLINE];
  // uri : "tiny:9999/cgi-bin/adder?123&456"
  strcpy(uri_arr, uri_constant);
  char slash = '/'; // uri의 앞부분이 될 슬래시
  char uri[MAXLINE];

  // '/' 문자 추가
  uri[0] = slash; // '/' 문자 추가
  uri[1] = '\0'; // 문자열 끝을 표시
  
  // ':'를 구분자로 tiny를 구한다
  server_name = strtok(uri, ":");
  
  // '/'를 구분자로 9999를 구한다
  server_port = strtok(NULL, "/");
  
  // 남은 부분을 그대로 uri2에 저장한다
  char *uri_no_slash = strtok(NULL, "");
  // 기존 문자열을 새로운 문자열에 이어붙임
  strcat(uri, uri_no_slash);

  // server_name, server_port, uri
  // tiny, 9999, /cgi-bin/adder?123&456


  // GET tiny:9999/ HTTP/1.1
  if (!strstr(uri, "cgi-bin")){ // 정적 컨텐츠

    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri)-1] == '/'){
      strcat(filename, "home.html");
    }
    return 1;
  }
  // GET tiny:9999/cgi-bin/adder?123&456 HTTP/1.1
  else{ // 동적 컨텐츠 
    ptr = index(uri, '?'); // uri에서 ? 위치(인덱스) 뽑아줌
    if (ptr){
      strcpy(cgiargs, ptr+1);
      *ptr='\0';
    }
    else{
      strcpy(cgiargs, "");
    }
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
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


void serve_static(char *method, int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  // 클라이언트에게 응답 헤더(response header)를 보낸다.
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf)); // 클라이언트에 보내기
  printf("Response headers:\n");
  printf("%s", buf);

  if (strcmp(method, "GET") == 0) {
  // 클라이언트에게 응답 본체(response body)를 보낸다.
    srcfd = Open(filename, O_RDONLY, 0); // 파일 열기
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 식별자랑 파일 정보들 매핑
    Close(srcfd); // 파일 닫기
    Rio_writen(fd, srcp, filesize); // 클라이언트에 보내기
    Munmap(srcp, filesize); // 매핑 삭제
  }

  /* Mmap => malloc 구현*/
  /*
  if (strcmp(method, "GET") == 0) {
  // 클라이언트에게 응답 본체(response body)를 보낸다.
    srcfd = Open(filename, O_RDONLY, 0); // 파일 열기
    //srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 식별자랑 파일 정보들 매핑
    srcp = (char *)malloc(filesize);
    Rio_readn(srcfd, srcp, filesize);
    
    Close(srcfd); // 파일 닫기
    Rio_writen(fd, srcp, filesize); // 클라이언트에 보내기
    
    free(srcp);
  }
  */
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

void serve_dynamic(char *method, int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = { NULL };

  // HTTP 응답의 첫 번째 파트 반환
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork()==0){ // 자식
    setenv("METHOD", method, 1);
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO); // 클라이언트에게 표준 출력 redirect
    Execve(filename, emptylist, environ); // CGI 프로그램 실행
  }
  Wait(NULL); // 부모가 자식을 기다리고 회수한다. doit 함수에서 한 번에 한 개의 HTTP 트랜잭션만 처리하기 때문이다.
}
