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

void SortingHill::AddWagon(Wagon wagon) {
    wagon_buffer_.push(wagon);
}

void SortingHill::PopWagon() {
    wagon_buffer_.pop();
}

bool SortingHill::IsWagonBuffer() const {
    return !wagon_buffer_.empty();
}


int SortingHill::WagonTypeIndex_(WagonType type) {
    switch (type) {
        case WagonType::kFreight: return 0;
        case WagonType::kPass: return 1;
        case WagonType::kDanger: return 2;
        case WagonType::kEmpty: return 3;
        default: return -1;
    }
}

WagonType SortingHill::TrainNumberToWagonType_(const std::string& train_number) {
    // train_number заканчивается на "Г/Л/О/П" (UTF-8). Берём последний символ как строку.
    if (train_number.size() < 2) return WagonType::kFreight;
    std::string suffix = train_number.substr(train_number.size() - 2);
    if (suffix == "Г") return WagonType::kFreight;
    if (suffix == "Л") return WagonType::kPass;
    if (suffix == "О") return WagonType::kDanger;
    if (suffix == "П") return WagonType::kEmpty;
    return WagonType::kFreight;
}

size_t SortingHill::GetRingWagons(WagonType type) const {
    int idx = WagonTypeIndex_(type);
    return (idx < 0) ? 0 : ring_by_type_[static_cast<size_t>(idx)];
}

size_t SortingHill::GetBufferWagonsLeft(WagonType type) const {
    int idx = WagonTypeIndex_(type);
    if (idx < 0) return 0;
    size_t i = static_cast<size_t>(idx);
    return wagon_buffer_initial_by_type_[i] - wagon_buffer_processed_by_type_[i];
}

size_t SortingHill::GetMissedWagons(WagonType type) const {
    return GetRingWagons(type) + GetBufferWagonsLeft(type);
}

