#include <func.h>
#include <asm-generic/socket.h>

//http协议, 以二进制文件方式传输给客户端png图片

int main(void)
{
    //创建套接字
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1){
        error(1, errno, "socket");
    }

    //填充网络地址
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(8080);
    serveraddr.sin_addr.s_addr = inet_addr("192.168.248.136");

    //设置网络地址可重用
    int reuse = 1;
    int ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    ERROR_CHECK(ret , 1 ,"setsockopt");

    //绑定IP地址和端口
    ret = bind(listenfd, (const struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if(ret == -1){
        error(1, errno, "bind");
    }

    //监听
    ret = listen(listenfd, 10);
    if(ret == -1){
        error(1, errno, "listen");
    }

    while(1){
        struct sockaddr_in clientAddr;
        memset(&clientAddr, 0, sizeof(clientAddr));
        socklen_t len = sizeof(clientAddr);
        int peerfd = accept(listenfd, (struct sockaddr*)&clientAddr, &len);
        printf("%s:%d has connected.\n",inet_ntoa(clientAddr.sin_addr),ntohs(clientAddr.sin_port));

        //读取图片文件
        FILE *png_file = fopen("1.png","rb");
        if(png_file == NULL){
            perror("fopen");
            exit(1);
        }
        //获取文件大小
        fseek(png_file, 0, SEEK_END);
        long filesize = ftell(png_file);
        rewind(png_file); //fseek(png_file, 0, SEEK_SET);

        //读取文件内容
        char* png_file_buff = (char*)malloc(filesize);    //png_file_buff存放响应体,即图片
        fread(png_file_buff, 1, filesize, png_file);
        fclose(png_file);

        //获取http请求报文
        char buff[4096] = {0};
        ret = recv(peerfd, buff, sizeof(buff), 0);    //buff用于存储从客户端接收的HTTP请求报文
        printf("ret: %d bytes.\n%s\n",ret, buff);

        //对请求报文进行响应操作
        const char* startLine = "HTTP/1.1 200 OK\r\n";
        const char* headers = "Server: MyHttpServer1.0\r\n"
                "Content-Type: image/png\r\n"
                /* "Content-Type: text/html\r\n" */
                "Content-Length: ";
        const char* emptyLine = "\r\n";
        /* const char* body = */
        /*     "<html>" */
        /*     "   <head>Test Image</head>" */
        /*     "   <body>" */
        /*             "<img src=\"1.png\" width=\"100%\">" */
        /*     "   </body>" */
        /*     "</html>"; */

        //清空缓冲区
        memset(buff, 0 ,sizeof(buff));    //清空重用buff，存储要发送的响应头
        /* sprintf(buff, "%s%s%ld\r\n%s%s", startLine, headers, strlen(body), emptyLine, body); */
        sprintf(buff, "%s%s%ld\r\n%s", startLine, headers, filesize, emptyLine);
        printf("response:\r\n%s\n", buff);

        //响应头和响应体分开发送:
        //发送响应头
        ret = send(peerfd, buff, strlen(buff), 0);
        printf("\nsend %d bytes.\n", ret);

        //发送响应体:图片内容
        send(peerfd, png_file_buff, filesize, 0);

        free(png_file_buff);
        close(peerfd);
    }

    close(listenfd);

    return 0;
}
