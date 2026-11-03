#pragma once

#include <QDoubleSpinBox>

class QFormLayout;
class QWidget;

// A spin box whose value() is ALWAYS in MHz (the unit used everywhere in the model),
// but which displays and accepts text in kHz, MHz or GHz. Pair it with a unit combo
// via frequencyEditor() / addFrequencyRow(). Trailing zeros are trimmed instead of
// using a fixed number of decimals, so 455 kHz, 433.92 MHz and 2.4 GHz stay compact.
class FrequencySpinBox final : public QDoubleSpinBox
{
    Q_OBJECT

public:
    enum class Unit { kHz = 0, MHz = 1, GHz = 2 };

    explicit FrequencySpinBox(QWidget* parent = nullptr);

    Unit unit() const { return unit_; }
    void setUnit(Unit unit);
    // kHz below 1 MHz, GHz from 1000 MHz, otherwise MHz.
    static Unit preferredUnit(double megahertz);

    QValidator::State validate(QString& text, int& position) const override;

protected:
    QString textFromValue(double megahertz) const override;
    double valueFromText(const QString& text) const override;

private:
    double toDisplay(double megahertz) const;
    double fromDisplay(double shown) const;
    Unit unit_ = Unit::MHz;
};

// Wraps the spin box and a kHz/MHz/GHz combo (object name: <spin name> + "Unit")
// in one widget, choosing the unit that suits the spin box's current value.
// Set the spin box's object name and value BEFORE calling this.
QWidget* frequencyEditor(FrequencySpinBox* spin);

// Creates a FrequencySpinBox (value in MHz) with its unit combo and adds it as a form row.
// The row's field widget is spin->parentWidget(), e.g. for QFormLayout::setRowVisible().
FrequencySpinBox* addFrequencyRow(QFormLayout* form, const QString& label,
                                  const QString& name, double valueMHz);
