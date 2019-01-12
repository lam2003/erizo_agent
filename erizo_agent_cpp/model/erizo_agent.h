#ifndef ERIZO_AGENT_H
#define ERIZO_AGENT_H

#include <string>

#include <json/json.h>

struct ErizoAgent
{
    std::string id;
    uint64_t last_update;
    int erizo_process_num;

    std::string toJSON() const
    {
        Json::Value root;
        root["id"] = id;
        root["last_update"] = last_update;
        root["erizo_process_num"] = erizo_process_num;

        Json::FastWriter writer;
        return writer.write(root);
    }

    static int fromJSON(const std::string &json, ErizoAgent &agent)
    {
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(json, root))
            return 1;

        if (!root.isMember("id") ||
            root["id"].type() != Json::stringValue ||
            !root.isMember("last_update") ||
            root["last_update"].type() != Json::uintValue)
            return 1;

        agent.id = root["id"].asString();
        agent.last_update = root["last_update"].asUInt64();
        agent.erizo_process_num = root["erizo_process_num"].asInt();
        return 0;
    }
};

#endif