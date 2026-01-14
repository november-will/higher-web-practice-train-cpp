#include "sorting_hill.h"
#include "random.h"

#include <algorithm>
#include <stdexcept>

SortingHill::SortingHill(size_t number_of_paths, std::vector<std::unique_ptr<SortingHandler>> handlers)
    : handlers_(std::move(handlers)),
      number_of_paths_(number_of_paths),
      paths_(number_of_paths) {
}

size_t SortingHill::GetNumberOfPaths() const {
    return number_of_paths_;
}

size_t SortingHill::GetNumberOfWagBuffer() const {
    return wagon_buffer_.size();
}

void SortingHill::AddWagon(const Wagon& wagon) {
    wagon_buffer_.push(wagon);
}

void SortingHill::PopWagon() {
    wagon_buffer_.pop();
}

bool SortingHill::IsWagonBuffer() const {
    return !wagon_buffer_.empty();
}

bool SortingHill::IsShiftEnding() const {
    return shift_ending_;
}

size_t SortingHill::GetPreparedPathsCount() const {
    return prepared_paths_count_;
}

size_t SortingHill::GetPlannedTrainsCount() const {
    return planned_trains_count_;
}

size_t SortingHill::GetArrivedLocosCount() const {
    return arrived_locos_count_;
}

size_t SortingHill::GetProcessedWagonsCount() const {
    return processed_wagons_count_;
}

size_t SortingHill::GetSentTrainsCount() const {
    return sent_trains_count_;
}

size_t SortingHill::GetRingTotal() const {
    return ring_total_;
}

size_t SortingHill::GetRingMax() const {
    return ring_max_;
}

int SortingHill::WagonTypeIndex_(WagonType type) {
    switch (type) {
        case WagonType::kFreight:
            return 0;
        case WagonType::kPass:
            return 1;
        case WagonType::kDanger:
            return 2;
        case WagonType::kEmpty:
            return 3;
        default:
            return -1;
    }
}

WagonType SortingHill::TrainNumberToWagonType_(const std::string& train_number) {
    // train_number заканчивается на "Г/Л/О/П" (UTF-8)
    if (train_number.size() < 2) {
        return WagonType::kFreight;
    }

    const std::string suffix = train_number.substr(train_number.size() - 2);
    if (suffix == "Г") {
        return WagonType::kFreight;
    }
    if (suffix == "Л") {
        return WagonType::kPass;
    }
    if (suffix == "О") {
        return WagonType::kDanger;
    }
    if (suffix == "П") {
        return WagonType::kEmpty;
    }

    return WagonType::kFreight;
}

size_t SortingHill::GetRingWagons(WagonType type) const {
    const int idx = WagonTypeIndex_(type);
    if (idx < 0) {
        return 0;
    }
    return ring_by_type_[static_cast<size_t>(idx)];
}

size_t SortingHill::GetBufferWagonsLeft(WagonType type) const {
    const int idx = WagonTypeIndex_(type);
    if (idx < 0) {
        return 0;
    }

    const size_t i = static_cast<size_t>(idx);
    return wagon_buffer_initial_by_type_[i] - wagon_buffer_processed_by_type_[i];
}

size_t SortingHill::GetMissedWagons(WagonType type) const {
    return GetRingWagons(type) + GetBufferWagonsLeft(type);
}

void SortingHill::ResetShiftState_() {
    paths_.assign(number_of_paths_, PathMeta{});
    trains_.clear();

    shift_ending_ = false;

    ring_total_ = 0;
    ring_max_ = 0;
    ring_by_type_.fill(0);

    wagon_buffer_initial_by_type_.fill(0);
    wagon_buffer_processed_by_type_.fill(0);

    {
        auto copy = wagon_buffer_;
        while (!copy.empty()) {
            const Wagon& w = copy.front();
            const int idx = WagonTypeIndex_(w.wagon_type);
            if (idx >= 0) {
                wagon_buffer_initial_by_type_[static_cast<size_t>(idx)]++;
            }
            copy.pop();
        }
    }

    prepared_paths_count_ = 0;
    planned_trains_count_ = 0;
    arrived_locos_count_ = 0;
    processed_wagons_count_ = 0;
    sent_trains_count_ = 0;
}

