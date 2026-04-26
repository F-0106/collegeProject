#include "QtWidgetsApplication.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>

#include <algorithm>

namespace {

QString panelStyle() {
    return "QWidget#Panel {"
           "background:#020407;"
           "border:1px solid #1134d9;"
           "}";
}

QString readoutStyle(const QString& color) {
    return "QLabel {"
           "background:#050607;"
           "border:1px solid #5e6572;"
           "color:" + color + ";"
           "font:700 25px Consolas;"
           "padding:8px;"
           "}";
}

}  // namespace

QtWidgetsApplication::QtWidgetsApplication(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Virtual Engine Performance Monitor");
    resize(1280, 860);
    setMinimumSize(1180, 800);
    setStyleSheet("QMainWindow { background:#000000; }"
                  "QWidget { background:#000000; color:#eeeeee; font-family:Consolas; }"
                  "QLabel#Title { color:#29f1e6; font:700 26px Consolas; }"
                  "QLabel#SmallTitle { color:#a7adb7; font:700 16px Consolas; }"
                  "QTextEdit { background:#020407; border:1px solid #1134d9; color:#eeeeee; font:18px Consolas; padding:8px; }");

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto* top = new QHBoxLayout();
    auto* fire = new QLabel("FIRE");
    fire->setStyleSheet("QLabel { background:#d10b20; color:#ffffff; font:700 38px Consolas; padding:4px 16px; }");
    auto* title = new QLabel("EICAS ENGINE PERFORMANCE");
    title->setObjectName("Title");
    title->setAlignment(Qt::AlignCenter);
    phase_ = new QLabel("OFF");
    phase_->setStyleSheet("QLabel { color:#d20072; font:700 28px Consolas; }");
    time_ = new QLabel("0.000 s");
    time_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    time_->setStyleSheet("QLabel { color:#29f1e6; font:700 22px Consolas; }");
    top->addWidget(fire);
    top->addWidget(title, 1);
    top->addWidget(phase_);
    top->addWidget(time_);
    root->addLayout(top);

    auto* main = new QHBoxLayout();
    main->setSpacing(10);

    auto* leftPanel = new QWidget();
    leftPanel->setObjectName("Panel");
    leftPanel->setStyleSheet(panelStyle());
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(12, 12, 12, 12);
    leftLayout->setSpacing(10);

    auto* gaugeGrid = new QGridLayout();
    gaugeGrid->setSpacing(8);
    n1Left_ = new eicas::GaugeWidget();
    n1Left_->setTitle("N1 L");
    n1Left_->setUnit("% RPM");
    n1Left_->setRange(0.0, 125.0);
    n1Right_ = new eicas::GaugeWidget();
    n1Right_->setTitle("N1 R");
    n1Right_->setUnit("% RPM");
    n1Right_->setRange(0.0, 125.0);
    egtLeft_ = new eicas::GaugeWidget();
    egtLeft_->setTitle("EGT L");
    egtLeft_->setUnit("DEG C");
    egtLeft_->setRange(0.0, 1200.0);
    egtRight_ = new eicas::GaugeWidget();
    egtRight_->setTitle("EGT R");
    egtRight_->setUnit("DEG C");
    egtRight_->setRange(0.0, 1200.0);
    gaugeGrid->addWidget(n1Left_, 0, 0);
    gaugeGrid->addWidget(n1Right_, 0, 1);
    gaugeGrid->addWidget(egtLeft_, 1, 0);
    gaugeGrid->addWidget(egtRight_, 1, 1);
    leftLayout->addLayout(gaugeGrid, 1);

    auto* lamps = new QHBoxLayout();
    startLamp_ = makeStatusLamp("START");
    runLamp_ = makeStatusLamp("RUN");
    lamps->addStretch();
    lamps->addWidget(startLamp_);
    lamps->addSpacing(22);
    lamps->addWidget(runLamp_);
    lamps->addStretch();
    leftLayout->addLayout(lamps);

    auto* readouts = new QGridLayout();
    readouts->setSpacing(8);
    fuelQty_ = makeReadout();
    fuelFlow_ = makeReadout();
    auto* fuelTitle = new QLabel("TOTAL FUEL");
    auto* flowTitle = new QLabel("FF(PPH)");
    fuelTitle->setStyleSheet("QLabel { color:#c9ced7; font:700 18px Consolas; }");
    flowTitle->setStyleSheet("QLabel { color:#c9ced7; font:700 18px Consolas; }");
    readouts->addWidget(fuelTitle, 0, 0);
    readouts->addWidget(fuelQty_, 0, 1);
    readouts->addWidget(flowTitle, 1, 0);
    readouts->addWidget(fuelFlow_, 1, 1);
    leftLayout->addLayout(readouts);

    auto* controls = new QHBoxLayout();
    auto* start = makeCommandButton("START", "#20f04a");
    auto* stop = makeCommandButton("STOP", "#f22535");
    auto* up = makeCommandButton("THRUST +", "#4da1ff");
    auto* down = makeCommandButton("THRUST -", "#4da1ff");
    auto* reset = makeCommandButton("RESET", "#d0d0d0");
    controls->addWidget(start);
    controls->addWidget(stop);
    controls->addWidget(up);
    controls->addWidget(down);
    controls->addWidget(reset);
    leftLayout->addLayout(controls);

    connect(start, &QPushButton::clicked, this, [this]() { core_.requestStart(); });
    connect(stop, &QPushButton::clicked, this, [this]() { core_.requestStop(); });
    connect(up, &QPushButton::clicked, this, [this]() { core_.increaseThrust(); });
    connect(down, &QPushButton::clicked, this, [this]() { core_.decreaseThrust(); });
    connect(reset, &QPushButton::clicked, this, [this]() {
        core_.reset();
        recorder_.openNewSession("output");
        visibleAlerts_.clear();
        alerts_->clear();
        recordInfo_->setText(QString("CSV %1\nLOG %2").arg(QString::fromStdString(recorder_.csvPath()), QString::fromStdString(recorder_.logPath())));
        for (auto& item : faultButtons_) {
            item.second->setChecked(false);
        }
    });

    auto* rightPanel = new QWidget();
    rightPanel->setObjectName("Panel");
    rightPanel->setStyleSheet(panelStyle());
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(12, 12, 12, 12);
    rightLayout->setSpacing(10);

    auto* alertTitle = new QLabel("CAS / ALERTS");
    alertTitle->setObjectName("SmallTitle");
    alerts_ = new QTextEdit();
    alerts_->setReadOnly(true);
    rightLayout->addWidget(alertTitle);
    rightLayout->addWidget(alerts_, 1);

    auto* faultTitle = new QLabel("FAULT TEST");
    faultTitle->setObjectName("SmallTitle");
    rightLayout->addWidget(faultTitle);

    auto* faultGrid = new QGridLayout();
    faultGrid->setSpacing(7);
    const std::vector<std::pair<QString, eicas::FaultCode>> faults = {
        {"N1 S1 FAIL", eicas::FaultCode::N1S1Fail},
        {"N1 S2 FAIL", eicas::FaultCode::N1S2Fail},
        {"EGT S1 FAIL", eicas::FaultCode::EGTS1Fail},
        {"EGT S2 FAIL", eicas::FaultCode::EGTS2Fail},
        {"N1 ENG FAIL", eicas::FaultCode::N1SFail},
        {"EGT ENG FAIL", eicas::FaultCode::EGTSFail},
        {"LOW FUEL", eicas::FaultCode::LowFuel},
        {"FUEL FAIL", eicas::FaultCode::FuelSFail},
        {"OVER FF", eicas::FaultCode::OverFF},
        {"OVER SPD 1", eicas::FaultCode::OverSpd1},
        {"OVER SPD 2", eicas::FaultCode::OverSpd2},
        {"OVER TEMP 1", eicas::FaultCode::OverTemp1},
        {"OVER TEMP 2", eicas::FaultCode::OverTemp2},
        {"OVER TEMP 3", eicas::FaultCode::OverTemp3},
        {"OVER TEMP 4", eicas::FaultCode::OverTemp4},
    };

    for (int i = 0; i < static_cast<int>(faults.size()); ++i) {
        QPushButton* button = makeFaultButton(faults[i].first, faults[i].second);
        faultGrid->addWidget(button, i / 3, i % 3);
        faultButtons_.push_back({faults[i].second, button});
    }
    rightLayout->addLayout(faultGrid);

    recordInfo_ = new QLabel();
    recordInfo_->setWordWrap(true);
    recordInfo_->setStyleSheet("QLabel { color:#9da5b4; font:13px Consolas; border:1px solid #242b39; padding:6px; }");
    rightLayout->addWidget(recordInfo_);

    main->addWidget(leftPanel, 3);
    main->addWidget(rightPanel, 2);
    root->addLayout(main, 1);

    if (recorder_.openNewSession("output")) {
        recordInfo_->setText(QString("CSV %1\nLOG %2").arg(QString::fromStdString(recorder_.csvPath()), QString::fromStdString(recorder_.logPath())));
    } else {
        recordInfo_->setText("Recorder open failed");
    }

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, [this]() {
        const eicas::Snapshot snapshot = core_.tick();
        recorder_.append(snapshot);
        updateUi(snapshot);
    });
    timer_->start(5);
    updateUi(core_.tick(0.0001));
}

