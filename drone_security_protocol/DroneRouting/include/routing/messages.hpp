#ifndef MESSAGES_HPP
#define MESSAGES_HPP
#include <string>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <netdb.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <vector>
#include <algorithm>

using json = nlohmann::json;
using std::cout;
using std::endl;
using std::string;

enum MESSAGE_TYPE {
    ROUTE_REQUEST = 0,
    ROUTE_REPLY, 
    ROUTE_ERROR,
    DATA,
    CERTIFICATE_VALIDATION,
    LEAVE_NOTIFICATION,
    INIT_ROUTE_DISCOVERY, // Everything below here is not apart of the actual protocol
    VERIFY_ROUTE,
    HELLO, // Broadcast Msg
    INIT_AUTO_DISCOVERY,
    EXIT
};

struct MESSAGE {
    MESSAGE_TYPE type;
    virtual string serialize() const = 0;
    virtual void deserialize(json& j) = 0;
    virtual ~MESSAGE() = default;
};

struct GCS_MESSAGE : public MESSAGE { // Repurposed to request data to be sent from current node via IPC terminal to other nodes
    std::string destAddr;

    GCS_MESSAGE() {
        this->type = DATA;
        this->destAddr = "NILL";
    }

    GCS_MESSAGE(std::string destAddr, std::string msg) {
        this->type = DATA;
        this->destAddr = destAddr;
    }

    std::string serialize() const override {
        json j = json{
            {"type", this->type},
            {"destAddr", this->destAddr},
        };
        return j.dump();
    }

    void deserialize(json& j) override {
        this->type = j["type"];
        this->destAddr = j["destAddr"];
    }
};

struct RERR : public MESSAGE {
    std::string retAddr; // Temp

    RERR() {
        this->type = ROUTE_ERROR;
    }

    void addRetAddr(const string& addr){
        this->retAddr = addr;
    }

    string serialize() const {
        json j = json::object();
        j["retAddr"] = this->retAddr;
        j["type"] = this->type;
        return j.dump();
    }

    void deserialize(json& j) override {
        this->type = j["type"];
        this->retAddr = j["retAddr"];
    }
};

struct RREQ : public MESSAGE {
    string srcAddr;
    string recvAddr; // temp field used to store next hop addr, since we are using services, cannnot directly extract last recieved ip
    string destAddr; 
    unsigned long srcSeqNum;
    unsigned long destSeqNum;
    string rootHash;
    unsigned long hopCount;
    int ttl; // Max number of hops allowed for RREQ to propagate through network

    RREQ() {
        this->type = ROUTE_REQUEST;
        this->srcSeqNum = 0;
        this->destSeqNum = 0;
        this->hopCount = 0;
        this->rootHash = "";
        this->ttl = 0;
    }

    RREQ(string srcAddr, string interAddr, string destAddr, unsigned long srcSeqNum, unsigned long destSeqNum, 
        unsigned long hopCount, int ttl, string rootHash) {
        this->type = ROUTE_REQUEST;
        this->srcAddr = srcAddr;
        this->recvAddr = interAddr;
        this->destAddr = destAddr;
        this->srcSeqNum = srcSeqNum;
        this->destSeqNum = destSeqNum;
        this->hopCount = hopCount;
        this->ttl = ttl;
        this->rootHash = rootHash;
    }

    string serialize() const override {
        json j = json{
            {"type", this->type},
            {"srcAddr", this->srcAddr},
            {"destAddr", this->destAddr},
            {"recvAddr", this->recvAddr},
            {"srcSeqNum", this->srcSeqNum},
            {"destSeqNum", this->destSeqNum},
            {"hopCount", this->hopCount},
            {"ttl", this->ttl},
            {"rootHash", this->rootHash},
        };

        return j.dump();
    }

    void deserialize(json& j) override {
        this->type = j["type"];
        this->srcAddr = j["srcAddr"];
        this->destAddr = j["destAddr"];
        this->recvAddr = j["recvAddr"];
        this->srcSeqNum = j["srcSeqNum"];
        this->destSeqNum = j["destSeqNum"];
        this->hopCount = j["hopCount"];
        this->ttl = j["ttl"];
        this->rootHash = j["rootHash"];
    }
};

struct RREP : public MESSAGE {
    string srcAddr;
    string recvAddr; // same temp field as RREQ
    string destAddr;
    unsigned long srcSeqNum;
    unsigned long destSeqNum;
    unsigned long hopCount;
    int ttl;

    RREP() {
        this->type = ROUTE_REPLY;
        this->srcSeqNum = 0;
        this->destSeqNum = 0;
        this->hopCount = 0;
        this->ttl = 0;
    }

    RREP(string srcAddr, string destAddr, unsigned long srcSeqNum, unsigned long destSeqNum, unsigned long hopCount, int ttl) {
        this->type = ROUTE_REPLY;
        this->srcAddr = srcAddr;
        this->destAddr = destAddr;
        this->srcSeqNum = srcSeqNum;
        this->destSeqNum = destSeqNum;
        this->hopCount = hopCount;
        this->ttl = ttl;
    }

    string serialize() const override {
        json j = json{
            {"type", this->type},
            {"srcAddr", this->srcAddr},
            {"destAddr", this->destAddr},
            {"recvAddr", this->recvAddr},
            {"srcSeqNum", this->srcSeqNum},
            {"destSeqNum", this->destSeqNum},
            {"hopCount", this->hopCount},
            {"ttl", this->ttl}
        };
        return j.dump();
    }

    void deserialize(json& j) override {
        this->type = j["type"];
        this->srcAddr = j["srcAddr"];
        this->destAddr = j["destAddr"];
        this->recvAddr = j["recvAddr"];
        this->srcSeqNum = j["srcSeqNum"];
        this->destSeqNum = j["destSeqNum"];
        this->hopCount = j["hopCount"];
        this->ttl = j["ttl"];
    }

};

struct INIT_MESSAGE : public MESSAGE { // Can possibly collapse this in the future with TESLA_MESSAGE
    string srcAddr;

    INIT_MESSAGE() {
        this->type = HELLO;
        srcAddr = "";
    }

    INIT_MESSAGE(string addr) {
        this->type = HELLO;
        this->srcAddr = addr;
    }

    string serialize() const override {
        json j = json{
            {"type", this->type},
            {"srcAddr", this->srcAddr}
        };
        return j.dump();
    }

    void deserialize(json& j) override {
        this->type = j["type"];
        this->srcAddr = j["srcAddr"];
    }
    
};

struct DATA_MESSAGE : public MESSAGE {
    bool isBroadcast;
    string destAddr;
    string srcAddr;
    string data;

    DATA_MESSAGE() {
        isBroadcast = false;
        this->type = DATA;
        this->destAddr = "";
        this->data = "";
    }

    DATA_MESSAGE(string destAddr, string srcAddr, string data, bool isBroadcast = false) {
        this->isBroadcast = isBroadcast;
        this->type = DATA;
        this->srcAddr = srcAddr;
        this->destAddr = destAddr;
        this->data = data;
    }

    string serialize() const override {
        json j = json{
            {"type", this->type},
            {"isBroadcast", this->isBroadcast},
            {"srcAddr", this->srcAddr},
            {"destAddr", this->destAddr},
            {"data", this->data}
        };
        return j.dump();
    }

    void deserialize(json& j) override {
        this->type = j["type"];
        this->isBroadcast = j["isBroadcast"];
        this->destAddr = j["destAddr"];
        this->srcAddr = j["srcAddr"];
        this->data = j["data"];
    }
};

#endif