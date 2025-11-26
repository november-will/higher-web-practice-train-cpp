#include "sorting_hill.h"
#include "random.h"

#include <stdexcept>

SortingHill::SortingHill(size_t number_of_paths, std::vector<std::unique_ptr<SortingHandler>> handlers)
    : handlers_(std::move(handlers)),
      number_of_paths_(number_of_paths) {
}

size_t SortingHill::GetNumberOfPaths() const {
    return number_of_paths_;
}

size_t SortingHill::GetNumberOfWagBuffer() const {
    return wagon_buffer_.size();
}

void SortingHill::AddWagon(Wagon wagon) {
    wagon_buffer_.push(wagon);
}

void SortingHill::PopWagon() {
    wagon_buffer_.pop();
}

bool SortingHill::IsWagonBuffer() const {
    return !wagon_buffer_.empty();
}

/* Проверка возможности события. */
bool SortingHill::CheckEvent(EventType event) const {
    switch (event) {
        case EventType::kPreparePath: {
            /* Проверить, что есть свободные пути. */
            return false;
        }
        case EventType::kTrainPlanned: {
            /* Проверить, что на назначенных путях нет поездов. */
            return false;
        }
        case EventType::kLocoArrived: {
            /* Проверить, что у поезда нет локомотива. */
            return false;
        }
        case EventType::kTrainReady: {
            /* Проверить, что количество вагонов соответствует локомотиву или буфер вагонов пустой. */
            return false;
        }
        default:
            return true;
    }
}

void SortingHill::HandleEvent(EventType event) {
    switch (event) {
        case EventType::kShiftStarted: {
            for (const auto& handler : handlers_) {
                handler->StartShift(*this);
            }
            break;
        }
        case EventType::kShiftEnded: {
            /* Здесь нужно освободить все пути. Можно отправлять локомотивы, даже без вагонов. */
            for (const auto& handler : handlers_) {
                handler->EndShift(*this);
            }
            break;
        }
        case EventType::kWagonArrived: {
            if (!IsWagonBuffer()) {
                break;
            }
            const auto& wagon = wagon_buffer_.front();
            OperationInfo operation_info;
            for (const auto& handler : handlers_) {
                handler->HandleWagon(*this, wagon, operation_info);
            }
            PopWagon();
            break;
        }
        case EventType::kLocoArrived: {
            auto loco_type = RandomGen::GetRandomElem<LocoType>(kLocoType);
            Locomotive locomotive{loco_type};
            OperationInfo operation_info;
            for (const auto& handler : handlers_) {
                handler->HandleLocomotive(*this, locomotive, operation_info);
            }
            break;
        }
        case EventType::kTrainPlanned: {
            OperationInfo operation_info;
            for (const auto& handler : handlers_) {
                handler->AllocatePathForTrain(*this, operation_info);
            }
            break;
        }
        case EventType::kTrainReady: {
            OperationInfo operation_info;
            for (const auto& handler : handlers_) {
                handler->SendTrain(*this, operation_info);
            }
            break;
        }
        case EventType::kPreparePath: {
            OperationInfo operation_info;
            for (const auto& handler : handlers_) {
                handler->PreparePath(*this, operation_info);
            }
            break;
        }
        default: {
            using namespace std::literals;
            throw std::out_of_range("Неожиданное событие"s);
        }
    }
}
