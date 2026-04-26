#pragma once

#include <array>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace eicas {

constexpr double kRatedRpm = 40000.0;
constexpr double kTickSeconds = 0.005;

enum class EnginePhase {
    Off,
    StartingLinear,
    StartingLog,
    Steady,
    Stopping
};

enum class ValueState {
    Normal,
    Caution,
    Warning,
    Invalid
};

enum class AlertColor {
    White,
    Amber,
    Red
};

enum class FaultCode {
    N1S1Fail,
    N1S2Fail,
    EGTS1Fail,
    EGTS2Fail,
    N1SFail,
    EGTSFail,
    LowFuel,
    FuelSFail,
    OverSpd1,
    OverSpd2,
    OverFF,
    OverTemp1,
    OverTemp2,
    OverTemp3,
    OverTemp4
};

struct SensorPair {
    std::optional<double> ch1;
    std::optional<double> ch2;
};

struct AlertEvent {
    std::string code;
    std::string text;
    AlertColor color = AlertColor::White;
};

struct EngineDisplayValue {
    std::optional<double> n1Percent;
    std::optional<double> egt;
    ValueState n1State = ValueState::Normal;
    ValueState egtState = ValueState::Normal;
};

struct Snapshot {
    double simTimeSec = 0.0;
    EnginePhase phase = EnginePhase::Off;
    bool startLight = false;
    bool runLight = false;

    std::array<double, 2> rpm{};
    std::array<double, 2> n1Percent{};
    std::array<double, 2> egt{};

    std::array<SensorPair, 2> n1Sensors{};
    std::array<SensorPair, 2> egtSensors{};
    std::optional<double> fuelQtySensor;
    double fuelFlow = 0.0;

    std::array<EngineDisplayValue, 2> display{};
    ValueState fuelQtyState = ValueState::Normal;
    ValueState fuelFlowState = ValueState::Normal;

    std::vector<AlertEvent> activeAlerts;
    std::vector<AlertEvent> newAlertsForLog;
};

class EngineCore {
public:
    EngineCore();

    void reset();
    void requestStart();
    void requestStop();
    void increaseThrust();
    void decreaseThrust();

    void setFault(FaultCode code, bool enabled);
    bool faultEnabled(FaultCode code) const;

    Snapshot tick(double dtSeconds = kTickSeconds);

private:
    struct BaseState {
        std::array<double, 2> rpm{0.0, 0.0};
        std::array<double, 2> egt{20.0, 20.0};
        double fuelQty = 20000.0;
        double fuelFlow = 0.0;
    };

    struct SensorState {
        std::array<SensorPair, 2> n1Sensors{};
        std::array<SensorPair, 2> egtSensors{};
        std::optional<double> fuelQty;
        double fuelFlow = 0.0;
    };

    void simulateBase(double dtSeconds);
    void enterStopping();
    void applyInjectedFaults();
    SensorState buildSensorState() const;
    void evaluateAlerts(Snapshot& snapshot);

    static double clamp(double value, double low, double high);
    static std::optional<double> averagePair(const SensorPair& pair);
    static bool isCriticalAlert(const std::string& code);
    static ValueState maxState(ValueState a, ValueState b);
    void addAlert(std::vector<AlertEvent>& out, const std::string& code, const std::string& text, AlertColor color) const;

    EnginePhase m_phase = EnginePhase::Off;
    BaseState m_state{};

    bool m_startRequested = false;
    bool m_stopRequested = false;

    int m_thrustStep = 0;
    double m_simTimeSec = 0.0;
    double m_startElapsedSec = 0.0;
    double m_stoppingElapsedSec = 0.0;

    std::array<double, 2> m_stoppingStartRpm{0.0, 0.0};
    std::array<double, 2> m_stoppingStartEgt{20.0, 20.0};

    std::unordered_map<FaultCode, bool> m_faults;
    std::unordered_map<std::string, double> m_lastLogTimeSec;
};

} 
