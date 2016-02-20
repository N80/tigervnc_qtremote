/* Copyright (C) 2007-2008 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright 2014 Pierre Ossman for Cendio AB
 * Copyright 2015 Davide Bettio for Ispirata Srl
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
// XPixelBuffer.cxx
//

#include <vector>
#include <rfb/Region.h>
#include <shmvncserver/XPixelBuffer.h>

using namespace rfb;

XPixelBuffer::XPixelBuffer(View *view, ImageFactory &factory,
                           const Rect &rect)
  : FullFramePixelBuffer(),
    m_poller(0),
    m_view(view),
    m_image(factory.newImage(view, rect.width(), rect.height())),
    m_offsetLeft(rect.tl.x),
    m_offsetTop(rect.tl.y)
{
  // Fill in the PixelFormat structure of the parent class.
  format = PixelFormat(m_image->bits_per_pixel,
                       m_image->depth,
                       0,
                       240,
                       m_image->red_mask   >> (ffs(m_image->red_mask) - 1),
                       m_image->green_mask >> (ffs(m_image->green_mask) - 1),
                       m_image->blue_mask  >> (ffs(m_image->blue_mask) - 1),
                       ffs(m_image->red_mask) - 1,
                       ffs(m_image->green_mask) - 1,
                       ffs(m_image->blue_mask) - 1);

  // Set up the remaining data of the parent class.
  width_ = rect.width();
  height_ = rect.height();
  data = (rdr::U8 *) m_image->data;

  // Calculate the distance in pixels between two subsequent scan
  // lines of the framebuffer. This may differ from image width.
  stride = m_image->bytes_per_line * 8 / m_image->bits_per_pixel;

  // Get initial screen image from the X display.
  m_image->get(NULL, m_offsetLeft, m_offsetTop);

  // PollingManager will detect changed pixels.
  m_poller = new PollingManager(view, getImage(), factory,
                                m_offsetLeft, m_offsetTop);
}

XPixelBuffer::~XPixelBuffer()
{
  delete m_poller;
  delete m_image;
}

void
XPixelBuffer::grabRegion(const rfb::Region& region)
{
  std::vector<Rect> rects;
  std::vector<Rect>::const_iterator i;
  region.get_rects(&rects);
  for (i = rects.begin(); i != rects.end(); i++) {
    grabRect(*i);
  }
}

