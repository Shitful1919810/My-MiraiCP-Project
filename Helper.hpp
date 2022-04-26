#pragma once
#include <string>
#include <vector>
#include <regex>
#include <random>
namespace Shitful
{
	inline std::random_device rd;
	unsigned long random(unsigned long upper) // [0,upper)
	{
		return rd() % upper;
	}

	long long calcSingleRand(std::string src)
	{
		using namespace std;
		static const regex splitSingle("(d?)([0-9]+)");
		long long ret = 1;
		regex_iterator<string::iterator> it(src.begin(), src.end(), splitSingle), end;
		int last = 0;
		while (it != end)
		{
			long long rst = stoll(it->str(2));
			if (it->str(1) == "d")
				rst = random(rst + 1);
			ret *= rst;

			last = it->position() + it->str().length();
			++it;
		}
		return ret;
	}

	long long calcExpr(std::string src)
	{
		using namespace std;
		if (src == "")
			return 0;

		static const regex splitMulti("\\s*\\*\\s*");
		src.push_back('*');
		long long ret = 1;
		regex_iterator<string::iterator> it(src.begin(), src.end(), splitMulti), end;
		int last = 0;
		while (it != end)
		{
			long long rst = calcSingleRand(src.substr(last, it->position() - last));
			ret *= rst;

			last = it->position() + it->str().length();
			++it;
		}
		return ret;
	}

	std::string procRandExpr(std::string src)
	{
		using namespace std;
		static const regex splitExpr("\\s*([+-])\\s*");

		src.push_back('+');
		long long ret = 0;
		regex_iterator<string::iterator> it(src.begin(), src.end(), splitExpr), end;
		int last = 0;
		bool lastOper = '+';
		while (it != end)
		{
			long long rst = calcExpr(src.substr(last, it->position() - last));
			if (lastOper)
				ret += rst;
			else
				ret -= rst;

			lastOper = (it->str(1) == "+");
			last = it->position() + it->str().length();
			++it;
		}

		return to_string(ret);
	}
}