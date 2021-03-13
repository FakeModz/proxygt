#include "events.h"
#include "gt.hpp"
#include "proton/hash.hpp"
#include "proton/rtparam.hpp"
#include "proton/variant.hpp"
#include "server.h"
#include "utils.h"
#include <thread>
#include <limits.h>

bool events::out::variantlist(gameupdatepacket_t* packet) {
    variantlist_t varlist{};
    varlist.serialize_from_mem(utils::get_extended(packet));
    PRINTS("varlist: %s\n", varlist.print().c_str());
    return false;
}

bool events::out::pingreply(gameupdatepacket_t* packet) {
    //since this is a pointer we do not need to copy memory manually again
    packet->m_vec2_x = 1000.f;  //gravity
    packet->m_vec2_y = 250.f;   //move speed
    packet->m_vec_x = 64.f;     //punch range
    packet->m_vec_y = 64.f;     //build range
    packet->m_jump_amount = 0;  //for example unlim jumps set it to high which causes ban
    packet->m_player_flags = 0; //effect flags. good to have as 0 if using mod noclip, or etc.
    return false;
}

bool find_command(std::string chat, std::string name) {
    bool found = chat.find("/" + name) == 0;
    if (found)
        gt::send_log("`6" + chat);
    return found;
}
bool wrench = false;
bool fastdrop = false;
bool fasttrash = false;
std::string mode = "pull";
bool events::out::generictext(std::string packet) {
    PRINTS("Generic text: %s\n", packet.c_str());
    auto& world = g_server->m_world;
    rtvar var = rtvar::parse(packet);
    if (!var.valid())
        return false;
        if (wrench == true) {
            if (packet.find("action|wrench") != -1) {
                g_server->send(false, packet);
                std::string sr = packet.substr(packet.find("netid|") + 6, packet.length() - packet.find("netid|") - 1);
                std::string motion = sr.substr(0, sr.find("|"));
                if (mode.find("pull") != -1) {
                    g_server->send(false, "action|dialog_return\ndialog_name|popup\nnetID|" + motion + "|\nnetID|" + motion + "|\nbuttonClicked|pull");
                }
                if (mode.find("kick") != -1) {
                    g_server->send(false, "action|dialog_return\ndialog_name|popup\nnetID|" + motion + "|\nnetID|" + motion + "|\nbuttonClicked|kick");
                }
                if (mode.find("ban") != -1) {
                    g_server->send(false, "action|dialog_return\ndialog_name|popup\nnetID|" + motion + "|\nnetID|" + motion + "|\nbuttonClicked|worldban");
                }
                if (mode.find("trade") != -1) {
                    g_server->send(false, "action|dialog_return\ndialog_name|popup\nnetID|" + motion + "|\nnetID|" + motion + "|\nbuttonClicked|trade");
                }
                return true;
            }
        }
    if (var.get(0).m_key == "action" && var.get(0).m_value == "input") {
        if (var.size() < 2)
            return false;
        if (var.get(1).m_values.size() < 2)
            return false;

        if (!world.connected)
            return false;

        auto chat = var.get(1).m_values[1];
        if (find_command(chat, "name ")) { //ghetto solution, but too lazy to make a framework for commands.
            std::string name = "``" + chat.substr(6) + "``";
            variantlist_t va{ "OnNameChanged" };
            va[1] = name;
            g_server->send(true, va, world.local.netid, -1);
            gt::send_log("`4name set to: " + name);
            return true;
        } else if (find_command(chat, "flag ")) {
            int flag = atoi(chat.substr(6).c_str());
            variantlist_t va{ "OnGuildDataChanged" };
            va[1] = 1;
            va[2] = 2;
            va[3] = flag;
            va[4] = 3;
            g_server->send(true, va, world.local.netid, -1);
            gt::send_log("flag set to item id: " + std::to_string(flag));
            return true;
        } else if (find_command(chat, "ghost")) {
            gt::ghost = !gt::ghost;
            if (gt::ghost)
                gt::send_log("Ghost is now enabled.");
            else
                gt::send_log("Ghost is now disabled.");
            return true;
            
        } else if (find_command(chat, "setcountry ")) {
            std::string cy = chat.substr(9);
            gt::flag = cy;
            gt::send_log("your country set to " + cy + ", (Relog to game to change it successfully!)");
            return true;
        }
        else if (find_command(chat, "fd")) {
            fastdrop = !fastdrop;
            if (fastdrop)
                gt::send_log("Fast Drop is now enabled.");
            else
                gt::send_log("Fast Drop is now disabled.");
            return true;
        }
        else if (find_command(chat, "ft")) {
            fasttrash = !fasttrash;
            if (fasttrash)
                gt::send_log("Fast Trash is now enabled.");
            else
                gt::send_log("Fast Trash is now disabled.");
            return true;
        }        
        else if (find_command(chat, "ws ")) {
            mode = chat.substr(10);
            gt::send_log("Wrench mode set to " + mode);
            return true;        
        }
        else if (find_command(chat, "wm")) {
            wrench = !wrench;
            if (wrench)
                gt::send_log("Wrench mode is on.");
            else
                gt::send_log("Wrench mode is off.");
            return true;
        }
        else if (find_command(chat, "uid ")) {
            std::string name = chat.substr(5);
            gt::send_log("resolving uid for " + name);
            g_server->send(false, "action|input\n|text|/ignore /" + name);
            g_server->send(false, "action|friends");
            gt::resolving_uid2 = true;
            return true;

        } else if (find_command(chat, "tp ")) {
            std::string name = chat.substr(4);
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            for (auto& player : g_server->m_world.players) {
                auto name_2 = player.name.substr(2); //remove color
                std::transform(name_2.begin(), name_2.end(), name_2.begin(), ::tolower);
                if (name_2.find(name) == 0) {
                    gt::send_log("`4Teleporting to" + player.name);
                    variantlist_t varlist{ "OnSetPos" };
                    varlist[1] = player.pos;
                    g_server->m_world.local.pos = player.pos;
                    g_server->send(true, varlist, g_server->m_world.local.netid, -1);
                    break;
                }
            }
            return true;
        } else if (find_command(chat, "warp ")) {
            std::string name = chat.substr(6);
            gt::send_log("`4Warping to " + name);
            g_server->send(false, "action|join_request\nname|" + name, 3);
            return true;
            
           } else if (find_command(chat, "pullall")) {
            std::string username = chat.substr(6);
            for (auto& player : g_server->m_world.players) {
                auto name_2 = player.name.substr(2); //remove color
                if (name_2.find(username)) {
                    g_server->send(false, "action|wrench\n|netid|" + std::to_string(player.netid));
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    g_server->send(false, "action|dialog_return\ndialog_name|popup\nnetID|" + std::to_string(player.netid) + "|\nbuttonClicked|pull"); 
                    // You Can |kick |trade |worldban 
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    gt::send_log("`4Pulled");
                  
                }
            }
          } else if (find_command(chat, "banall")) {
            std::string username = chat.substr(6);
            for (auto& player : g_server->m_world.players) {
                auto name_2 = player.name.substr(2); //remove color
                if (name_2.find(username)) {
                    g_server->send(false, "action|wrench\n|netid|" + std::to_string(player.netid));
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    g_server->send(false, "action|dialog_return\ndialog_name|popup\nnetID|" + std::to_string(player.netid) + "|\nbuttonClicked|worldban"); 
                    // You Can |kick |trade |worldban 
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    gt::send_log("`4banned all people in world");
                  
                }
            }
      
           } else if (find_command(chat, "tradeall")) {
            std::string username = chat.substr(6);
            for (auto& player : g_server->m_world.players) {
                auto name_2 = player.name.substr(2); //remove color
                if (name_2.find(username)) {
                    g_server->send(false, "action|wrench\n|netid|" + std::to_string(player.netid));
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    g_server->send(false, "action|dialog_return\ndialog_name|popup\nnetID|" + std::to_string(player.netid) + "|\nbuttonClicked|trade"); 
                    // You Can |kick |trade |worldban 
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    gt::send_log("`4Trade all People in world");
                  
                }
            }
     } else if (find_command(chat, "killall")) {
            std::string username = chat.substr(6);
            for (auto& player : g_server->m_world.players) {
                auto name_2 = player.name.substr(2); //remove color
                if (name_2.find(username)) {
                    g_server->send(false, "action|wrench\n|netid|" + std::to_string(player.netid));
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    g_server->send(false, "action|dialog_return\ndialog_name|popup\nnetID|" + std::to_string(player.netid) + "|\nbuttonClicked|kick"); 
                    // You Can |kick |trade |worldban 
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    gt::send_log("`4Killing all People in world");
                  
                }
            }

            return true;
        } else if (find_command(chat, "skin ")) {
            int skin = atoi(chat.substr(6).c_str());
            variantlist_t va{ "OnChangeSkin" };
            va[1] = skin;
            g_server->send(true, va, world.local.netid, -1);
            return true;
            
        } else if (find_command(chat, "wrench ")) {
            std::string name = chat.substr(6);
            std::string username = ".";
            for (auto& player : g_server->m_world.players) {
                auto name_2 = player.name.substr(2);
                std::transform(name_2.begin(), name_2.end(), name_2.begin(), ::tolower);
                    g_server->send(false, "action|wrench\n|netid|" + std::to_string(player.netid));
            }
            return true;
        } else if (find_command(chat, "phelp")) {
           // gt::send_log(
            //    "/tp [name] (teleports to a player in the world), /ghost (toggles ghost, you wont move for others when its enabled), /uid "
            //    "[name] (resolves name to uid), /flag [id] (sets flag to item id), /name [name] (Change name), /warp [world name] (warping world without SSUP)"
            //    " /ft (fast trash) , /fd (fast drop) , /wm (wrench mode) (this for pull/kick/ban wrench), /setcountry [country] (change Country still bug),"
            //    "/pullall (only owner world), /banall (only owner world), /tradeall (trade all people in world), /skin [Id] (change skin colour)");
           // return true;
            std::string paket;
            paket =
            "\nadd_label_with_icon|big|Android Proxy Gazette|left|32|"
                "\nadd_spacer|small"
                "\nadd_textbox|`9/phelp `2(shows proxy commands)|left|2480|"
                "\nadd_textbox|`9/tp `2(teleport to player)|left|2480|"
                "\nadd_textbox|`9/ghost `2(ghost mode)|left|2480|"
                "\nadd_textbox|`9/uid [name]`2(resolves name to uid)|left|2480|"
                "\nadd_textbox|`9/name `2(change name visual)|left|2480|"
                "\nadd_textbox|`9/flag `2(change flag like guild) |left|2480|"
                "\nadd_textbox|`9/setcountry `2change country flag still bug)|left|2480|"
                "\nadd_textbox|`9/warp `2(Warping world without super supporter)|left|2480|"
                "\nadd_textbox|`9/ft  `2(fast trash)|left|2480|"
                "\nadd_textbox|`9/fd  `2(fast drop)|left|2480|"
                "\nadd_textbox|`9/skin [id] `2(sets your skin)|left|2480|"
                "\nadd_textbox|`9/pullall `2(only for owner or admin)|left|2480|"
                "\nadd_textbox|`9/banall `2(only for owner or admin)|left|2480|"
                "\nadd_textbox|`9/killall `2(only for owner or admin)|left|2480|"
                "\nadd_textbox|`9/tradeall `2(trade all people in world)|left|2480|"
                "\nadd_textbox|`9/wm `2(wrenchmode pull, kick, ban, trade)|left|2480|"
                "\nadd_textbox|`9/ws [choose one]`2(set wrench mode to pull, kick, ban, or trade)|left|2480|"
                "\nadd_quick_exit|"
                "\nend_dialog|end|Cancel|Okay|";
            variantlist_t liste{ "OnDialogRequest" };
            liste[1] = paket;
            g_server->send(true, liste);
            return true;
            }
            
    }

    if (packet.find("game_version|") != -1) {
        rtvar var = rtvar::parse(packet);
        auto mac = utils::generate_mac();
        auto hash_str = mac + "RT";
        auto hash2 = utils::hash((uint8_t*)hash_str.c_str(), hash_str.length());
        var.set("mac", mac);
        var.set("wk", utils::generate_rid());
        var.set("rid", utils::generate_rid());
        var.set("fz", std::to_string(utils::random(INT_MIN, INT_MAX)));
        var.set("zf", std::to_string(utils::random(INT_MIN, INT_MAX)));
        var.set("hash", std::to_string(utils::random(INT_MIN, INT_MAX)));
        var.set("hash2", std::to_string(hash2));
        var.set("meta", utils::random(utils::random(6, 10)) + ".com");
        var.set("game_version", gt::version);
        var.set("country", gt::flag);

        /*
        AAP Bypass
        Only making this public because after 1 month being reported to ubi, nothing happened
        Then after a month (around 15.3) it got fixed for a whole single 1 day, and they publicly said it had been fixed
        And at that time we shared how to do it because thought its useless, and then aap bypass started working again
        and then 9999 new aap bypass services came to be public, and even playingo started selling it so no point keeping it private
        With publishing this I hope ubi actually does something this time
        */

        //Finally patched, I guess they finally managed to fix this after maybe a year!

        //if (var.find("tankIDName") && gt::aapbypass) {
        //    var.find("mac")->m_values[0] = "02:00:00:00:00:00";
        //    var.find("platformID")->m_values[0] = "4"; //android
        //    var.remove("fz");
        //    var.remove("rid");
        //}

        packet = var.serialize();
        gt::in_game = false;
        PRINTS("Spoofing login info\n");
        g_server->send(false, packet);
        return true;
    }

    return false;
}

