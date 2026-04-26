#pragma once

#include "EngineCore.h"
#include "GaugeWidget.h"
#include "Recorder.h"

#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtCore/QTimer>

#include <utility>
#include <vector>

class QtWidgetsApplication : public QMainWindow
{
    Q_OBJECT

public:
    QtWidgetsApplication(QWidget *parent = nullptr);
    ~QtWidgetsApplication();

private:
    struct TimedAlert {
        eicas::AlertEvent event;
        double shownAtSec = 0.0;
        double expireAtSec = 0.0;
    };

    QPushButton* makeCommandButton(const QString& text, const QString& color);
    QLabel* makeStatusLamp(const QString& text);
    QLabel* makeReadout();
    QPushButton* makeFaultButton(const QString& text, eicas::FaultCode code);

    void updateUi(const eicas::Snapshot& snapshot);
    void updateFaultButton(QPushButton* button, bool active);
    void updateAlerts(const eicas::Snapshot& snapshot);

    static QString phaseText(eicas::EnginePhase phase);
    static QString valueText(const std::optional<double>& value, int precision);
    static QString stateColor(eicas::ValueState state);
    static QString alertStyle(eicas::AlertColor color);

    eicas::EngineCore core_;
    eicas::Recorder recorder_;
    QTimer* timer_ = nullptr;

    eicas::GaugeWidget* n1Left_ = nullptr;
    eicas::GaugeWidget* n1Right_ = nullptr;
    eicas::GaugeWidget* egtLeft_ = nullptr;
    eicas::GaugeWidget* egtRight_ = nullptr;

    QLabel* fuelQty_ = nullptr;
    QLabel* fuelFlow_ = nullptr;
    QLabel* phase_ = nullptr;
    QLabel* time_ = nullptr;
    QLabel* startLamp_ = nullptr;
    QLabel* runLamp_ = nullptr;
    QLabel* recordInfo_ = nullptr;
    QTextEdit* alerts_ = nullptr;

    std::vector<std::pair<eicas::FaultCode, QPushButton*>> faultButtons_;
    std::vector<TimedAlert> visibleAlerts_;
};
