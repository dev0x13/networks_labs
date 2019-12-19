#include <iostream>
#include <unordered_map>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "../worker_node.h"
#include "../messages.h"

namespace pt = boost::property_tree;

int main(int argc, char* argv[]) {
    if (argc < 1) {
        std::cout << "Usage: common_router_bin <path to config>\n";
        return 1;
    }

    // 1. Parse and process config

    pt::ptree ptree;
    pt::read_json(argv[1], ptree);

    // 2. Create worker node

    std::unordered_map<NodeIndex, Cost> neighbours{};

    for (const pt::ptree::value_type &neighbour : ptree.get_child("neighbours")) {
        neighbours[neighbour.first] = neighbour.second.get_value<Cost>();
    }

    const std::string routerID = ptree.get_child("id").get_value<NodeIndex>();

    const pt::ptree &coord = ptree.get_child("coord");

    WorkerNode wn(
        routerID,
        neighbours,
        {coord.get_child("x").get_value<float>(), coord.get_child("y").get_value<float>()}
    );

    return 0;
}
