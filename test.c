#include <stdio.h>
#include <string.h>


int main(void){
    char uri[100]="localhost:9999/";
    char server_name[100];
    char server_port[100];
    char parsed_uri[100];
    
    char *parser_ptr=uri;
    int i=0;
    while(*parser_ptr!=':'){
        server_name[i]=*parser_ptr;
        i++;
        parser_ptr++;
    }
    i=0;
    parser_ptr++;
    while(*parser_ptr!='/'){
        server_port[i]=*parser_ptr;
        i++;
        parser_ptr++;
    }
    i=0;
    while(*parser_ptr){
        parsed_uri[i]=*parser_ptr;
        i++;
        parser_ptr++;
    }
    strcpy(uri,parsed_uri);
    
    printf("server_name: %s\n",server_name);
    printf("server_port: %s\n",server_port);
    printf("uri: %s\n",uri);
    printf("parsed_uri: %s\n",parsed_uri);
    
}