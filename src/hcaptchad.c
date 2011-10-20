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
#include "config.h"

#define MIN(a,b)        ((a)>(b)?(b):(a))

static memcached_st *memc = NULL;

struct configObject cfg;

struct s_fts {
  int   x;
  int   y;
  char  c;
  gdImagePtr  i;
} fts[36];
int s_fts_len = 36;

void signal_handler(int sig)
{
  switch (sig) {
    case SIGTERM:
    case SIGHUP:
    case SIGQUIT:
    case SIGINT:
      event_loopbreak();
      printf("\nSignal Stop %s [OK]\n\n", HCS_SIGNATURE);
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
  
  gdFTStringExtra strex = {0};
  
  gdImagePtr im;
  
  s_fts_len = strlen(cfg.symbols);
  
  for (i = 0; i < s_fts_len; i++) {
  
    if (!isalpha(cfg.symbols[i]) && !isdigit(cfg.symbols[i])) {
      fprintf(stderr, "failed to create response buffer\n");
      exit(1);
    }
    
    sprintf(s, "%c", cfg.symbols[i]);
    err = gdImageStringFT(NULL,&brect[0],0,cfg.font,cfg.font_size,0.,0,0,s);
    if (err) {
      fprintf(stderr, "Failed to font '%s'\n", cfg.font);
      exit(1);
    }

    x = brect[2] - brect[6];
    y = brect[3] - brect[7];
    
    fts[i].c  = cfg.symbols[i];
    fts[i].x  = x;
    fts[i].y  = y;
    
    im = gdImageCreateTrueColor(x, y);
    gdImageAlphaBlending(im, 0);
    gdImageSaveAlpha(im, 1);
    
    bg_color = gdImageColorAllocate(im, 255, 255, 255);
    ft_color = gdImageColorAllocate(im, 0, 0, 0);
    trans    = gdImageColorAllocateAlpha(im, 255, 255, 255, 127);    
    
    gdImageFilledRectangle(im, 0, 0, 74, 74, trans);
    
    x = 0 - brect[6];
    y = 0 - brect[7];
    err = gdImageStringFTEx(im, brect, ft_color, cfg.font, cfg.font_size, 0.0, x, y, s, &strex);
    if (err) {
      fprintf(stderr, "Failed to font '%s'\n", cfg.font);
      exit(1);
    }
    
    fts[i].i = im;
  }
}

char * data_build(char *key, size_t *imosize)
{
  int i, j;
  gdImagePtr img, imo;

  img = gdImageCreateTrueColor(cfg.img_width, cfg.img_height);
  gdImageAlphaBlending(img, 1);

  int white = gdImageColorAllocate(img, 255, 255, 255);
  
  gdImageFilledRectangle(img, 0, 0, cfg.img_width - 1, cfg.img_height - 1, white);

  int x = 1, y = 1, shift = 0, sx, sy;
  int rgb, opacity, left, px, py; 
  int odd = rand()%2;
  if (odd == 0) odd =-1;
  float color, color_x, color_y, color_xy;
  
  int word_len = (rand() % (cfg.length[1] - cfg.length[0] + 1)) + cfg.length[0];
  char word[word_len];
  
  for (i = 0; i < word_len; i++) {
  
    j = rand() % s_fts_len;
    
    y = ((i%2) * cfg.fluctuation_amplitude - cfg.fluctuation_amplitude/2) * odd
      + (rand() % ((int)(cfg.fluctuation_amplitude*0.33 + 0.5) * 2 + 1) - ((int)(cfg.fluctuation_amplitude*0.33 + 0.5)))
      + (cfg.img_height - cfg.font_size)/2;

    shift = 0;

    if (i > 0) {

      shift=10000;
      
      //printf("x %d y %d\n", fts[j].x, fts[j].y);
      
      for (sy = 1; sy < fts[j].y; sy++) {
        for (sx = 1; sx < fts[j].x; sx++) {
          //printf("sx %d sy %d\n", sx, sy);      
          rgb = gdImageTrueColorPixel(fts[j].i, sx, sy);
          opacity = rgb>>24;
          if (opacity < 127) {
            left = sx - 1 + x;
            py = sy + y;
            if (py > cfg.img_height) break;
            for (px = MIN(left, cfg.img_width-1); px > left-200 && px >= 0; px -= 1) {
              color = gdImageTrueColorPixel(img, px, py) & 0xff;
              if (color + opacity<170) { // 170 - threshold
                if (shift > left - px) {
                  shift = left - px;
                }
                break;
              }
            }
            break;
          }
        }
      }
      
      if (shift == 10000){
        shift = rand() % 3 + 4;
      }
    }
    
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
  
  float newr, newg, newb;
  float frsx, frsy, frsx1, frsy1;
  float newcolor, newcolor0;
  
  imo = gdImageCreateTrueColor(cfg.img_width, cfg.img_height);
  gdImageAlphaBlending(imo, 1);
  int bg = gdImageColorAllocate(imo, cfg.img_bg_color[0], cfg.img_bg_color[1], cfg.img_bg_color[2]);
  gdImageFilledRectangle(imo, 0, 0, cfg.img_width - 1, cfg.img_height - 1, bg);

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


int main(int argc, char **argv)
{
  int opt;
  char *config_file = NULL;

  while ((opt = getopt(argc, argv, "c:")) != -1) {
    switch (opt) {
      case 'c': config_file = strdup(optarg); break;
      default : break;
    }
  }
  
  if (config_file == NULL) {
    fprintf(stderr, "Fatal error, no config file setting '%s'\n\n", "-c /path/of/hcaptcha.conf");
    exit(1);
  }

  initConfig();
  loadConfig(config_file);
  
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
  FILE *fp_pidfile = fopen(cfg.pidfile, "w");
  if (fp_pidfile == NULL) {
    fprintf(stderr, "Error: Can not create pidfile '%s'\n\n", cfg.pidfile);
    exit(1);
  }
  fprintf(fp_pidfile, "%d\n", getpid());
  fclose(fp_pidfile);

  //
  storage_setup_memcached();
  font_setup();

  printf("Starting %s [OK]\n\n", HCS_SIGNATURE);

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
