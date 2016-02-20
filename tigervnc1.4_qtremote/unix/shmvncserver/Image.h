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
// Image.h
//

#ifndef __IMAGE_H__
#define __IMAGE_H__

class View;

class XImage;
typedef unsigned long Window;

//
// Image class is an Xlib-based implementation of screen image storage.
//

class Image {

public:

  Image(View *v);
  Image(View *v, int width, int height);
  virtual ~Image();

  bool isTrueColor() const { return trueColor; }

  virtual const char *className() const {
    return "Image";
  }
  virtual const char *classDesc() const {
    return "basic Xlib image";
  }

  virtual void get(Window wnd, int x = 0, int y = 0);
  virtual void get(Window wnd, int x, int y, int w, int h,
                   int dst_x = 0, int dst_y = 0);

  // Copying pixels from one image to another.
  virtual void updateRect(XImage *src, int dst_x = 0, int dst_y = 0);
  virtual void updateRect(Image *src, int dst_x = 0, int dst_y = 0);
  virtual void updateRect(XImage *src, int dst_x, int dst_y, int w, int h);
  virtual void updateRect(Image *src, int dst_x, int dst_y, int w, int h);
  virtual void updateRect(XImage *src, int dst_x, int dst_y,
                          int src_x, int src_y, int w, int h);
  virtual void updateRect(Image *src, int dst_x, int dst_y,
                          int src_x, int src_y, int w, int h);

  // Pointer to corresponding XImage, made public for efficiency.
  // NOTE: if this field is NULL, then no methods other than Init()
  //       may be called.
#if 0
  XImage *xim;
#endif

//Not XImage
  int depth;
  int bytes_per_line;
  int bits_per_pixel;
  unsigned int red_mask;
  unsigned int green_mask;
  unsigned int blue_mask;
  int width;
  int height;

  static char *mem;
  static char *data;

  // Get a pointer to the data corresponding to the given coordinates.
  inline char *locatePixel(int x, int y) const {
    return ((char *) data +
            (y * bytes_per_line +
            x * bits_per_pixel / 8));
  }
protected:

  void Init(int width, int height);

  // Like updateRect(), but does not check arguments.
  void copyPixels(XImage *src,
                  int dst_x, int dst_y,
                  int src_x, int src_y,
                  int w, int h);

  View *view;
  bool trueColor;

};

//
// ImageFactory class is used to produce instances of Image-derived
// objects that are most appropriate for current X server and user
// settings.
//

class ImageFactory {

public:

  ImageFactory(bool allowShm, bool allowOverlay);
  virtual ~ImageFactory();

  bool isShmAllowed()     { return mayUseShm; }
  bool isOverlayAllowed() { return mayUseOverlay; }

  virtual Image *newImage(View *v, int width, int height);

protected:

  bool mayUseShm;
  bool mayUseOverlay;

};

#endif // __IMAGE_H__
