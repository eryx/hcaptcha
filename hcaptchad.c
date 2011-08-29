#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <err.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <gd.h>
#include <event.h>
#include <evhttp.h>
#include <libmemcached/memcached.h>

#define MIN(a,b)        ((a)>(b)?(b):(a))
#define HCS_PID         "/tmp/hcaptchad.pid"
#define HCS_SIGNATURE   "hcaptcha/1.0.0lab"
#define KEY_LEN         16

static memcached_st *memc = NULL;

struct s_fts {
  int   x;
  int   y;
  char  c;
  gdImagePtr  i;
} fts[36];
int s_fts_len = 36;

struct app_cfg {
  int   port;
  bool  daemon;
  int   timeout;
  char  *chars;
  char  *servers;
  int   width;
  int   height;
  int   len;
  double  ftsize;
} cfg;

void signal_handler(int sig)
{
  switch (sig) {
    case SIGTERM:
    case SIGHUP:
    case SIGQUIT:
    case SIGINT:
      event_loopbreak();
      break;
  }
}

void storage_setup_memcached()
{
  memcached_return rc;
  memcached_server_st *servers;
  
  memc = memcached_create(NULL);
  //process_hash_option(memc, opt_hash);
  rc = memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, MEMCACHED_HASH_DEFAULT);
  if (rc != MEMCACHED_SUCCESS) {
    fprintf(stderr, "hash: memcache error %s\n", memcached_strerror(memc, rc));
    exit(1);
  }
  
  servers = memcached_servers_parse(cfg.servers);
  memcached_server_push(memc, servers);
  memcached_server_list_free(servers);
  //memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 0);

  return;
}


void font_setup()
{
  char *f = "./fonts/cmr10.ttf";
  
  int brect[8];
  int i, x, y;
  char *err;
  int ft_color, bg_color, trans;

  char s[2];// = "a"; /* String to draw. */
  
  gdImagePtr im;
  
  s_fts_len = strlen(cfg.chars);
  
  for (i = 0; i < s_fts_len; i++) {
  
    if (!isalpha(cfg.chars[i]) && !isdigit(cfg.chars[i])) {
      //err(1, "failed to create response buffer");
      continue;
    }
    
    sprintf(s, "%c", cfg.chars[i]);
    err = gdImageStringFT(NULL,&brect[0],0,f,cfg.ftsize,0.,0,0,s);
    if (err) {
      //fprintf(stderr,err);
      continue;
    }

    x = brect[2] - brect[6];
    y = brect[3] - brect[7];
    
    fts[i].c  = cfg.chars[i];
    fts[i].x  = x;
    fts[i].y  = y;
    
    im = gdImageCreate(x, y);
    
    bg_color = gdImageColorResolve(im, 255, 255, 255);
    ft_color = gdImageColorResolve(im, 0, 0, 0);
    trans  = gdImageColorAllocate(im, 0, 0, 0);
    gdImageFilledRectangle(im, 0, 0, x, y, trans);
    gdImageColorTransparent(im, trans);
    
    x = 0 - brect[6];
    y = 0 - brect[7];
    err = gdImageStringFT(im, &brect[0], ft_color, f, cfg.ftsize, 0.0, x, y, s);
    if (err) {
      //fprintf(stderr,err); 
      //return 1;
      continue;
    }
    
    fts[i].i  = im;
  }
}

