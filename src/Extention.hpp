#pragma once
#include <vector>
#include <map>
#include <set>
#include <algorithm>

template<class T>
        std::ostream& operator<<(std::ostream &os, const std::vector<T>& d){
    for (size_t i = 0; i < d.size(); ++i) {
        os << d[i] << " ";
    }
    return (os);
}
template<class T1, class T2>
        std::ostream& operator<<(std::ostream &os, const std::pair<T1,T2>& d){
    return (os << d.first << " " << d.second);
}
template<class T1, class T2>
        std::ostream& operator<<(std::ostream &os, const std::map<T1,T2>& d){
    for (typename std::map<T1,T2>::const_iterator it = d.begin();it != d.end() ; it++){
        os << *it;
    }
    return (os);
}
template<class T1>
        std::ostream& operator<<(std::ostream &os, const std::set<T1>& d){
    for (typename std::set<T1>::const_iterator it = d.begin();it != d.end() ; it++){
        os << *it;
    }
    return (os);
}
template <typename RandomIt>
std::pair<RandomIt, RandomIt>
FindStartsWith(RandomIt range_begin, RandomIt range_end,
               const std::string& prefix)
               {
    return std::equal_range(range_begin,range_end,prefix,
                            [&prefix](const std::string& a,const std::string& b)
                            {
        return a.compare(0,prefix.length(),b.substr(0,prefix.length())) < 0;
                            });
               }