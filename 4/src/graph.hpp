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
#include <algorithm>
#include <queue>
#include <stack>

namespace hust_xxxx {
    template <typename data_t>
    class [[deprecated, "junk class"]] basic_graph : public GCObject {
    public:
        using weight_t = uint32_t;
        using edge_t = std::pair<node_t *, weight_t>;
        struct node_t : public GCObject {
            node_t() = default;
            explicit node_t(const data_t &dat) : dat(dat) {}
            data_t dat;
            std::vector<edge_t> neighbors;
        };

        basic_graph() = default;
        virtual ~basic_graph() = default;

    protected:
        template <typename _data_t>
        static _data_t stringToDataObj(const std::string &str) {
            std::stringstream ss;
            _data_t val;
            ss << str;
            ss >> val;
            return std::move(val);
        }
        static std::string dataObjToString(const data_t &dat) {
            std::stringstream ss;
            ss << dat;
            return std::move(ss.str());
        }
        std::string toNodeLanguage(const node_t &node) {
            std::stringstream ss;
            ss << node.dat << '`' << std::hex << (uint64_t)(&node) << std::dec;
            return std::move(ss.str());
        }
        virtual std::string toEdgeLanguage(const node_t &from, const edge_t &to) {
            std::stringstream ss;
            ss << to.second << '`' << std::hex << (uint64_t)(&from) << '`' << (uint64_t)(&to.first) << std::dec;
            return std::move(ss.str());
        }
        auto fromNodeLanguage(const std::string &lang, bool newIfInvalidAddr = false, bool assignIfNew = false, bool assignIfExist = false) {
            auto pos = lang.find('`');
            if(pos == std::string::npos)
                throw std::invalid_argument("fromNodeLanguage want a nodeLanguage with address, but got bad format.");
            data_t val = stringToDataObj(lang.substr(0, pos));
            static_assert(std::is_same<uint64_t, unsigned long long>::value, "unsigned long long isn't uint64_t");
            uint64_t addr = std::stoull(lang.substr(pos + 1), nullptr, 16);

            auto target = nodes.end();
            try {
                target = nodePointerToIter(reinterpret_cast<node_t *>(addr));
            }
            catch(std::invalid_argument &) {}

            if(target != nodes.end()) {
                if(assignIfExist)
                    target->dat = val;
                return target;
            }

            if(!newIfInvalidAddr)
                throw std::invalid_argument(rlib::format_string("Can not find node_t at {}{}", std::hex, addr));
            if(assignIfNew)
                nodes.push_back(*new node_t(val));
            else
                nodes.push_back(*new node_t());
            return nodes.end() - 1;
        }
        virtual auto fromEdgeLanguage(const std::string &lang, bool newIfInvalidAddr = false) {
            auto arg = rlib::splitString(lang, '`');
            if(arg.size() != 3)
                throw std::invalid_argument("bad edge language");
            weight_t val = stringToDataObj<weight_t>(arg[0]);
            static_assert(std::is_same<uint64_t, unsigned long long>::value, "unsigned long long isn't uint64_t");
            node_t *addrFrom = reinterpret_cast<node_t *>(std::stoull(arg[1], nullptr, 16));
            node_t *addrTo = reinterpret_cast<node_t *>(std::stoull(arg[2], nullptr, 16));

            //silly clion can't deduct this type, help him out.
            decltype(nodes.begin()) target = nodePointerToIter(addrFrom); //throws std::invalid_argument
            nodePointerToIter(addrTo); //Confirm that nodeTo do exists.

            auto pos = std::find_if(target->neighbors.begin(), target->neighbors.end(), [&](const edge_t &e){
                return (uint64_t)addrTo == (uint64_t)e.first;
            });
            if(pos != target->neighbors.end()) {
                return pos;
            }
            else {
                if(newIfInvalidAddr) {
                    target->neighbors.push_back(std::make_pair(addrTo, val));
                    return target->neighbors.end() - 1;
                }
                else
                    throw std::invalid_argument("requested edge not exist");
            }
        }
        size_t nodePointerToIndex(node_t *ptr) {
            node_t *begin = nodes.data();
            if(ptr - begin >= nodes.size() || ptr - begin < 0)
                throw std::invalid_argument("nodePointerToIter failed: not found.");
            return ptr - begin;
        }
        auto nodePointerToIter(node_t *ptr) {
            return nodes.begin() + nodePointerToIndex(ptr);
        }

