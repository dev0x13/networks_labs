#include <iostream>

#include <boost/interprocess/ipc/message_queue.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: clean_channels_bin <path to config>\n";
        return 1;
    }

    // 1. Read nodes ID list from config (assumed that DR config is provided)

    pt::ptree ptree;
    pt::read_json(argv[1], ptree);

    // 2. Iterate through all possible channels names

    for (const pt::ptree::value_type &neighbour : ptree.get_child("nodes")) {
        for (const pt::ptree::value_type &neighbour1 : ptree.get_child("nodes")) {
            boost::interprocess::message_queue::remove((neighbour.first + "_" + neighbour1.first + "_forward").c_str());
            boost::interprocess::message_queue::remove((neighbour.first + "_" + neighbour1.first + "_backward").c_str());
            boost::interprocess::message_queue::remove((neighbour1.first + "_" + neighbour.first + "_forward").c_str());
            boost::interprocess::message_queue::remove((neighbour1.first + "_" + neighbour.first + "_backward").c_str());
        }
    }

    return 0;
}