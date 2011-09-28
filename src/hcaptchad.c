#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <paths.h>
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

#include "../deps/hiredis/sds.h"    /* Dynamic safe strings */

#define MIN(a,b)        ((a)>(b)?(b):(a))
#define HCS_PID         "/tmp/hcaptchad.pid"
#define HCS_SIGNATURE   "hcaptcha/1.0.0lab"
#define KEY_LEN         16
#define CONFIGLINE_MAX  1024


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
  char  *symbols;
  char  *servers;
  int   img_width;
  int   img_height;
  int   len;
  char  *fonts;
  double  ftsize;
  int   fluctuation_amplitude;
  char  *log;
  int   img_timeout;
  int   img_bg_color[3];
  int   img_fg_color[3];
  int   length[2];
  char  *file;
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
  int i, x, y, brect[8];
  char *err, s[2];
  int ft_color, bg_color, trans;
  
  gdImagePtr im;
  
  s_fts_len = strlen(cfg.symbols);
  
  for (i = 0; i < s_fts_len; i++) {
  
    if (!isalpha(cfg.symbols[i]) && !isdigit(cfg.symbols[i])) {
      //err(1, "failed to create response buffer");
      continue;
    }
    
    sprintf(s, "%c", cfg.symbols[i]);
    err = gdImageStringFT(NULL,&brect[0],0,cfg.fonts,cfg.ftsize,0.,0,0,s);
    if (err) {
      //fprintf(stderr,err);
      continue;
    }

    x = brect[2] - brect[6];
    y = brect[3] - brect[7];
    
    fts[i].c  = cfg.symbols[i];
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
    err = gdImageStringFT(im, &brect[0], ft_color, cfg.fonts, cfg.ftsize, 0.0, x, y, s);
    if (err) {
      continue;
    }
    
    fts[i].i  = im;
  }
}

