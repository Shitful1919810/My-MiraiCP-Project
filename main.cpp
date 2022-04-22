#include "main.h"
#include "Helper.hpp"

using namespace std;
using namespace nlohmann;

// 绑定当前插件实例
void MiraiCP::enrollPlugin() {
  MiraiCP::enrollPlugin0(new Main());
}

void Main::OnGroupMessage(GroupMessageEvent e)
{
	static const regex rules[] = { regex("!(draw)\\s*(.*)"),regex("!(r)\\s*(.*)") };
	string msg = e.message.toMiraiCode();
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
			sumWeight += 100;

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