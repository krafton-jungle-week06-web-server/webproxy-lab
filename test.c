#include <stdio.h>
#include <string.h>

int parse_uri_detail(char *hostname, char *port, char *remainder){

    char *input = "tiny:9999/cgi-bin/adder?123&456";
    char uri[100];
    // // strcpy 함수를 사용하여 문자열 복사
    strcpy(uri,input);
    char *hostname, *portname, *uri2;
    

    char addedChar = '/'; // 추가할 문자        // 새로운 문자열을 저장할 배열 선언
    char newStr[50];

    // '/' 문자 추가
    newStr[0] = addedChar; // '/' 문자 추가
    newStr[1] = '\0'; // 문자열 끝을 표시

    // 기존 문자열을 새로운 문자열에 이어붙임
    
    // ':'를 구분자로 tiny를 구한다
    hostname = strtok(uri, ":");
    
    // '/'를 구분자로 9999를 구한다
    portname = strtok(NULL, "/");
    
    // 남은 부분을 그대로 uri2에 저장한다
    uri2 = strtok(NULL, "");
    strcat(newStr, uri2);
    printf("Hostname: %s\n", hostname);
    printf("Portname: %s\n", portname);
    printf("URI2: %s\n", newStr);

    return 0;
}
    // char *uri = "tiny:9999/cgi-bin/adder?123&456";

    // char *host_ptr = index(uri, ':');
    // *host_ptr=NULL;
    // char *port_ptr = index(uri, '/');
    // *port_ptr=NULL;
    // char *parser_ptr;
    // parser_ptr=uri;

    // char *host_name;
    // char *port_name;
    // char *
    // while (parser_ptr!=NULL){
    //   parser_ptr
    // }

    // char *tiny;
    // char *number;
    // char *cgi;

    // // strtok 함수를 사용하여 ':' 또는 '/'을 기준으로 문자열 분리
    // tiny = strtok(input, ":/");
    // number = strtok(NULL, ":/");
    // cgi = strtok(NULL, ":/");

    // // 결과 출력
    // printf("tiny: %s\n", tiny);
    // printf("number: %s\n", number);
    // printf("uri: %s\n", cgi);

    // return 0;
 