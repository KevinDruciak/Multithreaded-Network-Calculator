#include <stdio.h>      /* for snprintf */
#include "csapp.h"      /* for rio_ functions */
#include "calc.h"

/* buffer size for reading lines of input from user */
#define LINEBUF_SIZE 1024

struct connInfo {
  int clientfd;
};

struct Calc *calc;

void chat_with_client(struct Calc *calc, int client_fd);
void fatal(const char *msg);
void *worker(void *arg);

int main(int argc, char *argv[]) {
  calc = calc_create();
  if (argc != 2) {
    fatal("Could not open server socket\n");
  }
  if (atoi(argv[1]) < 1024) {
    fatal("Could not open server socket\n");
  }
  int server_fd = open_listenfd(argv[1]);
  if (server_fd < 0) {
    fatal("Could not open server socket\n");
  }
  int keep_going = 1;
  while (keep_going) {
    int client_fd = Accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
      fatal("Error: error accepting client connection");
    }
    //struct Calc *calc = calc_create();

    //set_clientfd(calc, client_fd);
    struct connInfo *info = malloc(sizeof(struct connInfo));
    info->clientfd = client_fd;
    
    pthread_t thr_id;
    if (pthread_create(&thr_id, NULL, worker, info) != 0) {
      fatal("Error: pthread_create failed");
    }
    //close(client_fd);
    //if (client_fd > 0) {
    //keep_going = chat_with_client(calc, client_fd);
    //close(client_fd);
    //}
  }
  //pthread_exit(NULL);
  close(server_fd);
  calc_destroy(calc);
  return 0;
}

void *worker(void *arg) {
  struct connInfo *info = arg;
  pthread_detach(pthread_self());

  chat_with_client(calc, info->clientfd);
  close(info->clientfd);
  free(info);
  //pthread_join(pthread_self(), NULL);
  //pthread_exit(NULL);
  //calc_destroy(calc);
  return NULL;
}

//prints error messages and exits
void fatal(const char *msg) {
  fprintf(stderr, "%s\n", msg);
  exit(1);
}

//conneciton with client
void chat_with_client(struct Calc *calc, int client_fd) {
  rio_t in;
  char linebuf[LINEBUF_SIZE];
  /* wrap standard input (which is file descriptor 0) */
  rio_readinitb(&in, client_fd);
  int done = 0;
  while (!done) {
    ssize_t n = rio_readlineb(&in, linebuf, LINEBUF_SIZE);
    if (n <= 0) {
      /* error or end of input */
      //return 0;
      done = 1;
    } else if (strcmp(linebuf, "quit\n") == 0 || strcmp(linebuf, "quit\r\n") == 0) {
      /* quit command */
      //return 1;
      done = 1;
    } else if (strcmp(linebuf, "shutdown\r\n") == 0 || strcmp(linebuf, "shutdown\n") == 0) {
      /* shutdown command */
      //return 0;
      done = 1;
    } else {
      /* process input line */
      int result;
      if (calc_eval(calc, linebuf, &result) == 0) {
	/* expression couldn't be evaluated */
	rio_writen(client_fd, "Error\n", 6);
      } else {
	/* output result */
	int len = snprintf(linebuf, LINEBUF_SIZE, "%d\n", result);
	if (len < LINEBUF_SIZE) {
	  rio_writen(client_fd, linebuf, len);
	}
      }
      //return 1;
      //done = 1;
    }
  }
  //return 1;
}
