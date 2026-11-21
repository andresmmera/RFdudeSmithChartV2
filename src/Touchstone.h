#pragma once

#include "RfModel.h"

#include <string_view>
#include <utility>

namespace rf {

TouchstoneData parseTouchstone(std::string_view text, int ports, const std::string& name);
std::string formatTouchstone(const std::vector<ResponseSample>& samples, int ports,
                             double referenceOhms);
void validate(const TouchstoneData& data);
SParameters measuredParameters(const TouchstoneData& data, double frequencyMHz, double portOhms);
Impedance startingImpedance(const Document& document);
Impedance startingImpedance(const Document& document, double frequencyMHz);
std::pair<double, double> measuredFrequencyRange(const Document& document);
double clampMeasuredFrequency(const Document& document, double frequencyMHz);
std::vector<double> measuredFrequencies(const Document& document);
double nearestMeasuredFrequency(const Document& document, double frequencyMHz);
double stepMeasuredFrequency(const Document& document, double frequencyMHz, int direction);

}
