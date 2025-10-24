#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <network/network.h>

#include <xenon_smc/xenon_smc.h>
#include <xenon_uart/xenon_uart.h>
#include <input/input.h>
#include <lwip/ip_addr.h>
#include <network/network.h>
#include <xenos/xenos.h>
#include <ppc/timebase.h>
#include <elf/elf.h>
#include <usb/usbmain.h>
#include <console/console.h>
#include <xenon_soc/xenon_power.h>

#include "xellconf.h"
#include "../utils/util.h"

char conf_buf[MAX_XELLCONF_SIZE];
struct xellconf conf;

ip_addr_t oldipaddr, oldnetmask, oldgateway;
char *xellconf_tftp;

/* network.h */
extern struct netif netif;

int xellconf_parse(void)
{
    char *lp = conf_buf;
    int lineno = 1;

    memset(&conf, 0, sizeof(conf));

    conf.videomode = -1;
    conf.speedup = 0;

    while(*lp) {
        char *newline = strchr(lp, '\n');
        char *next;
        if (newline) {
            *newline = 0;
            next = newline+1;
        } else {
            next = lp+strlen(lp);
        }

        lp = strip(lp);
        if (!*lp)
            goto nextline;

        char *left, *right;

        split(lp, &left, &right, '=');
        if (!right) {
            if(strncmp(left,"#",1) && strncmp(left,";",1))
                printf("xell.conf: parse error (line %d)\n", lineno);
                //PRINT_WARN("xell.conf: parse error (line %d)\n", lineno);
            goto nextline;
        }

        while(*right == '"' || *right == '\'')
            right++;
        char *rend = right + strlen(right) - 1;
        while(*rend == '"' || *rend == '\'')
            *rend-- = 0;

        if (!strcmp(left, "videomode")) {
            conf.videomode = atoi(right);
        } else if (!strcmp(left, "speedup")) {
            conf.speedup = atoi(right);
        } else if (!strcmp(left, "tftp_server")) {
            conf.tftp_server = right;
        } else if (!strcmp(left, "ip")) {
            conf.ipaddress = right;
        } else if (!strcmp(left, "netmask")) {
            conf.netmask = right;
        } else if (!strcmp(left, "gateway")) {
            conf.gateway = right;
        } else if (!strncmp(left, "#", 1)||!strncmp(left, ";", 1)) {
            goto nextline;
        }

        nextline:
            lp = next;
            lineno++;
    }
    return 0;
}

void xell_set_config(void)
{

    int setnetconfig = 0;
    static int oldvideomode = -1;
    ip_addr_t ipaddr, netmask, gateway, tftpserver;

    if(conf.tftp_server != NULL)
        if (ipaddr_aton(conf.tftp_server,&tftpserver)) {
            xellconf_tftp = malloc(strlen(conf.tftp_server) + 1);
            strcpy(xellconf_tftp, conf.tftp_server);
        }

    if(conf.ipaddress != NULL)
        if (ipaddr_aton(conf.ipaddress,&ipaddr) && ip_addr_cmp(&oldipaddr,&ipaddr) == 0)
        {
            printf(" * taking network down to set config values\n");
            setnetconfig = 1;
            netif_set_down(&netif);

            netif_set_ipaddr(&netif,&ipaddr);
            ip_addr_set(&oldipaddr,&ipaddr);
        }

    if(conf.netmask != NULL)
        if (ipaddr_aton(conf.netmask,&netmask) && setnetconfig){
            netif_set_netmask(&netif,&netmask);
            ip_addr_set(&oldnetmask,&netmask);
        }

    if(conf.gateway != NULL)
        if (ipaddr_aton(conf.gateway,&gateway) && setnetconfig){
            netif_set_gw(&netif,&gateway);
            ip_addr_set(&oldgateway,&gateway);
        }


    if (setnetconfig){
        printf(" * bringing network back up...\n");
        netif_set_up(&netif);
        network_print_config();
    }

    if(conf.videomode > VIDEO_MODE_AUTO && conf.videomode <= VIDEO_MODE_NTSC && oldvideomode != conf.videomode){
        oldvideomode = conf.videomode;
        xenos_init(conf.videomode);
        console_init();
        printf(" * Xenos re-initalized\n");
    }

    if(conf.speedup >= XENON_SPEED_FULL && conf.speedup <= XENON_SPEED_1_3){ //speedmode: drivers/xenon_soc/xenon_power.h
        printf("Speeding up CPU\n");
        xenon_make_it_faster(conf.speedup);
    }
}

int try_xellconf(void * addr, unsigned len){
    if (len > MAX_XELLCONF_SIZE)
    {
        PRINT_ERR("file is bigger than %u bytes\n",MAX_XELLCONF_SIZE);
        PRINT_ERR("Aborting\n");
        return -1;
    }

    memcpy(conf_buf,addr,len);
    conf_buf[len] = 0; //ensure null-termination

    xellconf_parse();
    xell_set_config();

    memset(conf_buf,0,MAX_XELLCONF_SIZE);
    conf.speedup = 0;

    return 0;
}
