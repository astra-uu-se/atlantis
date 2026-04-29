#include "atlantis/search/annealing/annealingScheduleFactory.hpp"

#include <fstream>
#include <utility>

#define JSON_NO_IO
#define JSON_HAS_CPP_17
#include <nlohmann/json.hpp>

#include "atlantis/search/annealing/AnnealingScheduleContainerFactory.hpp"

namespace atlantis::search {

using namespace nlohmann;

AnnealingScheduleFactory::AnnealingScheduleFactory(
    const std::optional<std::filesystem::path>& scheduleDefinition) {

  if (scheduleDefinition.has_value()) SetAnnealingSchedule(scheduleDefinition.value());
  else setDefaultAnnealingSchedule();
}

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

static std::unique_ptr<AnnealingScheduleContainerFactory> parseSchedule(const std::string& name,
                                                        const json& value);

static std::unique_ptr<AnnealingScheduleContainerFactory> parseHeatingSchedule(
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

  return std::make_unique<HeatingScheduleFactory>(
          value["heatingRate"].get<double>(),
          value["minimumUphillAcceptanceRatio"].get<double>());
}

static std::unique_ptr<AnnealingScheduleContainerFactory> parseCoolingSchedule(
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

  return std::make_unique<CoolingScheduleFactory>(
      value["coolingRate"].get<double>(),
      value["successiveFutileRoundsThreshold"].get<UInt>());
}

static std::unique_ptr<AnnealingScheduleContainerFactory> parseScheduleSequence(
    const json& value) {
  if (!value.is_array() || value.empty()) {
    throw AnnealingScheduleCreationError(
        "Invalid JSON for a schedule sequence. Expected an object with at "
        "least one schedule as a member.");
  }

  std::vector<std::unique_ptr<AnnealingScheduleContainerFactory>> schedules;
  for (const auto& item : value) {
    if (!item.is_object() || item.size() != 1) {
      throw AnnealingScheduleCreationError(
          "Invalid schedule item. Expected an object with exactly one schedule in each sequence list item.");
    }
    schedules.emplace_back(parseSchedule(item.begin().key(), item.begin().value()));
  }

  return std::make_unique<SequenceFactory>(std::move(schedules));
}

static std::unique_ptr<AnnealingScheduleContainerFactory> parseScheduleLoop(const json& value) {
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
  auto schedule = parseSchedule(it.key(), it.value());
  return std::make_unique<LoopScheduleFactory>(std::move(schedule), iterationCount);
}

static std::unique_ptr<AnnealingScheduleContainerFactory> parseSchedule(const std::string& name,
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

void AnnealingScheduleFactory::SetAnnealingSchedule(
    const std::filesystem::path& scheduleDefinition) {

  _scheduleDefinition = scheduleDefinition;

  auto contents = readFileToString(*_scheduleDefinition);
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
  _factories.reserve(_armCount);
  for (size_t i = 0; i < _armCount; ++i) {
    const auto& scheduleObject = schedulesArray[i];

    if (!scheduleObject.is_object() || scheduleObject.size() != 1) {
      throw AnnealingScheduleCreationError(
          "Each schedule in the array must be an object with a single member "
          "which describes the schedule.");
    }

    const auto it = scheduleObject.begin();
    auto res = parseSchedule(it.key(), it.value());
    _factories.push_back(std::move(res));
  }
}

std::unique_ptr<AnnealingSchedule> AnnealingScheduleFactory::create(const size_t index) const {
  return _factories[index]->create();
}

void AnnealingScheduleFactory::setDefaultAnnealingSchedule() {
  std::vector<std::unique_ptr<AnnealingScheduleContainerFactory>> vec;
  vec.reserve(2);
  vec.push_back(std::make_unique<HeatingScheduleFactory>(1.2, 0.75));
  vec.push_back(std::make_unique<CoolingScheduleFactory>(0.99, 4));
  auto seq = std::make_unique<SequenceFactory>(std::move(vec));

  _factories.push_back(std::make_unique<LoopScheduleFactory>(std::move(seq), 5));
}

}  // namespace atlantis::search
