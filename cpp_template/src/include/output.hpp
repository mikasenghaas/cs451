#pragma once

/**
 * @brief Output
 *
 * @details Write to a file and flush the buffer when the object is destroyed.
 */
class OutputFile
{
private:
    std::ofstream file;

public:
    OutputFile(const std::string file_name)
    {
        this->file.open(file_name, std::ofstream::out);
        if (!this->file.is_open()) {
            throw std::runtime_error("Failed to open output file: " + file_name);
        }
    }

    ~OutputFile()
    {
        this->flush();
        this->close();
    }

    void write(const std::string &output)
    {
        this->file << output;
    }

    void flush()
    {
        this->file.flush();
    }

    void close()
    {
        this->file.close();
    }
};