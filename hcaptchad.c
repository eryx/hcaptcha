#include <stdio.h>
#include <stdlib.h>

#include <string.h>
//#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
//#include <sys/random.h>
#include <err.h>

#include <time.h>

#include <ctype.h>

#include <math.h>
#include <gd.h>
#include <event.h>
#include <evhttp.h>
#include <libmemcached/memcached.h>

#define MIN(a,b)        ((a)>(b)?(b):(a))
#define HCS_PID         "/tmp/hcs.pid"
#define HCS_SIGNATURE   "hcaptcha/1.0.0lab"
#define KEY_LEN         16

static memcached_st *memc = NULL;

// fonts
int ffont_height;

struct s_fsymbol {
    int     start;
    int     end;
    char    symbol;
} fsymbol[36];

struct s_fts {
    int     x;
    int     y;
    char    c;
    gdImagePtr  i;
} fts[36];
int s_fts_len = 36;

struct httpd_options {
    int     port;
    bool    daemon;
    int     timeout;
    char    *symbols;
    char    *servers;
} options;

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

void memcached_setup()
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
    
  	servers = memcached_servers_parse(options.servers);
  	memcached_server_push(memc, servers);
  	memcached_server_list_free(servers);
  	//memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 0);

	return;
}


void fonts_setup()
{
    char *f = "./fonts0/cmr10.ttf";
    
    int brect[8];
    int x, y;
    char *err;
    int ft_color, bg_color, trans;

    char s[2];// = "a"; /* String to draw. */
    double sz = 30.0;
    
    gdImagePtr im;
    int i;
    
    s_fts_len = strlen(options.symbols);
    
    for (i=0; i<s_fts_len; i++) {
    
        if (!isalpha(options.symbols[i]) && !isdigit(options.symbols[i])) {
            //err(1, "failed to create response buffer");
            continue;
        }
        
        sprintf(s, "%c", options.symbols[i]);
        err = gdImageStringFT(NULL,&brect[0],0,f,sz,0.,0,0,s);
        if (err) {
            //fprintf(stderr,err);
            continue;
        }

        x = brect[2] - brect[6];
        y = brect[3] - brect[7];
        
        fts[i].c    = options.symbols[i];
        fts[i].x    = x;
        fts[i].y    = y;
        
        im = gdImageCreate(x, y);
        
        bg_color = gdImageColorResolve(im, 255, 255, 255);
        ft_color = gdImageColorResolve(im, 0, 0, 0);
        trans    = gdImageColorAllocate(im, 0, 0, 0);
        gdImageFilledRectangle(im, 0, 0, x, y, trans);
        gdImageColorTransparent(im, trans);
        
        x = 0 - brect[6];
        y = 0 - brect[7];
        err = gdImageStringFT(im, &brect[0], ft_color, f, sz, 0.0, x, y, s);
        if (err) {
            //fprintf(stderr,err); 
            //return 1;
            continue;
        }
        
        fts[i].i    = im;
    }
}

