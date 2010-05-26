/* TCP/IP communication for GLUT based programs
 * Server (GLUT) side. 
 *
 * Alexey Zhelezov, DESY/ITEP, 2005 
 * July 2005, Jörgen Samson: small fix to keep
 *            TCP/IP connection alive if data
 *            is temporary not available
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>

struct __glutSocketList {
  struct __glutSocketList *next;
  int fd;
  void  (*read_func)(struct __glutSocketList *sock);
} *__glutSockets=0;


static void add_socket(struct __glutSocketList *sock){
  sock->next=__glutSockets;
  __glutSockets=sock;
}

static void delete_socket(struct __glutSocketList *sock){
  struct __glutSocketList *list=__glutSockets;
  if(list==sock){
    __glutSockets=sock->next;
  } else {
    while(list->next && (list->next!=sock))
      list=list->next;
    if(!list->next){
      fprintf(stderr,"ERROR: attempt to delete unknown socket\n");
      return;
    }
    list->next=sock->next;
  }
  sock->next=0;
  free(sock);
}

/* internal, TCP server */
typedef struct {
  struct __glutSocketList list;
  void (*user_func)(void *data);
} tcp_srv_sock;

static void tcp_server_read(struct __glutSocketList *list){
  static unsigned char *buf=0;
  static unsigned buf_size=0;
  unsigned got_sum;
  int size,need_size=0;

  if(buf_size<8){
    buf=realloc(buf,8);
    buf_size=8;
  }
  if((size=read(list->fd,buf,8))){
    need_size=*((unsigned *)buf);
    //if((need_size >=8) && (need_size<10000000)){
    if((need_size >=8)) { //&& (need_size<1000000000)){
      if(need_size>8){
	if((unsigned)need_size>buf_size){
	  buf=realloc(buf,need_size);
	  buf_size=need_size;
	}
	got_sum=8;
	while(got_sum<need_size){
	    size=read(list->fd,buf+got_sum,need_size-got_sum);
	    if(size <= 0){
	      if (errno == EAGAIN)
		continue; // keep trying..
	      perror("In glut_socks::tcp_server_read");
	      need_size=0; // problem
	      break;
	    }
	    got_sum+=size;
	}
      }
    } else
      need_size=0;
  }
  if(need_size<8){
    fprintf(stderr,"INFO: client is disconnected\n");
    close(list->fd);
    delete_socket(list);
    return;
  }
  (*(((tcp_srv_sock *)list)->user_func))(buf);
}

static void tcp_server_accept(struct __glutSocketList *list){
  int fd;
  tcp_srv_sock *nl;

  fd=accept(list->fd,0,0);
  if(fd<0){
    perror("WARNING: can't accept connection");
    return;
  }

  nl=malloc(sizeof(*nl));
  nl->list.fd=fd;
  nl->list.read_func=tcp_server_read;
  nl->user_func=((tcp_srv_sock *)list)->user_func;
  add_socket(&nl->list);
  fprintf(stderr,"INFO: new client - socketID: %d\n", nl->list.fd  );
  (*(((tcp_srv_sock *)list)->user_func))(0);
}  

/* API */
int glut_tcp_server(unsigned short port,
		    void (*user_func)(void *data)){
  int fd=socket(PF_INET,SOCK_STREAM,0);
  struct sockaddr_in addr;
  int one=1;
  long flags;
  tcp_srv_sock *list;

#ifdef __APPLE__
  setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,&one,sizeof(one));
#else
  setsockopt(fd,SOL_TCP,TCP_NODELAY,&one,sizeof(one));  
#endif    

  setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));
  flags=fcntl(fd,F_GETFL);
  flags|=O_NONBLOCK;
  fcntl(fd,F_SETFL,flags);

  addr.sin_family=AF_INET;
  addr.sin_port=htons(port);
  addr.sin_addr.s_addr=INADDR_ANY;
  if(bind(fd,(struct sockaddr *)&addr,sizeof(addr))){
    perror("ERROR: can't bind to server port");
    close(fd);
    return -1;
  }
  if(listen(fd,10)){
    perror("ERROR: can't listen on server port");
    close(fd);
    return -1;
  }
  list=malloc(sizeof(*list));
  list->list.fd=fd;
  list->list.read_func=tcp_server_accept;
  list->user_func=user_func;
  add_socket(&list->list);
  return 0;
}
