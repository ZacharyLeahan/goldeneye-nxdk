"""Attach downstream LAN movement and viewport callbacks to build copies only."""
from pathlib import Path
import sys


def adapt(unit, source, *, xbox=False):
    controls = "" if xbox else "if (getPlayerCount() >= 2) "
    def replace(before, after):
        nonlocal source
        if source.count(before) != 1:
            raise ValueError(f"Review LAN adapter after change to {unit}: {before}")
        source = source.replace(before, after)
    source = '#include "port/lan/lan.h"\n' + source
    if unit == "boss":
        source = '#include "simulant_probe.h"\n' + source
        replace("lvlStageLoad(g_StageNum);", "lvlStageLoad(g_StageNum);\n        simulantProbeStageReady(g_StageNum);")
        replace("lvlUnloadStageTextData();", "simulantProbeStageTeardown();\n        lvlUnloadStageTextData();")
    if unit == "lv":
        source = '#include "simulant_probe.h"\n' + source
        replace("g_GlobalTimer += g_ClockTimer;", "g_GlobalTimer += g_ClockTimer;\n    simulantProbePoll();")
    if unit == "radar":
        source = '#include "simulant_probe.h"\n#include "mp_roster.h"\n' + source
        replace("for (i = 0; i < player_count; i++)",
                "int participants = mpRosterCount() > player_count ? mpRosterCount() : player_count;\n    for (i = 0; i < participants; i++)")
        replace("if (g_playerPointers[i]->bonddead == FALSE)",
                "other_player_prop = i >= player_count ? simulantProbeGetProp() : (g_playerPointers[i]->bonddead ? NULL : g_playerPointers[i]->prop);\n            if (other_player_prop)")
        replace("other_player_prop = g_playerPointers[i]->prop;", "")
    if unit == "chraction":
        source = '#include "mp_combat.h"\n' + source
        anchor = "self->chrflags |= CHRFLAG_WAS_DAMAGED;"
        site = source.index(anchor, source.index("bool handles_shot_actors("))
        source = source[:site] + "if (damageToCause > 0) mpCombatNoteSimulantHit(self, isPlayer ? get_cur_playernum() : -1);\n            " + source[site:]
        anchor = "increment_num_kills_display_text_in_MP();"
        if source.count(anchor) != 2:
            raise ValueError("Review Simulant kill attribution in chraction")
        source = source.replace(anchor, "if (mpCombatShouldCountNativeKill(self)) " + anchor)
        anchor = "if (chrlvAttackAnimationRelated7F026F30(self, &endframe2))"
        replace(anchor, "if (mpCombatSimulantKeepsMoving(self)) return TRUE;\n            " + anchor)
    if unit in ("boss", "lv"):
        source = '#include "port/lan/frame_trace.h"\n' + source
    if unit == "boss":
        anchor = "if (mainTickElapsed < MAIN_LOOP_TICK_INTERVAL)"
        replace(anchor, "ftRecord(FT_GATE, ftGameId + 1, (pendingGfx << 1) | (mainTickElapsed < MAIN_LOOP_TICK_INTERVAL));\n                    " + anchor)
        anchor = "if (get_is_ramrom_flag())\n                            {"
        replace(anchor, "ftGameStart(g_StageNum == LEVELID_EGYPT && lanActive());\n                            " + anchor)
        replace("gdl = lvlRender(gdl);", "ftRecord(FT_BUILD, ftGameId, 0);\n                            gdl = lvlRender(gdl);")
        anchor = "rspGfxTaskStart(firstGdl, gdl, 0, (OSMesg)&localGfxDoneMsg);"
        replace(anchor, "ftRecord(FT_SUBMIT, ftGameId, (uintptr_t)firstGdl);\n                            " + anchor)
    elif unit == "model":
        # These two leaf functions only append model display-list commands.
        # Keep the traversal, animation switches, and RNG-consuming nodes.
        for signature in ("void modelRenderNodeGundl(ModelRenderData* renderdata, ModelNode* arg1)",
                          "void modelRenderNodeDl(ModelRenderData *renderdata, Model *model, ModelNode *node)"):
            replace(signature + "\n{", signature + "\n{\n    if (lanSkipViewDraw) return;")
    elif unit == "bg":
        # Retain room loading and model_bin_loaded updates in both passes.
        for mapping in ("ptr_expanded_mapping_info", "ptr_secondary_expanded_mapping_info"):
            first = "            gdl = applyRoomMatrixToDisplayList(gdl, room_index);"
            last = f"            gSPDisplayList(gdl++, OS_K0_TO_PHYSICAL(g_BgRoomInfo[room_index].{mapping}));"
            end = source.index(last) + len(last)
            block = source[source.rindex(first, 0, end):end]
            replace(block, "            if (!lanSkipViewDraw) {\n" +
                    "\n".join("    " + line if line else "" for line in block.splitlines()) + "\n            }")
        # The cartridge linker placed the next global immediately after the
        # table. COFF can put it in a different section or before the table.
        replace("end = (u8 *)&g_BgCurrentRoom;", "end = specialportalarray + sizeof(specialportalarray);")
        replace("} while ((u32)ptr < (u32)end);", "} while (ptr < end);")
    elif unit == "bondview2":
        replace("#undef ZERO_7F08991C", "mpConfirmTargetHit(playerid, get_cur_playernum());\n#undef ZERO_7F08991C")
        if xbox:
            source = '#include "port/lan/frame_trace.h"\n' + source
            anchor = "gunUpdateAndFireBothHands();"
            replace(anchor, "ftRecord(FT_BEFORE_GUN, ftGameId, get_cur_playernum());\n    " + anchor +
                    "\n    ftRecord(FT_AFTER_GUN_UPDATE, ftGameId, get_cur_playernum());")
            anchor = "gunRenderFirstPersonGunModels(&gdl);"
            replace(anchor, anchor + "\n    ftRecord(FT_AFTER_GUN_MODEL, ftGameId, get_cur_playernum());")
            for call, event in (("die_blood_image_routine(0);", "BLOOD"),
                                ("set_missionstate(6);", "DEATH_MUSIC")):
                replace(call, f"ftRecord(FT_BEFORE_{event}, ftGameId, get_cur_playernum());\n                {call}\n                ftRecord(FT_AFTER_{event}, ftGameId, get_cur_playernum());")
        replace("if (joyGetButtons(get_cur_playernum(), 0xB000))",
                "if (lanRespawnButton(joyGetButtons(get_cur_playernum(), 0xB000)))")
        respawn_end = "currentPlayerStartChrFade(120.0f, 1.0f);\n}"
        replace(respawn_end, "currentPlayerStartChrFade(120.0f, 1.0f);\n    lanNewLife();\n    mpClearSeatHits(get_cur_playernum());\n}")
        replace("f32 damage_dealt = g_playerPerm->handicap * damage_amount;",
                "if (lanActive() && !lanIsHost()) return;\n    f32 damage_dealt = g_playerPerm->handicap * damage_amount;")
        # Supply motion to the original third-person animation selector only.
        # Feeding replicated axes into MoveBond would simulate travel twice.
        begin = source.index("        if (playerGetCrouchPos(index) == 1)", source.index("s32 playerTick(PropRecord *prop)"))
        end = source.index("        struct firing_anim_struct *fa", begin)
        selector = source[begin:end]
        inputs = ("speedforwards", "speedsideways", "headanim")
        locals_ = ("animForward", "animStrafe", "animRun")
        for field, local in zip(inputs, locals_):
            selector = selector.replace(f"ppointers[index]->{field}", local)
        source = source[:begin] + """        float animForward = ppointers[index]->speedforwards;
        float animStrafe = ppointers[index]->speedsideways;
        int animRun = ppointers[index]->headanim;
        if (index == 1 && lanActive()) {
            LanPose remote = {0};
            lanRemote(&remote);
            animForward = remote.forward;
            animStrafe = remote.strafe;
            animRun = remote.run != 0;
        }
""" + selector + source[end:]
        entry = "void MoveBond(s8 stick_x, s8 stick_y, u16 buttons, u16 oldbuttons)"
        replace(entry, '#include "port/controls/finals_game.inc"\n'
                       '#include "port/lan/target_hits.inc"\n'
                       '#include "port/lan/pose.inc"\n' + entry)
        replace("bondviewProcessInput(stick_x, stick_y, buttons, oldbuttons);",
                controls + "xboxFinalsProcessInput(stick_x, stick_y, buttons, oldbuttons);" +
                ("" if xbox else "\n    else bondviewProcessInput(stick_x, stick_y, buttons, oldbuttons);"))
        for draw in ("gunRenderCasings(&gdl);", "gdl = display_red_blue_on_radar(gdl);"):
            replace(draw, "if (!lanSkipViewDraw) " + draw)
        replace("gunRenderFirstPersonGunModels(&gdl);",
                "if (" + ("" if xbox else "getPlayerCount() < 2 || ") +
                "!xboxDrawHeldPad(&gdl)) gunRenderFirstPersonGunModels(&gdl);")
        anchor = "bondviewUpdatePlayerY(use_stanHeight, sp390);"
        replace(anchor, controls + "xboxFinalsVertical(&use_stanHeight, &sp390);\n    " +
                anchor + "\n    lanMovePlayer();")
        for suffix, value in (("Height", "viGetY()"), ("Uly", "0")):
            entry = f"s16 bondviewGetCurrentPlayerViewport{suffix}(void)\n{{"
            replace(entry, entry + f"\n    if (lanActive()) return {value};")
    elif unit == "gun":
        marker = "void generate_player_thrown_grenade(s32 hand)\n{"
        replace(marker, """static unsigned finalsGrenadeThrows[MAXCONTROLLERS];
unsigned finalsGetGrenadeThrows(int slot) { return finalsGrenadeThrows[slot]; }
""" + marker)
        marker = "wor->projectile->unk94 = 0.13333333f;\n            wor->projectile->refreshrate = THROWN_ITEM_REFRESH_RATE;"
        replace(marker, marker + """
            ++finalsGrenadeThrows[get_cur_playernum()];
#if defined(_WIN32) && !defined(__XBOX__)
            if (getenv("GE_SPLIT_TRACE") && !lanActive())
                osSyncPrintf("Split grenade launched seat %d\\n", get_cur_playernum());
#endif""")
    elif unit == "propobj":
        marker = "void objFree(ObjectRecord* obj, s32 freeprop, s32 canregen)\n{"
        replace(marker, "static void lanForgetDrop(ObjectRecord *obj);\n"
                "static void lanCollectDrop(ObjectRecord *obj);\n" + marker +
                "\n    if (lanActive()) lanForgetDrop(obj);")
        marker = "void drop_inventory(void)"
        replace(marker, '#include "port/lan/drops.inc"\n\n' + marker)
        marker = "chrSetWeaponFlag4(playerchr, GUNLEFT);"
        replace(marker, marker + "\n    if (lanActive() && !lanIsHost()) return;")
        replace("                objDrop(prop);", "                if (objDrop(prop) && lanIsHost()) lanTrackDrop(prop);")
        marker = "if ((op == TICKOP_FREE) && ((obj->runtime_bitflags & RUNTIMEBITFLAG_TAGGED) == 0))"
        replace(marker, "if (op != TICKOP_NONE && lanIsHost()) lanCollectDrop(obj);\n    " + marker)
        entry = "TICKOP propPickupByPlayer(PropRecord *prop, bool showstring)\n{"
        replace(entry, entry + """
    if (lanActive() && !lanIsHost()) return TICKOP_NONE;
#if defined(_WIN32) && !defined(__XBOX__)
    extern int lanTestAllowPickup(PropRecord *);
    if (lanIsHost() && !lanTestAllowPickup(prop)) return TICKOP_NONE;
#endif""")
    elif unit == "chrprop":
        anchor = "if (prop->timetoregen > 0)"
        replace(anchor, "extern int lanHoldPickupTimer(PropRecord *);\n            "
                        "if (prop->timetoregen > 0 && !lanHoldPickupTimer(prop))")
        replace("if (!chrpropIsFarFromPlayers(prop))",
                "if ((!lanActive() || lanIsHost()) && !chrpropIsFarFromPlayers(prop))")
    elif unit == "mpmenu":
        replace("if (menu_count == player_count)", "if (!lanActive() && menu_count == player_count)")
        anchor = "(char *) langGet(getStringID(LMPMENU, MPMENU_STR_17_STARTTTOEXIT))"
        replace(anchor, 'lanActive() ? "START FOR REMATCH" : ' + anchor)
        anchor = "case MENU_FINISHED:\n                text = (char *) ascii_MP_watch_menu_BLANK;"
        replace(anchor, anchor.replace("(char *)", 'lanActive() ? "READY FOR REMATCH" : (char *)'))
    elif unit == "lv":
        replace("lvlSetMpPoint(get_mp_pointlimit());",
                "lvlSetMpPoint(lanActive() ? (lanIsHost() ? lanScoreLimit() : 0) : get_mp_pointlimit());")
        replace("lvlSetMpTime(get_mp_timelimit());",
                "lvlSetMpTime(lanActive() ? 0 : get_mp_timelimit());")
        anchor = "g_CurrentStageToLoad = stage;"
        if xbox:
            source = "static unsigned profileStageGeneration;\n" + source
        replace(anchor, anchor + ("\n    ++profileStageGeneration;" if xbox else "") +
                "\n    if (lanActive() && stage != LEVELID_TITLE) { extern void lanEnterRound(void); lanEnterRound(); }")
        source += """
#ifdef __XBOX__
int xboxProfileKills(int attacker, int victim)
{
    return g_playerPlayerData[attacker].kill_counts[victim];
}
unsigned xboxProfileGame(int *stage, int *shots, unsigned *throws)
{
    extern unsigned finalsGetGrenadeThrows(int slot);
    *stage = lvlGetCurrentStageToLoad();
    *shots = 0;
    *throws = 0;
    for (int slot = 0; slot < getPlayerCount(); ++slot) {
        *shots += g_playerPlayerData[slot].shot_count[0];
        *throws += finalsGetGrenadeThrows(slot);
    }
    return profileStageGeneration;
}
#endif
"""
        start_mp = 'lanActive() ? "2" : getenv("GE_STARTMP")'
        setup = ''
        if xbox:
            setup = 'extern int xboxLocalFourTestActive(void);\n            '
            start_mp = 'lanActive() ? "2" : (xboxLocalFourTestActive() ? "4,2" : getenv("GE_STARTMP"))'
        replace('const char *smp = getenv("GE_STARTMP");', setup + f'const char *smp = {start_mp};')
        anchor = "if (comma) MP_stage_selected = (s32)strtol(comma + 1, NULL, 0);"
        replace(anchor, anchor + "\n                if (lanActive()) MP_stage_selected = lanArena();"
                + ("\n                else if (xboxLocalFourTestActive()) MP_stage_selected = xboxLocalFourTestActive();" if xbox else ""))
        replace("init_mp_options_for_scenario(np);", """init_mp_options_for_scenario(np);
                if (lanActive()) {
                    player_char[0] = lanIsHost() ? 0 : 1;
                    player_char[1] = lanIsHost() ? 1 : 0;
                }""")
        source = "int lanSkipViewDraw;\n" + source
        for draw in ("DL = glassRenderShards(DL);", "DL = explosionRenderFlyingParticles(DL);"):
            replace(draw, "if (!lanSkipViewDraw) " + draw)
        replace("DL = weaponRenderTracers(DL);",
                controls + "if (!lanSkipViewDraw) { extern Gfx *xboxDrawWorldPads(Gfx *); "
                "DL = xboxDrawWorldPads(DL); }\n            DL = weaponRenderTracers(DL);")
        # The original renderer also ticks props per player. Preserve that work
        # and only submit the local camera's display list to the renderer.
        anchor = "set_cur_player(get_nth_player_from_shuffled(i));"
        replace(anchor, "Gfx *viewStart = DL;\n            " + anchor +
                "\n            ftRecord(FT_SEAT, ftGameId, get_cur_playernum());" +
                "\n            lanSkipViewDraw = lanActive() && get_cur_playernum() != 0 && ftSkipRemoteDraw();" +
                "\n            if (lanActive()) { extern void lanTickSeat(void); lanTickSeat(); }")
        for token, event in (("bgRoomVisibilityRelated();", "FT_VIS"), ("propsTick();", "FT_PROPS"),
                             ("DL = bgLevelRender(DL);", "FT_BG")):
            replace(token, f"ftRecord({event}, ftGameId, get_cur_playernum());\n            " + token)
        replace("DL = bgLevelRender(DL);", "DL = bgLevelRender(DL);\n            ftRecord(FT_AFTER_BG, ftGameId, get_cur_playernum());")
        replace("    g_GlobalTimer = 0;", "    g_GlobalTimer = 0;\n    extern void mpClearTargetHits(void);\n    mpClearTargetHits();")
        if xbox:
            replace("DL = mp_watch_menu_display(DL);", "extern Gfx *mpDrawTargetHits(Gfx *);\n            DL = mpDrawTargetHits(DL);\n            DL = mp_watch_menu_display(DL);")
        anchor = "DL = mp_watch_menu_display(DL);"
        if xbox:
            particles = "DL = explosionRenderFlyingParticles(DL);"
            replace(particles, particles + "\n            ftRecord(FT_AFTER_PARTICLES, ftGameId, get_cur_playernum());")
            replace(anchor, "ftRecord(FT_BEFORE_WATCH, ftGameId, get_cur_playernum());\n            " + anchor)
        replace(anchor, anchor + "\n            ftRecord(FT_SEAT_END, ftGameId, ((uint64_t)get_cur_playernum() << 32) | (uint32_t)((char *)DL - (char *)viewStart));" +
                "\n            if (lanActive() && get_cur_playernum() != 0) DL = viewStart;" +
                "\n            lanSkipViewDraw = 0;")
        replace("DL = viSetupScreensForNumPlayers(DL);",
                """if (lanActive()) {
                /* The native multiplayer sky inherits the black fill and full
                 * scissor from viSetupScreensForNumPlayers before its dividers. */
                gDPSetCycleType(DL++, G_CYC_FILL);
                gDPSetScissor(DL++, G_SC_NON_INTERLACE, 0, 0, viGetX(), viGetY());
                gDPSetFillColor(DL++, (GPACK_RGBA5551(0,0,0,1) << 16) | GPACK_RGBA5551(0,0,0,1));
            } else DL = viSetupScreensForNumPlayers(DL);""")
    return source


if __name__ == "__main__":
    src, dst = map(Path, sys.argv[1:])
    dst.write_text(adapt(src.stem, src.read_text()))