QtWidgetsApplication::~QtWidgetsApplication()
{
    recorder_.close();
}

QPushButton* QtWidgetsApplication::makeCommandButton(const QString& text, const QString& color) {
    auto* button = new QPushButton(text);
    button->setMinimumHeight(54);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet("QPushButton { background:#050607; border:2px solid " + color + "; color:" + color +
                          "; font:700 20px Consolas; padding:6px 12px; }"
                          "QPushButton:hover { background:#10151c; }"
                          "QPushButton:pressed { background:" + color + "; color:#000000; }");
    return button;
}

QLabel* QtWidgetsApplication::makeStatusLamp(const QString& text) {
    auto* label = new QLabel(text);
    label->setAlignment(Qt::AlignCenter);
    label->setMinimumSize(132, 54);
    label->setStyleSheet("QLabel { background:#061006; border:2px solid #1f6e24; color:#275b2a; font:700 25px Consolas; }");
    return label;
}

QLabel* QtWidgetsApplication::makeReadout() {
    auto* label = new QLabel("--");
    label->setAlignment(Qt::AlignCenter);
    label->setMinimumHeight(54);
    label->setStyleSheet(readoutStyle("#eeeeee"));
    return label;
}

QPushButton* QtWidgetsApplication::makeFaultButton(const QString& text, eicas::FaultCode code) {
    auto* button = new QPushButton(text);
    button->setCheckable(true);
    button->setMinimumHeight(42);
    button->setCursor(Qt::PointingHandCursor);
    updateFaultButton(button, false);
    connect(button, &QPushButton::toggled, this, [this, button, code](bool checked) {
        core_.setFault(code, checked);
        updateFaultButton(button, checked);
    });
    return button;
}

