#include <iostream>
#include <unordered_map>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "router.h"

namespace pt = boost::property_tree;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: router <designated router flag, 0 or 1> <path to config>\n";
        return 1;
    }

    bool isDesignatedRouter = strcmp(argv[1], "1") == 0;

    // 1. Read neighbours list from config

    pt::ptree ptree;
    pt::read_json(argv[2], ptree);

    if (!isDesignatedRouter) {
        std::unordered_map<NodeIndex, Cost> neighbours;

        for (const pt::ptree::value_type &neighbour : ptree.get_child("neighbours")) {
            neighbours[neighbour.first] = neighbour.second.get_value<Cost>();
        }

        const std::string routerID = ptree.get_child("id").get_value<NodeIndex>();

        CommonRouter cr(routerID, neighbours);
    } else {
        std::vector<NodeIndex> neighbours;

        for (const pt::ptree::value_type &neighbour : ptree.get_child("neighbours")) {
            neighbours.push_back(neighbour.first);
        }

        DesignatedRouter dr(neighbours);
    }

    return 0;
}