void SortingHill::ResetShiftState_() {
    // вагонный буфер не трогаем: он формируется до начала смены
    paths_.assign(number_of_paths_, PathMeta{});
    trains_.clear();

    shift_ending_ = false;

    ring_total_ = 0;
    ring_max_ = 0;
    ring_by_type_.fill(0);

    // Снимок входного буфера по типам (для "пропущенных" вагонов)
    wagon_buffer_initial_by_type_.fill(0);
    wagon_buffer_processed_by_type_.fill(0);
    {
        auto copy = wagon_buffer_;
        while (!copy.empty()) {
            const Wagon& w = copy.front();
            int idx = WagonTypeIndex_(w.wagon_type);
            if (idx >= 0) wagon_buffer_initial_by_type_[static_cast<size_t>(idx)]++;
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
    // Для корректной статистики по кольцу важно знать дельту.
    const size_t prev_ring_total = ring_total_;

    if (op.ring_total) {
        ring_total_ = *op.ring_total;
    }
    if (op.ring_max) {
        ring_max_ = std::max(ring_max_, *op.ring_max);
    }

    auto apply_ring_drain_for_train = [&]() {
        if (!op.train_number) return;
        if (prev_ring_total <= ring_total_) return;

        const size_t drained = prev_ring_total - ring_total_;
        const WagonType wt = TrainNumberToWagonType_(*op.train_number);
        const int idx = WagonTypeIndex_(wt);
        if (idx < 0) return;

        size_t& cur = ring_by_type_[static_cast<size_t>(idx)];
        cur = (drained >= cur) ? 0 : (cur - drained);
    };

    switch (op.event_type) {
        case EventType::kPreparePath: {
            if (op.success && op.path_id) {
                int pid = *op.path_id;
                if (pid >= 0 && static_cast<size_t>(pid) < paths_.size()) {
                    if (!paths_[pid].prepared) {
                        paths_[pid].prepared = true;
                        prepared_paths_count_++;
                    }
                }
            }
            break;
        }

        case EventType::kTrainPlanned: {
            if (op.success && op.path_id && op.train_number) {
                int pid = *op.path_id;
                const std::string& tn = *op.train_number;

                if (pid >= 0 && static_cast<size_t>(pid) < paths_.size()) {
                    paths_[pid].occupied = true;
                    paths_[pid].train_number = tn;
                    // путь был подготовлен, остаётся prepared=true
                }

                TrainMeta meta;
                meta.path_id = pid;
                if (op.loco_attached && op.loco_capacity) {
                    meta.has_loco = true;
                    meta.capacity = *op.loco_capacity;
                }
                if (op.train_wagons) {
                    meta.wagons = static_cast<int>(*op.train_wagons);
                }
                trains_[tn] = meta;
                planned_trains_count_++;

                // Если локомотив прицепили сразу (из резерва), возможно, выгрузили вагоны с кольца.
                apply_ring_drain_for_train();
            }
            break;
        }

        case EventType::kLocoArrived: {
            if (op.loco_attached && op.train_number) {
                auto& meta = trains_[*op.train_number];
                meta.has_loco = true;
                if (op.loco_capacity) meta.capacity = *op.loco_capacity;
                if (op.train_wagons) meta.wagons = static_cast<int>(*op.train_wagons);
                if (op.path_id) meta.path_id = *op.path_id;

                apply_ring_drain_for_train();
            }
            break;
        }

        case EventType::kWagonArrived: {
            if (!op.success) break;

            if (op.wagon_to_ring && *op.wagon_to_ring) {
                if (op.wagon) {
                    const int idx = WagonTypeIndex_(op.wagon->wagon_type);
                    if (idx >= 0) ring_by_type_[static_cast<size_t>(idx)]++;
                }

                // если runtime не прислал ring_total, обновим сами
                if (!op.ring_total) {
                    ring_total_++;
                    ring_max_ = std::max(ring_max_, ring_total_);
                }
            } else {
                if (op.train_number) {
                    auto& meta = trains_[*op.train_number];
                    if (op.train_wagons) meta.wagons = static_cast<int>(*op.train_wagons);
                    if (op.train_capacity) meta.capacity = *op.train_capacity;
                }
            }
            break;
        }

        case EventType::kTrainReady: {
            if (op.train_sent && op.train_number) {
                const std::string& tn = *op.train_number;
                auto it = trains_.find(tn);
                if (it != trains_.end()) {
                    int pid = it->second.path_id;
                    trains_.erase(it);

                    if (pid >= 0 && static_cast<size_t>(pid) < paths_.size()) {
                        paths_[pid].occupied = false;
                        paths_[pid].prepared = false;
                        paths_[pid].train_number.clear();
                    }
                } else {
                    // если поезда в карте нет (зеркало разошлось), попробуем освободить путь по op.path_id
                    if (op.path_id) {
                        int pid = *op.path_id;
                        if (pid >= 0 && static_cast<size_t>(pid) < paths_.size()) {
                            paths_[pid].occupied = false;
                            paths_[pid].prepared = false;
                            paths_[pid].train_number.clear();
                        }
                    }
                }
                sent_trains_count_++;
            }
            break;
        }

        default:
            break;
    }
}

/* Проверка возможности события. */
bool SortingHill::CheckEvent(EventType event) const {
    switch (event) {
        case EventType::kPreparePath: {
            // есть свободный НЕподготовленный путь
            for (const auto& p : paths_) {
                if (!p.occupied && !p.prepared) return true;
            }
            return false;
        }
        case EventType::kTrainPlanned: {
            // есть подготовленный свободный путь
            for (const auto& p : paths_) {
                if (!p.occupied && p.prepared) return true;
            }
            return false;
        }
        case EventType::kLocoArrived: {
            // Разрешаем, если есть поезд без локомотива.
            // Если поездов нет вовсе - разрешаем тоже (локомотив уйдёт в резерв).
            if (trains_.empty()) return true;
            for (const auto& [tn, tr] : trains_) {
                (void)tn;
                if (!tr.has_loco) return true;
            }
            return false;
        }
        case EventType::kTrainReady: {
            // 1) Если есть полный поезд - можно отправлять
            for (const auto& [tn, tr] : trains_) {
                (void)tn;
                if (tr.has_loco && tr.capacity > 0 && tr.wagons >= tr.capacity) return true;
            }

            // 2) Частичная отправка - только когда входных вагонов больше не будет
            if (IsWagonBuffer()) return false;
            if (ring_total_ > 0) return false;

            for (const auto& [tn, tr] : trains_) {
                (void)tn;
                if (tr.has_loco && tr.wagons > 0) return true;
            }
            return false;
        }
        default:
            return true;
    }
}

void SortingHill::HandleEvent(EventType event) {
    switch (event) {
        case EventType::kShiftStarted: {
            ResetShiftState_();
            for (const auto& handler : handlers_) {
                handler->StartShift(*this);
            }
            break;
        }
        case EventType::kShiftEnded: {
            // В конце смены освобождаем все пути.
            // Поезда с локомотивом можно отправлять даже без вагонов.
            shift_ending_ = true;

            // 1) Отправляем все поезда с локомотивами (в режиме force).
            while (true) {
                OperationInfo operation_info;
                for (const auto& handler : handlers_) {
                    handler->SendTrain(*this, operation_info);
                }
                ApplyOperationInfo_(operation_info);

                if (!operation_info.train_sent) {
                    break; // больше нечего отправлять
                }
            }

            // 2) Освобождаем пути, если остались поезда без локомотива.
            for (const auto& [tn, tr] : trains_) {
                (void)tn;
                int pid = tr.path_id;
                if (pid >= 0 && static_cast<size_t>(pid) < paths_.size()) {
                    paths_[pid].occupied = false;
                    paths_[pid].prepared = false;
                    paths_[pid].train_number.clear();
                }
            }
            trains_.clear();

            shift_ending_ = false;

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
            processed_wagons_count_++;
            {
                int idx = WagonTypeIndex_(wagon.wagon_type);
                if (idx >= 0) wagon_buffer_processed_by_type_[static_cast<size_t>(idx)]++;
            }
            ApplyOperationInfo_(operation_info);
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
            arrived_locos_count_++;
            ApplyOperationInfo_(operation_info);
            break;
        }
        case EventType::kTrainPlanned: {
            OperationInfo operation_info;
            for (const auto& handler : handlers_) {
                handler->AllocatePathForTrain(*this, operation_info);
            }
            ApplyOperationInfo_(operation_info);
            break;
        }
        case EventType::kTrainReady: {
            OperationInfo operation_info;
            for (const auto& handler : handlers_) {
                handler->SendTrain(*this, operation_info);
            }
            ApplyOperationInfo_(operation_info);
            break;
        }
        case EventType::kPreparePath: {
            OperationInfo operation_info;
            for (const auto& handler : handlers_) {
                handler->PreparePath(*this, operation_info);
            }
            ApplyOperationInfo_(operation_info);
            break;
        }
        default: {
            using namespace std::literals;
            throw std::out_of_range("Неожиданное событие"s);
        }
    }
}
