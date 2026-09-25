/// @file Tunables.cpp
/// @brief Tiny JSON float field parser for handling tunables.

#include "sim/Tunables.hpp"

#include <fstream>
#include <sstream>

namespace kart::sim {
namespace {

bool parse_float_field(const std::string& json, const char* key, float& out) {
    const std::string pattern = std::string("\"") + key + "\"";
    const auto pos = json.find(pattern);
    if (pos == std::string::npos) {
        return false;
    }
    const auto colon = json.find(':', pos + pattern.size());
    if (colon == std::string::npos) {
        return false;
    }
    std::size_t idx = colon + 1;
    while (idx < json.size() && (json[idx] == ' ' || json[idx] == '\t')) {
        ++idx;
    }
    try {
        std::size_t consumed = 0;
        out = std::stof(json.substr(idx), &consumed);
        return consumed > 0;
    } catch (const std::exception&) {
        return false;
    }
}

}  // namespace

KartTunables load_tunables_or_default(const std::string& path) {
    KartTunables t{};
    std::ifstream file(path);
    if (!file) {
        return t;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string json = ss.str();

    (void)parse_float_field(json, "max_speed", t.max_speed);
    (void)parse_float_field(json, "accel", t.accel);
    (void)parse_float_field(json, "brake_decel", t.brake_decel);
    (void)parse_float_field(json, "coast_decel", t.coast_decel);
    (void)parse_float_field(json, "steer_rate", t.steer_rate);
    (void)parse_float_field(json, "steer_speed_falloff", t.steer_speed_falloff);
    return t;
}

}  // namespace kart::sim
