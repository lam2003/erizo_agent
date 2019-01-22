#ifndef ERIZO_H
#define ERIZO_H

#include <string>

#include <json/json.h>

struct Erizo
{
    std::string id;
    std::string room_id;
    std::string bridge_ip;
    uint16_t bridge_port;

    // std::string toJSON() const
    // {
    //     Json::Value root;
    //     root["id"] = id;
    //     root["room_id"] = room_id;

    //     Json::FastWriter writer;
    //     return writer.write(root);
    // }

    // static int fromJSON(const std::string &json, Erizo &erizo)
    // {
    //     Json::Value root;
    //     Json::Reader reader;
    //     if (!reader.parse(json, root))
    //         return 1;

    //     if (!root.isMember("id") ||
    //         root["id"].type() != Json::stringValue ||
    //         !root.isMember("room_id") ||
    //         root["room_id"].type() != Json::stringValue)
    //         return 1;

    //     erizo.id = root["id"].asString();
    //     erizo.room_id = root["room_id"].asString();
    //     return 0;
    // }
};

#endif