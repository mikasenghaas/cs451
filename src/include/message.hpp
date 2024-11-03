#pragma once

#include <cstdint>
#include <cstring>

struct Message
{
    uint8_t *payload;
    size_t payload_size;
    size_t send_id;
    size_t recv_id;

    // Returns the size of the serialized message
    size_t serialized_size() const
    {
        return sizeof(payload_size) + payload_size + sizeof(send_id) + sizeof(recv_id);
    }

    // Serializes the message into the given buffer
    void serialize(char *buffer) const
    {
        size_t offset = 0;
        memcpy(buffer + offset, &payload_size, sizeof(payload_size));
        offset += sizeof(payload_size);
        memcpy(buffer + offset, payload, payload_size);
        offset += payload_size;
        memcpy(buffer + offset, &send_id, sizeof(send_id));
        offset += sizeof(send_id);
        memcpy(buffer + offset, &recv_id, sizeof(recv_id));
    }

    // Deserializes the message from the given buffer
    void deserialize(const char *buffer)
    {
        size_t offset = 0;
        memcpy(&payload_size, buffer + offset, sizeof(payload_size));
        offset += sizeof(payload_size);
        payload = new uint8_t[payload_size];
        memcpy(payload, buffer + offset, payload_size);
        offset += payload_size;
        memcpy(&send_id, buffer + offset, sizeof(send_id));
        offset += sizeof(send_id);
        memcpy(&recv_id, buffer + offset, sizeof(recv_id));
    }
};