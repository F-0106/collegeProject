#pragma once

#include "EngineCore.h"

#include <QWidget>

#include <optional>

namespace eicas {

class GaugeWidget : public QWidget {
public:
    explicit GaugeWidget(QWidget* parent = nullptr);

    void setTitle(const QString& title);
    void setUnit(const QString& unit);
    void setRange(double minValue, double maxValue);
    void setValue(std::optional<double> value, ValueState state);

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override;

private:
    QColor colorForState(ValueState state) const;
    QString valueText() const;

    QString m_title = "N1";
    QString m_unit = "%";
    double m_minValue = 0.0;
    double m_maxValue = 125.0;
    std::optional<double> m_value;
    ValueState m_state = ValueState::Normal;
};

}  // namespace eicas
