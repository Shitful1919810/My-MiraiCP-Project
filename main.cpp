#include "main.h"
#include "Helper.hpp"
#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "Shlwapi.lib")
using namespace std;
using namespace nlohmann;

// 绑定当前插件实例
void MiraiCP::enrollPlugin() {
  MiraiCP::enrollPlugin0(new Main());
}

void Main::OnGroupMessage(GroupMessageEvent e)
{
	static const regex rules[] = { regex("[!.](draw)\\s*(.*)"),regex("[!.](r)\\s*(.*)"),regex("[!.](uploadimg)\\s*(\\w+)\\s*.*"),regex("[!.](img)\\s*(\\w+)\\s*.*") };
	if (e.message.vector().size() == 0 || SingleMessage::messageType[e.message.vector().front().type()] != "plainText")
		return;
	string msg = e.message.vector().front().get<PlainText>().content;
	for (const regex& reg : rules)
	{
		smatch matches;
		if (regex_match(msg, matches, reg))
		{
			string instr = matches.str(1);
			if (msgCall.find(instr) == msgCall.end())
			{
				e.group.sendMessage("不支持的指令:"s + instr);
				break;
			}
			string args = matches.str(2);
			msgCall[instr](args, e);
			break;
		}
	}
}

void Main::registerMsgFunctions()
{
	msgCall["draw"] = [this](const string& src, GroupMessageEvent& e)
	{
		if (cards.find(src) == cards.end())
			e.group.sendMessage("找不到牌堆:"s + src);
		else
			e.group.sendMessage(drawCard(cards[src].second, cardJson[cards[src].first])); };

	msgCall["r"] = [this](const string& src, GroupMessageEvent& e)
	{
		e.group.sendMessage(e.sender.nickOrNameCard(), "抽取的随机数为:\n", Shitful::procRandExpr(src));
	};

	msgCall["uploadimg"] = [this](const string& src, GroupMessageEvent& e)
	{
		printf("src:%s\n", src.c_str());
		
		for (auto& msg : e.message.vector())
		{
			if (SingleMessage::messageType[msg.type()] == "image")
			{
				Image img = msg.get<Image>();
				img.refreshInfo();
				if (img.url)
				{
					const string& imgURL = *(img.url);
					const string& name = src;
					printf("tp:%d\n", img.imageType);
					printf("name:%s\n", name.c_str());
					if (PathFileExists(("uploads/"s + name).c_str()))
					{
						e.group.sendMessage("上传失败: ", name, "已存在");
						return;
					}
					printf("%s\n", name.c_str());
					HRESULT ret = URLDownloadToFile(
						nullptr,
						imgURL.c_str(), // 在这里写上网址
						("uploads/"s + name).c_str(),
						0,
						nullptr
					);
					if (ret == S_OK)
						e.group.sendMessage("上传成功，文件名为: ", name);
					else if(ret == E_OUTOFMEMORY)
						e.group.sendMessage("上传失败: 内存不足 ");
					else
						e.group.sendMessage("上传失败: 网络异常 ");
					return;
						 
				}
				else
					e.group.sendMessage("上传失败: 图片url无效");

				return;
			}
		}

		e.group.sendMessage("未接受到图片");
	};

	msgCall["img"] = [this](const string& src, GroupMessageEvent& e)
	{
		if (!PathFileExists(("uploads\\" + src).c_str()))
		{
			e.group.sendMessage("图片不存在: ", src);
			return;
		}
		Image img = e.group.uploadImg(cwd + "uploads\\" + src);
		e.group.sendMessage(img);
	};
}

void Main::loadCards()
{
	ifstream cardsList("cards.json"), readJson;
	if (!cardsList.good())
	{
		Logger::logger.warning("加载牌堆文件失败");
		return;
	}
	json cata;
	cardsList >> cata;
	unordered_map<string, int> cache;
	for (auto& it : cata)
	{
		const string& name = it["name"];
		const string& dir = it["dir"];
		const string& major = it["main"];
		if (cards.find(name) != cards.end())
		{
			Logger::logger.warning(name, "已经被", dir, "的", major, "占用");
			continue;
		}
		unordered_map<string, int>::iterator it;
		if ((it = cache.find(dir)) != cache.end())
		{
			cards[name] = { it->second,major };
		}
		else
		{
			cache[dir] = cardJson.size();
			cards[name] = { cardJson.size(),major };
			readJson.open(dir);
			cardJson.emplace_back();
			readJson >> cardJson.back();
			readJson.close();
		}
		Logger::logger.info("成功加载牌堆:", name, " ", dir, " ", major);


	}
	cardsList.close();
}

string Main::drawCard(const string& handle, const json& target)
{
	static const regex removeProbability("^\\n*(::([1-9][0-9]*)::)?\\n*([^]*)$");
	smatch matches;

	const json& load = target.find(handle) == target.end() ? target["_" + handle] : target[handle];
	vector<unsigned long> weight;
	unsigned long sumWeight = 0;
	for (auto& entry : load)
	{
		string temp = entry;
		if (regex_match(temp, matches, removeProbability) && matches.str(2) != "")
			sumWeight += stoul(matches.str(2));
		else
			sumWeight += 1;

		weight.push_back(sumWeight);
	}


	size_t pick = upper_bound(weight.begin(), weight.end(), Shitful::random(sumWeight)) - weight.begin();
	string ret = load[pick];
	if (regex_match(ret, matches, removeProbability))
		ret = matches.str(3);
	ret = drawRand(ret, target);
	ret = drawRecur(ret, target);
	return ret;
}

string Main::drawRecur(string src, const json& target)
{
	static const regex findRefer("\\{(%_)?([^\\{\\}]*)\\}");
	string ret;
	int last = 0;
	regex_iterator<string::iterator> it(src.begin(), src.end(), findRefer), end;
	while (it != end)
	{
		ret += src.substr(last, it->position() - last);
		ret += drawCard(it->str(2), target);
		last = it->position() + it->str().length();
		++it;
	}
	ret += src.substr(last);
	return ret;
}

string Main::drawRand(string src, const json& target)
{
	static const regex findRand("\\[([^\\[\\]]*)\\]");
	string ret;
	int last = 0;
	regex_iterator<string::iterator> it(src.begin(), src.end(), findRand), end;
	while (it != end)
	{
		ret += src.substr(last, it->position() - last);
		ret += Shitful::procRandExpr(it->str(1));
		last = it->position() + it->str().length();
		++it;
	}
	ret += src.substr(last);
	return ret;
}