void QtWidgetsApplication::updateUi(const eicas::Snapshot& snapshot) {
    n1Left_->setValue(snapshot.display[0].n1Percent, snapshot.display[0].n1State);
    n1Right_->setValue(snapshot.display[1].n1Percent, snapshot.display[1].n1State);
    egtLeft_->setValue(snapshot.display[0].egt, snapshot.display[0].egtState);
    egtRight_->setValue(snapshot.display[1].egt, snapshot.display[1].egtState);

    fuelQty_->setText(valueText(snapshot.fuelQtySensor, 0));
    fuelQty_->setStyleSheet(readoutStyle(stateColor(snapshot.fuelQtyState)));
    fuelFlow_->setText(QString::number(snapshot.fuelFlow, 'f', 1));
    fuelFlow_->setStyleSheet(readoutStyle(stateColor(snapshot.fuelFlowState)));

    phase_->setText(phaseText(snapshot.phase));
    time_->setText(QString("%1 s").arg(snapshot.simTimeSec, 0, 'f', 3));
    startLamp_->setStyleSheet(snapshot.startLight
        ? "QLabel { background:#1bff38; border:2px solid #1bff38; color:#001800; font:700 25px Consolas; }"
        : "QLabel { background:#061006; border:2px solid #1f6e24; color:#275b2a; font:700 25px Consolas; }");
    runLamp_->setStyleSheet(snapshot.runLight
        ? "QLabel { background:#1bff38; border:2px solid #1bff38; color:#001800; font:700 25px Consolas; }"
        : "QLabel { background:#061006; border:2px solid #1f6e24; color:#275b2a; font:700 25px Consolas; }");

    updateAlerts(snapshot);
}

