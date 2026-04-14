#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

#include "request.h"
#include "config.h"

extern char *getenv();
extern char *strdup();

#define TRUE  (1)
#define FALSE (0)

is_number(s)
char *s;
{ while (*s==' ' || *s=='\t') s++;
  while (*s=='-' || *s=='+') s++;
  while (*s>='0' && *s<='9') s++;
  return !*s;
}

help(prg)
char *prg;
{ fprintf(stderr,"%s: syntax: %s [-s|q|Q|c] [-n] [-d <display>] [-p <port>] [[<host>] <side>] [passwd]\n",prg,prg);
  exit(0);
}

main(argc,argv)
char **argv;
{ int sock;
  int server_port=SERVER_PORT;
  char *prg;

  struct sockaddr_in server;
  struct hostent *hp, *gethostbyname();
  char buf[1024];
  char rsc[1024];
  char *s;

  char *display;
  char *home;
  char *host;
  char *passwd;
  char localhost[1024];
  int side;

  FILE *fp;
  int n;
  int no_update=FALSE;
  XCRequest data;


  if (!(prg=strrchr(*argv,'/'))) prg=*argv++;
  else argv++;

  argc--;
  home=getenv("HOME");
  display=getenv("DISPLAY");
  gethostname(localhost,1024);

  if (!strcmp(prg,"xcquery")) data.cmd=XC_QUERY;
  else if (!strcmp(prg,"xcsave")) data.cmd=XC_SAVE;
  else if (!strcmp(prg,"xcquit")) data.cmd=XC_QUIT;
  else data.cmd=XC_LOGIN;

  side=-1;
  host=0;
  passwd=0;
  sprintf(rsc,"%s/.xcconnect",home);
  if (fp=fopen(rsc,"r")) {
    if ((n=fscanf(fp,"%s %d %d\n",buf,&side,&server_port))!=3) {
      if (n==2) server_port=SERVER_PORT;
      else {
	fprintf(stderr,"illegal data in .xcconnect\n");
	exit(1);
      }
    }
    host=strdup(buf);
    fclose(fp);
  }

  while (argc && *(s=*argv)=='-') {
    while (*++s) {
      switch (*s) {
	case 'c': data.cmd=XC_LOGIN;
		  break;
	case 'q': data.cmd=XC_QUERY;
		  break;
	case 's': data.cmd=XC_SAVE;
		  break;
	case 'Q': data.cmd=XC_QUIT;
		  break;
	case 'n': no_update=TRUE;
		  break;
	case 'd': if (argc<2) {
		    fprintf(stderr,"%s: display missing\n",prg);
		    exit(1);
		  }
		  display=argv[1];
		  argc--;
		  argv++;
		  break;
	case 'p': if (argc<2) {
		    fprintf(stderr,"%s: port number missing\n",prg);
		    exit(1);
		  }
		  server_port=atoi(argv[1]);
		  argc--;
		  argv++;
		  break;
	case 'I': side=-1;
		  host=0;
		  break;
	case 'h':
	case '?': help(prg);
		  break;
      }
    }
    argc--;
    argv++;
  }

  if (argc>=1) {
    if (is_number(*argv)) {
      side=atoi(*argv++);
      argc--;
    }
    else {
      if (argc>=2 && is_number(argv[1])) {
	host=*argv++;
	side=atoi(*argv++);
	argc-=2;
      }
    }
  }

  if (argc>=1) {
    passwd=*argv++;
    argc--;
  }

  if (!host) {
    host=localhost;
  }

  if (display) {
    if (*display==':') strcpy(data.display,localhost);
    else *data.display=0;
    strcat(data.display,display);
  }
  else {
    fprintf(stderr,"%s: display missing\n",prg);
    exit(1);
  }
  data.side=htons(side);
  if (passwd) strcpy(data.passwd,passwd);
  else *data.passwd=0;
  
  sock=socket(AF_INET, SOCK_STREAM, 0);
  if (sock<0) {
    perror("opening stream socket");
    exit(1);
  }

  server.sin_family=AF_INET;
  hp=gethostbyname(host);
  if (!hp) {
    fprintf(stderr,"%s: unknown host %d\n",prg,host);
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
  if (!no_update && (fp=fopen(rsc,"w"))) {
    fprintf(fp,"%s %d %d\n",host,side,server_port);
    fclose(fp);
  }
  exit(0);
}
