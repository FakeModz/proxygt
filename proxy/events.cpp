#include "events.h"
#include "gt.hpp"
#include "proton/hash.hpp"
#include "proton/rtparam.hpp"
#include "proton/variant.hpp"
#include "server.h"
#include "utils.h"
#include <thread>
#include <limits.h>
#include <stdio.h> /* printf, NULL */ 
#include <stdlib.h> /* srand, rand */ 
#include <time.h> /* time */



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
bool wrenchmsg = false; 
bool wrenchspam = false; 
bool automessage = false; 
bool autopull = false; 
bool setmsg = false;
std::string message = "";
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
                if (mode.find("add") != -1) {
                    g_server->send(false, "action|dialog_return\ndialog_name|popup\nnetID|" + motion + "|\nnetID|" + motion + "|\nbuttonClicked|friend_add");
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
            gt::send_log("`#name set to: " + name);
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
                gt::send_log("`#Ghost is now enabled.");
            else
                gt::send_log("`#Ghost is now disabled.");
            return true;
        } else if (find_command(chat, "country ")) {
            std::string cy = chat.substr(9);
            gt::flag = cy;
            gt::send_log("`#your country set to " + cy + ", (Relog to game to change it successfully!)");
            return true;
        }
        else if (find_command(chat, "fd")) {
            fastdrop = !fastdrop;
            if (fastdrop)
                gt::send_log("`#Fast Drop is now enabled.");
            else
                gt::send_log("`#Fast Drop is now disabled.");
            return true;
        }
        else if (find_command(chat, "ft")) {
            fasttrash = !fasttrash;
            if (fasttrash)
                gt::send_log("`#Fast Trash is now enabled.");
            else
                gt::send_log("`#Fast Trash is now disabled.");
            return true;
        }        
        else if (find_command(chat, "wrenchmsg")) {
            wrenchmsg = !wrenchmsg;
            if (wrenchmsg)
                gt::send_log("`#wrenchmsg is now enabled.");
            else
                gt::send_log("`#wrenchmsg is now disabled.");
            return true;
         } 
             else if (find_command(chat, "automsg")) {
            automessage = !automessage;
            if (automessage)
                gt::send_log("`#automsg is now enabled.");
            else
                gt::send_log("`#automsg is now disabled.");
            return true;
          } 
             else if (find_command(chat, "autopull")) {
            autopull = !autopull;
            if (autopull)
                gt::send_log("`#autopull is now enabled.");
            else
                gt::send_log("`#autopull is now disabled.");
            return true;
         }          
        else if (find_command(chat, "wrenchspam")) {
            wrenchspam = !wrenchspam;
            if (wrenchspam)
                gt::send_log("`#wrenchspam is now enabled.");
            else
                gt::send_log("`#wrenchspam is now disabled.");
            return true;
        }  
     else if (find_command(chat, "setmsg ")) {
       message = chat.substr(7);
       gt::send_log("Set Message to"+ message); 
       return true;
         }

        
        else if (find_command(chat, "wrenchset ")) {
            mode = chat.substr(10);
            gt::send_log("`#Wrench mode set to " + mode);
            return true;        
        }
        else if (find_command(chat, "wrenchmode")) {
            wrench = !wrench;
            if (wrench)
                gt::send_log("`#Wrench mode is on.");
            else
                gt::send_log("`#Wrench mode is off.");
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
                    gt::send_log("`#Teleporting to " + player.name);
                    variantlist_t varlist{ "OnSetPos" };
                    varlist[1] = player.pos;
                    g_server->m_world.local.pos = player.pos;
                    g_server->send(true, varlist, g_server->m_world.local.netid, -1);
                    break;
                }
            }
         

         
        } else if (find_command(chat, "warp ")) {
            std::string name = chat.substr(6);
            gt::send_log("`#Warping to " + name);
            g_server->send(false, "action|join_request\nname|" + name, 3);
            return true;
          
      } else if (find_command(chat, "door ")) {
            std::string worldname = g_server->m_world.name.c_str();
            std::string idkntl = chat.substr(6);
            g_server->send(false, "action|join_request\nname|" + worldname + "|" + idkntl, 3);
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
                    gt::send_log("`4Pulling all people");
                  
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
                    gt::send_log("`4Kill All People in world");
                  
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
                    gt::send_log("`4Trade all people in world");
                  
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
                    gt::send_log("`4Banned all people in world");
                  
                }
            }

} else if (find_command(chat, "msgall")) {
           std::string msgtext = "              Message from FakeModz YT";
            std::string username = chat.substr(6);
            for (auto& player : g_server->m_world.players) {
                auto name_2 = player.name.substr(2); //remove color
                if (name_2.find(username)) {
                  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                  g_server->send(false, "action|input\n|text|/msg "  +        player.name         +                   msgtext);
                 // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                 // g_server->send(false, "action|input\n|text|/msg "  +        player2.name         +                   msgtext);
                  //std::this_thread::sleep_for(std::chrono::milliseconds(200));
                 // gt::send_log("`4Message all people in world");
                  
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


}else if (find_command(chat, "pinfo")) {
                   std::string paket;
            paket =
                "\nadd_label_with_icon|big|Proxy information|left|20|"
               "\nadd_image_button|banner|interface/large/special_event.rttex|bannerlayout|||"
                "\nadd_spacer|small"
                "\nadd_textbox|`9This Proxy Made by Ama6nen and Re-Edit By FakeModz#1192|left|2480|"
                "\nadd_textbox|`9Command List for command list please do /phelp|left|2480|"
                "\nadd_textbox|`9Thanks to :|left|2480|"
                "\nadd_textbox|`9Gucktube YT|left|2480|"
                "\nadd_textbox|`9Ama6nen|left|2480|"
                "\nadd_textbox|`9Genta 7740|left|2480|"
                "\nadd_textbox|`9BotHax YT|left|2480|"
                "\nadd_textbox|`9SrMotion|left|2480|"
                "\nadd_textbox|`9If you Want Re-Edit this proxy please|left|2480|"
                "\nadd_textbox|`9Dont Edit/Delete The Credits!!!|left|2480|"
                "\nadd_textbox|`9or you will dieee !!!!!|left|2480|"
                "\nadd_quick_exit|"
                "\nend_dialog|end|Cancel|Okay|";
            variantlist_t liste{ "OnDialogRequest" };
            liste[1] = paket;
            g_server->send(true, liste);
            return true;
        
        } else if (find_command(chat, "phelp")) {
           // gt::send_log(
            //    "`2/tp [name] (teleports to a player in the world), /ghost (toggles ghost, you wont move for others when its enabled), /uid "
            //    "`2[name] (resolves name to uid), /flag [id] (sets flag to item id), /name [name] (sets name to name), /banall, /kickall, /tradeall"
            //    "`2/warp [world name] (warping world without SSUP), /skin [Id] (change skin colours), /wrenchmode (for wrench pull, kick, pull, ban, trade)"
           //     "`2/ft (fast trash), /fd (fast drop), /setcountry (bug), /wrenchset (for set wrenchmode : pull,kick,ban,trade,add friend),/msgall (bug), /pinfo"
            //    "`2/wrenchmsg (Auto Msg Wrench People), /setmsg (for set message text)");
           std::string paket1;
            paket1 =
                "\nadd_label_with_icon|big|Proxy Commands Gazette|left|20|"
                "\nadd_image_button|banner|interface/large/news_banner.rttex|bannerlayout|||"
                "\nadd_spacer|small"
                "\nadd_textbox|`2/tp [name] (teleports to a player in the world)|left|2480|"
                "\nadd_textbox|`2/ghost (toggles ghost, you wont move for others when its enabled)|left|2480|"
                "\nadd_textbox|`2/uid [name] (resolves name to uid)|left|2480|"
                "\nadd_textbox|`2/flag [id] (sets flag to item id)|left|2480|"
                "\nadd_textbox|`2/name [name] (Change Name Visual)|left|2480|"
                "\nadd_textbox|`2/banall (World Ban All People in world)|left|2480|"
                "\nadd_textbox|`2/killall (Kick all People in world)|left|2480|"
                "\nadd_textbox|`2/tradeall (trade all people in the world|left|2480|"
                "\nadd_textbox|`2/warp [world name] (warping world without SSUP)|left|2480|"
                "\nadd_textbox|`2/skin [Id] (change skin colours)|left|2480|"
                "\nadd_textbox|`2/wrenchmode (wrench modefor wrench pull, kick, pull, ban, trade, add)|left|2480|"
                "\nadd_textbox|`2/wrenchset (for set wrenchmode : pull,kick,ban,trade,add friend)|left|2480|"
                "\nadd_textbox|`2/ft (fast trash) |left|2480|"
                "\nadd_textbox|`2/fd (fast drop) |left|2480|"
                "\nadd_textbox|`2/wrenchmsg (Auto Msg when wrench people) |left|2480|"
                "\nadd_textbox|`2/setmsg (Costum Text for Wrenchmsg and wrenchspam) |left|2480|"
                "\nadd_textbox|`2/setcountry (bug) |left|2480|"
                "\nadd_textbox|`2/msgall (not really worked because spam detected) |left|2480|"
                "\nadd_textbox|`2/wrenchspam (wrench spam like wrench msg do/setspam for set text) |left|2480|"
                "\nadd_textbox|`2/automsg (auto msg when people enter world) |left|2480|"
                "\nadd_textbox|`2/door (teleport to id door (you must know the id door)) |left|2480|"
                "\nadd_textbox|`2/pinfo (Proxy information) |left|2480|"
                "\nadd_textbox|`2/autopull (auto pull when people enter world) |left|2480|"
                "\nadd_spacer|small|\n\nadd_url_button||`$YouTube``|NOFLAGS|https://youtube.com/c/FakeModzGT|Open link?|0|0|"
                "\nadd_spacer|small|\n\nadd_url_button||`$Discord``|NOFLAGS|https://discord.com/invite/YfnMbjWjpP|Open link?|0|0|"
                "\nadd_quick_exit|"
                "\nend_dialog|end|Cancel|Okay|";
            variantlist_t liste{ "OnDialogRequest" };
            liste[1] = paket1;
            g_server->send(true, liste);
            g_server->send(true,  "action|play_sfxfile|audio/ogg/dabstep.ogg");
            return true;
        } 
        return false;
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
            varlist[1] = "`#[FakeModz]`` " + varlist[1].get_string();
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
      
      if (wrenchmsg == true) {
    if (content.find("embed_data|netID") !=-1) {
     if(content.find("Add as friend") !=-1) {
        //std::string yourmsg = "Message from FakeModz YT";
        std::string titit = content.substr(content.find("add_label_with_icon|big|`w") + 26, content.length() - content.find("add_label_with_icon|big|`w") - 1);
        titit.erase(titit.begin() + titit.find(" (`2"), titit.end());
        std::string memq = titit + " ";
        std::string ada = "`9"+ message;
        std::string bab = "`8"+ message;
	std::string cok = "`b"+ message;
	std::string ded = "`6"+ message;
        std::string ert = "`$"+ message;
        std::string fuy = "`e"+ message;
	std::string gog = "`c"+ message;
	std::string hew = "`4"+ message;
        std::string ire = "`3"+ message;
        std::string jok = "`2"+ message;
	std::string klo = "`1"+ message;
	std::string lol = "`a"+ message;
        srand(time(NULL)); 
        std::string Message[12] = {ada, bab, cok, ded, ert, fuy, gog, hew, ire, jok, klo, lol};
        int Random = rand() % 12; 
        g_server->send(false, "action|input\n|text|/msg " + memq + Message[Random]);
        gt::send_log("Message Send to "  + memq); 
 
        return true;
    }
} 
}
if (wrenchspam == true) {
    if (content.find("embed_data|netID") !=-1) {
     if(content.find("Add as friend") !=-1) {
        //std::string yourmsg = "Message from FakeModz YT";
        std::string titit1 = content.substr(content.find("add_label_with_icon|big|`w") + 26, content.length() - content.find("add_label_with_icon|big|`w") - 1);
        //titit1.erase(titit1.begin() + titit1.find(" (`2"), titit1.end());
        //std::string memq = titit1 + " ";
        g_server->send(false, "action|input\n|text|"+message);
     
 
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
                auto uid = ignore_substring.substr(ignore_substring.rfind("add_button|") + 11);
                auto uid_int = atoi(uid.c_str());
                if (uid_int == 0) {
                    gt::resolving_uid2 = false;
                    gt::send_log("name resolving seems to have failed.");
                } else {
                    gt::send_log("Target UID: " + uid);
                    g_server->send(false, "action|dialog_return\ndialog_name|friends\nbuttonClicked|" + uid);
                    g_server->send(false, "action|dialog_return\ndialog_name|friends_remove\nfriendID|" + uid + "|\nbuttonClicked|remove");
                }
                return true;
            }
        } break;
        case fnv32("OnRemove"): {
            auto text = varlist.get(1).get_string();
            if (text.find("netID|") == 0) {
                auto netid = atoi(text.substr(6).c_str());

                if (netid == g_server->m_world.local.netid)
                    g_server->m_world.local = {};

                auto& players = g_server->m_world.players;
                for (size_t i = 0; i < players.size(); i++) {
                    auto& player = players[i];
                    if (player.netid == netid) {
                        players.erase(std::remove(players.begin(), players.end(), player), players.end());
                        break;
                    }
                }
            }
        } break;
        case fnv32("OnSpawn"): {
            std::string meme = varlist.get(1).get_string();
            rtvar var = rtvar::parse(meme);
            auto name = var.find("name");
            auto netid = var.find("netID");
            auto onlineid = var.find("onlineID");

            if (name && netid && onlineid) {
                player ply{};

                if (var.find("invis")->m_value != "1") {
                    ply.name = name->m_value;
                    ply.country = var.get("country");
                    name->m_values[0] += " `4[" + netid->m_value + "]``";
                    auto pos = var.find("posXY");
                    if (pos && pos->m_values.size() >= 2) {
                        auto x = atoi(pos->m_values[0].c_str());
                        auto y = atoi(pos->m_values[1].c_str());
                        ply.pos = vector2_t{ float(x), float(y) };
                    }
                } else {
                    ply.mod = true;
                    ply.invis = true;
                }
                if (var.get("mstate") == "1" || var.get("smstate") == "1")
                    ply.mod = true;
                ply.userid = var.get_int("userID");
                ply.netid = var.get_int("netID");
                if (meme.find("type|local") != -1) {
                    //set mod state to 1 (allows infinite zooming, this doesnt ban cuz its only the zoom not the actual long punch)
                    var.find("mstate")->m_values[0] = "1";
                    g_server->m_world.local = ply;
                }
                g_server->m_world.players.push_back(ply);
                auto str = var.serialize();
                utils::replace(str, "onlineID", "onlineID|");
                varlist[1] = str;
                PRINTC("new: %s\n", varlist.print().c_str());
                g_server->send(true, varlist, -1, -1);
                if (automessage == true) {
                    try {
                        std::string jokey = "`2   [1]"+ message;
	                std::string klore = "`1   [2]"+ message;
	                std::string loler = "`8   [3]"+ message;
                        std::string jokuy = "`6   [4]"+ message;
	                std::string klori = "`9   [5]"+ message;
	                std::string lolir = "`4   [6]"+ message;
                        srand(time(NULL)); 
                        std::string Message2[6] = {jokey, klore, loler, jokuy, klori, lolir};
                        int Random2 = rand() % 6; 
                        std::this_thread::sleep_for(std::chrono::milliseconds(250));
                        g_server->send(false, "action|input\n|text|/msg " + ply.name +     Message2[Random2]);
                    } catch (std::exception) { gt::send_log("Critical Error : Invalid String Position"); }
                }
                 if (autopull == true) {
                    try { 
                       g_server->send(false, "action|input\n|text|/pull " +ply.name.substr(2));
                      //  gt::send_log("Cooming Soon if possible");
                        
   

                    } catch (std::exception) { gt::send_log("Critical Error : Invalid String Position"); }
                }
                return true;
            }

            
        } break;
    }
    return false;
}

bool events::in::generictext(std::string packet) {
    PRINTC("Generic text: %s\n", packet.c_str());
    return false;
}

bool events::in::gamemessage(std::string packet) {
    PRINTC("Game message: %s\n", packet.c_str());

    if (gt::resolving_uid2) {
        if (packet.find("PERSON IGNORED") != -1) {
            g_server->send(false, "action|dialog_return\ndialog_name|friends_guilds\nbuttonClicked|showfriend");
            g_server->send(false, "action|dialog_return\ndialog_name|friends\nbuttonClicked|friend_all");
        } else if (packet.find("Nobody is currently online with the name") != -1) {
            gt::resolving_uid2 = false;
            gt::send_log("Target is offline, cant find uid.");
        } else if (packet.find("Clever perhaps") != -1) {
            gt::resolving_uid2 = false;
            gt::send_log("Target is a moderator, can't ignore them.");
        }
    }
    return false;
}

bool events::in::sendmapdata(gameupdatepacket_t* packet) {
    g_server->m_world = {};
    auto extended = utils::get_extended(packet);
    extended += 4;
    auto data = extended + 6;
    auto name_length = *(short*)data;

    char* name = new char[name_length + 1];
    memcpy(name, data + sizeof(short), name_length);
    char none = '\0';
    memcpy(name + name_length, &none, 1);

    g_server->m_world.name = std::string(name);
    g_server->m_world.connected = true;
    delete[] name;
    PRINTC("world name is %s\n", g_server->m_world.name.c_str());
    return false;
}

bool events::in::state(gameupdatepacket_t* packet) {
    if (!g_server->m_world.connected)
        return false;
    if (packet->m_player_flags == -1)
        return false;

    auto& players = g_server->m_world.players;
    for (auto& player : players) {
        if (player.netid == packet->m_player_flags) {
            player.pos = vector2_t{ packet->m_vec_x, packet->m_vec_y };
            PRINTC("player %s position is %.0f %.0f\n", player.name.c_str(), player.pos.m_x, player.pos.m_y);
            break;
        }
    }
    return false;
}

bool events::in::tracking(std::string packet) {
    PRINTC("Tracking packet: %s\n", packet.c_str());
    return true;
}
