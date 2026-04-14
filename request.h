
typedef struct r {
  u_short cmd;
  char display[200];
  char passwd[100];
  u_short side;
} XCRequest;

#define XC_LOGIN	1
#define XC_QUERY	2
#define XC_SAVE		3
#define XC_QUIT		4
