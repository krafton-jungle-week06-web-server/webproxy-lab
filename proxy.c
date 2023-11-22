#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct cacheNode{
  char* url;                
  char fileData[MAXLINE];   
  struct cacheNode* prev;          
  struct cacheNode* next;
  int objSize;          
}cacheNode;       //8.092kB

cacheNode* head = NULL;

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *server_name, char *server_port, char *uri, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void proxy_to_tiny(char *server_name, char *server_port, char *uri, int fd, char* url);
void *thread(void *vargp);
cacheNode* findCache(char* url);
void addCache(char* url, char* buf);
void cacheInit();
void deleteCache();


/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
  printf("%s", user_agent_hdr);

  int listenfd, *connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;

  /* Check command line args */
  if (argc != 2) { // ./proxy 5555
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  cacheInit();
  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Malloc(sizeof(int));
    *connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    
    Pthread_create(&tid, NULL, thread, connfd);

    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
  }


  return 0;
}

void doit(int fd)
{
  struct stat sbuf;
  char buf[MAXLINE], version[MAXLINE], method[MAXLINE], uri[MAXLINE], url[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE], server_name[MAXLINE], server_port[MAXLINE];
  rio_t rio;
  cacheNode* targetNode;

  // request line과 header를 읽는다.
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE); // GET https://localhost:9999/cgi-bin/adder?123&456 HTTP/1.1

  sscanf(buf, "%s %s %s", method, uri, version);
  strcpy(url, uri);
  if ((strcasecmp(method, "GET") !=0) && (strcasecmp(method, "HEAD") !=0)) { // method가 HEAD나 GET이 아닐 때
    clienterror(fd, method, "501", "Not implemented", "Proxy does not implement this method");
    return;
  }
  
  targetNode = findCache(uri);   // uri: https://localhost:9999/cgi-bin/adder?123&456
  printf("targetNode: %s\n",targetNode);
  if(targetNode==NULL)    //캐시에 노드가 존재하지 않으면
  {
    read_requesthdrs(&rio);

    // GET 요청으로부터 URI 분할하기
    // GET localhost:9999/cgi-bin/adder?123&456 HTTP/1.1
    parse_uri(server_name, server_port, uri, filename, cgiargs);
    // server_name: localhost, server_port: 9999, uri: /cgi-bin/adder?123&456
    memset(buf,'\0', MAXLINE);
    proxy_to_tiny(server_name, server_port, uri, fd, url);
  }
  else{     //캐시에 노드가 존재하면
    Rio_writen(fd, targetNode->fileData, strlen(targetNode->fileData));
  }
  
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
  }
  return;
}

int parse_uri(char *server_name, char *server_port, char *uri, char *filename, char *cgiargs)
{
    // http://localhost:9999/cgi-bin/adder?123&456
    char parsed_uri[MAXLINE]={0};
    
    char *parser_ptr = strstr(uri, "//") ? strstr(uri, "//") + 2 : uri;

    int i=0;
    // int length=strlen(*uri);
    // int cnt=0;

    while(*parser_ptr!=':'){
        server_name[i]=*parser_ptr;
        i++;
        // cnt++;
        parser_ptr++;
    }
    i=0;
    // cnt++;
    parser_ptr++;
    while(*parser_ptr!='/'){
        server_port[i]=*parser_ptr;
        i++;
        // cnt++;
        parser_ptr++;
    }
    i=0;
    while(*parser_ptr){
        parsed_uri[i]=*parser_ptr;
        i++;
        // cnt++;        
        parser_ptr++;
    }

    strcpy(uri,parsed_uri);

    return 0;
}