char * data_build(char *key, size_t *imosize)
{
  int i, j;
  gdImagePtr img, imo;

  img = gdImageCreateTrueColor(cfg.img_width, cfg.img_height);
  gdImageAlphaBlending(img, 1);

  int white = gdImageColorAllocate(img, 255, 255, 255);
  //int black = gdImageColorAllocate(img, 0, 0, 0);
  
  gdImageFilledRectangle(img, 0, 0, cfg.img_width - 1, cfg.img_height - 1, white);

  int x = 1, y = 1, shift = 0, sx, sy;
  int odd = rand()%1;
  if (odd == 0) odd =-1;
  float color, color_x, color_y, color_xy;
  
  srand(time(0));
  
  int word_len = (rand() % (cfg.length[1] - cfg.length[0] + 1)) + cfg.length[0];
  char word[word_len];
  
  for (i = 0; i < word_len; i++) {
  
    j = rand() % s_fts_len;
    
    y = ((i%2) * cfg.fluctuation_amplitude - cfg.fluctuation_amplitude/2) * odd
      + (rand() % ((int)(cfg.fluctuation_amplitude*0.66 + 0.5)) - ((int)(cfg.fluctuation_amplitude*0.33 + 0.5)) )
      + (cfg.img_height - cfg.ftsize)/2;
					
    if (i > 0) shift = rand() % 2 + 4;
    
    gdImageCopy(img, fts[j].i, x - shift, y, 0, 0, fts[j].x, fts[j].y);
    
    x += fts[j].x - shift;
    word[i] = fts[j].c;
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
  
  
  //int fg_color[3] = {0, 0, 0};  
  //int bg_color[3] = {220, 230, 255};
  float newr, newg, newb;
  float frsx, frsy, frsx1, frsy1;
  float newcolor, newcolor0;
  
  imo = gdImageCreateTrueColor(cfg.img_width, cfg.img_height);
  gdImageAlphaBlending(imo, 1);
  //int fg = gdImageColorAllocate(imo, cfg.img_fg_color[0], cfg.img_fg_color[1], cfg.img_fg_color[2]);
  int bg = gdImageColorAllocate(imo, cfg.img_bg_color[0], cfg.img_bg_color[1], cfg.img_bg_color[2]);
  gdImageFilledRectangle(imo, 0, 0, cfg.img_width - 1, cfg.img_height - 1, bg);
  //gdImageFilledRectangle(imo, 0, cfg.img_height, cfg.img_width - 1, cfg.img_height, fg);

  for (x = 0; x < cfg.img_width; x++) {
    for (y = 0; y < cfg.img_height; y++) {

      sx = x +(sin(x*rand1+rand5)+sin(y*rand3+rand6))*rand9-cfg.img_width/2+center+1;
      sy = y +(sin(x*rand2+rand7)+sin(y*rand4+rand8))*rand10;

      if (sx<0 || sy<0 || sx>=cfg.img_width-1 || sy>=cfg.img_height-1){
        continue;
      } else {
        color   = gdImageTrueColorPixel(img, sx, sy)    & 0xFF;
        color_x = gdImageTrueColorPixel(img, sx+1, sy)  & 0xFF;
        color_y = gdImageTrueColorPixel(img, sx, sy+1)  & 0xFF;
        color_xy= gdImageTrueColorPixel(img, sx+1, sy+1)& 0xFF;
      }

      if (color==255 && color_x==255 && color_y==255 && color_xy==255){
        continue;
      } else if(color==0 && color_x==0 && color_y==0 && color_xy==0){
        newr  = cfg.img_fg_color[0];
        newg  = cfg.img_fg_color[1];
        newb  = cfg.img_fg_color[2];
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

        newr  = newcolor0 * cfg.img_fg_color[0] + newcolor * cfg.img_bg_color[0];
        newg  = newcolor0 * cfg.img_fg_color[1] + newcolor * cfg.img_bg_color[1];
        newb  = newcolor0 * cfg.img_fg_color[2] + newcolor * cfg.img_bg_color[2];
      }

      gdImageSetPixel(imo, x, y, gdImageColorAllocate(imo, newr, newg, newb));
    }
  }


  //int imosize;
  int imos;
  char *imodata = (char *) gdImagePngPtr(imo, &imos);
  if (!imodata) {
    return NULL;
  }

  memcached_return rc;
  // Save Key-Word
  char key2[100];
  sprintf(key2, "%sv", key);
  rc = memcached_set(memc, key2, strlen(key2), word, strlen(word), cfg.img_timeout, 0);
  if (rc != MEMCACHED_SUCCESS) {
    //fprintf(stderr, "hash: memcache error %s\n", memcached_strerror(memc, rc));
    return NULL;
  } 
  // Save Key-Binary
  rc = memcached_set(memc, key, strlen(key), imodata, imos, cfg.img_timeout, 0);
  if (rc != MEMCACHED_SUCCESS) {
    //fprintf(stderr, "hash: memcache error %s\n", memcached_strerror(memc, rc));
    return NULL;
  }  
  *imosize = (size_t)imos;
  
  //free(imodata);
  gdImageDestroy(img);
  gdImageDestroy(imo);
  
  return imodata;
}

char * data_get(char *k, size_t *imosize)
{
  memcached_return rc;
  uint32_t flags;
  
  char *v2 = memcached_get(memc, k, strlen(k), imosize, &flags, &rc);
  if (rc != MEMCACHED_SUCCESS) {
    return NULL;
  }
  
  return v2;
}

int data_del(char *k)
{
  memcached_return rc = memcached_delete(memc, k, strlen(k), 0);
  
  if (rc != MEMCACHED_SUCCESS) {
    return 1;
  }
  
  return 0;
}

int data_exists(char *k)
{  
  memcached_return rc;
  size_t ims;
  uint32_t flags;

  char *v2 = memcached_get(memc, k, strlen(k), &ims, &flags, &rc);
  if (rc != MEMCACHED_SUCCESS) {
    return 1;
  }
  free(v2);
  
  return 0;
}

int data_check(char *k, char *v)
{
  char *k2 = strcat(k, "v");
  
  memcached_return rc;
  size_t ims;
  uint32_t flags;
  char *v2 = memcached_get(memc, k2, strlen(k2), &ims, &flags, &rc);
  if (rc != MEMCACHED_SUCCESS) {
    return 1;
  }  
  if (strcmp(v, v2) != 0) {
    return 1;
  }
  
  return 0;
}

void http_service_handler(struct evhttp_request *req, void *arg)
{
  /**  **/
  struct evbuffer *evbuf;
  evbuf = evbuffer_new();
  if (evbuf == NULL) err(1, "failed to create response buffer");
  
  evhttp_add_header(req->output_headers, "Server", HCS_SIGNATURE);
  
  /** Image Builder **/
  char *im = NULL;
  size_t ims;
  
  struct evkeyvalq hcs_http_query;
  evhttp_parse_query(evhttp_request_uri(req), &hcs_http_query);
  char *skey = (char *)evhttp_find_header (&hcs_http_query, "session");
  char *word = (char *)evhttp_find_header (&hcs_http_query, "word");
  char *action = (char *)evhttp_find_header (&hcs_http_query, "action");
  if (action == NULL) {
    action = "get";
  }
  
  if (skey == NULL || strlen(skey) < 1) {
  
    //evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=utf-8");
    evbuffer_add_printf(evbuf, "%d,%s", HTTP_BADREQUEST, "BAD REQUEST");
    evhttp_send_reply(req, HTTP_BADREQUEST, "BAD REQUEST", evbuf);
    
  } else if (word != NULL) {
  
    if (data_check(skey, word) == 0)
      evbuffer_add_printf(evbuf, "%s", "OK");  
    else
      evbuffer_add_printf(evbuf, "%s", "ERROR");  
    
    data_del(skey);   
    evhttp_send_reply(req, HTTP_OK, "OK", evbuf);
    
  } else if (strcmp(action, "new") == 0) {
  
    im = data_build(skey, &ims);
    if (im == NULL) {
      evhttp_send_reply(req, HTTP_SERVUNAVAIL, "SERVUNAVAIL", evbuf);
      evbuffer_add_printf(evbuf, "%s", "SERVUNAVAIL");  
    } else {
      evhttp_add_header(req->output_headers, "Content-Type", "image/png");
      evbuffer_add(evbuf, im, ims);
      evhttp_send_reply(req, HTTP_OK, "OK", evbuf);
    }
    
  } else {
  
    if (data_exists(skey) == 0)
      im = data_get(skey, &ims);
    else 
      im = data_build(skey, &ims);
    
    if (im == NULL) {
      evhttp_send_reply(req, HTTP_SERVUNAVAIL, "SERVUNAVAIL", evbuf);
      evbuffer_add_printf(evbuf, "%s", "SERVUNAVAIL");
    } else {
      evhttp_add_header(req->output_headers, "Content-Type", "image/png");
      evbuffer_add(evbuf, im, ims);
      evhttp_send_reply(req, HTTP_OK, "OK", evbuf);
    }
  }
  //evhttp_send_reply(req, HTTP_SERVUNAVAIL, "SERVUNAVAIL", evbuf);
  //evbuffer_add_printf(evbuf, "%s", "SERVUNAVAIL");  
  evhttp_add_header(req->output_headers, "Connection", "close");  
  

  evhttp_clear_headers(&hcs_http_query);
  evbuffer_free(evbuf);
  
  free(im);
}

void loadConfig(char *filename) 
{
    FILE *fp;
    char buf[CONFIGLINE_MAX+1];
    int linenum = 0;
    sds line = NULL;

    if (filename[0] == '-' && filename[1] == '\0')
        fp = stdin;
    else {
        if ((fp = fopen(filename,"r")) == NULL) {
            //redisLog(REDIS_WARNING, "Fatal error, can't open config file '%s'", filename);
            exit(1);
        }
    }
    
    while(fgets(buf,CONFIGLINE_MAX+1,fp) != NULL) {
        sds *argv;
        int argc;

        linenum++;
        line = sdsnew(buf);
        line = sdstrim(line," \t\r\n");

        /* Skip comments and blank lines*/
        if (line[0] == '#' || line[0] == '\0') {
            sdsfree(line);
            continue;
        }

        /* Split into arguments */
        argv = sdssplitargs(line,&argc);
        sdstolower(argv[0]);
        
        
        /* Execute config directives */
        if (!strcasecmp(argv[0],"timeout") && argc == 2) {
            cfg.timeout = atoi(argv[1]);
        } else if (!strcasecmp(argv[0],"port") && argc == 2) {
            cfg.port = atoi(argv[1]);
        } else if (!strcasecmp(argv[0],"servers") && argc == 2) {
            cfg.servers = strdup(argv[1]);
        } else if (!strcasecmp(argv[0],"symbols") && argc == 2) {
            cfg.symbols = strdup(argv[1]);
        } else if (!strcasecmp(argv[0],"img_size") && argc == 3) {
            cfg.img_width   = atoi(argv[1]);
            cfg.img_height  = atoi(argv[2]);
        } else if (!strcasecmp(argv[0],"fluctuation_amplitude") && argc == 2) {
            cfg.fluctuation_amplitude  = atoi(argv[1]);
        } else if (!strcasecmp(argv[0],"img_foreground_color") && argc == 4) {
            cfg.img_fg_color[0] = atoi(argv[1]);
            cfg.img_fg_color[1] = atoi(argv[2]);
            cfg.img_fg_color[2] = atoi(argv[3]);
        } else if (!strcasecmp(argv[0],"img_background_color") && argc == 4) {
            cfg.img_bg_color[0] = atoi(argv[1]);
            cfg.img_bg_color[1] = atoi(argv[2]);
            cfg.img_bg_color[2] = atoi(argv[3]);
        } else if (!strcasecmp(argv[0],"length") && argc == 3) {
            cfg.length[0] = atoi(argv[1]);
            cfg.length[1] = atoi(argv[2]);
        } 
        
    }
}

int main(int argc, char **argv)
{
  int opt;

  cfg.port    = 9527;
  cfg.daemon  = 0;
  cfg.timeout = 3;
  cfg.servers = "127.0.0.1:11211";
  
  cfg.fonts   = "../fonts/cmr10.ttf";
  cfg.symbols   = "23456789abcdegikpqsvxyz";
  
  cfg.img_width  = 160;
  cfg.img_height = 60;
  
  cfg.ftsize    = cfg.img_height / 2;
  cfg.fluctuation_amplitude     = 10; // symbol's vertical fluctuation amplitude
  
  cfg.log       = "/var/log/hcaptcha.log";
  cfg.file      = "../hcaptchad.conf";
  
  cfg.img_bg_color[0] = 230;
  cfg.img_bg_color[1] = 230;
  cfg.img_bg_color[2] = 230;
  
  cfg.img_fg_color[0] = 0;
  cfg.img_fg_color[1] = 0;
  cfg.img_fg_color[2] = 2030;
  
  cfg.length[0] = 4;
  cfg.length[1] = 6;
  
  cfg.img_timeout = 3600;

  while ((opt = getopt(argc, argv, "df:p:t:c:s:w:h:")) != -1) {
    switch (opt) {
      case 'd': cfg.daemon  = true; break;
      case 'f': cfg.fonts   = strdup(optarg); break;
      case 'p': cfg.port    = atoi(optarg); break;      
      case 'c': cfg.file    = strdup(optarg); break;
      case 's': cfg.servers = strdup(optarg); break;
      case 't': cfg.img_timeout = atoi(optarg); break;
      default : break;
    }
  }

  loadConfig(cfg.file);
  
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
