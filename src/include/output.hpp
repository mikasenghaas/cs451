#pragma once

#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>

class OutputFile
{
private:
    std::ofstream file;

public:
    OutputFile(const std::string file_name)
        : file()
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