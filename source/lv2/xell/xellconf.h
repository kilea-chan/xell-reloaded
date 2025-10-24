#ifndef XELLCONF_H
#define XELLCONF_H

#define MAX_XELLCONF_SIZE 16384

struct xellconf {
    int videomode;
    int speedup;
    char *tftp_server;
    char *ipaddress;
    char *netmask;
    char *gateway;
};

int xellconf_parse(void);
int try_xellconf(void * addr, unsigned len);

extern char *xellconf_tftp;

#endif
