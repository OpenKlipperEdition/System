/*
 *  Copyright (C) 2016, Zhang YanMing <jamincheung@126.com>
 *
 *  Linux recovery updater
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef PLAY_H
#define PLAY_H

#include <types.h>

struct play {
    void (*construct)(struct play* this);
    void (*destruct)(struct play* this);

    int (*init)(struct play* this);
    int (*deinit)(struct play* this);

    void (*play_progress)(enum update_stage_t stage, int progress);
};

void construct_play(struct play* this);
void destruct_play(struct play* this);

#endif /* PLAY_H */
