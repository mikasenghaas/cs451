/**
 * @brief: Helper to serialize/ deserialize
 */
template<typename T>
struct Serializable {
    // Write bytes from variable into buffer at specific offset
    static void serialize(char* buffer, size_t& offset, const T& value) {
        std::memcpy(buffer + offset, &value, sizeof(T));
        offset += sizeof(T);
    }

    // Read bytes into value from buffer at offset
    static T deserialize(const char* buffer, size_t& offset) {
        T value;
        std::memcpy(&value, buffer + offset, sizeof(T));
        offset += sizeof(T);
        return value;
    }
};