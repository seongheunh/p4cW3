#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //read, write, fork 시스템 함수 사용
#include <sys/socket.h> //socket 사용
#include <netinet/in.h> // sockaddr_in 사용
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> // open(), O_RDONLY 사용
#include <sys/stat.h> // 파일크기를 가져오기 위해 사용

void get_type(char* type, char* path){
    char *extension = strrchr(path, '.');
    if(!strcmp(extension, ".html")){
        strcpy(type, "text/html");
    }
    else if(!strcmp(extension, ".jpeg")){
        strcpy(type, "image/jpeg");
    }
    else if(!strcmp(extension, ".png")){
        strcpy(type, "image/png");
    }
    else{
        strcpy(type, "text/plain");
    }
}

int main(){
    int server_sock = 0;
    struct sockaddr_in server_sock_addr;
    memset(&server_sock_addr, 0, sizeof(server_sock_addr));

    int client_sock = 0;
    struct sockaddr_in client_sock_addr;
    memset(&client_sock_addr, 0, sizeof(client_sock_addr));

    // socket open
    if((server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        printf("socket error\n");
        exit(1);
    }

    // bind setting
    server_sock_addr.sin_family = AF_INET;
    server_sock_addr.sin_addr.s_addr = htonl(INADDR_ANY); //현재 IP주소를 자동으로 할당, sin_addr은 32비트 IP주소
    server_sock_addr.sin_port = htons(12345); // sin_port는 16비트 포트번호

    //bind
    if((bind(server_sock, (struct sockaddr *)&server_sock_addr, sizeof(server_sock_addr))) < 0){
        printf("bind error\n");
        exit(1);
    }

    //listen
    if((listen(server_sock, 100)) < 0){
        printf("listen error\n");
        exit(1);
    }

    // client accept, read request, response
    char buf[10000];
    while(1){
        //client accept
        printf("waiting... \n");
        int client_sock_addr_size = sizeof(client_sock_addr); //accept의 세번째 인자는 socklen_t 이나, int로 해야 정상 작동
        if((client_sock = accept(server_sock, (struct sockaddr *)&client_sock_addr, &client_sock_addr_size)) < 0){
            printf("accept error\n");
            continue;
        }
        //request 가져오기
        read(client_sock, buf, 10000);
        printf("Received :\n%s\n", buf);

        //request 해석하기
        char* method = strtok(buf, " "); // GET
        char* path = strtok(NULL, " "); // /index.html
        char* entire_path = (char*)malloc(65536);
        getcwd(entire_path, 65536); // 현재 실행 중인 위치를 받아옴, 동적 할당으로 변경
        strcpy((entire_path + strlen(entire_path)), path); // /home/seongheunh/Desktop/p4cW3/index.html
        
        if(strlen(entire_path) >= 65536){
            printf("invalid url : too long\n");
            exit(1);
        }

        //response - 404 오류, 정상 처리 및 전송
        char response_header[1000];
        char* response_body;
        int response_body_size = 0;
        memset(&response_header, 0, sizeof(response_header));
        memset(&response_body, 0, sizeof(response_body));
        char type[20]; //확장자 구분용 배열
        memset(&type, 0, sizeof(type));
        struct stat st; //파일 크기를 확인하기 위한 변수
        stat(entire_path, &st);

        int fd = open(entire_path, O_RDONLY);
        if(fd < 0){
            // 404
            printf("open error detected\n");
            char* error404 = "HTTP/1.1 404 Not found\nContent-Length: 23\nContent-Type: text/html\n\n<h1>404 Not Found</h1>\n";
            write(client_sock, error404, strlen(error404));
            free(entire_path);
            continue;
        }else{
            // 정상코드
            printf("open %s\n", entire_path);
            get_type(type, entire_path);
            sprintf(response_header, "HTTP/1.1 200 OK\nContent-Length: %ld\nContent-Type: %s\n\n", st.st_size, type);
            write(client_sock, response_header, strlen(response_header));

            response_body = (char*)malloc(st.st_size);
            response_body_size = read(fd, response_body, st.st_size);
            printf("send : %s\n", response_body);
            write(client_sock, response_body, response_body_size);
            free(response_body);
            free(entire_path);
            continue;
        }

        
    }
    
}
