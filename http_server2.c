//若图片较大，需要循环发送。或使用send_all()函数

#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <func.h>

int main()
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(listenfd, -1, "socket");

    int on = 1;
    int ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    ERROR_CHECK(ret, -1, "setsockopt");

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(8080);
    serveraddr.sin_addr.s_addr = inet_addr("0.0.0.0");//代表本机地址

    ret = bind(listenfd, 
               (const struct sockaddr*)&serveraddr, sizeof(serveraddr));
    ERROR_CHECK(ret, -1, "bind");

    ret = listen(listenfd, 10);
    ERROR_CHECK(ret, -1, "listen");

    while(1) {
        int peerfd = accept(listenfd, NULL, NULL);

        char buff[4096] = {0};
        ret = recv(peerfd, buff, sizeof(buff), 0);//read

        printf("buff:\n%s\n", buff);

        //构建一个响应报文
        const char * startLine = "HTTP/1.1 200 OK\r\n";
        const char * headers = "Server: MyHttpServer1.0\r\n"
            "Content-Type: image/jpg\r\n"
            "Content-Length: ";
        const char * emptyLine = "\r\n";

        //获取图片的长度
        int fd = open("image.jpg", O_RDONLY);
        ERROR_CHECK(fd, -1, "open");
        struct stat st;
        fstat(fd, &st);
        printf("image's length: %ld\n", st.st_size);

        memset(buff, 0, sizeof(buff));
        sprintf(buff, "%s%s%ld\r\n%s",
                startLine,
                headers,
                st.st_size,
                emptyLine);
        //先发送报文头信息
        ret = send(peerfd, buff, strlen(buff), 0);
        printf("1 send %d bytes.\n", ret);
        //再发送报文体
        char * pbody = (char*)calloc(1, st.st_size);
        read(fd, pbody, st.st_size);

        //因为图片比较大，需要进行循环发送
        //确保所有的图片数据被客户端接收完毕
        char * pbuf = pbody;
        int sendBytes = 0;
        while(sendBytes < st.st_size) {
            ret = send(peerfd, pbuf, st.st_size - sendBytes, 0);
            sendBytes += ret;
            pbuf += ret;
        }
        printf("2 send %d bytes.\n", sendBytes);


        free(pbody);
    }
    close(listenfd);

    return 0;
}
