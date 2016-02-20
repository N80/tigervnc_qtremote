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

#ifndef _QTTOREMOTE_H_
#define _QTTOREMOTE_H_

#define SCREEN_UPDATED_MSG 2001

#include <sys/ipc.h>
#include <sys/shm.h>

struct MouseEventMessage
{
    int msgType;
    int xPos;
    int yPos;
    int changedButton;
    int buttonsMask;
} __attribute__ ((__packed__));

struct KeyboardEventMessage
{
    int msgType;
    int key;
} __attribute__ ((__packed__));

#endif
