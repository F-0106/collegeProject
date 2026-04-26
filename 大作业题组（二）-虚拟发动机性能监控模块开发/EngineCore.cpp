#include "EngineCore.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

namespace eicas {

namespace {

constexpr double kRoomTemp = 20.0;
constexpr double kMinTemp = -5.0;
constexpr double kMaxTemp = 1200.0;
constexpr double kMaxFuelFlow = 50.0;
constexpr double kMinFuel = 0.0;
constexpr double kMaxFuel = 20000.0;
constexpr double kLinearStartSec = 2.0;
constexpr double kSteadyN1 = 95.0;
constexpr double kSteadyRpm = kRatedRpm * kSteadyN1 / 100.0;
constexpr double kSteadyEgt = 830.0;
constexpr double kSteadyFlow = 24.0;
constexpr double kStopDurationSec = 10.0;
constexpr double kStoppingBase = 0.01;

double rpmToN1(double rpm) {
    return (rpm / kRatedRpm) * 100.0;
}

}  // namespace

EngineCore::EngineCore() {
    reset();
}

void EngineCore::reset() {
    m_phase = EnginePhase::Off;
    m_state = BaseState{};
    m_startRequested = false;
    m_stopRequested = false;
    m_thrustStep = 0;
    m_simTimeSec = 0.0;
    m_startElapsedSec = 0.0;
    m_stoppingElapsedSec = 0.0;
    m_stoppingStartRpm = {0.0, 0.0};
    m_stoppingStartEgt = {kRoomTemp, kRoomTemp};
    m_faults.clear();
    m_lastLogTimeSec.clear();
}

void EngineCore::requestStart() {
    m_startRequested = true;
}

void EngineCore::requestStop() {
    m_stopRequested = true;
}

void EngineCore::increaseThrust() {
    m_thrustStep = std::min(m_thrustStep + 1, 10);
}

void EngineCore::decreaseThrust() {
    m_thrustStep = std::max(m_thrustStep - 1, -10);
}

void EngineCore::setFault(FaultCode code, bool enabled) {
    m_faults[code] = enabled;
}

bool EngineCore::faultEnabled(FaultCode code) const {
    const auto it = m_faults.find(code);
    return it != m_faults.end() && it->second;
}

Snapshot EngineCore::tick(double dtSeconds) {
    dtSeconds = std::max(dtSeconds, 0.0001);

    if (m_stopRequested) {
        enterStopping();
        m_stopRequested = false;
    } else if (m_startRequested) {
        if (m_phase == EnginePhase::Off) {
            m_phase = EnginePhase::StartingLinear;
            m_startElapsedSec = 0.0;
        }
        m_startRequested = false;
    }

    simulateBase(dtSeconds);
    applyInjectedFaults();

    Snapshot snapshot{};
    snapshot.simTimeSec = m_simTimeSec;
    snapshot.phase = m_phase;
    snapshot.rpm = m_state.rpm;
    snapshot.egt = m_state.egt;
    snapshot.n1Percent = {rpmToN1(m_state.rpm[0]), rpmToN1(m_state.rpm[1])};

    SensorState sensors = buildSensorState();
    snapshot.n1Sensors = sensors.n1Sensors;
    snapshot.egtSensors = sensors.egtSensors;
    snapshot.fuelQtySensor = sensors.fuelQty;
    snapshot.fuelFlow = sensors.fuelFlow;

    for (size_t i = 0; i < 2; ++i) {
        snapshot.display[i].n1Percent = averagePair(snapshot.n1Sensors[i]);
        snapshot.display[i].egt = averagePair(snapshot.egtSensors[i]);

        if (!snapshot.display[i].n1Percent.has_value()) {
            snapshot.display[i].n1State = ValueState::Invalid;
        }
        if (!snapshot.display[i].egt.has_value()) {
            snapshot.display[i].egtState = ValueState::Invalid;
        }
    }

    snapshot.startLight = (m_phase == EnginePhase::StartingLinear || m_phase == EnginePhase::StartingLog);
    snapshot.runLight = (m_phase == EnginePhase::Steady && snapshot.n1Percent[0] >= 95.0 && snapshot.n1Percent[1] >= 95.0);

    evaluateAlerts(snapshot);

    m_simTimeSec += dtSeconds;
    return snapshot;
}

void EngineCore::simulateBase(double dtSeconds) {
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<double> pctNoise(-0.03, 0.03);

    switch (m_phase) {
    case EnginePhase::Off:
        m_state.rpm = {0.0, 0.0};
        m_state.egt = {kRoomTemp, kRoomTemp};
        m_state.fuelFlow = 0.0;
        break;
    case EnginePhase::StartingLinear: {
        m_startElapsedSec += dtSeconds;
        for (size_t i = 0; i < 2; ++i) {
            m_state.rpm[i] += 104.0 * dtSeconds;
            m_state.egt[i] += 15.0 * dtSeconds;
        }
        m_state.fuelFlow += 5.0 * dtSeconds;
        if (m_startElapsedSec >= kLinearStartSec) {
            m_phase = EnginePhase::StartingLog;
        }
        break;
    }
    case EnginePhase::StartingLog: {
        m_startElapsedSec += dtSeconds;
        const double x = std::max(1.001, m_startElapsedSec - 1.0);
        const double targetFlow = 42.0 * std::log10(x) + 10.0 + static_cast<double>(m_thrustStep);
        const double targetRpm = 23000.0 * std::log10(x) + 20000.0;
        const double targetEgt = 900.0 * std::log10(x) + kRoomTemp;

        for (size_t i = 0; i < 2; ++i) {
            m_state.rpm[i] += (targetRpm - m_state.rpm[i]) * 0.10;
            m_state.egt[i] += (targetEgt - m_state.egt[i]) * 0.10;
        }
        m_state.fuelFlow += (targetFlow - m_state.fuelFlow) * 0.12;

        if (m_state.rpm[0] >= 0.95 * kRatedRpm && m_state.rpm[1] >= 0.95 * kRatedRpm) {
            m_phase = EnginePhase::Steady;
        }
        break;
    }
    case EnginePhase::Steady: {
        const double nScale = 1.0 + static_cast<double>(m_thrustStep) * 0.02;
        const double tScale = 1.0 + static_cast<double>(m_thrustStep) * 0.03;
        const double targetRpm = kSteadyRpm * nScale;
        const double targetEgt = kSteadyEgt * tScale;
        const double targetFlow = std::max(0.0, kSteadyFlow + static_cast<double>(m_thrustStep));

        for (size_t i = 0; i < 2; ++i) {
            m_state.rpm[i] += (targetRpm - m_state.rpm[i]) * 0.06;
            m_state.egt[i] += (targetEgt - m_state.egt[i]) * 0.06;
            m_state.rpm[i] *= 1.0 + 0.25 * pctNoise(rng);
            m_state.egt[i] *= 1.0 + 0.20 * pctNoise(rng);
        }
        m_state.fuelFlow += (targetFlow - m_state.fuelFlow) * 0.10;
        m_state.fuelFlow *= 1.0 + pctNoise(rng);
        break;
    }
    case EnginePhase::Stopping: {
        m_stoppingElapsedSec += dtSeconds;
        const double decayA = std::pow(kStoppingBase, 1.0 / kStopDurationSec);
        const double decay = std::pow(decayA, m_stoppingElapsedSec);
        m_state.fuelFlow = 0.0;
        for (size_t i = 0; i < 2; ++i) {
            m_state.rpm[i] = m_stoppingStartRpm[i] * decay;
            m_state.egt[i] = kRoomTemp + (m_stoppingStartEgt[i] - kRoomTemp) * decay;
        }
        if (m_stoppingElapsedSec >= kStopDurationSec || (m_state.rpm[0] < 50.0 && m_state.rpm[1] < 50.0)) {
            m_phase = EnginePhase::Off;
            m_state.rpm = {0.0, 0.0};
            m_state.egt = {kRoomTemp, kRoomTemp};
            m_state.fuelFlow = 0.0;
        }
        break;
    }
    }

    m_state.fuelFlow = clamp(m_state.fuelFlow, 0.0, 80.0);
    for (size_t i = 0; i < 2; ++i) {
        m_state.rpm[i] = clamp(m_state.rpm[i], 0.0, kRatedRpm * 1.35);
        m_state.egt[i] = clamp(m_state.egt[i], kMinTemp, kMaxTemp + 200.0);
    }

    m_state.fuelQty -= m_state.fuelFlow * dtSeconds;
    m_state.fuelQty = clamp(m_state.fuelQty, kMinFuel, kMaxFuel);
}

void EngineCore::enterStopping() {
    if (m_phase == EnginePhase::Off || m_phase == EnginePhase::Stopping) {
        return;
    }
    m_phase = EnginePhase::Stopping;
    m_stoppingElapsedSec = 0.0;
    m_stoppingStartRpm = m_state.rpm;
    m_stoppingStartEgt = m_state.egt;
}

void EngineCore::applyInjectedFaults() {
    if (faultEnabled(FaultCode::LowFuel)) {
        m_state.fuelQty = std::min(m_state.fuelQty, 900.0);
    }
    if (faultEnabled(FaultCode::OverFF)) {
        m_state.fuelFlow = std::max(m_state.fuelFlow, 55.0);
    }
    if (faultEnabled(FaultCode::OverSpd1)) {
        for (double& rpm : m_state.rpm) {
            rpm = std::max(rpm, kRatedRpm * 1.06);
        }
    }
    if (faultEnabled(FaultCode::OverSpd2)) {
        for (double& rpm : m_state.rpm) {
            rpm = std::max(rpm, kRatedRpm * 1.21);
        }
    }
    if (faultEnabled(FaultCode::OverTemp1)) {
        for (double& t : m_state.egt) {
            t = std::max(t, 860.0);
        }
    }
    if (faultEnabled(FaultCode::OverTemp2)) {
        for (double& t : m_state.egt) {
            t = std::max(t, 1010.0);
        }
    }
    if (faultEnabled(FaultCode::OverTemp3)) {
        for (double& t : m_state.egt) {
            t = std::max(t, 960.0);
        }
    }
    if (faultEnabled(FaultCode::OverTemp4)) {
        for (double& t : m_state.egt) {
            t = std::max(t, 1110.0);
        }
    }
}

EngineCore::SensorState EngineCore::buildSensorState() const {
    SensorState sensors{};
    sensors.fuelQty = m_state.fuelQty;
    sensors.fuelFlow = m_state.fuelFlow;

    for (size_t i = 0; i < 2; ++i) {
        const double n1 = rpmToN1(m_state.rpm[i]);
        sensors.n1Sensors[i].ch1 = n1;
        sensors.n1Sensors[i].ch2 = n1;
        sensors.egtSensors[i].ch1 = m_state.egt[i];
        sensors.egtSensors[i].ch2 = m_state.egt[i];
    }

    if (faultEnabled(FaultCode::N1S1Fail)) {
        sensors.n1Sensors[0].ch1.reset();
    }
    if (faultEnabled(FaultCode::N1S2Fail)) {
        sensors.n1Sensors[1].ch1.reset();
    }
    if (faultEnabled(FaultCode::EGTS1Fail)) {
        sensors.egtSensors[0].ch1.reset();
    }
    if (faultEnabled(FaultCode::EGTS2Fail)) {
        sensors.egtSensors[1].ch1.reset();
    }
    if (faultEnabled(FaultCode::N1SFail)) {
        sensors.n1Sensors[0].ch1.reset();
        sensors.n1Sensors[0].ch2.reset();
    }
    if (faultEnabled(FaultCode::EGTSFail)) {
        sensors.egtSensors[0].ch1.reset();
        sensors.egtSensors[0].ch2.reset();
    }
    if (faultEnabled(FaultCode::FuelSFail)) {
        sensors.fuelQty.reset();
    }

    return sensors;
}

void EngineCore::evaluateAlerts(Snapshot& snapshot) {
    std::vector<AlertEvent> alerts;

    const auto countInvalid = [](const SensorPair& pair) {
        int c = 0;
        if (!pair.ch1.has_value()) {
            ++c;
        }
        if (!pair.ch2.has_value()) {
            ++c;
        }
        return c;
    };

    const int n1InvalidL = countInvalid(snapshot.n1Sensors[0]);
    const int n1InvalidR = countInvalid(snapshot.n1Sensors[1]);
    const int egtInvalidL = countInvalid(snapshot.egtSensors[0]);
    const int egtInvalidR = countInvalid(snapshot.egtSensors[1]);

    if (n1InvalidL + n1InvalidR == 1) {
        addAlert(alerts, "N1_SENSOR_FAIL", "Single N1 sensor failed", AlertColor::White);
    }
    if (egtInvalidL + egtInvalidR == 1) {
        addAlert(alerts, "EGT_SENSOR_FAIL", "Single EGT sensor failed", AlertColor::White);
    }
    if (n1InvalidL == 2 || n1InvalidR == 2) {
        addAlert(alerts, "N1_ENGINE_SENSOR_FAIL", "One engine N1 sensors invalid", AlertColor::Amber);
    }
    if (egtInvalidL == 2 || egtInvalidR == 2) {
        addAlert(alerts, "EGT_ENGINE_SENSOR_FAIL", "One engine EGT sensors invalid", AlertColor::Amber);
    }
    if ((n1InvalidL == 2 && n1InvalidR == 2) || (egtInvalidL == 2 && egtInvalidR == 2)) {
        addAlert(alerts, "DUAL_ENGINE_SENSOR_FAIL", "Both engines sensor failed, auto stopping", AlertColor::Red);
    }

    if (!snapshot.fuelQtySensor.has_value()) {
        addAlert(alerts, "FUEL_SENSOR_FAIL", "Fuel quantity sensor invalid", AlertColor::Red);
    } else if (snapshot.fuelQtySensor.value() < 1000.0 && snapshot.phase != EnginePhase::Off && snapshot.phase != EnginePhase::Stopping) {
        addAlert(alerts, "LOW_FUEL", "Fuel quantity low", AlertColor::Amber);
    }

    if (snapshot.fuelFlow > kMaxFuelFlow) {
        addAlert(alerts, "OVER_FF", "Fuel flow over limit", AlertColor::Amber);
    }

    const double n1Left = snapshot.n1Percent[0];
    const double n1Right = snapshot.n1Percent[1];
    const double egtLeft = snapshot.egt[0];
    const double egtRight = snapshot.egt[1];

    if (n1Left > 105.0 || n1Right > 105.0) {
        addAlert(alerts, "OVER_SPD_1", "N1 overspeed level 1", AlertColor::Amber);
    }
    if (n1Left > 120.0 || n1Right > 120.0) {
        addAlert(alerts, "OVER_SPD_2", "N1 overspeed level 2, auto stopping", AlertColor::Red);
    }

    const bool startLikePhase = (snapshot.phase == EnginePhase::StartingLinear || snapshot.phase == EnginePhase::StartingLog);
    const bool steadyPhase = (snapshot.phase == EnginePhase::Steady);

    if (startLikePhase && (egtLeft > 850.0 || egtRight > 850.0)) {
        addAlert(alerts, "OVER_TEMP_1", "EGT over temp level 1 during start", AlertColor::Amber);
    }
    if (startLikePhase && (egtLeft > 1000.0 || egtRight > 1000.0)) {
        addAlert(alerts, "OVER_TEMP_2", "EGT over temp level 2 during start, auto stopping", AlertColor::Red);
    }
    if (steadyPhase && (egtLeft > 950.0 || egtRight > 950.0)) {
        addAlert(alerts, "OVER_TEMP_3", "EGT over temp level 3 at steady", AlertColor::Amber);
    }
    if (steadyPhase && (egtLeft > 1100.0 || egtRight > 1100.0)) {
        addAlert(alerts, "OVER_TEMP_4", "EGT over temp level 4 at steady, auto stopping", AlertColor::Red);
    }

    for (const AlertEvent& alert : alerts) {
        if (isCriticalAlert(alert.code)) {
            enterStopping();
            break;
        }
    }

    snapshot.activeAlerts = alerts;
    snapshot.newAlertsForLog.clear();

    for (const AlertEvent& alert : snapshot.activeAlerts) {
        const auto it = m_lastLogTimeSec.find(alert.code);
        const bool shouldLog = (it == m_lastLogTimeSec.end()) || ((snapshot.simTimeSec - it->second) >= 5.0);
        if (shouldLog) {
            m_lastLogTimeSec[alert.code] = snapshot.simTimeSec;
            snapshot.newAlertsForLog.push_back(alert);
        }
    }

    for (size_t i = 0; i < 2; ++i) {
        if (!snapshot.display[i].n1Percent.has_value()) {
            snapshot.display[i].n1State = ValueState::Invalid;
        } else {
            ValueState state = ValueState::Normal;
            const double n1 = snapshot.display[i].n1Percent.value();
            if (n1 > 120.0) {
                state = ValueState::Warning;
            } else if (n1 > 105.0 || ((i == 0 && n1InvalidL == 2) || (i == 1 && n1InvalidR == 2))) {
                state = ValueState::Caution;
            }
            snapshot.display[i].n1State = maxState(snapshot.display[i].n1State, state);
        }

        if (!snapshot.display[i].egt.has_value()) {
            snapshot.display[i].egtState = ValueState::Invalid;
        } else {
            ValueState state = ValueState::Normal;
            const double t = snapshot.display[i].egt.value();
            if ((startLikePhase && t > 1000.0) || (steadyPhase && t > 1100.0)) {
                state = ValueState::Warning;
            } else if ((startLikePhase && t > 850.0) || (steadyPhase && t > 950.0) ||
                       ((i == 0 && egtInvalidL == 2) || (i == 1 && egtInvalidR == 2))) {
                state = ValueState::Caution;
            }
            snapshot.display[i].egtState = maxState(snapshot.display[i].egtState, state);
        }
    }

    snapshot.fuelQtyState = ValueState::Normal;
    if (!snapshot.fuelQtySensor.has_value()) {
        snapshot.fuelQtyState = ValueState::Invalid;
    } else if (snapshot.fuelQtySensor.value() < 1000.0 && snapshot.phase != EnginePhase::Off && snapshot.phase != EnginePhase::Stopping) {
        snapshot.fuelQtyState = ValueState::Caution;
    }

    snapshot.fuelFlowState = (snapshot.fuelFlow > kMaxFuelFlow) ? ValueState::Caution : ValueState::Normal;
}

double EngineCore::clamp(double value, double low, double high) {
    return std::max(low, std::min(high, value));
}

std::optional<double> EngineCore::averagePair(const SensorPair& pair) {
    if (pair.ch1.has_value() && pair.ch2.has_value()) {
        return (pair.ch1.value() + pair.ch2.value()) / 2.0;
    }
    if (pair.ch1.has_value()) {
        return pair.ch1;
    }
    if (pair.ch2.has_value()) {
        return pair.ch2;
    }
    return std::nullopt;
}

bool EngineCore::isCriticalAlert(const std::string& code) {
    return code == "OVER_SPD_2" || code == "OVER_TEMP_2" || code == "OVER_TEMP_4" || code == "DUAL_ENGINE_SENSOR_FAIL";
}

ValueState EngineCore::maxState(ValueState a, ValueState b) {
    auto rank = [](ValueState v) {
        switch (v) {
        case ValueState::Normal:
            return 0;
        case ValueState::Caution:
            return 1;
        case ValueState::Warning:
            return 2;
        case ValueState::Invalid:
            return 3;
        }
        return 0;
    };
    return (rank(a) >= rank(b)) ? a : b;
}

void EngineCore::addAlert(std::vector<AlertEvent>& out, const std::string& code, const std::string& text, AlertColor color) const {
    const auto it = std::find_if(out.begin(), out.end(), [&](const AlertEvent& alert) { return alert.code == code; });
    if (it == out.end()) {
        out.push_back(AlertEvent{code, text, color});
    }
}

}  // namespace eicas
