/* Copyright (C) 2002-2003 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2004-2005 Constantin Kaplinsky.  All Rights Reserved.
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
//
// Image.cxx
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

//#ifdef HAVE_MITSHM
#include <sys/ipc.h>
#include <sys/shm.h>
//#endif

#include <rfb/LogWriter.h>
#include <shmvncserver/Image.h>
#include <shmvncserver/View.h>

//
// ImageCleanup is used to delete Image instances automatically on
// program shutdown. This is important for shared memory images.
//

#include <list>

char *Image::data;
char *Image::mem;

class ImageCleanup {
public:
  std::list<Image *> images;

  ~ImageCleanup()
  {
    while (!images.empty()) {
      delete images.front();
    }
  }
};

ImageCleanup imageCleanup;

//
// Image class implementation.
//

static rfb::LogWriter vlog("Image");

Image::Image(View *v)
  : view(v), trueColor(true),
   depth(v->depth), bytes_per_line(v->bytes_per_line), bits_per_pixel(v->bits_per_pixel),
   red_mask(v->red_mask), green_mask(v->green_mask), blue_mask(v->blue_mask),
   width(v->width), height(v->height)
{
  imageCleanup.images.push_back(this);
}

Image::Image(View *v, int width, int height)
  : view(v), trueColor(true),
   depth(v->depth), bytes_per_line(v->bytes_per_line), bits_per_pixel(v->bits_per_pixel),
   red_mask(v->red_mask), green_mask(v->green_mask), blue_mask(v->blue_mask),
   width(width), height(height)
{
  imageCleanup.images.push_back(this);
  Init(width, height);
}

void Image::Init(int width, int height)
{
  if (!data) {
  data = (char *) malloc(bytes_per_line * height);
  if (data == NULL) {
    vlog.error("malloc() failed");
    exit(1);
  }

  mem = (char *) View::mem;
  }
}

Image::~Image()
{
  imageCleanup.images.remove(this);
}

void Image::get(Window wnd, int x, int y)
{
  get(wnd, x, y, width, height);
}

void Image::get(Window wnd, int x, int y, int w, int h,
                int dst_x, int dst_y)
{
    unsigned long offset = View::offset();
    memcpy(data, mem + offset, w*h*4);
}

//
// Copying pixels from one image to another.
//
// FIXME: Use Point and Rect structures?
// FIXME: Too many similar methods?
//

inline
void Image::copyPixels(XImage *src,
                       int dst_x, int dst_y,
                       int src_x, int src_y,
                       int w, int h)
{
  printf("WARNING!! Update copy pixels called!!!\n");
}

void Image::updateRect(XImage *src, int dst_x, int dst_y)
{
  printf("WARNING!! Update rect called!!!\n");
}

void Image::updateRect(Image *src, int dst_x, int dst_y)
{
  printf("WARNING!! Update rect called!!!\n");
}

void Image::updateRect(XImage *src, int dst_x, int dst_y, int w, int h)
{
  printf("WARNING!! Update rect called!!!\n");
}

void Image::updateRect(Image *src, int dst_x, int dst_y, int w, int h)
{
  printf("WARNING!! Update rect called!!!\n");
}

void Image::updateRect(XImage *src, int dst_x, int dst_y,
                       int src_x, int src_y, int w, int h)
{
  printf("WARNING!! Update rect called!!!\n");
}

void Image::updateRect(Image *src, int dst_x, int dst_y,
                       int src_x, int src_y, int w, int h)
{
  printf("WARNING!! Update rect called!!!\n");
}

ImageFactory::ImageFactory(bool allowShm, bool allowOverlay)
  : mayUseShm(allowShm), mayUseOverlay(allowOverlay)
{
}

ImageFactory::~ImageFactory()
{
}

Image *ImageFactory::newImage(View *v, int width, int height)
{
  Image *image = NULL;

  image = new Image(v, width, height);
  return image;
}
