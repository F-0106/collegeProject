#include "Recorder.h"

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace eicas {

Recorder::~Recorder() {
    close();
}

bool Recorder::openNewSession(const std::string& outputDir) {
    close();

    std::ostringstream mkdirCmd;
#ifdef _WIN32
    mkdirCmd << "if not exist \"" << outputDir << "\" mkdir \"" << outputDir << "\"";
#else
    mkdirCmd << "mkdir -p \"" << outputDir << "\"";
#endif
    if (std::system(mkdirCmd.str().c_str()) != 0) {
        return false;
    }

    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream fileTag;
    fileTag << std::put_time(&tm, "%Y%m%d_%H%M%S");

    m_csvPath = outputDir + "/engine_data_" + fileTag.str() + ".csv";
    m_logPath = outputDir + "/engine_alert_" + fileTag.str() + ".log";
    m_logHeaderWritten = false;

    m_csv.open(m_csvPath.c_str(), std::ios::out | std::ios::trunc);
    if (!m_csv.is_open()) {
        close();
        return false;
    }

    m_csv << "time_sec,phase,rpm_l,rpm_r,n1_pct_l,n1_pct_r,egt_l,egt_r,fuel_qty,fuel_flow,"
             "n1_l_state,n1_r_state,egt_l_state,egt_r_state,fuel_qty_state,fuel_flow_state,"
             "start_light,run_light\n";
    return true;
}

void Recorder::append(const Snapshot& snapshot) {
    if (m_csv.is_open()) {
        m_csv << fixed(snapshot.simTimeSec, 3) << ","
              << phaseToText(snapshot.phase) << ","
              << fixed(snapshot.rpm[0], 2) << ","
              << fixed(snapshot.rpm[1], 2) << ","
              << optToText(snapshot.display[0].n1Percent, 2) << ","
              << optToText(snapshot.display[1].n1Percent, 2) << ","
              << optToText(snapshot.display[0].egt, 2) << ","
              << optToText(snapshot.display[1].egt, 2) << ","
              << optToText(snapshot.fuelQtySensor, 2) << ","
              << fixed(snapshot.fuelFlow, 2) << ","
              << stateToText(snapshot.display[0].n1State) << ","
              << stateToText(snapshot.display[1].n1State) << ","
              << stateToText(snapshot.display[0].egtState) << ","
              << stateToText(snapshot.display[1].egtState) << ","
              << stateToText(snapshot.fuelQtyState) << ","
              << stateToText(snapshot.fuelFlowState) << ","
              << (snapshot.startLight ? "1" : "0") << ","
              << (snapshot.runLight ? "1" : "0") << "\n";
        m_csv.flush();
    }

    if (!snapshot.newAlertsForLog.empty()) {
        if (!m_log.is_open()) {
            m_log.open(m_logPath.c_str(), std::ios::out | std::ios::trunc);
        }
        if (m_log.is_open() && !m_logHeaderWritten) {
            m_log << "time_sec,level,code,text\n";
            m_logHeaderWritten = true;
        }

        if (m_log.is_open()) {
            for (const AlertEvent& alert : snapshot.newAlertsForLog) {
                std::string level = "WHITE";
                if (alert.color == AlertColor::Amber) {
                    level = "AMBER";
                } else if (alert.color == AlertColor::Red) {
                    level = "RED";
                }
                m_log << fixed(snapshot.simTimeSec, 3) << ","
                      << level << ","
                      << alert.code << ","
                      << alert.text << "\n";
            }
            m_log.flush();
        }
    }
}

void Recorder::close() {
    if (m_csv.is_open()) {
        m_csv.close();
    }
    if (m_log.is_open()) {
        m_log.close();
    }
    m_logHeaderWritten = false;
}

bool Recorder::logCreated() const {
    return m_logHeaderWritten;
}

std::string Recorder::phaseToText(EnginePhase phase) {
    switch (phase) {
    case EnginePhase::Off:
        return "OFF";
    case EnginePhase::StartingLinear:
        return "START_LINEAR";
    case EnginePhase::StartingLog:
        return "START_LOG";
    case EnginePhase::Steady:
        return "STEADY";
    case EnginePhase::Stopping:
        return "STOPPING";
    }
    return "UNKNOWN";
}

std::string Recorder::stateToText(ValueState state) {
    switch (state) {
    case ValueState::Normal:
        return "NORMAL";
    case ValueState::Caution:
        return "CAUTION";
    case ValueState::Warning:
        return "WARNING";
    case ValueState::Invalid:
        return "INVALID";
    }
    return "UNKNOWN";
}

std::string Recorder::optToText(const std::optional<double>& value, int precision) {
    if (!value.has_value()) {
        return "--";
    }
    return fixed(value.value(), precision);
}

std::string Recorder::fixed(double value, int precision) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}

}  
