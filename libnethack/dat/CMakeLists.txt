# dat/CMakeLists.txt: generate game data files
include(CMakeParseArguments)

configure_file(history history COPYONLY)

set (LEVEL_DESCRIPTIONS
    ${LNH_DAT}/Arch.des
    ${LNH_DAT}/Barb.des
    ${LNH_DAT}/bigroom.des
    ${LNH_DAT}/castle.des
    ${LNH_DAT}/Caveman.des
    ${LNH_DAT}/endgame.des
    ${LNH_DAT}/gehennom.des
    ${LNH_DAT}/Healer.des
    ${LNH_DAT}/Knight.des
    ${LNH_DAT}/knox.des
    ${LNH_DAT}/medusa.des
    ${LNH_DAT}/mines.des
    ${LNH_DAT}/Monk.des
    ${LNH_DAT}/Potter.des
    ${LNH_DAT}/Priest.des
    ${LNH_DAT}/Ranger.des
    ${LNH_DAT}/Rogue.des
    ${LNH_DAT}/Samurai.des
    ${LNH_DAT}/sokoban.des
    ${LNH_DAT}/Tourist.des
    ${LNH_DAT}/tower.des
    ${LNH_DAT}/Valkyrie.des
    ${LNH_DAT}/Wizard.des
    ${LNH_DAT}/yendor.des 
    )

# the compiled levels that will be added to nhdat
set (COMPILED_LEVELS
    air.lev      Arc-fila.lev Arc-filb.lev Arc-goal.lev
    Arc-loca.lev Arc-strt.lev asmodeus.lev astral.lev
    baalz.lev    Bar-fila.lev Bar-filb.lev Bar-goal.lev
    Bar-loca.lev Bar-strt.lev
    bigrm-1.lev  bigrm-2.lev bigrm-3.lev  bigrm-4.lev 
    bigrm-5.lev  bigrm-6.lev bigrm-7.lev  bigrm-8.lev
    castle-1.lev castle-2.lev castle-3.lev 
    Cav-fila.lev Cav-filb.lev Cav-goal.lev Cav-loca.lev
    Cav-strt.lev cthulhu.lev earth.lev    fakewiz1.lev fakewiz2.lev
    fire.lev     Hea-fila.lev Hea-filb.lev Hea-goal.lev
    Hea-loca.lev Hea-strt.lev juiblex.lev  Kni-fila.lev
    Kni-filb.lev Kni-goal.lev Kni-loca.lev Kni-strt.lev
    knox.lev  lilith.lev lolth.lev mammon.lev
    medusa-1.lev medusa-2.lev medusa-3.lev medusa-4.lev
    minefill.lev
    minend-1.lev minend-2.lev minend-3.lev minetn-1.lev
    minetn-2.lev minetn-3.lev minetn-4.lev minetn-5.lev
    minetn-6.lev minetn-7.lev Mon-fila.lev Mon-filb.lev
    Mon-goal.lev Mon-loca.lev Mon-strt.lev Potter.lev
    orcus.lev    Pri-fila.lev Pri-filb.lev Pri-goal.lev
    Pri-loca.lev Pri-strt.lev Ran-fila.lev Ran-filb.lev
    Ran-goal.lev Ran-loca.lev Ran-strt.lev Rog-fila.lev
    Rog-filb.lev Rog-goal.lev Rog-loca.lev Rog-strt.lev
    Sam-fila.lev Sam-filb.lev Sam-goal.lev Sam-loca.lev
    Sam-strt.lev sammael.lev sanctum.lev
    soko1-1.lev  soko1-2.lev  soko1-3.lev  soko1-4.lev
    soko2-1.lev  soko2-2.lev  soko2-3.lev  soko2-4.lev
    soko2-7.lev  soko3-1.lev  soko3-2.lev  soko3-3.lev
    soko3-4.lev  soko3-5.lev  soko3-6.lev  soko3-7.lev
    soko4-1.lev  soko4-2.lev  soko4-3.lev  soko4-4.lev
    soko4-5.lev  soko4-6.lev  soko4-7.lev  soko4-8.lev
    soko4-8.lev  Tou-fila.lev Tou-filb.lev
    Tou-goal.lev Tou-loca.lev Tou-strt.lev tower1.lev
    tower2.lev   tower3.lev   Val-fila.lev Val-filb.lev
    Val-goal.lev valley.lev   Val-loca.lev Val-strt.lev
    water.lev    wizard1.lev  wizard2.lev  wizard3.lev
    Wiz-fila.lev Wiz-filb.lev Wiz-goal.lev Wiz-loca.lev
    Wiz-strt.lev
    )

set (NHDAT_SRC dungeon quest.dat rumors oracles ${COMPILED_LEVELS} history data)

get_property(MAKEDEFS_BIN TARGET makedefs PROPERTY LOCATION)
get_property(DGN_COMP_BIN TARGET dgn_comp PROPERTY LOCATION)
get_property(LEV_COMP_BIN TARGET lev_comp PROPERTY LOCATION)
get_property(DLB_BIN TARGET dlb PROPERTY LOCATION)

# makedefs -d
add_custom_command (OUTPUT data
                    COMMAND makedefs
                    ARGS -d ${LNH_DAT}/data.base data
                    MAIN_DEPENDENCY data.base
                    DEPENDS makedefs)