void img_build(char *key)
{
  //printf("log: img_build #%s \n", key);
  
  int i, j;
  gdImagePtr img, imo;

  img = gdImageCreateTrueColor(cfg.width, cfg.height);
  gdImageAlphaBlending(img, 1);

  int white = gdImageColorAllocate(img, 255, 255, 255);
  //int black = gdImageColorAllocate(img, 0, 0, 0);
  
  gdImageFilledRectangle(img, 0, 0, cfg.width - 1, cfg.height - 1, white);

  int x = 1, y = 1, shift = 0, sx, sy;
  int odd = rand()%1;
  if (odd == 0) odd =-1;
  float color, color_x, color_y, color_xy;
  
  srand(time(0));
  
  int word_len = rand() % 3 + 4;

  for (i = 0; i < word_len; i++) {
  
    j = rand() % s_fts_len;
    
    y = ((i%2) * 8 - 4) * odd
      + (rand() % 1  + 2)
      + (cfg.height - cfg.ftsize)/2;
    
    if (i > 0) shift = rand() % 2 + 4;
    
    gdImageCopy(img, fts[j].i, x - shift, y, 0, 0, fts[j].x, fts[j].y);
    
    x += fts[j].x - shift;
  }
  
  int center = x/2;

  // periods
  float rand1 = (rand() % 45 + 75)/1000.0;
  float rand2 = (rand() % 45 + 75)/1000.0;
  float rand3 = (rand() % 45 + 75)/1000.0;
  float rand4 = (rand() % 45 + 75)/1000.0;
  // phases
  float rand5 = (rand() % 314)/100.0;
  float rand6 = (rand() % 314)/100.0;
  float rand7 = (rand() % 314)/100.0;
  float rand8 = (rand() % 314)/100.0;
  // amplitudes
  float rand9 = (rand() %  90 + 330)/110.0;
  float rand10= (rand() % 120 + 330)/100.0;
  
  
  int fg_color[3] = {0, 0, 0};  
  int bg_color[3] = {220, 230, 255};
  float newr, newg, newb;
  float frsx, frsy, frsx1, frsy1;
  float newcolor, newcolor0;
  
  imo = gdImageCreateTrueColor(cfg.width, cfg.height);
  gdImageAlphaBlending(imo, 1);
  //int fg = gdImageColorAllocate(imo, fg_color[0], fg_color[1], fg_color[2]);
  int bg = gdImageColorAllocate(imo, bg_color[0], bg_color[1], bg_color[2]);
  gdImageFilledRectangle(imo, 0, 0, cfg.width - 1, cfg.height - 1, bg);
  //gdImageFilledRectangle(imo, 0, cfg.height, cfg.width - 1, cfg.height, fg);

  for (x = 0; x < cfg.width; x++) {
    for (y = 0; y < cfg.height; y++) {

      sx = x +(sin(x*rand1+rand5)+sin(y*rand3+rand6))*rand9-cfg.width/2+center+1;
      sy = y +(sin(x*rand2+rand7)+sin(y*rand4+rand8))*rand10;

      if (sx<0 || sy<0 || sx>=cfg.width-1 || sy>=cfg.height-1){
        continue;
      } else {
        color   = gdImageTrueColorPixel(img, sx, sy)  & 0xFF;
        color_x = gdImageTrueColorPixel(img, sx+1, sy)  & 0xFF;
        color_y = gdImageTrueColorPixel(img, sx, sy+1)  & 0xFF;
        color_xy= gdImageTrueColorPixel(img, sx+1, sy+1)& 0xFF;
      }

      if (color==255 && color_x==255 && color_y==255 && color_xy==255){
        continue;
      } else if(color==0 && color_x==0 && color_y==0 && color_xy==0){
        newr  = fg_color[0];
        newg  = fg_color[1];
        newb  = fg_color[2];
      } else {
        frsx  = sx - floor(sx);
        frsy  = sy - floor(sy);
        frsx1 = 1 - frsx;
        frsy1 = 1 - frsy;

        newcolor = ( (color * frsx1 * frsy1) + (color_x * frsx * frsy1)
          + (color_y * frsx1 * frsy) + (color_xy * frsx * frsy) );

        if (newcolor > 255) newcolor = 255;
        newcolor  = newcolor/255;
        newcolor0 = 1 - newcolor;

        newr  = newcolor0 * fg_color[0] + newcolor * bg_color[0];
        newg  = newcolor0 * fg_color[1] + newcolor * bg_color[1];
        newb  = newcolor0 * fg_color[2] + newcolor * bg_color[2];
      }

      gdImageSetPixel(imo, x, y, gdImageColorAllocate(imo, newr, newg, newb));
    }
  }


  int imosize;
  char *imodata = (char *) gdImagePngPtr(imo, &imosize);
  if (!imodata) {
    /* Error */
  }

  memcached_return rc;
  //rc = memcached_set(memc, key, strlen(key), imodata, imosize, 60, 0);
  rc = memcached_set(memc, key, strlen(key), imodata, imosize, 60, 0);
  if (rc != MEMCACHED_SUCCESS) {
    fprintf(stderr, "hash: memcache error %s\n", memcached_strerror(memc, rc));
    //exit(1);
  }
  
  free(imodata);
  gdImageDestroy(img);
  gdImageDestroy(imo);
  
  return;
}


