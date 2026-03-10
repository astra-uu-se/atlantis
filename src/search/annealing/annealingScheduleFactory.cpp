#include "atlantis/search/annealing/annealingScheduleFactory.hpp"

#include <fstream>
#include <utility>

#define JSON_NO_IO
#define JSON_HAS_CPP_17
#include <nlohmann/json.hpp>

#include "atlantis/search/annealing/geometricCoolingSchedule.hpp"
#include "atlantis/search/annealing/geometricHeatingSchedule.hpp"
#include "atlantis/search/annealing/scheduleSequence.hpp"
#include "atlantis/search/annealing/scheduleLoop.hpp"

#include "./annealerHelper.hpp"

namespace atlantis::search {
  using namespace nlohmann;

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

static std::unique_ptr<AnnealingSchedule> parseSchedule(const std::string& name,
                                                        const json& value);

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

  return annealingScheduleHeating(
          value["heatingRate"].get<double>(),
          value["minimumUphillAcceptanceRatio"].get<double>());
}

static std::unique_ptr<AnnealingSchedule> parseCoolingSchedule(
    const json& value) {
  if (!value.is_object() || !value.contains("coolingRate") ||
      !value.contains("successiveFutileRoundsThreshold") ||
      !value["coolingRate"].is_number_float() ||
      !value["successiveFutileRoundsThreshold"].is_number_unsigned()) {
    throw AnnealingScheduleCreationError(
        "Invalid JSON for a cooling schedule. Expected an object with the "
        "fields 'coolingRate' (double) and 'successiveFutileRoundsThreshold' "
        "(uint).");
  }

  return annealingScheduleCooling(
      value["coolingRate"].get<double>(),
      value["successiveFutileRoundsThreshold"].get<UInt>());
}

static std::unique_ptr<AnnealingSchedule> parseScheduleSequence(
    const json& value) {
  if (!value.is_array() || value.empty()) {
    throw AnnealingScheduleCreationError(
        "Invalid JSON for a schedule sequence. Expected an object with at "
        "least one schedule as a member.");
  }

  std::vector<std::unique_ptr<AnnealingSchedule>> schedules;
  schedules.reserve(value.size());
  for (const auto& item : value) {
    if (!item.is_object() || item.size() != 1) {
      throw AnnealingScheduleCreationError(
          "Invalid schedule item. Expected an object with exactly one schedule in each sequence list item.");
    }
    schedules.emplace_back(parseSchedule(item.begin().key(), item.begin().value()));
  }

  return annealingScheduleSequence(std::move(schedules));
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

  const auto iterationCount =
      value["maximumConsecutiveFutileRounds"].get<UInt>();
  const auto it = value["inner"].begin();
  return annealingScheduleLoop(parseSchedule(it.key(), it.value()), iterationCount);
}

static std::unique_ptr<AnnealingSchedule> parseSchedule(const std::string& name,
                                                        const json& value) {
  if (name == "heating") {
    return parseHeatingSchedule(value);
  }
  if (name == "cooling") {
    return parseCoolingSchedule(value);
  }
  if (name == "sequence") {
    return parseScheduleSequence(value);
  }
  if (name == "loop") {
    return parseScheduleLoop(value);
  }
  throw AnnealingScheduleCreationError(
      std::string("Unknown schedule key: ").append(name));
}

  AnnealingScheduleFactory::AnnealingScheduleFactory() {
    std::vector<std::unique_ptr<AnnealingSchedule>> vec;
    vec.reserve(2);
    vec.emplace_back(std::make_unique<GeometricHeatingSchedule>(1.2, 0.75));
    vec.emplace_back(std::make_unique<GeometricCoolingSchedule>(0.99, 4));
    _schedules.reserve(1);
    _schedules.emplace_back(std::make_unique<ScheduleLoop>(std::make_unique<ScheduleSequence>(std::move(vec)), 5));
  }

  AnnealingScheduleFactory::AnnealingScheduleFactory(const std::filesystem::path& scheduleDefinition) {

    printf("\nMaking non-default annealing schedule:\n");

    auto contents = readFileToString(scheduleDefinition);
    auto parsedJson = json::parse(contents);

    if (!parsedJson.is_object() || !parsedJson.contains("schedules")) {
      throw AnnealingScheduleCreationError(
          "Expected an object with a 'schedules' array.");
    }

    const auto& schedulesArray = parsedJson["schedules"];

    if (!schedulesArray.is_array() || schedulesArray.empty()) {
      throw AnnealingScheduleCreationError(
          "Expected 'schedules' to be a non-empty array.");
    }

    _armCount = schedulesArray.size();
    _schedules.reserve(_armCount);
    for (size_t i = 0; i < _armCount; ++i) {
      const auto& scheduleObject = schedulesArray[i];

      if (!scheduleObject.is_object() || scheduleObject.size() != 1) {
        throw AnnealingScheduleCreationError(
            "Each schedule in the array must be an object with a single member "
            "which describes the schedule.");
      }

      const auto object = scheduleObject.begin();
      _schedules.emplace_back(parseSchedule(object.key(), object.value()));
    }
  }

std::unique_ptr<AnnealingSchedule> AnnealingScheduleFactory::create(const size_t index) const {
  return _schedules[index]->clone();
}

}  // namespace atlantis::search
