#ifndef __CONFIG_H
#define __CONFIG_H

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


#define HCS_SIGNATURE   "hcaptcha/1.0.4"
#define CONFIGLINE_MAX  1024

struct configObject {
    int   port;
    int   daemon;
    int   http_timeout;
    char  *symbols;
    char  *servers;
    int   img_width;
    int   img_height;
    int   len;
    
    char   *font;
    double  font_size;
    
    int   fluctuation_amplitude;
    char  *log;
    
    int   img_timeout;
    int   img_bg_color[3];
    int   img_fg_color[3];
    
    int   length[2];
    
    char  *pidfile;
} cfg;

void initConfig();
void loadConfig(char *filename);

#endif