    public:
        std::string findNode(const data_t &val) {
            for(auto &node : nodes) {
                if(node.dat == val)
                    return toNodeLanguage(node);
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
            auto node = fromNodeLanguage(lang);
            return node->neighbors.empty() ? "`" : toNodeLanguage(*node->neighbors.begin());
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

        using node_visiter = std::function<void(node_t &)>;
        node_visiter printer = [](node_t &nd){rlib::printf("{} ", nd.dat);};
        void dfs(node_visiter &func) {
            std::vector<bool> masks(nodes.size(), false);
            dfs_helper(func, masks, *nodes.begin());
        }
        void dfs_helper(node_visiter &func, std::vector<bool> &masks, const node_t &curr) {
            for(auto &edge : curr.neighbors) {
                node_t &next = *edge.first;
                size_t index = nodePointerToIndex(&next);
                if(masks[index])
                    return;
                else {
                    masks[index] = true;
                    func(next);
                    dfs_helper(func, masks, next);
                }
            }
        }
        void bfs(node_visiter &func) {
            std::vector<bool> masks(nodes.size(), false);
            bfs_helper(func, masks, *nodes.begin());
        }
        void bfs_helper(node_visiter &func, std::vector<bool> &masks, const std::list<node_t &> &curr) {
            std::list<node_t &> next;
            for(node_t &node : curr) {
                for(auto &edge : node.neighbors) {
                    node_t &nextNode = *edge.first;
                    size_t index = nodePointerToIndex(&nextNode);
                    if(masks[index])
                        continue;
                    else {
                        masks[index] = true;
                        next.push_back(nextNode);
                    }
                }
            }
            std::for_each(nodes.begin(), nodes.end(), func);
            if(!next.empty())
                bfs_helper(func, masks, next);
        }

        virtual void insertEdge(const std::string &lang) = 0;
        virtual void removeEdge(const std::string &lang) = 0;
    protected:
        std::vector<node_t> nodes;
    };


    template <typename data_t>
    class directed_weighted_graph : public basic_graph {
    public:
        directed_weighted_graph() = default;
        virtual ~directed_weighted_graph() = default;

        virtual void insertEdge(const std::string &lang) override {
            fromEdgeLanguage(lang, true);
        }
        virtual void removeEdge(const std::string &lang) override {
            auto nodeLang = "`"s + rlib::splitString(lang, '`')[1];
            fromNodeLanguage(lang)->neighbors.erase(fromEdgeLanguage(lang));
        }
    };

    template <typename data_t>
    class undirected_weighted_graph : public directed_weighted_graph {
    public:
        undirected_weighted_graph() = default;
        virtual ~undirected_weighted_graph() = default;

        virtual void insertEdge(const std::string &lang) override {
            auto parts = rlib::splitString(lang, '`');
            std::string reversedLang = rlib::joinString('`', std::array<std::string, 3>{parts[0], parts[2], parts[1]});
            directed_weighted_graph::insertEdge(lang);
            directed_weighted_graph::insertEdge(reversedLang);
        }
        virtual void removeEdge(const std::string &lang) override {
            auto parts = rlib::splitString(lang, '`');
            std::string reversedLang = rlib::joinString('`', std::array<std::string, 3>{parts[0], parts[2], parts[1]});
            directed_weighted_graph::removeEdge(lang);
            directed_weighted_graph::removeEdge(reversedLang);
        }
    };

    template <typename data_t>
    class directed_unweighted_graph : public directed_weighted_graph {
    public:
        directed_unweighted_graph() = default;
        virtual ~directed_unweighted_graph() = default;

        virtual void insertEdge(const std::string &lang) override {
            auto parts = rlib::splitString(lang, '`');
            parts[0] = '1';
            directed_weighted_graph::insertEdge(rlib::joinString('`', parts));
        }
    };

    template <typename data_t>
    class undirected_unweighted_graph : public undirected_weighted_graph {
    public:
        undirected_unweighted_graph() = default;
        virtual ~undirected_unweighted_graph() = default;

        virtual void insertEdge(const std::string &lang) override {
            auto parts = rlib::splitString(lang, '`');
            parts[0] = '1';
            undirected_weighted_graph::insertEdge(rlib::joinString('`', parts));
        }
    };
}





#endif