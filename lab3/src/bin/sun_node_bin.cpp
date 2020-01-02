#include <iostream>
#include <unordered_map>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "../sun_node.h"
#include "../messages.h"

namespace pt = boost::property_tree;

int main(int argc, char* argv[]) {
    if (argc < 1) {
        std::cout << "Usage: sun_node_bin <path to config>\n";
        return 1;
    }

    // 1. Parse and process config

    pt::ptree ptree;
    pt::read_json(argv[1], ptree);

    // 2. Create Sun node

    std::vector<NodeIndex> workers;

    for (const pt::ptree::value_type &neighbour : ptree.get_child("workers")) {
        workers.push_back(neighbour.first);
    }

    const std::string routerID = ptree.get_child("id").get_value<NodeIndex>();
    const float absMovingBound = ptree.get_child("abs_moving_bound").get_value<float>();
    const float movingSpeed = ptree.get_child("moving_speed").get_value<float>();

    const pt::ptree &coord = ptree.get_child("initial_coord");

    SunNode sn(
        routerID,
        workers,
        {coord.get_child("x").get_value<float>(), coord.get_child("y").get_value<float>()},
        movingSpeed,
        absMovingBound
    );

    return 0;
}
