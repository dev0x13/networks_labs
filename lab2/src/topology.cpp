#include <fstream>
#include <limits>
#include <cassert>

#include "topology.h"

bool Topology::saveToDot(const std::string& filePath) const {
    std::ofstream os(filePath);

    if (!os.is_open()) {
        return false;
    }

    std::unordered_set<std::pair<NodeIndex, NodeIndex>, SymmetricPairHasher, SymmetricPairComparator> printedEdges;

    os << "graph network_topology {" << std::endl;
    os << "overlap = false;" << std::endl;

    for (const auto& node: graph) {
        os << node.first << ";" << std::endl;
    }

    for (const auto& node: graph) {
        for (const auto& n: node.second) {
            if (n.first != node.first) {
                const auto tmp = std::make_pair(node.first, n.first);
                if (printedEdges.count(tmp) == 0) {
                    os << node.first << " -- " << n.first << " [label=" << n.second << "] ;" << std::endl;
                    printedEdges.insert(tmp);
                }
            }
        }
    }

    os << "}" << std::endl;

    os.close();

    return true;
}

Topology::ShortestPaths Topology::getShortestPaths(const NodeIndex& src) const {
    assert(graph.count(src) != 0);

    // Stores nodes `visited` flags
    std::unordered_map<NodeIndex, bool> nodesFlags;

    // Stores costs from src node to others
    std::unordered_map<NodeIndex, Cost> costs;

    // Stores last node in path from src node to dest node
    std::unordered_map<NodeIndex, NodeIndex> subPaths{};

    // 1. Fill `visited` flags with false and costs with inf

    for (const auto& node : graph) {
        nodesFlags[node.first] = false;

        for (const auto& node1 : graph) {
            costs[node1.first] = node1.first == src ? 0 : std::numeric_limits<Cost>::max();
        }
    }

    // 2. Find the shortest paths

    NodeIndex currentNode = src;

    for (;;) {
        // 2.1. Inspect current node neighbours

        for (const auto& neighbour : graph.at(currentNode)) {
            const Cost newCost = costs[currentNode] + neighbour.second;
            if (newCost < costs[neighbour.first]) {
                costs[neighbour.first] = newCost;
                subPaths[neighbour.first] = currentNode;
            }
        }

        // 2.2. Mark current node as visited

        nodesFlags[currentNode] = true;

        // 2.3. Select the next node to inspect

        Cost minCost = std::numeric_limits<Cost>::max();

        for (const auto& neighbour : graph.at(currentNode)) {
            if (neighbour.second < minCost && !nodesFlags[neighbour.first]) {
                minCost = neighbour.second;
                currentNode = neighbour.first;
            }
        }

        // 2.4. Break, if no node was selected (all nodes were visited)

        if (minCost == std::numeric_limits<Cost>::max()) {
            break;
        }
    }

    // 3. Build shortest paths from subpaths

    ShortestPaths shortestPaths{};

    for (const auto& subPath : subPaths) {
        shortestPaths[{src, subPath.first}] = {subPath.second, costs[subPath.first]};
    }

    return shortestPaths;
}