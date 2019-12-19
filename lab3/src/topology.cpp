#include <fstream>

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