void QtWidgetsApplication::updateFaultButton(QPushButton* button, bool active) {
    if (active) {
        button->setStyleSheet("QPushButton { background:#ffc823; color:#000000; border:1px solid #ffc823; font:700 13px Consolas; padding:4px; }"
                              "QPushButton:hover { background:#ffe06a; }");
    } else {
        button->setStyleSheet("QPushButton { background:#4f5262; color:#ffffff; border:1px solid #4f5262; font:700 13px Consolas; padding:4px; }"
                              "QPushButton:hover { background:#666a7e; }");
    }
}

void QtWidgetsApplication::updateAlerts(const eicas::Snapshot& snapshot) {
    for (const eicas::AlertEvent& alert : snapshot.newAlertsForLog) {
        visibleAlerts_.push_back({alert, snapshot.simTimeSec, snapshot.simTimeSec + 5.0});
    }

    visibleAlerts_.erase(std::remove_if(visibleAlerts_.begin(), visibleAlerts_.end(),
        [&](const TimedAlert& item) { return item.expireAtSec < snapshot.simTimeSec; }), visibleAlerts_.end());

    alerts_->clear();
    for (const TimedAlert& item : visibleAlerts_) {
        const QString line = QString("<span style=\"%1\">%2  %3  %4</span>")
            .arg(alertStyle(item.event.color),
                 QString::number(item.shownAtSec, 'f', 3).rightJustified(8, ' '),
                 QString::fromStdString(item.event.code),
                 QString::fromStdString(item.event.text));
        alerts_->append(line);
    }
}

QString QtWidgetsApplication::phaseText(eicas::EnginePhase phase) {
    switch (phase) {
    case eicas::EnginePhase::Off:
        return "OFF";
    case eicas::EnginePhase::StartingLinear:
    case eicas::EnginePhase::StartingLog:
        return "START";
    case eicas::EnginePhase::Steady:
        return "RUN";
    case eicas::EnginePhase::Stopping:
        return "STOP";
    }
    return "UNKNOWN";
}

QString QtWidgetsApplication::valueText(const std::optional<double>& value, int precision) {
    if (!value.has_value()) {
        return "--";
    }
    return QString::number(value.value(), 'f', precision);
}

QString QtWidgetsApplication::stateColor(eicas::ValueState state) {
    switch (state) {
    case eicas::ValueState::Normal:
        return "#eeeeee";
    case eicas::ValueState::Caution:
        return "#ffc823";
    case eicas::ValueState::Warning:
        return "#f22535";
    case eicas::ValueState::Invalid:
        return "#9a9a9a";
    }
    return "#eeeeee";
}

QString QtWidgetsApplication::alertStyle(eicas::AlertColor color) {
    switch (color) {
    case eicas::AlertColor::White:
        return "color:#eeeeee;";
    case eicas::AlertColor::Amber:
        return "color:#ffc823;";
    case eicas::AlertColor::Red:
        return "color:#f22535;font-weight:700;";
    }
    return "color:#eeeeee;";
}
