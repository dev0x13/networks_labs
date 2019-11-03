#pragma once

#include "one_way_transducer.h"

#include <string>
#include <thread>
#include <chrono>

using NodeIndex = std::string;
using Cost = int64_t;
using Graph = std::unordered_map<NodeIndex, std::unordered_map<NodeIndex, Cost>>;

struct PairHasher {
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ h2;
    }
};

struct Path {
    NodeIndex lastNodeBeforeDest;
    Cost totalCost;

    Path() = default;

    Path(const NodeIndex& lastNodeBeforeDest_, const Cost& totalCost_)
        : lastNodeBeforeDest(lastNodeBeforeDest_), totalCost(totalCost_) {}
};

// This structure uses twice more memory, than it could,
// but let it be for code simplicity.
class Topology {
public:
    using ShortestPaths = std::unordered_map<std::pair<NodeIndex, NodeIndex>, Path, PairHasher>;

    void merge(const Topology& other) {
        for (const auto& node: other.graph) {
            graph[node.first] = node.second;
        }
    }

    void addEdge(const NodeIndex& n1, const NodeIndex& n2, const Cost& cost) {
        graph[n1][n2] = cost;
        graph[n2][n1] = cost;
    }

    bool operator!=(const Topology& other) const {
        return graph.size() != other.graph.size()
               || std::equal(graph.begin(), graph.end(),
                             other.graph.begin());
    }

    Cost getCost(const NodeIndex& n1, const NodeIndex& n2) {
        if (graph.count(n1) == 0) {
            throw std::runtime_error("No such node in graph");
        }

        return graph[n1].count(n2) == 0 ? std::numeric_limits<Cost>::max() : graph[n1][n2];
    }

    void serialize(std::ostream& os) const {
        os << graph.size() << std::endl;

        for (const auto& node : graph) {
            os << node.first << std::endl;
            os << node.second.size() << std::endl;

            for (const auto& cost : node.second) {
                os << cost.first << std::endl;
                os << cost.second << std::endl;
            }
        }
    }

    static Topology deserialize(std::istream& is) {
        Topology topology;

        size_t numNodes = 0;
        is >> numNodes;

        for (size_t i = 0; i < numNodes; ++i) {
            std::string nodeIndex;
            size_t nodeNumEdges;

            is >> nodeIndex;
            is >> nodeNumEdges;

            for (size_t j = 0; j < nodeNumEdges; ++j) {
                std::string secondNodeIndex;
                Cost edgeCost;

                is >> secondNodeIndex;
                is >> edgeCost;

                topology.graph[nodeIndex][secondNodeIndex] = edgeCost;
            }
        }

        return topology;
    }

    ShortestPaths getShortestPaths(NodeIndex src) {
        std::unordered_map<NodeIndex, bool> nodesFlags;
        std::unordered_map<NodeIndex, Cost> costs;
        std::unordered_map<NodeIndex, NodeIndex> subPaths;

        for (const auto& node : graph) {
            nodesFlags[node.first] = false;

            for (const auto& node1 : graph) {
                costs[node1.first] = node1.first == src ? 0 : std::numeric_limits<Cost>::max();
            }
        }

        NodeIndex& currentNode = src;

        for (;;) {
            for (const auto& neighbour : graph[currentNode]) {
                const Cost newCost = costs[currentNode] + neighbour.second;
                if (newCost < costs[neighbour.first]) {
                    costs[neighbour.first] = newCost;
                    subPaths[neighbour.first] = currentNode;
                }
            }

            nodesFlags[currentNode] = true;

            Cost minCost = std::numeric_limits<Cost>::max();

            for (const auto& neighbour : graph[currentNode]) {
                if (neighbour.second < minCost) {
                    minCost = neighbour.second;
                    currentNode = neighbour.first;
                }
            }

            if (minCost == std::numeric_limits<Cost>::max()) {
                break;
            }
        }

        ShortestPaths shortestPaths;

        for (const auto& subPath : subPaths) {
            shortestPaths[{src, subPath.first}] = {subPath.second, costs[subPath.first]};
        }

        return shortestPaths;
    }

private:
    Graph graph;
};

class CommonRouter {
public:
    CommonRouter(const std::string& routerID, const std::unordered_map<NodeIndex, Cost>& neighbours) :
        id(routerID), channelDR("DR_" + id + "_send")
    {
        // For simplicity I implemented only static OSPF, without dynamic
        // topology rebuilt thus without any communication between
        // common routers
        for (const auto& n : neighbours) {
            knownTopology.addEdge(id, n.first, n.second);
        }

        sendLsaToDr();

        receiveLsaFromDrJob();
    }

    void sendLsaToDr() {
        OneWayTransducer<TransducerMode::SENDING> ch("DR_" + id + "_receive");

        std::stringstream ss;

        knownTopology.serialize(ss);

        ch.send(ss.str());
    }

    void receiveLsaFromDrJob() {
        std::string serializedLsa;

        for (;;) {
            if (channelDR.receive(serializedLsa)) {
                std::stringstream ss;
                ss << serializedLsa;
                const Topology& newTopology = Topology::deserialize(ss);

                if (newTopology != knownTopology) {
                    knownTopology.merge(newTopology);
                    shortestPaths = knownTopology.getShortestPaths(id);
                    std::cout << "Shortest paths found" << std::endl;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(shortestPathsRebuildDelayMs));
        }
    }
private:
    const size_t shortestPathsRebuildDelayMs = 100;
    const NodeIndex id;
    Topology knownTopology;
    Topology::ShortestPaths shortestPaths;

    OneWayTransducer<TransducerMode::RECEIVING> channelDR;
};

class DesignatedRouter {
public:
    DesignatedRouter(const std::vector<NodeIndex>& neighbours) {
        for (const auto& n : neighbours) {
            auto receiveCh = new OneWayTransducer<TransducerMode::RECEIVING>("DR_" + n + "_receive");
            lsaReceive.push_back(receiveCh);

            auto sendCh = new OneWayTransducer<TransducerMode::SENDING>("DR_" + n + "_send");
            lsaBroadcast.push_back(sendCh);
        }

        lsaReceiveJob();
    }

    ~DesignatedRouter() {
        for (auto &ch : lsaReceive) {
            delete ch;
        }

        for (auto &ch : lsaBroadcast) {
            delete ch;
        }
    }

    void lsaReceiveJob() {
        std::string serializedLsa;

        while (lastTopologyRebuiltCount < completeTopologyCriterion) {
            for (auto& ch : lsaReceive) {
                if (ch->receive(serializedLsa)) {
                    std::cout << "Received LSA from " << ch->getMQName() << std::endl;

                    std::stringstream ss;
                    ss << serializedLsa;
                    const Topology &newTopology = Topology::deserialize(ss);

                    if (newTopology != knownTopology) {
                        knownTopology.merge(newTopology);
                        broadcastLsa();
                        lastTopologyRebuiltCount = 0;
                    } else {
                        lastTopologyRebuiltCount++;
                    }
                }
            }
        }
    }

    void broadcastLsa() {
        std::stringstream ss;
        knownTopology.serialize(ss);
        const std::string serializedLsa = ss.str();

        for (auto& ch : lsaBroadcast) {
            ch->send(serializedLsa);
        }
    }

private:
    static const constexpr size_t completeTopologyCriterion = 100;
    size_t lastTopologyRebuiltCount = 0;
    Topology knownTopology;
    std::vector<OneWayTransducer<TransducerMode::RECEIVING>*> lsaReceive;
    std::vector<OneWayTransducer<TransducerMode::SENDING>*> lsaBroadcast;
};