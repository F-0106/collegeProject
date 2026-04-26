#pragma once

#include "EngineCore.h"

#include <fstream>
#include <string>

namespace eicas {

class Recorder {
public:
    Recorder() = default;
    ~Recorder();

    bool openNewSession(const std::string& outputDir);
    void append(const Snapshot& snapshot);
    void close();
    bool logCreated() const;

    std::string csvPath() const { return m_csvPath; }
    std::string logPath() const { return m_logPath; }

private:
    static std::string phaseToText(EnginePhase phase);
    static std::string stateToText(ValueState state);
    static std::string optToText(const std::optional<double>& value, int precision = 2);
    static std::string fixed(double value, int precision = 2);

    std::ofstream m_csv;
    std::ofstream m_log;
    std::string m_csvPath;
    std::string m_logPath;
    bool m_logHeaderWritten = false;
};

} 