void captcha_create(char *key)
{
    printf("log: captcha_create #%s \n", key);
    
    int i, j;    
    gdImagePtr img, imo;
    int width = 560, height = 80;

    img = gdImageCreateTrueColor(width, height);
    gdImageAlphaBlending(img, 1);

    int white = gdImageColorAllocate(img, 255, 255, 255);
    //int black = gdImageColorAllocate(img, 0, 0, 0);
    
    gdImageFilledRectangle(img, 0, 0, width - 1, height - 1, white);

    int x = 1, y, sx, sy;
    int odd = rand()%1;
    if (odd == 0) odd =-1;
    int shift = 1;
    float fluctuation_amplitude = 8.0;
    float color, color_x, color_y, color_xy;

    srand(time(0));
    
    int word_len = rand() % 2 + 4;
    
    for (i = 0; i < s_fts_len; i++) {
        j = rand() % s_fts_len;        
        // printf("fts x:%d, y:%d, c:%c \n", fts[j].x, fts[j].y, fts[j].c);
        gdImageCopy(img, fts[j].i, x, 20, 0, 0, fts[j].x, fts[j].y);
        x += fts[j].x;
    }
    
    int center = x/2;

    // periods
    float rand1 = (rand() % 450000 + 750000)/10000000.0;
    float rand2 = (rand() % 450000 + 750000)/10000000.0;
    float rand3 = (rand() % 450000 + 750000)/10000000.0;
    float rand4 = (rand() % 450000 + 750000)/10000000.0;
    // phases
    float rand5 = (rand() % 31415926)/10000000.0;
    float rand6 = (rand() % 31415926)/10000000.0;
    float rand7 = (rand() % 31415926)/10000000.0;
    float rand8 = (rand() % 31415926)/10000000.0;
    // amplitudes
    float rand9 = (rand() % 90 + 330)/110.0;
    float rand10= (rand() % 120 + 330)/100.0;
    
    
    int foreground_color[3] = {0, 0, 0};    
    int background_color[3] = {220, 230, 255};
    float newred, newgreen, newblue;
    float frsx, frsy, frsx1, frsy1;
    float newcolor, newcolor0;
    
    imo = gdImageCreateTrueColor(width, height);
    gdImageAlphaBlending(imo, 1);
    //int foreground = gdImageColorAllocate(imo, foreground_color[0], foreground_color[1], foreground_color[2]);
    int background = gdImageColorAllocate(imo, background_color[0], background_color[1], background_color[2]);
    gdImageFilledRectangle(imo, 0, 0, width - 1, height - 1, background);
    //gdImageFilledRectangle(imo, 0, height, width - 1, height, foreground);

    for (x = 0; x < width; x++) {
        for (y = 0; y < height; y++) {

            sx = x +(sin(x*rand1+rand5)+sin(y*rand3+rand6))*rand9-width/2+center+1;
            sy = y +(sin(x*rand2+rand7)+sin(y*rand4+rand8))*rand10;

            //printf("sin: %d, %d, %d, %d \n", x, y, sx, sy);

            if (sx<0 || sy<0 || sx>=width-1 || sy>=height-1){
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
                newred  = foreground_color[0];
                newgreen= foreground_color[1];
                newblue = foreground_color[2];
            } else {
                frsx    = sx - floor(sx);
                frsy    = sy - floor(sy);
                frsx1   = 1 - frsx;
                frsy1   = 1 - frsy;

                newcolor = (
                        (color   * frsx1 * frsy1) +
                        (color_x * frsx  * frsy1) +
                        (color_y * frsx1 * frsy) +
                        (color_xy* frsx  * frsy)
                        );
                //printf("rand10: %5.10f\n", newcolor);

                if (newcolor > 255) {
                    newcolor = 255;
                }
                newcolor    = newcolor/255;
                newcolor0   = 1 - newcolor;

                newred  = newcolor0 * foreground_color[0] + newcolor * background_color[0];
                newgreen= newcolor0 * foreground_color[1] + newcolor * background_color[1];
                newblue = newcolor0 * foreground_color[2] + newcolor * background_color[2];
            }
            //printf("1");
            gdImageSetPixel(imo, x, y, gdImageColorAllocate(imo, newred, newgreen, newblue));
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


char * string_rand(int len) 
{
    char *str = (char *)malloc(len + 1);
    int i;

    if (str == 0) return 0;

    srand(time(NULL) + rand() + clock());
    //srand(rand());
    //printf("%d \n", rand());
    //printf("%d \n", rand());
    //int randomData = open("/dev/random", O_WRONLY);
    //randomize();
    //printf("%d \n", randomData);
    
    for (i = 0; i < len; i++)
        str[i] = rand()%10 + 48;

    str[i] = 0;

    return str;
}

void service_handler(struct evhttp_request *req, void *arg)
{
    char *key = string_rand(KEY_LEN);
    captcha_create(key);
    //printf("\nlog: service_handler #%s \n", key);
    
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

    options.port    = 9527;
    options.daemon  = 0;
    options.timeout = 3;
    options.symbols = "23456789abcdegikpqsvxyz";
    options.servers = "127.0.0.1:11211";

    while ((opt = getopt(argc, argv, "dp:t:c:s:")) != -1) {
        switch (opt) {
            case 'd':
                options.daemon = true;
                break;
            case 'p':
                options.port = atoi(optarg);
                break;
            case 't':
                options.timeout = atoi(optarg);
                break;
            case 'c':
                options.symbols = strdup(optarg);
                break;
            case 's':
                options.servers = strdup(optarg);
                break;
            case 'h':
            default:
                break;
        }
    }

    if (options.daemon == true) {
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
    FILE *fp_pidfile;
    fp_pidfile = fopen(HCS_PID, "w");
    fprintf(fp_pidfile, "%d\n", getpid());
    fclose(fp_pidfile);

    //
    memcached_setup();
    fonts_setup();

    //
    struct evhttp *httpd;
    event_init();
    httpd = evhttp_start("0.0.0.0", options.port);
    if (httpd == NULL) {
        fprintf(stderr, "Error: Unable to listen on 0.0.0.0:%d\n\n", options.port);
        exit(1);
    }
    evhttp_set_timeout(httpd, options.timeout);

    /* Set a callback for requests to "/specific". */
    /* evhttp_set_cb(httpd, "/select", select_handler, NULL); */

    /* Set a callback for all other requests. */
    evhttp_set_gencb(httpd, service_handler, NULL);

    event_dispatch();

    /* Not reached in this code as it is now. */
    evhttp_free(httpd);

    if (memc)
	    memcached_free(memc);
    
    return 0;
}
