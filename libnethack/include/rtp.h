#ifndef RTP_H
#define RTP_H

#include <stdbool.h>

#include "extern.h"
#include "hack.h"
#include "monst.h"

struct monst *name_rtp(struct monst *mtmp);

void player_killed_rtp(struct level *lev);

struct obj *create_rtp_corpse(struct level *lev, int x, int y, enum rng rng);

#endif // RTP_H