bool events::out::gamemessage(std::string packet) {
    PRINTS("Game message: %s\n", packet.c_str());
    if (packet == "action|quit") {
        g_server->quit();
        return true;
    }

    return false;
}

bool events::out::state(gameupdatepacket_t* packet) {
    if (!g_server->m_world.connected)
        return false;

    g_server->m_world.local.pos = vector2_t{ packet->m_vec_x, packet->m_vec_y };
    PRINTS("local pos: %.0f %.0f\n", packet->m_vec_x, packet->m_vec_y);

    if (gt::ghost)
        return true;
    return false;
}

bool events::in::variantlist(gameupdatepacket_t* packet) {
    variantlist_t varlist{};
    auto extended = utils::get_extended(packet);
    extended += 4; //since it casts to data size not data but too lazy to fix this
    varlist.serialize_from_mem(extended);
    PRINTC("varlist: %s\n", varlist.print().c_str());
    auto func = varlist[0].get_string();

    //probably subject to change, so not including in switch statement.
    if (func.find("OnSuperMainStartAcceptLogon") != -1)
        gt::in_game = true;

    switch (hs::hash32(func.c_str())) {
        //solve captcha
        case fnv32("onShowCaptcha"): {
            auto menu = varlist[1].get_string();
            gt::solve_captcha(menu);
            return true;
        } break;
        case fnv32("OnRequestWorldSelectMenu"): {
            auto& world = g_server->m_world;
            world.players.clear();
            world.local = {};
            world.connected = false;
            world.name = "EXIT";
        } break;
        case fnv32("OnSendToServer"): g_server->redirect_server(varlist); return true;

        case fnv32("OnConsoleMessage"): {
            varlist[1] = "`b[ANDROID PROXY]`` " + varlist[1].get_string();
            g_server->send(true, varlist);
            return true;
        } break;
        case fnv32("OnDialogRequest"): {
            auto content = varlist[1].get_string();

            if (content.find("set_default_color|`o") != -1) 
            {
                if (content.find("end_dialog|captcha_submit||Submit|") != -1) 
                {
                    gt::solve_captcha(content);
                    return true;
                }
            }
        if (wrench == true) {
            if (content.find("add_button|report_player|`wReport Player``|noflags|0|0|") != -1) {
                if (content.find("embed_data|netID") != -1) {
                    return true; // block wrench dialog
                }
            }
        }
        if (fastdrop == true) {
            std::string itemid = content.substr(content.find("embed_data|itemID|") + 18, content.length() - content.find("embed_data|itemID|") - 1);
            std::string count = content.substr(content.find("count||") + 7, content.length() - content.find("count||") - 1);
            if (content.find("embed_data|itemID|") != -1) {
                if (content.find("Drop") != -1) {
                    g_server->send(false, "action|dialog_return\ndialog_name|drop_item\nitemID|" + itemid + "|\ncount|" + count);
                    return true;
                }
            }
        }
        if (fasttrash == true) {
            std::string itemid = content.substr(content.find("embed_data|itemID|") + 18, content.length() - content.find("embed_data|itemID|") - 1);
            std::string count = content.substr(content.find("you have ") + 9, content.length() - content.find("you have ") - 1);
            std::string delimiter = ")";
            std::string token = count.substr(0, count.find(delimiter));
            if (content.find("embed_data|itemID|") != -1) {
                if (content.find("Trash") != -1) {
                    g_server->send(false, "action|dialog_return\ndialog_name|trash_item\nitemID|" + itemid + "|\ncount|" + token);
                    return true;
                }
            }
        }            
        
            //hide unneeded ui when resolving
            //for the /uid command
            if (gt::resolving_uid2 && (content.find("friend_all|Show offline") != -1 || content.find("Social Portal") != -1) ||
                content.find("Are you sure you wish to stop ignoring") != -1) {
                return true;
            } else if (gt::resolving_uid2 && content.find("Ok, you will now be able to see chat") != -1) {
                gt::resolving_uid2 = false;
                return true;
            } else if (gt::resolving_uid2 && content.find("`4Stop ignoring") != -1) {
                int pos = content.rfind("|`4Stop ignoring");
                auto ignore_substring = content.substr(0, pos);
        }
