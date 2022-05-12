#include "search/annealing/annealingScheduleFactory.hpp"

#define JSON_NO_IO
#define JSON_HAS_CPP_17
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

#include "search/annealing/annealerFacade.hpp"

using namespace nlohmann;
using namespace search;

static std::string readFileToString(const std::filesystem::path& path) {
  std::ifstream is{path};
  if (!is.good()) {
    throw AnnealingScheduleCreationError("Could not open definition file.");
  }

  std::ostringstream stringStream;
  is >> stringStream.rdbuf();

  if (is.fail() && !is.eof()) {
    throw AnnealingScheduleCreationError(
        "Failed to read contents of definition file.");
  }

  return stringStream.str();
}

static std::unique_ptr<AnnealingSchedule> parseSchedule(
    const std::string& name, const json& value);

static std::unique_ptr<AnnealingSchedule> parseHeatingSchedule(
    const json& value) {
  if (!value.is_object() || !value.contains("heatingRate") ||
      !value.contains("minimumUphillAcceptanceRatio") ||
      !value["heatingRate"].is_number_float() ||
      !value["minimumUphillAcceptanceRatio"].is_number_float()) {
    throw AnnealingScheduleCreationError(
        "Invalid JSON for a heating schedule. Expected an object with the "
        "fields 'heatingRate' (double) and 'minimumUphillAcceptanceRatio' "
        "(double).");
  }

  return AnnealerFacade::heating(
      value["heatingRate"].get<double>(),
      value["minimumUphillAcceptanceRatio"].get<double>());
}

static std::unique_ptr<AnnealingSchedule> parseCoolingSchedule(
    const json& value) {
  if (!value.is_object() || !value.contains("coolingRate") ||
      !value.contains("moveAcceptanceRatio") ||
      !value["coolingRate"].is_number_float() ||
      !value["moveAcceptanceRatio"].is_number_float()) {
    throw AnnealingScheduleCreationError(
        "Invalid JSON for a cooling schedule. Expected an object with the "
        "fields 'coolingRate' (double) and 'moveAcceptanceRatio' "
        "(double).");
  }

  return AnnealerFacade::cooling(
      value["coolingRate"].get<double>(),
      value["moveAcceptanceRatio"].get<double>());
}

static std::unique_ptr<AnnealingSchedule> parseScheduleSequence(
    const json& value) {
  if (!value.is_object() || value.empty()) {
    throw AnnealingScheduleCreationError(
        "Invalid JSON for a schedule sequence. Expected an object with at "
        "least one schedule as a member.");
  }

  std::vector<std::unique_ptr<AnnealingSchedule>> schedules;
  for (auto memberIt = value.begin(); memberIt != value.end(); ++memberIt) {
    schedules.push_back(parseSchedule(memberIt.key(), memberIt.value()));
  }

  return AnnealerFacade::sequence(std::move(schedules));
}

static std::unique_ptr<AnnealingSchedule> parseScheduleLoop(const json& value) {
  if (!value.is_object() || !value.contains("maximumConsecutiveFutileRounds") ||
      !value.contains("inner") ||
      !value["maximumConsecutiveFutileRounds"].is_number_unsigned() ||
      !value["inner"].is_object() || value["inner"].size() != 1) {
    throw AnnealingScheduleCreationError(
        "Invalid JSON for a schedule loop. Expected an object with the fields "
        "'maximumConsecutiveFutileRounds' (uint) and 'inner' (schedule).");
  }

  auto iterationCount = value["maximumConsecutiveFutileRounds"].get<UInt>();
  auto it = value["inner"].begin();
  auto schedule = parseSchedule(it.key(), it.value());
  return AnnealerFacade::loop(std::move(schedule), iterationCount);
}

static std::unique_ptr<AnnealingSchedule> parseSchedule(
    const std::string& name, const json& value) {
  if (name == "heating") {
    return parseHeatingSchedule(value);
  } else if (name == "cooling") {
    return parseCoolingSchedule(value);
  } else if (name == "sequence") {
    return parseScheduleSequence(value);
  } else if (name == "loop") {
    return parseScheduleLoop(value);
  } else {
    throw AnnealingScheduleCreationError(
        std::string("Unknown schedule key: ").append(name));
  }
}

std::unique_ptr<AnnealingSchedule> AnnealingScheduleFactory::create() const {
  if (!_scheduleDefinition) {
    return AnnealerFacade::cooling(0.95, 0.001);
  }

  auto contents = readFileToString(*_scheduleDefinition);
  auto parsedJson = json::parse(contents);

  if (!parsedJson.is_object() || parsedJson.size() != 1) {
    throw AnnealingScheduleCreationError(
        "Expected an object with a single member which describes the "
        "schedule.");
  }

  auto it = parsedJson.begin();
  return parseSchedule(it.key(), it.value());
}
