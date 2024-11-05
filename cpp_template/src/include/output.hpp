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