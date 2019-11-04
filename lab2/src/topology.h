#pragma once

#include <unordered_set>
#include <unordered_map>
#include <string>
#include <ostream>
#include <istream>

using NodeIndex = std::string;
using Cost = int64_t;
using Graph = std::unordered_map<NodeIndex, std::unordered_map<NodeIndex, Cost>>;

/*
 * Helper structures used for map with symmetric std::pair keys.
 */

struct SymmetricPairHasher {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ h2;
    }
};

struct SymmetricPairComparator {
    template <class T1, class T2>
    bool operator()(const std::pair<T1, T2>& p1, const std::pair<T1, T2>& p2) const {
        return (p1.first == p2.first && p1.second == p2.second) ||
               (p1.first == p2.second && p1.second == p2.first);
    }
};

/*
 * Path in graph class. It stores only total path cost and last
 * node before destination node, so it makes sense to use it only
 * together with some structure containing results of Dijkstra
 * algorithm or so.
 */

struct Path {
    NodeIndex lastNodeBeforeDest;
    Cost totalCost = -1;

    Path() = default;

    Path(const NodeIndex& lastNodeBeforeDest_, const Cost& totalCost_)
        : lastNodeBeforeDest(lastNodeBeforeDest_), totalCost(totalCost_) {}
};

/*
 * This is a network topology structure, which utilizes graph to
 * store routers and connections. It is not efficient in any sense
 * (nor memory, nor performance), but it's OK to use it for
 * demonsrtation purposes.
 */
class Topology {
public:
    using ShortestPaths =
        std::unordered_map<
            std::pair<NodeIndex, NodeIndex>,
            Path,
            SymmetricPairHasher,
            SymmetricPairComparator
        >;

    bool operator==(const Topology& other) const {
        if (other.graph.size() != graph.size()) {
            return false;
        }

        for (const auto& e: graph) {
            if (other.graph.count(e.first) == 0) {
                return false;
            }

            if (e.second.size() != other.graph.at(e.first).size()) {
                return false;
            }

            if (!std::equal(
                    e.second.begin(),
                    e.second.end(),
                    other.graph.at(e.first).begin()))
            {
                return false;
            }
        }

        return true;
    }

    // Merges two topologies with replace conflict resolution.
    void merge(const Topology& other) {
        for (const auto& node: other.graph) {
            graph[node.first] = node.second;
        }
    }

    // Adds connection to topology. Returns true if topology was updated and false if
    // there was such connection in topology before.
    bool addConnection(const NodeIndex& n1, const NodeIndex& n2, const Cost& cost) {
        if (graph.count(n1) != 0 && graph[n1].count(n2) != 0) {
            return false;
        }

        graph[n1][n2] = cost;
        graph[n2][n1] = cost;

        return true;
    }

    bool isConnected(const NodeIndex& n1, const NodeIndex& n2) const {
        return graph.count(n1) != 0 && graph.at(n1).count(n2) != 0;
    }

    void removeConnection(const NodeIndex& n1, const NodeIndex& n2) {
        assert(graph.count(n1) != 0);
        assert(graph.count(n2) != 0);

        graph[n1].erase(n2);
        graph[n2].erase(n1);
    }

    // Save topology graph to DOT file, which can be visualized with GraphViz
    bool saveToDot(const std::string& filePath) const;

    // Serializes topology graph to a stream
    void serialize(std::ostream& os) const;

    // Deserializes topology graph from a stream
    static Topology deserialize(std::istream& is);

    // Returns shortest paths from the given node to the others.
    // Utilizes Dijkstra algorithm. Again, this implementation
    // is very straightforward and is not efficient and its
    // author knows about it.
    ShortestPaths getShortestPaths(const NodeIndex& src) const;

private:
    Graph graph;
};