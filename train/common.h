#pragma once

#include "enums.h"

#include <array>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

struct Wagon {
    int number;
    WagonType wagon_type;
};

struct Locomotive {
    LocoType loco_type;
};

// Информация о выполненной операции.
// Заполняется оператором и затем используется:
//  SortingHill: обновление состояния и метрик
//  SortingReporterImpl: печать отчёта
struct OperationInfo {
    EventType event_type = EventType::kShiftStarted;
    bool success = false;

    // Где применилось (если применилось)
    std::optional<int> path_id;
    std::optional<std::string> train_number;

    // Локомотив
    std::optional<LocoType> loco_type;
    std::optional<int> loco_capacity;
    bool loco_attached = false;   // true, если локомотив был прицеплен к конкретному поезду
    bool loco_reserved = false;   // true, если локомотив ушёл в резерв (нет поездов без локомотива)

    // Вагон
    std::optional<Wagon> wagon;
    std::optional<bool> wagon_to_ring;  // true -> на кольцевой путь, false -> в поезд
    std::optional<size_t> train_wagons; // текущее кол-во вагонов в поезде после добавления
    std::optional<int> train_capacity;  // вместимость локомотива поезда (если известна)

    // Кольцевой путь
    std::optional<size_t> ring_total; // текущее заполнение кольца
    std::optional<size_t> ring_max;   // максимум за смену

    // Отправка поезда
    bool train_sent = false;

    std::string message; // для отладки/логов
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

// cоответствие комментариям в enums.h
// Freight -> "Г", Pass -> "Л", Danger -> "О"
inline std::ostream& operator<<(std::ostream& os, TrainType train_type) {
    using namespace std::literals;
    switch (train_type) {
        case TrainType::kFreight:
            os << "Г"s;
            break;
        case TrainType::kDanger:
            os << "О"s;
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