void proxy_to_tiny(char *server_name, char *server_port, char *uri, int fd, char *url){
  int clientfd;   //소켓식별자
  char *host, *port, buf[MAXLINE], buf2[MAXLINE];
  rio_t rio;

  host = server_name;     // 서버의 IP주소
  port = server_port;     // 서버의 포트

  clientfd = Open_clientfd(host, port);
  Rio_readinitb(&rio, clientfd);

    // 클라이언트가 보낸 요청을 tiny 서버에 전달
  sprintf(buf, "GET %s HTTP/1.1\r\n", uri);
  sprintf(buf, "%sHost: %s\r\n", buf, host);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%s\r\n", buf);

  Rio_writen(clientfd, buf, strlen(buf));
  memset(buf2, '\0', MAXLINE);
  // tiny 서버로부터의 응답을 클라이언트에 전달
  while (Rio_readlineb(&rio, buf, MAXLINE) > 0) {
      strcat(buf2, buf);
      Rio_writen(fd, buf, strlen(buf));
  }
  
  // 노드 용량제한 체크
  if(strlen(buf)>MAX_OBJECT_SIZE){  //최대 데이터 사이즈를 초과하면
    printf("Too large Object Size: %d", (int)strlen(buf));
  }
  else{   //최대 데이터 사이를 초과하지 않으면
    addCache(url, buf2);     //cache 노드 추가
  }
    Close(clientfd);
    return;
}

void *thread(void *vargp){
  int connfd = *((int *)vargp);
  Pthread_detach(pthread_self());
  Free(vargp);
  doit(connfd);   // line:netp:tiny:doit
  Close(connfd);  // line:netp:tiny:close
  return NULL;
}

void cacheInit(){
  cacheNode* newNode = (cacheNode*)malloc(sizeof(cacheNode));
  newNode->prev= newNode;
  newNode->next=newNode;
  newNode->url=NULL;
  newNode->objSize=0;
  memset(newNode->fileData, '\0', sizeof(MAXLINE));
  head = newNode;
  return;
}

cacheNode* findCache(char* url){
  char _url[MAXLINE];
  memset(_url, '\0', strlen(url));
  strcpy(_url, url);

  cacheNode* current= head;
  // printf("current : %pprev : %pnext : %p objsize: %d\n", current, current->prev, current->next, current->objSize);
  // printf("current->next: %p", current->next);
  for(current; current->next!=head;current=current->next){
    // printf("Start for\n");
    // printf("curruent->url: %s, _url: %s\n",current->url, _url);
    // printf("curruent->objSize: %d\n",current->objSize);
    printf("current-url: %s\n", current->url);
    if(current->url!=NULL&& (strcmp(current->url, _url)==0)){    //cache에 해당 노드 존재하면
      printf("findCache 함수 for 문 안에 if문 \n");
      // 해당 노드 헤더로 땡기기
      current->prev->next = current->next;
      current->next->prev = current->prev;
      current->prev = head->prev;
      // 더미 노드와 설정 더미노드포인터: head
      current->prev = head;
      current->next = head->next;
      head->next = current;
      return current;
    }
    // printf("current : %pprev : %pnext : %p objsize: %d\n", current, current->prev, current->next, current->objSize);
  }
  return NULL;
}
//void deleteCache(char* url)

void addCache(char* url, char* buf){
  char _url[MAXLINE];
  char _buf[MAXLINE];
  memset(_url, '\0', strlen(url));
  memset(_buf, '\0', strlen(buf));
  strcpy(_url, url);
  strcpy(_buf, buf);
  // memset(url, '\0', strlen(url));
  // memset(buf, '\0', strlen(buf));
  //캐시메모리 제한 체크
  
  head->objSize += strlen(_buf);
  while(head->objSize>MAX_CACHE_SIZE){
    deleteCache();
  }
  cacheNode* newNode = (cacheNode*)malloc(sizeof(cacheNode));
  newNode->url = _url;
  newNode->objSize = strlen(_buf);
  //printf("newNode->url : %s, _url : %s", newNode->url, _url);
  strcpy(newNode->fileData,_buf);
  newNode->prev = head;
  newNode->next= head->next;
  head->next->prev = newNode;
  head->next = newNode;
  return;
}

void deleteCache(){
  printf("Start deleteCache\n");
  cacheNode* lastNode = head->prev;
  head->objSize -= strlen(head->prev->fileData);
  head->prev->prev->next= head;
  head->prev = head->prev->prev;
  free(lastNode);
}