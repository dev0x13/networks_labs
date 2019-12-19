#pragma once

#include <unordered_set>
#include <unordered_map>
#include <string>
#include <ostream>
#include <istream>
#include <cassert>

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

/* Topology operation class. Used to distribute
 * topology changes all over the network. */
struct TopologyOperation {
    enum ChangeType {
        CONNECTION_ADD,
        CONNECTION_REMOVE,
        UNDEFINED
    };

    TopologyOperation(const NodeIndex& node1_, const NodeIndex& node2_)
            : node1(node1_), node2(node2_), cost(-1), changeType(CONNECTION_REMOVE) {}

    TopologyOperation(const NodeIndex& node1_, const NodeIndex& node2_, const Cost& cost_)
            : node1(node1_), node2(node2_), cost(cost_), changeType(CONNECTION_ADD) {}

    TopologyOperation() : node1(""), node2(""), cost(-1) {}

    bool isValid() const {
        return !node1.empty() && !node2.empty() && !(cost == -1 && changeType == CONNECTION_ADD) && changeType != UNDEFINED;
    }

    // Serializes topology operation to a stream
    void serialize(std::ostream& os) const {
        os << node1 << std::endl;
        os << node2 << std::endl;;
        os << cost << std::endl;;
    }

    // Deserializes topology operation from a stream
    static TopologyOperation deserialize(std::istream& is) {
        NodeIndex node1;
        NodeIndex node2;
        Cost cost;

        is >> node1;
        is >> node2;
        is >> cost;

        if (cost == -1) {
            return {node1, node2};
        } else {
            return {node1, node2, cost};
        }
    }

    const NodeIndex node1;
    const NodeIndex node2;
    const Cost cost;
    const ChangeType changeType{ UNDEFINED };
};

/*
 * This is a network topology structure, which utilizes graph to
 * store routers and connections. It is not efficient in any sense
 * (nor memory, nor performance), but it's OK to use it for
 * demonstration purposes.
 */
class Topology {
public:
    // Adds connection to topology. Returns true if topology was updated and false if
    // there was such connection in topology before.
    bool addConnection(const NodeIndex& n1, const NodeIndex& n2, const Cost& cost) {
        const bool res = !isConnected(n1, n2);

        graph[n1][n2] = cost;
        graph[n2][n1] = cost;

        return res;
    }

    bool isConnected(const NodeIndex& n1, const NodeIndex& n2) const {
        return graph.count(n1) != 0 && graph.at(n1).count(n2) != 0;
    }

    bool removeConnection(const NodeIndex& n1, const NodeIndex& n2) {
        assert(graph.count(n1) != 0);
        assert(graph.count(n2) != 0);

        const bool res = !isConnected(n1, n2);

        graph[n1].erase(n2);
        graph[n2].erase(n1);

        return res;
    }

    bool applyOperation(const TopologyOperation& op) {
        if (!op.isValid()) {
            return false;
        }

        switch (op.changeType) {
            case TopologyOperation::CONNECTION_ADD:
                return addConnection(op.node1, op.node2, op.cost);
            case TopologyOperation::CONNECTION_REMOVE:
                return removeConnection(op.node1, op.node2);
            default:
                return false;
        }
    }

    // Save topology graph to DOT file, which can be visualized with GraphViz
    bool saveToDot(const std::string& filePath) const;

private:
    Graph graph;
};
