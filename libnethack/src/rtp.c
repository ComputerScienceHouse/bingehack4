#include "rtp.h"

typedef const struct {
    const char* name;
    const bool female;
} rtpEntry;

rtpEntry rtpNames[] = {
    {"Angelo DiNardi", false},
    {"Chris Lockfort", false},
    {"Dan Willemsen", false},
    {"Derek Gonyeo", false},
    {"Ethan House", false},
    {"Grant Cohoe", false},
    {"Jordan Rodgers", false},
    {"Kevin Thompson", false},
    {"Liam Middlebrook", false},
    {"McSaucy", false},
    {"Rob Glossop", false},
    {"Russ Harmon", false},
    {"Stephanie Miller", true},
    {"Steve Greene", false},
    {"Will Orr", false},
    {"William Dignazio", false},
};

struct monst *
name_rtp(struct monst *mtmp)
{
    int rtp_id = rn2(sizeof(rtpNames) / sizeof(rtpEntry));

    if (rtpNames[rtp_id].female) {
        mtmp->female = TRUE;
    }

    return christen_monst(mtmp, msg_from_string(rtpNames[rtp_id].name));
}

struct obj *
create_rtp_corpse(struct level *lev, int x, int y, enum rng rng)
{
    struct obj *obj = NULL;
    struct obj *orig_obj = NULL;
    // There should be a significant reward in order to tempt the player into
    // trying to kill an RTP. Otherwise it's risk without reward.

    // In the future this should be a random selection from a slew of different
    // items ranging in usefulness.
    obj = mksobj_at(MAGIC_MARKER, lev, x, y, TRUE, FALSE, rng);


    orig_obj = obj;

    // What else are RTPs good for :D
    obj = oname(obj, "The Root Password");
    return obj;
}

void
player_killed_rtp(struct level *lev)
{
    pline("You hear a faint whisper in the air: \"I'll shred your world\"");

    // get a large sample set
    int random = rn2(100);

    // Enumerate through all the possibilities when the player kills an
    // RTP
    //
    if (!(random / 10)) {
        change_luck(-3);
    }

    // 5% chance that the player hallucinates for a long while
    if (!(random / 20)) {
        make_hallucinated(rn2(420) + 50, TRUE);
    }

    // 1% chance the player gets sick and dies after 42 turns
    if (random == 42) {
        make_sick(42, "Right before you killed that RTP 42 turns ago they gave you Heartbleed </3", TRUE, SICK_VOMITABLE);
    }

    // 33% chance that alignment changes
    if (!(random / 3)) {
        aligntyp player_align = u.ualign.type;
        aligntyp new_align = A_NEUTRAL;

        // If we're not neutral switch to opposite or neutral
        if (player_align) {
            new_align = rn2(1) * -player_align;
        } else {
            new_align = rn2(1)? 1: -1;
        }

        if (uarmh && uarmh->otyp == HELM_OF_OPPOSITE_ALIGNMENT) {
            u.ualignbase[A_CURRENT] = new_align;
        } else {
            u.ualign.type = u.ualignbase[A_CURRENT] = new_align;
        }
    }

    // 25% chance that you anger your god
    if(!(random / 4)) {
        gods_upset(u.ualign.type);
    }

    // 16.67% chance that player loses a level (if > 1)
    if (!(random / 6) && u.ulevel > 1) {
        losexp(NULL, FALSE);
    }

    // 20% chance to spawn a random (suitable for the level) angry monster near
    // the player
    if (!(random / 5)) {
        makemon(NULL, lev, u.ux, u.uy, MM_ANGRY);
    }

    if (!(random / 10)) {
        // Neat that this function already exists for our usage!
        rndcurse();
    }

    if (!(random / 10)) {
        if(uarmg) erode_obj(uarmg, NULL, ERODE_CORRODE, TRUE, TRUE);
    }

    if (!(random / 15)) {
        polyself(FALSE);
    }
}
