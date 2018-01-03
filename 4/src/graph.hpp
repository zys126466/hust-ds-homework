#ifndef HUST_XXXX_GRAPH_HPP_
#define HUST_XXXX_GRAPH_HPP_

#include <gc/gc.h>
#include <rlib/string/string.hpp>

#include <list>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <cstdint>
#include <stdexcept>
#include <exception>

namespace hust_xxxx {
    template <typename data_t>
    class [[deprecated, "junk class"]] graph {
    public:
        struct node : public GCObject {
            node() = default;
            explicit node(const data_t &dat) : dat(dat) {}
            data_t dat;
            std::vector<node *> neighbors;
        };

        graph() = default;
        static data_t stringToDataObj(const std::string &str) {
            std::stringstream ss;
            data_t val;
            ss << str;
            ss >> val;
            return std::move(val);
        }
        static std::string dataObjToString(const data_t &dat) {
            std::stringstream ss;
            ss << dat;
            return std::move(ss.str());
        }
        std::string toNodeLanguage(const node &nd) {
            std::stringstream ss;
            ss << nd.dat << '`' << std::hex << (uint64_t)(&nd) << std::dec;
            return std::move(ss.str());
        }
        node *fromNodeLanguage(const std::string &lang, bool newIfInvalidAddr = false, bool assignIfNew = false, bool assignIfExist = false) {
            auto pos = lang.find('`');
            if(pos == std::string::npos)
                throw std::invalid_argument("fromNodeLanguage want a nodeLanguage with address, but got bad format.");
            std::string dat = lang.substr(0, pos);
            static_assert(std::is_same<uint64_t, unsigned long long>::value, "unsigned long long isn't uint64_t");
            uint64_t addr = std::stoull(lang.substr(pos + 1), nullptr, 16);

            node *target = nullptr;
            for(node &nd : nodes) {
                if((uint64_t)(&nd) == addr)
                    target = &nd;
            }

            if((assignIfNew || assignIfExist) && dat.empty())
                throw std::invalid_argument(rlib::format_string("fromNodeLanguage can not find data string from nodeLang `{}`, which is required.", lang));
            if(target && assignIfExist)
                target->dat = stringToDataObj(dat);
            if(target)
                return target;
            if(!newIfInvalidAddr)
                throw std::invalid_argument(rlib::format_string("Can not find node at {}{}", std::hex, addr));
            if(!assignIfNew)
                return new node();
            return new node(stringToDataObj(dat));
        }
        auto nodePointerToIter(node *ptr) {
            for(auto iter = nodes.begin(); iter != nodes.end(); ++iter) {
                if(&*iter == ptr)
                    return iter;
            }
            throw std::invalid_argument("nodePointerToIter failed: not found.");
        }


        std::string findNode(const data_t &val) {
            for(auto &nd : nodes) {
                if(nd.dat == val)
                    return toNodeLanguage(nd);
            }
            return "`";
        }
        std::string getNodeValue(const std::string &lang) {
            return toNodeLanguage(*fromNodeLanguage(lang));
        }
        void setNodeValue(const std::string &lang) {
            fromNodeLanguage(lang, true, true, true);
        }
        std::string findFirstNearNode(const std::string &lang) {
            auto nd = fromNodeLanguage(lang);
            return nd->neighbors.empty() ? "`" : toNodeLanguage(*nd->neighbors.begin());
        }
        std::string findNextNearNode(const std::string &centerNd, const std::string &posNd) {
            auto center = fromNodeLanguage(centerNd);
            auto pos = fromNodeLanguage(posNd);
            for(auto iter = center->neighbors.begin(); iter != center->neighbors.end(); ++iter) {
                if (&*iter == pos) {
                    ++iter;
                    if(iter == center->neighbors.end())
                        return "`";
                    else
                        return toNodeLanguage(*iter);
                }
            }
            return "`";
        }
        void removeNode(const std::string &lang) {
            nodes.erase(nodePointerToIter(fromNodeLanguage(lang)));
        }


    private:
        std::list<node> nodes;
    };
}

#endif