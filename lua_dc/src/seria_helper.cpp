#include "seria_helper.h"

std::unordered_map<std::string, _LUA_DC::EValueType> _LUA_DC::MSeriaHelper::m_types;

namespace _LUA_DC
{

void _LUA_DC::MSeriaHelper::ParseTypeRecursive(const std::vector<std::string>& str_tb, 
    int head, int tail, int& type_end, std::vector<Type>& types)
{
    Type t;
    t.type = str_tb[head];
    assert(!MTools::IsTokenChar(t.type[0]));

    if (str_tb.size() == 1)
    {
        type_end = head;
    }
    else if (str_tb[head + 1] == "<")
    {
        std::vector<Type> childs;
        ParseTypeRecursive(str_tb, head + 2, tail, type_end, childs);
        t.params = childs;
        type_end = type_end + 1;
        if (type_end < tail - 1)
        {
            if (str_tb[type_end + 1] == ",")
            {
                ParseTypeRecursive(str_tb, type_end + 2, tail, type_end, childs);
                for (auto& key : childs)
                {
                    types.push_back(key);
                }
            }
        }
        assert((type_end <= tail) || (str_tb[type_end] == ">"));
    }
    else if (str_tb[head + 1] == ",")
    {
        std::vector<Type> childs;
        ParseTypeRecursive(str_tb, head + 2, tail, type_end, childs);
        for (auto& key : childs)
        {
            types.push_back(key);
        }
    }
    else
    {
        type_end = head;
    }

    types.push_back(t);
}

}