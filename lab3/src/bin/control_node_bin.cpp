#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "../control_node.h"

namespace pt = boost::property_tree;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: control_node_bin <path to config>\n";
        return 1;
    }

    // 1. Read neighbours list from config

    pt::ptree ptree;
    pt::read_json(argv[1], ptree);

    // 2. Create control node

    std::vector<NodeIndex> neighbours;

    for (const pt::ptree::value_type &neighbour : ptree.get_child("workers")) {
        neighbours.push_back(neighbour.first);
    }

    ControlNode dr(neighbours);

    return 0;
}