# makedefs -e
add_custom_command (OUTPUT dungeon.pdf
                    COMMAND makedefs
                    ARGS -e ${LNH_DAT}/dungeon.def dungeon.pdf
                    MAIN_DEPENDENCY dungeon.def
                    DEPENDS makedefs)
# makedefs -q
add_custom_command (OUTPUT quest.dat
                    COMMAND makedefs
                    ARGS -q ${LNH_DAT}/quest.txt quest.dat
                    MAIN_DEPENDENCY quest.txt
                    DEPENDS makedefs)
# makedefs -r
add_custom_command (OUTPUT rumors
                    COMMAND makedefs
                    ARGS -r ${LNH_DAT}/rumors.tru
                            ${LNH_DAT}/rumors.fal
                            ${LNH_DAT}/rumors.pot rumors
                    DEPENDS makedefs)
# makedefs -h
add_custom_command (OUTPUT oracles
                    COMMAND makedefs
                    ARGS -h ${LNH_DAT}/oracles.txt oracles
                    MAIN_DEPENDENCY oracles.txt
                    DEPENDS makedefs)

add_custom_command (OUTPUT dungeon
                    COMMAND dgn_comp
                    ARGS dungeon.pdf
                    MAIN_DEPENDENCY dungeon.pdf)

# Level files
macro(level_file)
    cmake_parse_arguments(LEVEL_FILE "" "INPUT" "OUTPUTS" ${ARGN})
    set (REAL_OUTPUTS)
    foreach (OUTPUT ${LEVEL_FILE_OUTPUTS})
        set(REAL_OUTPUTS ${REAL_OUTPUTS} ${OUTPUT}.lev)
    endforeach(OUTPUT)
    set (COMPILED_LEVELS ${COMPILED_LEVELS} ${REAL_OUTPUTS})
    add_custom_command (OUTPUT ${REAL_OUTPUTS}
                        COMMAND lev_comp
                        ARGS ${LNH_DAT}/${LEVEL_FILE_INPUT}
                        MAIN_DEPENDENCY ${LEVEL_FILE_INPUT})
endmacro(level_file)

macro(quest_file BASE INPUT)
    level_file(INPUT ${INPUT}
               OUTPUTS ${BASE}-strt ${BASE}-fila ${BASE}-loca ${BASE}-filb
                       ${BASE}-goal)
endmacro(quest_file)

# DoD
level_file(INPUT Potter.des OUTPUTS Potter)
level_file(INPUT bigroom.des OUTPUTS bigrm-1 bigrm-2 bigrm-3 bigrm-4 bigrm-5 bigrm-6 bigrm-7 bigrm-8)
level_file(INPUT medusa.des OUTPUTS medusa-1 medusa-2 medusa-3 medusa-4)
level_file(INPUT castle.des OUTPUTS castle-1 castle-2 castle-3)

# Mines
level_file(INPUT mines.des
           OUTPUTS minefill minend-1 minend-2 minend-3
                   minetn-1 minetn-2 minetn-3 minetn-4
                   minetn-5 minetn-6 minetn-7)

# Sokoban
level_file(INPUT sokoban.des
           OUTPUTS soko1-1 soko1-2 soko1-3 soko1-4
                   soko2-1 soko2-2 soko2-3 soko2-4 soko2-5 soko2-6 soko2-7
                   soko3-1 soko3-2 soko3-3 soko3-4 soko3-5 soko3-6 soko3-7
                   soko4-1 soko4-2 soko4-3 soko4-4 soko4-5 soko4-6 soko4-7 soko4-8 soko4-9)

# Quests
quest_file(Arc Arch.des)
quest_file(Bar Barb.des)
quest_file(Cav Caveman.des)
quest_file(Hea Healer.des)
quest_file(Kni Knight.des)
quest_file(Mon Monk.des)
quest_file(Pri Priest.des)
quest_file(Ran Ranger.des)
quest_file(Rog Rogue.des)
quest_file(Sam Samurai.des)
quest_file(Tou Tourist.des)
quest_file(Val Valkyrie.des)
quest_file(Wiz Wizard.des)

# Knox
level_file(INPUT knox.des OUTPUTS knox)

# Gehennom
level_file(INPUT gehennom.des OUTPUTS asmodeus baalz cthulhu juiblex lilith lolth mammon orcus sammael sanctum valley)
level_file(INPUT yendor.des OUTPUTS fakewiz1 fakewiz2 wizard1 wizard2 wizard3)

# Vlad's
level_file(INPUT tower.des OUTPUTS tower1 tower2 tower3)

# End Game
level_file(INPUT endgame.des OUTPUTS earth air fire water astral)

set (NHDAT_SRC dungeon quest.dat rumors oracles ${COMPILED_LEVELS} history data)

add_custom_command (OUTPUT nhdat
                    COMMAND dlb
                    ARGS cf nhdat ${NHDAT_SRC}
                    DEPENDS ${NHDAT_SRC})

# pseudo-target: depend on all other generated data
add_custom_target (nethack_data_files ALL
                   DEPENDS nhdat)

install(FILES ${LNH_DAT_GEN}/nhdat
        DESTINATION ${DATADIR})
install(FILES license
        DESTINATION ${DATADIR})

# vim:et
