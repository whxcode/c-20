#include <netinet/in.h>
#include <stdatomic.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>

int main() {
    auto cfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(8080);               // short 型
    serv.sin_addr.s_addr = htonl(INADDR_ANY);  // int 型
                                               //

    auto ret = connect(cfd, (sockaddr*)&serv, sizeof(serv));

    if (ret < 0) {
        perror("connect");
    }

    int n = 0;
    char buf[256]{0};
    while (1) {
        memset(buf, 0, sizeof(buf));
        // 这一步很重要；等待用户下一步请求；而不是直接
        // send/recv 否则会出现死循环
        n = read(STDERR_FILENO, buf, sizeof(buf));

        send(cfd, buf, (size_t)n, 0);
        memset(buf, 0, sizeof(buf));
        n = read(cfd, buf, sizeof(buf));

        if (n <= 0) {
            printf("read error or server exit [%d]\n", n);
            break;
        }

        printf("n == [%d],buf =[%s]\n", n, buf);
    }

    close(cfd);

    return 0;
}
