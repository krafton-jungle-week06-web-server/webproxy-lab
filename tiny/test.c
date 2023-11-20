#include <stdio.h>
#include <string.h>


int parse_uri(char *server_name, char *server_port, char *uri, char *filename, char *cgiargs)
{
    char uri2[100];
    strcpy(uri2, uri);
    
    char uri_with_slash[100];
    uri_with_slash[0] = '/'; // '/' 문자 추가
    uri_with_slash[1] = '\0'; // 문자열 끝을 표시
    
    // ':'를 구분자로 tiny를 구한다
    char *server_name = strtok(uri2, ":");
    // '/'를 구분자로 9999를 구한다
    char *server_port = strtok(NULL, "/");
    // 남은 부분을 그대로 uri2에 저장한다
    char *uri_no_slash = strtok(NULL, "");
    // 기존 문자열을 새로운 문자열에 이어붙임
    
    strcat(uri_with_slash, uri_no_slash);    // 결과 출력
    
    // printf("server_name: %s\n", server_name);
    // printf("server_port: %s\n", server_port);
    // printf("uri_with_slash: %s\n", uri_with_slash);
    strcpy(uri,uri_with_slash);
    // printf("uri: %s\n", uri);

    return 0;
}