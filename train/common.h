#pragma once

#include "enums.h"

#include <array>
#include <iostream>
#include <optional>
#include <vector>
#include <string>

struct Wagon {
    int number;
    WagonType wagon_type;
};

struct Locomotive {
    LocoType loco_type;
};

struct OperationInfo {

};

inline constexpr std::array<EventType, 17> kEventsBalanced = {
    EventType::kWagonArrived, EventType::kWagonArrived, EventType::kWagonArrived, EventType::kWagonArrived,
    EventType::kWagonArrived, EventType::kWagonArrived, EventType::kWagonArrived, EventType::kWagonArrived,
    EventType::kWagonArrived, EventType::kLocoArrived, EventType::kLocoArrived, EventType::kLocoArrived,
    EventType::kLocoArrived, EventType::kPreparePath, EventType::kPreparePath, EventType::kTrainPlanned,
    EventType::kTrainReady
};

inline constexpr std::array<TrainType, 3> kTrainType = {
    TrainType::kFreight, TrainType::kPass, TrainType::kDanger
};

inline constexpr std::array<WagonType, 4> kWagonType = {
    WagonType::kEmpty, WagonType::kFreight, WagonType::kPass, WagonType::kDanger
};

inline constexpr std::array<LocoType, 4> kLocoType = {
    LocoType::kElectro16, LocoType::kElectro32, LocoType::kDiesel24, LocoType::kDiesel64
};

inline std::ostream& operator<<(std::ostream& os, EventType event_type) {
    using namespace std::literals;
    switch (event_type) {
        case EventType::kShiftStarted:
            os << "начало работ"s;
            break;
        case EventType::kPreparePath:
            os << "готовьте путь"s;
            break;
        case EventType::kTrainPlanned:
            os << "формируйте состав"s;
            break;
        case EventType::kLocoArrived:
            os << "подача локомотива"s;
            break;
        case EventType::kWagonArrived:
            os << "вагон на сортировку"s;
            break;
        case EventType::kTrainReady:
            os << "отправляйте поезд"s;
            break;
        case EventType::kShiftEnded:
            os << "окончание работ"s;
            break;
        default:
            os << "неизвестно"s;
            break;
    }
    return os;
}

inline std::ostream& operator<<(std::ostream& os, TrainType train_type) {
    using namespace std::literals;
    switch (train_type) {
        case TrainType::kFreight:
            os << "О"s;
            break;
        case TrainType::kDanger:
            os << "Г"s;
            break;
        case TrainType::kPass:
            os << "Л"s;
            break;
        default:
            os << ""s;
            break;
    }
    return os;
}
