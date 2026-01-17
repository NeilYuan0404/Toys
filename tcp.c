#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <netinet/tcp.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <pthread.h>


#include <errno.h>
#include <fcntl.h>


#define BUFFER_LENGTH 1024
#define EPOLL_SIZE 1024

void *client_routine(void *arg) {
  int clientfd = *(int *)arg;//?

  while (1) {

    char buffer[BUFFER_LENGTH] = {0};
    int len = recv(clientfd, buffer, BUFFER_LENGTH, 0);
    
    if (len < 0) {
      close(clientfd);
      break;
    } else if (len == 0) { //disconnect
      close(clientfd);
      break;
    } else {
      printf("Recv: %s, %d byte(s)\n", buffer, len);
    }
  }
}


int main(int argc, char *argv[]) {


  if (argc < 2) {
    printf("param error\n");
    return -1;
  }

  int port = atoi(argv[1]);
  
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY; //0.0.0.0 任意地址
  
  //socketfd和addr进行绑定
  if (bind(sockfd, (struct sockaddr*)&addr, sizeof(sockaddr_in)) < 0) {
    perror("bind"); //这个函数直接把错误errono打印出来
    return 2;
  }
  
  if (listen(sockfd, 5) < 0) {
    perror("listen");
    return 3; 
  }//listen的描述符数目最多为5

#if 0 //一请求一线程的实现
  
  while (1) {

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));

    //为客户端介绍一个新的服务员
    int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);

    pthread_t thread_id;
    pthread_create(&thread_id)
    
  }
#else

  int epfd = epoll_create(1);

  struct epoll_event events[EPOLL_SIZE] = {0};

  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = sockfd;
  epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

  while (1) {
    int nready = epoll_wait(epfd, events, EPOLL_SIZE, -1);//-1, 0, 5

    if (nready == -1) continue;

    int i = 0;
    for (i = 0; i < nready; i++) {
      if (events[i].data.fd == sockfd) {
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(struct sockaddr_in));
    
	int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);

	ev.events = EPOLLIN || EPOLLET;//边沿触发
	ev.data.fd = clientfd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);//添加
      } else {
	int clientfd = events[i].data.fd;
	char buffer[BUFFER_LENGTH] = 0;
	int len = recv(clientfd, buffer, BUFFER_LENGTH, 0);
	if (len < 0) {
	  close(clientfd);
	  ev.events = EPOLLIN;
	  ev.data.fd = clientfd;
	  epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, &ev);//删除

	} else if (len == 0) {
	  close(clientfd);
	  ev.events = EPOLLIN;
	  ev.data.fd = clientfd;
	  epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, &ev);//删除
	} else {
	  printf("Recv: %s, %d byte(s)\n", buffer, len);
	}
      } 
    }
  }


#endif
    return 0;
}
