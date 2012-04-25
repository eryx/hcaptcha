#include "config.h"
#include "../deps/hiredis/sds.h"

void initConfig()
{
    cfg.port    = 9527;
    cfg.daemon  = 0;
    cfg.pidfile = "/tmp/hcaptcha.pid";
    cfg.log     = "/var/log/hcaptcha.log";
    cfg.http_timeout = 3;
    
    cfg.servers = "127.0.0.1:11211";  
    
    cfg.img_width  = 120;
    cfg.img_height = 60;
    
    cfg.font      = "../fonts/cmr10.ttf";
    cfg.symbols   = "23456789abcdegikpqsvxyz";
    cfg.font_size = cfg.img_height * 0.618;
    cfg.length[0] = 4;
    cfg.length[1] = 5;
    cfg.fluctuation_amplitude = 10; // symbol's vertical fluctuation amplitude
    
    cfg.img_bg_color[0] = 230;
    cfg.img_bg_color[1] = 230;
    cfg.img_bg_color[2] = 230;
    
    cfg.img_fg_color[0] = 0;
    cfg.img_fg_color[1] = 0;
    cfg.img_fg_color[2] = 0;
     
    cfg.img_timeout = 1800;
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
            fprintf(stderr, "Fatal error, can't open config file '%s'\n\n", filename);
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
        if (!strcasecmp(argv[0],"http_timeout") && argc == 2) {
            cfg.http_timeout = atoi(argv[1]);
        } else if (!strcasecmp(argv[0],"port") && argc == 2) {
            cfg.port = atoi(argv[1]);
        } else if (!strcasecmp(argv[0],"daemonize") && argc == 2) {
            if (!strcasecmp(argv[1],"yes")) cfg.daemon = 1;
        } else if (!strcasecmp(argv[0],"pidfile") && argc == 2) {
            cfg.pidfile = strdup(argv[1]);
        } else if (!strcasecmp(argv[0],"servers") && argc == 2) {
            cfg.servers = strdup(argv[1]);
        } else if (!strcasecmp(argv[0],"font") && argc == 2) {
            cfg.font = strdup(argv[1]);
        } else if (!strcasecmp(argv[0],"symbols") && argc == 2) {
            cfg.symbols = strdup(argv[1]);
        } else if (!strcasecmp(argv[0],"img_size") && argc == 3) {
            cfg.img_width   = atoi(argv[1]);
            cfg.img_height  = atoi(argv[2]);
            cfg.font_size   = cfg.img_height / 2;
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
        } else if (!strcasecmp(argv[0],"img_timeout") && argc == 2) {
            cfg.img_timeout = atoi(argv[1]);
        } 
        
    }
}

