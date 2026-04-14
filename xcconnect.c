#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>

#include "request.h"
#include "config.h"


main(argc,argv)
char **argv;
{ int sock;
  struct sockaddr_in server;
  struct hostent *hp, *gethostbyname();
  char buf[1024];
  XCRequest data;
  char *s;
  int server_port=SERVER_PORT;

  data.cmd=XC_LOGIN;

  s=(char*)getenv("DISPLAY");
  if (s) {
    if (*s==':') gethostname(data.display,sizeof(data.display));
    else *data.display=0;
    strcat(data.display,s);
  }
  else {
    if (argc<5) {
      fprintf(stderr,"%s: display missing\n",argv[0]);
      exit(1);
    }
  }
  if (argc<2) {
    fprintf(stderr,"%s: please give host\n",argv[0]);
    fprintf(stderr,"%s: %s <host> <side> [ <passwd> [ <display> [ <port> ]]]\n",
		       argv[0],argv[0]);
    exit(1);
  }
  if (argc>2) {
    data.side=htons(atoi(argv[2]));
  }
  else data.side=htons(-1);

  if (argc>3) {
    strcpy(data.passwd,argv[3]);
  }
  else *data.passwd=0;

  if (argc>4) {
    strcpy(data.display,argv[4]);
  }
  if (argc>5) {
    server_port=atoi(argv[5]);
  }
  
  sock=socket(AF_INET, SOCK_STREAM, 0);
  if (sock<0) {
    perror("opening stream socket");
    exit(1);
  }

  server.sin_family=AF_INET;
  hp=gethostbyname(argv[1]);
  if (!hp) {
    fprintf(stderr,"%s: unknown host %d\n",argv[0],argv[1]);
    exit(2);
  }
  bcopy((char*)hp->h_addr,(char*)&server.sin_addr,hp->h_length);
  server.sin_port=htons(server_port);

  if (connect(sock,&server,sizeof(server))<0) {
    perror("connecting stream socket");
    exit(3);
  }

  if (write(sock,&data,sizeof(data))<0) {
    perror("writing on connection");
    exit(4);
  }

  if (read(sock,buf,1024)<0) {
    perror("reading on connection");
    exit(4);
  }

  printf("%s\n",buf);
  exit(0);
}
