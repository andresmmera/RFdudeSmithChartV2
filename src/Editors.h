#pragma once

#include "RfModel.h"

#include <QString>
#include <optional>

class QWidget;

 /// @brief Shows the modal component editor.
 /// RLC blocks the dialog:
 ///    - Resistance, Inductance or Capacitance as the keyboard tuning parameter
 ///    - Transmission lines, coax, microstrip and stubs it offers Length or Characteristic impedance.
 /// The choice is stored in rf::Component::tune and the matching value field receives initial focus.
 ///
 /// @param parent    Parent widget for the dialog.
 /// @param component Component to edit; measured blocks are shown read-only.
 /// @return The edited component, or std::nullopt if the dialog was cancelled.
std::optional<rf::Component> editComponent(QWidget* parent, const rf::Component& component);
std::optional<rf::Document> editSettings(QWidget* parent, const rf::Document& document,
                                       const QString& focusField = {});

/// @brief Returns a one-line summary of a component for lists, tooltips and exports.
QString componentDescription(const rf::Component& component);
QString measuredDescription(const rf::TouchstoneData& data);