void SortingHill::ApplyOperationInfo_(const OperationInfo& op) {
    const size_t prev_ring_total = ring_total_;

    if (op.ring_total) {
        ring_total_ = *op.ring_total;
    }
    if (op.ring_max) {
        ring_max_ = std::max(ring_max_, *op.ring_max);
    }

    switch (op.event_type) {
        case EventType::kPreparePath: {
            ApplyPreparePath_(op);
            break;
        }

        case EventType::kTrainPlanned: {
            ApplyTrainPlanned_(op);
            ApplyRingDrainForTrain_(prev_ring_total, op);
            break;
        }

        case EventType::kLocoArrived: {
            ApplyLocoArrived_(op);
            ApplyRingDrainForTrain_(prev_ring_total, op);
            break;
        }

        case EventType::kWagonArrived: {
            ApplyWagonArrived_(op);
            break;
        }

        case EventType::kTrainReady: {
            ApplyTrainReady_(op);
            break;
        }

        default: {
            break;
        }
    }
}

void SortingHill::ApplyRingDrainForTrain_(size_t prev_ring_total, const OperationInfo& op) {
    if (!op.train_number) {
        return;
    }
    if (prev_ring_total <= ring_total_) {
        return;
    }

    size_t drained = prev_ring_total - ring_total_;
    const WagonType wagon_type = TrainNumberToWagonType_(*op.train_number);
    const int idx = WagonTypeIndex_(wagon_type);
    if (idx < 0) {
        return;
    }

    size_t& cur = ring_by_type_[static_cast<size_t>(idx)];
    if (drained >= cur) {
        cur = 0;
        return;
    }
    cur -= drained;
}

void SortingHill::ApplyPreparePath_(const OperationInfo& op) {
    if (!op.success || !op.path_id) {
        return;
    }

    const int pid = *op.path_id;
    if (pid < 0 || static_cast<size_t>(pid) >= paths_.size()) {
        return;
    }

    PathMeta& path = paths_[static_cast<size_t>(pid)];
    if (!path.prepared) {
        path.prepared = true;
        prepared_paths_count_++;
    }
}

void SortingHill::ApplyTrainPlanned_(const OperationInfo& op) {
    if (!op.success || !op.path_id || !op.train_number) {
        return;
    }

    const int pid = *op.path_id;
    if (pid < 0 || static_cast<size_t>(pid) >= paths_.size()) {
        return;
    }

    const std::string& train_number = *op.train_number;

    PathMeta& path = paths_[static_cast<size_t>(pid)];
    path.occupied = true;
    path.train_number = train_number;

    TrainMeta meta;
    meta.path_id = pid;

    if (op.loco_attached && op.loco_capacity) {
        meta.has_loco = true;
        meta.capacity = *op.loco_capacity;
    }
    if (op.train_wagons) {
        meta.wagons = static_cast<int>(*op.train_wagons);
    }

    trains_[train_number] = meta;
    planned_trains_count_++;
}

void SortingHill::ApplyLocoArrived_(const OperationInfo& op) {
    if (!op.loco_attached || !op.train_number) {
        return;
    }

    const std::string& train_number = *op.train_number;
    TrainMeta& meta = trains_[train_number];

    meta.has_loco = true;

    if (op.loco_capacity) {
        meta.capacity = *op.loco_capacity;
    }
    if (op.train_wagons) {
        meta.wagons = static_cast<int>(*op.train_wagons);
    }
    if (op.path_id) {
        meta.path_id = *op.path_id;
    }
}

void SortingHill::ApplyWagonArrived_(const OperationInfo& op) {
    if (!op.success) {
        return;
    }

    if (op.wagon_to_ring && *op.wagon_to_ring) {
        if (op.wagon) {
            const int idx = WagonTypeIndex_(op.wagon->wagon_type);
            if (idx >= 0) {
                ring_by_type_[static_cast<size_t>(idx)]++;
            }
        }

        if (!op.ring_total) {
            ring_total_++;
            ring_max_ = std::max(ring_max_, ring_total_);
        }
        return;
    }

    if (op.train_number) {
        TrainMeta& meta = trains_[*op.train_number];
        if (op.train_wagons) {
            meta.wagons = static_cast<int>(*op.train_wagons);
        }
        if (op.train_capacity) {
            meta.capacity = *op.train_capacity;
        }
    }
}

void SortingHill::ApplyTrainReady_(const OperationInfo& op) {
    if (!op.train_sent) {
        return;
    }

    if (op.train_number) {
        const std::string& train_number = *op.train_number;
        auto it = trains_.find(train_number);
        if (it != trains_.end()) {
            const int pid = it->second.path_id;
            trains_.erase(it);
            FreePath_(pid);
        } else if (op.path_id) {
            FreePath_(*op.path_id);
        }

        sent_trains_count_++;
        return;
    }

    if (op.path_id) {
        FreePath_(*op.path_id);
        sent_trains_count_++;
    }
}

