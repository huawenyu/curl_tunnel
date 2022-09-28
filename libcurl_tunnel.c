#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <curl/curl.h>
#include <getopt.h>
#include <unistd.h>

#define _FILE_OFFSET_BITS 64 /* for curl_off_t magic */
#define SIZE 655536

#define VERSION "0.1-dev"

struct gengetopt_args_info {
  char *proxy_arg;
  char *user_arg;
  char *dest_arg;

  int help_given;
  int verbose_given;
  int user_given;
  int proxy_given;
  int dest_given;
};

static void print_options(void)
{
  printf("curltunnel\nCopyright 2007 curltunnel Project\nV " VERSION "\n"
      "\n"
      "-h       --help            Print help and exit\n"
      "-p       --proxy=STRING    Proxy host:port combination to connect to\n"
      "-u       --user=STRING     User:Password combination to send to proxy\n"
      "-d       --dest=STRING     Host:port combination to tunnel to\n"
      "-v       --verbose         Print messages about what's going on\n");
}

static char * gengetopt_strdup(char *s)
{
  char *n, *pn, *ps=s;
  while(*ps) ps++;
  n=(char *)malloc(1+ps-s);
  if(n!=NULL)
  {
    for(ps=s,pn=n;*ps;ps++,pn++)
      *pn = *ps;
    *pn=0;
  }
  return n;
}

static int command_line_parser(int argc, char * const *argv, struct gengetopt_args_info *args_info)
{
  int c;

#define clear_args()\
{\
  args_info->help_given = 0;\
  args_info->verbose_given = 0;\
  args_info->user_given = 0;\
  args_info->proxy_given = 0;\
  args_info->dest_given = 0;\
  args_info->proxy_arg = NULL;\
  args_info->user_arg = NULL;\
  args_info->dest_arg = NULL;\
}
  clear_args();

  optarg = 0;
  optind = 1;
  opterr = 1;
  optopt = '?';

  while(1)
  {
    int option_index = 0;

    static struct option long_options[] = {
      { "help", 0, NULL, 'h' },
      { "verbose", 0, NULL, 'v' },
      { "proxy", 1, NULL, 'p' },
      { "user", 1, NULL, 'u' },
      { "dest", 1, NULL, 'd' },
      { NULL, 0, NULL, 0}
    };

    c = getopt_long(argc,argv,"hvp:u:d:", long_options, &option_index);

    if(c==-1) break;

    switch(c)
    {
      case 'h':
        clear_args();
        print_options();
        exit(0);
      case 'v':
        args_info->verbose_given = !(args_info->verbose_given);
        if(args_info->verbose_given)
          printf("Verbose mode on\n");
        break;
      case 'p':
        if(args_info->proxy_given)
        {
          fprintf(stderr,"curltunnel: `--proxy' (`-p') option given more than once\n");
          clear_args();
          exit(1);
        }
        args_info->proxy_given = 1;
        args_info->proxy_arg = gengetopt_strdup(optarg);
        break;
      case 'u':
        if(args_info->user_given)
        {
          fprintf(stderr,"curltunnel: `--user' (`-u') option given more than once\n");
          clear_args();
          exit(1);
        }
        args_info->user_given = 1;
        args_info->user_arg = gengetopt_strdup(optarg);
        break;
      case 'd':
        if(args_info->dest_given)
        {
          fprintf(stderr,"curltunnel: `--dest' (`-d') option given more than once\n");
          clear_args();
          exit(1);
        }
        args_info->dest_given = 1;
        args_info->dest_arg = gengetopt_strdup(optarg);
        break;
    }
  }

  if(!args_info->proxy_given || !args_info->dest_given)
  {
    clear_args();
    print_options();
    exit(0);
  }

  return 0;
}

static int fd_read(int fd, void *buf, size_t len)
{
  int bytes_read;
  bytes_read = read(fd,buf,len);
  return bytes_read;
}

static int fd_write(int fd, void *buf, size_t len)
{
  int bytes_written;
  bytes_written = write(fd,buf,len);
  return bytes_written;
}

static int fdcopy(int fdin, int fdout)
{
  char buf[SIZE];
  int n;

  if((n=fd_read(fdin,buf,SIZE))<0)
  {
    fprintf(stderr,"Read Error\n");
    exit(1);
  }

  if(n==0)
    return 1;

  if(fd_write(fdout,buf,n)!=n)
  {
    fprintf(stderr,"Write Error\n");
    exit(1);
  }

  return 0;
}

static void wait_and_act(int sockfd, int verbose)
{
  struct timeval timeout;
  int rc; /* select() return code */
  long timeout_ms = 10000;

  fd_set fdread;
  fd_set fdwrite;
  fd_set fdexcep;
  int maxfd=sockfd;

  while(1==1)
  {
    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);

    /* set timeout to wait */
    timeout.tv_sec = timeout_ms/1000;
    timeout.tv_usec = (timeout_ms%1000)*1000;

    FD_SET(fileno(stdin), &fdread);
    FD_SET(sockfd, &fdread);
    /* FD_SET(sockfd, &fdwrite); */

    rc = select(maxfd+1,
          (fd_set *)&fdread,
          (fd_set *)&fdwrite,
          (fd_set *)&fdexcep, &timeout);

    if(rc==-1) {
      /* select error */
      fprintf(stderr,"select() error\n");
      break;
    }
    if(rc==0){
      /* timeout! */
      /* fprintf(stderr,"Timeout\n"); */
      if(verbose)
          fprintf(stderr,"VERBOSE: Timeout after %ldms\n", timeout_ms);
    }
    else
    {
      /* use FD_ISSET() to check what happened, then read/write accordingly */
      if (FD_ISSET(fileno(stdin), &fdread))
      {
        rc=fdcopy(fileno(stdin),sockfd);
        if(!rc)
          break;
        if(verbose)
          fprintf(stderr,"VERBOSE: Sent %d bytes from stdin to socket\n", rc);
      }
      if (FD_ISSET(sockfd,&fdread))
      {
        rc=fdcopy(sockfd,fileno(stdout));
        if(!rc)
          break;
        if(verbose)
          fprintf(stderr,"VERBOSE: Sent %d bytes from socket to stdin\n", rc);
      }
    }
  }
}

int main(int argc, char *argv[])
{
  CURLcode ret;
  long sckt;
  struct gengetopt_args_info args_info;

  command_line_parser(argc,argv,&args_info);

  CURL *hnd = curl_easy_init();

  curl_easy_setopt(hnd, CURLOPT_URL, args_info.dest_arg);
  curl_easy_setopt(hnd, CURLOPT_PROXY, args_info.proxy_arg);
  curl_easy_setopt(hnd, CURLOPT_PROXYUSERPWD, args_info.user_arg);
  curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curltunnel/" VERSION);
  curl_easy_setopt(hnd, CURLOPT_HTTPPROXYTUNNEL, 1);
  curl_easy_setopt(hnd, CURLOPT_CONNECT_ONLY, 1);
  if(args_info.verbose_given)
    curl_easy_setopt(hnd, CURLOPT_VERBOSE, 1);
  ret = curl_easy_perform(hnd);

  if(ret == CURLE_OK)
  {
    /* Extract socket, and select() */
    ret = curl_easy_getinfo(hnd, CURLINFO_LASTSOCKET, &sckt);
  }
    if((sckt==-1) || ret){
      fprintf(stderr,"[ERR] Couldn't get socket");
    return 1;
  }

  wait_and_act((int)sckt, args_info.verbose_given);

  curl_easy_cleanup(hnd);
  return 0;
}
