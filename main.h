#pragma once
#include <fstream>
#include <MiraiCP.hpp>
#include <direct.h>
#include <json.hpp>
#include <string>
#include <random>
#include <regex>
#include <functional>
#include <algorithm>
#include <Urlmon.h>
#include <Shlwapi.h>
#include <direct.h>
#include "Helper.hpp"
using namespace MiraiCP;
// ���ʵ��
class Main : public CPPPlugin {
public:
    // ���ò����Ϣ
    Main() :cwd(_getcwd(0, 0) + std::string("\\")), CPPPlugin(PluginConfig("Plugin id", "Plugin name", "Version", "Author name", "Plugin description", "Publish time")) {}
    void onEnable() override {
    // ����
        Event::processor.registerEvent<GroupMessageEvent>([this](GroupMessageEvent e)
            {
                OnGroupMessage(e);
            });
        registerMsgFunctions();
        loadCards();

    }

    void onDisable() override {
        /*�������*/
    }
private:
    using MsgFunc = std::function<void(const std::string&, GroupMessageEvent&)>;
    std::unordered_map<std::string, MsgFunc> msgCall;

    void OnGroupMessage(GroupMessageEvent e);
    void registerMsgFunctions();
    void loadCards();
    std::string drawCard(const std::string& handle, const nlohmann::json& target);
    std::string drawRecur(std::string src, const nlohmann::json& target);
    std::string drawRand(std::string src, const nlohmann::json& target);

    const std::string cwd;
    std::vector<nlohmann::json> cardJson;
    std::unordered_map<std::string, std::pair<int, std::string>> cards;
};