/*
 * Copyright (C) 2015 Davide Bettio for Ispirata Srl
 *    
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */


#include <shmvncserver/View.h>

#include <cstdlib>
#include <strings.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "messages.h"

#include <unistd.h>
#include <fcntl.h>      /* for fcntl */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>       /* for mmap */
#include <sys/ioctl.h>
#include <linux/fb.h>

char *View::mem;
int View::fbfd;

View::View()
    : width(0), height(0)
{
    int s, len;
    struct sockaddr_un remote;

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, "/tmp/qttoremoteserver");
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(s, (struct sockaddr *)&remote, len) == -1) {
        perror("connect");
        exit(1);
    }
    ssocket = s;

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
 
    fbfd = open("/dev/fb0",O_RDWR);
    if (!fbfd) {
        printf("Error: cannot open framebuffer device.\n");
        exit(1);
    }
 
    if (ioctl (fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        printf("Error reading fixed information.\n");
        exit(2);
    }
 
    if (ioctl (fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        printf("Error reading variable information.\n");
        exit(3);
    }

    mem = (char*) mmap(0, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);

    width = vinfo.xres;
    height = vinfo.yres;
    depth = 24;
    bytes_per_line = width * 4;
    bits_per_pixel = 32;
    blue_mask = 0x0000FF;
    green_mask = 0x00FF00;
    red_mask = 0xFF0000;
}

View::~View()
{
}

unsigned long View::offset()
{
    struct fb_var_screeninfo vinfo;
    if (ioctl (fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        printf("Error reading screen panning.\n");
        return 0;
    }
    return vinfo.yoffset * vinfo.xres * 4;
}
