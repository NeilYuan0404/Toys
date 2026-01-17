#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>



#define DNS_SERVER_PORT 53
#define DNS_SERVER_IP "114.114.114.114"

#define DNS_HOST 0x01
#define DNS_CNAME 0x05

// transaction, question, answer etc.
struct dns_header {

  unsigned short id;
  unsigned short flags;

  unsigned short question;
  unsigned short answer;
  
  unsigned short authority;
  unsigned short additional;
    
};

//长度不固定
struct dns_question {

  int length;
  unsigned short qtype;
  unsigned short qclass;
  unsigned char *name;
    
};


//client header sendto server
int dns_create_header(struct dns_header *header) {
  if (header == NULL) return -1;
  memset(header, 0, sizeof(struct dns_header));

  //random
  srandom(time(NULL));
  header->id = random();

  //调整为网络字节序
  header->flags = htons(0x0100);
  header->question = htons(1);

  return 0;
}

//header填充
int dns_create_question(struct dns_question *question, const char *hostname) {
  
  if (question == NULL || hostname == NULL) return -1;
  memset(question, 0, sizeof(struct dns_question));

  question->name = (char*)malloc(strlen(hostname) + 2);
  if (question->name == NULL) {
    return -2;
  }

  question->length = strlen(hostname) + 2;
  
  question->qtype = htons(1);
  question->qclass = htons(1);
  

  // name,查询名字格式:3www6google3com
  const char delim[2] = ".";
  char *qname = question->name;
  
  char *hostname_dup = strdup(hostname);
  char *token = strtok(hostname_dup, delim);
  
  while (token != NULL) {

    size_t len = strlen(token);

    *qname = len;
    qname ++;

    strncpy(qname, token, len+1);//这里已经包括了最后的'\0'
    qname += len;

    token = strtok(NULL, delim);
    
  }
  
  free(hostname_dup);
  return 0;
}  

//将header和question整合进request里面
    int dns_build_request(struct dns_header *header, struct dns_question *question, char *request, int rlen) {
  if (header == NULL || question == NULL || request == NULL) return -1;

  memset(request, 0, rlen);

  memcpy(request, header, sizeof(struct dns_header));
  int offset = sizeof(struct dns_header);

  memcpy(request+offset, question->name, question->length);
  offset += question->length;

  memcpy(request+offset, &question->qtype, sizeof(question->qtype));
  offset += sizeof(question->qtype);
    
  memcpy(request+offset, &question->qclass, sizeof(question->qclass));
  offset += sizeof(question->qclass);

  return offset;
	 
}
int dns_client_commit(const char *domain) {
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  if (sockfd < 0) {
    return -1;
  }

  struct sockaddr_in servaddr = {0};
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(DNS_SERVER_PORT);
  servaddr.sin_addr.s_addr = inet_addr(DNS_SERVER_IP);

  int ret = connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
  printf("connect: %d\n", ret);
  
  struct dns_header header = {0};
  dns_create_header(&header);

  struct dns_question question = {0};
  dns_create_question(&question, domain);

  char request[1024] = {0};
  int length = dns_build_request(&header, &question, request, 1024);

  //request
  int slen = sendto(sockfd, request, length, 0, (struct sockaddr*) &servaddr, sizeof(struct sockaddr_in));

  //recvfrom
  char response[1024] = {0};
  struct sockaddr_in addr;
  size_t addr_len = sizeof(struct sockaddr_in);
  
  int n = recvfrom(sockfd, response, sizeof(response), 0, (struct sockaddr*) &addr, (socklen_t*) &addr_len);

  printf("recvfrom : %d, %s\n", n, response);

  return n;
}

int main(int argc, char *argv[]) {
  if (argc < 2) return -1;

  dns_client_commit(argv[1]);
}