char * utils_string_rand(int len) 
{
  char *str = (char *)malloc(len + 1);
  int i;

  if (str == 0) return 0;

  srand(time(NULL) + rand() + clock());
  
  for (i = 0; i < len; i++)
    str[i] = rand()%10 + 48;

  str[i] = 0;

  return str;
}

void http_service_handler(struct evhttp_request *req, void *arg)
{
  char *key = utils_string_rand(KEY_LEN);
  img_build(key);
  
  memcached_return rc;
  size_t ims;
  uint32_t flags;
  char *im = memcached_get(memc, key, strlen(key), &ims, &flags, &rc);
  if (rc != MEMCACHED_SUCCESS) {
    fprintf(stderr, "hash: memcache error %s\n", memcached_strerror(memc, rc));
    //exit(1);
  }

  struct evbuffer *evbuf;
  evbuf = evbuffer_new();
  if (evbuf == NULL)
    err(1, "failed to create response buffer");
    
  evhttp_add_header(req->output_headers, "Server", HCS_SIGNATURE);
  
  evhttp_add_header(req->output_headers, "Content-Type", "image/png");
  //evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=GB2312");
  
  evhttp_add_header(req->output_headers, "Connection", "close");
  //evhttp_add_header(req->output_headers, "Keep-Alive", "120");

  
  evbuffer_add(evbuf, im, ims);
  //evbuffer_add_printf(evbuf, "%s", "test");
  
  evhttp_send_reply(req, HTTP_OK, "OK", evbuf);
  evbuffer_free(evbuf);
  
  free(im);
  free(key);
}

int main(int argc, char **argv)
{
  int opt;

  cfg.port  = 9527;
  cfg.daemon  = 0;
  cfg.timeout = 3;
  cfg.chars   = "23456789abcdegikpqsvxyz";
  cfg.servers = "127.0.0.1:11211";
  cfg.width   = 160;
  cfg.height  = 60;
  cfg.ftsize  = cfg.height / 2;

  while ((opt = getopt(argc, argv, "dp:t:c:s:w:h:")) != -1) {
    switch (opt) {
      case 'd': cfg.daemon  = true; break;
      case 'p': cfg.port    = atoi(optarg); break;
      case 't': cfg.timeout = atoi(optarg); break;
      case 'c': cfg.chars   = strdup(optarg); break;
      case 's': cfg.servers = strdup(optarg); break;
      case 'w': cfg.width   = atoi(optarg); break;
      case 'h': cfg.height  = atoi(optarg); break;
      default : break;
    }
  }

  if (cfg.daemon == true) {
    pid_t pid = fork();    
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);
  }

  //
  signal(SIGHUP,  signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGINT,  signal_handler);
  signal(SIGQUIT, signal_handler);
  //signal(SIGPIPE, SIG_IGN);

  //
  FILE *fp_pidfile = fopen(HCS_PID, "w");
  fprintf(fp_pidfile, "%d\n", getpid());
  fclose(fp_pidfile);

  //
  storage_setup_memcached();
  font_setup();

  //
  struct evhttp *httpd;
  event_init();
  httpd = evhttp_start("0.0.0.0", cfg.port);
  if (httpd == NULL) {
    fprintf(stderr, "Error: Unable to listen on 0.0.0.0:%d\n\n", cfg.port);
    exit(1);
  }
  evhttp_set_timeout(httpd, cfg.timeout);

  /* Set a callback for requests to "/specific". */
  /* evhttp_set_cb(httpd, "/select", select_handler, NULL); */

  /* Set a callback for all other requests. */
  evhttp_set_gencb(httpd, http_service_handler, NULL);

  event_dispatch();

  /* Not reached in this code as it is now. */
  evhttp_free(httpd);

  if (memc)
    memcached_free(memc);
  
  return 0;
}
