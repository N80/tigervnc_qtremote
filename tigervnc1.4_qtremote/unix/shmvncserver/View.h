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

#ifndef __VIEW_H__
#define __VIEW_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

class View
{
    public:
        View();
        virtual ~View();

        int width;
        int height;
        int depth;
        int bytes_per_line;
        int bits_per_pixel;
        unsigned int red_mask;
        unsigned int green_mask;
        unsigned int blue_mask;

        /*
         Communication socket
         */
        int ssocket;

        /*
         Screen y-offset in bytes
         */
        static unsigned long offset();

        /*
         Frame buffer memory pointer
         */
        static char *mem;

      private:
        static int fbfd;
};

#endif