void SortingHill::FreePath_(int path_id) {
    if (path_id < 0 || static_cast<size_t>(path_id) >= paths_.size()) {
        return;
    }

    PathMeta cleared;
    cleared.prepared = false;
    cleared.occupied = false;
    cleared.train_number.clear();

    paths_[static_cast<size_t>(path_id)] = cleared;
}

bool SortingHill::CheckEvent(EventType event) const {
    switch (event) {
        case EventType::kPreparePath: {
            for (const auto& path : paths_) {
                if (!path.occupied && !path.prepared) {
                    return true;
                }
            }
            return false;
        }

        case EventType::kTrainPlanned: {
            for (const auto& path : paths_) {
                if (!path.occupied && path.prepared) {
                    return true;
                }
            }
            return false;
        }

        case EventType::kLocoArrived: {
            for (const auto& [train_number, train_meta] : trains_) {
                (void)train_number;
                if (!train_meta.has_loco) {
                    return true;
                }
            }
            return false;
        }

        case EventType::kTrainReady: {
            // 1) Если есть полный поезд - можно отправлять
            for (const auto& [train_number, train_meta] : trains_) {
                (void)train_number;
                if (train_meta.has_loco && train_meta.capacity > 0 && train_meta.wagons >= train_meta.capacity) {
                    return true;
                }
            }

            // 2) Частичная отправка - только когда входных вагонов больше не будет
            if (IsWagonBuffer() || ring_total_ > 0) {
                return false;
            }

            for (const auto& [train_number, train_meta] : trains_) {
                (void)train_number;
                if (train_meta.has_loco && train_meta.wagons > 0) {
                    return true;
                }
            }
            return false;
        }

        default: {
            return true;
        }
    }
}

void SortingHill::HandleEvent(EventType event) {
    OperationInfo operation_info;
    bool should_apply = false;

    switch (event) {
        case EventType::kShiftStarted: {
            ResetShiftState_();
            for (const auto& handler : handlers_) {
                handler->StartShift(*this);
            }
            return;
        }

        case EventType::kShiftEnded: {
            shift_ending_ = true;

            bool was_sent = false;
            do {
                OperationInfo send_info;
                for (const auto& handler : handlers_) {
                    handler->SendTrain(*this, send_info);
                }
                ApplyOperationInfo_(send_info);
                was_sent = send_info.train_sent;
            } while (was_sent);

            // Освобождаем пути, если остались поезда без локомотива.
            for (const auto& [train_number, train_meta] : trains_) {
                (void)train_number;
                FreePath_(train_meta.path_id);
            }
            trains_.clear();

            shift_ending_ = false;

            for (const auto& handler : handlers_) {
                handler->EndShift(*this);
            }
            return;
        }

        case EventType::kWagonArrived: {
            if (!IsWagonBuffer()) {
                break;
            }

            const Wagon wagon = wagon_buffer_.front();
            for (const auto& handler : handlers_) {
                handler->HandleWagon(*this, wagon, operation_info);
            }

            processed_wagons_count_++;
            {
                const int idx = WagonTypeIndex_(wagon.wagon_type);
                if (idx >= 0) {
                    wagon_buffer_processed_by_type_[static_cast<size_t>(idx)]++;
                }
            }

            PopWagon();
            should_apply = true;
            break;
        }

        case EventType::kLocoArrived: {
            const auto loco_type = RandomGen::GetRandomElem<LocoType>(kLocoType);
            const Locomotive locomotive{loco_type};

            for (const auto& handler : handlers_) {
                handler->HandleLocomotive(*this, locomotive, operation_info);
            }

            arrived_locos_count_++;
            should_apply = true;
            break;
        }

        case EventType::kTrainPlanned: {
            for (const auto& handler : handlers_) {
                handler->AllocatePathForTrain(*this, operation_info);
            }
            should_apply = true;
            break;
        }

        case EventType::kTrainReady: {
            for (const auto& handler : handlers_) {
                handler->SendTrain(*this, operation_info);
            }
            should_apply = true;
            break;
        }

        case EventType::kPreparePath: {
            for (const auto& handler : handlers_) {
                handler->PreparePath(*this, operation_info);
            }
            should_apply = true;
            break;
        }

        default: {
            using namespace std::literals;
            throw std::out_of_range("Неожиданное событие"s);
        }
    }

    if (should_apply) {
        ApplyOperationInfo_(operation_info);
    }
}
