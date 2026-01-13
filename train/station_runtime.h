#pragma once

#include "sorting_hill.h"
#include "common.h"

#include <algorithm>
#include <array>
#include <deque>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// Бизнес-логика "сортировочного оператора".
// Хранит состояние станции и выполняет операции из SortingHandler.
class StationRuntime {
public:
    void StartShift(size_t number_of_paths) {
        paths_.assign(number_of_paths, PathState{});
        trains_.clear();
        train_order_.clear();
        free_locos_.clear();
        for (auto& q : ring_) q.clear();
        ring_total_ = 0;
        ring_max_ = 0;
        next_train_id_ = 1;
        kind_rotation_ = 0;
        last_sent_train_.clear();
    }

    // Окончание смены: приводим внутренние структуры в согласованное состояние.
    void EndShift() {
        trains_.clear();
        train_order_.clear();
        free_locos_.clear();

        for (auto& p : paths_) {
            p.prepared = false;
            p.train_id = -1;
        }

        for (auto& q : ring_) {
            q.clear();
        }

        ring_total_ = 0;
        // ring_max_ - метрика смены. Сбрасывается в StartShift().
        last_sent_train_.clear();
    }

    bool PreparePath(OperationInfo* op) {
        if (op) ResetOp_(*op, EventType::kPreparePath);

        for (size_t i = 0; i < paths_.size(); ++i) {
            if (!paths_[i].prepared && paths_[i].train_id == -1) {
                paths_[i].prepared = true;
                if (op) {
                    op->success = true;
                    op->path_id = static_cast<int>(i);
                    op->message = "Путь #" + std::to_string(i) + " подготовлен";
                }
                return true;
            }
        }

        if (op) {
            op->success = false;
            op->message = "Нет свободных путей для подготовки";
        }
        return false;
    }

    bool AllocateTrain(OperationInfo* op) {
        if (op) ResetOp_(*op, EventType::kTrainPlanned);

        int path_id = -1;
        for (size_t i = 0; i < paths_.size(); ++i) {
            if (paths_[i].prepared && paths_[i].train_id == -1) {
                path_id = static_cast<int>(i);
                break;
            }
        }
        if (path_id == -1) {
            if (op) {
                op->success = false;
                op->message = "Нет подготовленных свободных путей для поезда";
            }
            return false;
        }

        TrainKind kind = ChooseKindForNewTrain_();
        int id = next_train_id_++;

        TrainState tr;
        tr.id = id;
        tr.kind = kind;
        tr.path_id = path_id;
        tr.train_number = MakeTrainNumber_(id, kind);

        paths_[path_id].train_id = id;

        trains_.emplace(id, tr);
        train_order_.push_back(id);

        // Если есть свободный локомотив - прицепляем сразу
        if (!free_locos_.empty()) {
            Locomotive loco = free_locos_.front();
            free_locos_.pop_front();
            AttachLocoToTrain_(id, loco, /*op=*/op);
        }

        if (op) {
            op->success = true;
            op->path_id = path_id;
            op->train_number = trains_.at(id).train_number;
            op->message = "Запланирован поезд " + trains_.at(id).train_number + " на пути #" + std::to_string(path_id);
        }
        return true;
    }

    bool HandleLocomotive(const Locomotive& loco, OperationInfo* op) {
        if (op) {
            ResetOp_(*op, EventType::kLocoArrived);
            op->loco_type = loco.loco_type;
        }

        int train_id = FindOldestTrainWithoutLoco_();
        if (train_id == -1) {
            free_locos_.push_back(loco);
            if (op) {
                op->success = true;
                op->loco_reserved = true;
                op->message = "Локомотив отправлен в резерв (нет поездов без локомотива)";
            }
            return true;
        }

        AttachLocoToTrain_(train_id, loco, op);

        if (op) {
            op->success = true;
            op->train_number = trains_.at(train_id).train_number;
            op->loco_attached = true;
            op->loco_capacity = trains_.at(train_id).capacity;
            op->train_capacity = trains_.at(train_id).capacity;
            op->train_wagons = trains_.at(train_id).wagons.size();
            op->message = "Локомотив прицеплен к поезду " + trains_.at(train_id).train_number;
        }
        return true;
    }

    bool HandleWagon(const Wagon& wagon, OperationInfo* op) {
        if (op) {
            ResetOp_(*op, EventType::kWagonArrived);
            op->wagon = wagon;
        }

        TrainKind kind = WagonKind_(wagon.wagon_type);
        int train_id = FindOldestTrainWithLocoAndSpace_(kind);

        if (train_id == -1) {
            ring_[KindIndex_(kind)].push_back(wagon);
            ++ring_total_;
            ring_max_ = std::max(ring_max_, ring_total_);

            if (op) {
                op->success = true;
                op->wagon_to_ring = true;
                op->ring_total = ring_total_;
                op->ring_max = ring_max_;
                op->message = "Вагон отправлен на кольцевой путь";
            }
            return true;
        }

        TrainState& tr = trains_.at(train_id);
        tr.wagons.push_back(wagon);

        if (op) {
            op->success = true;
            op->wagon_to_ring = false;
            op->train_number = tr.train_number;
            op->train_wagons = tr.wagons.size();
            op->train_capacity = tr.capacity;
            op->message = "Вагон прицеплен к поезду " + tr.train_number;
        }
        return true;
    }

    // Отправка: приоритет - полный поезд. Частичный - только если входящих вагонов уже не будет.
    bool SendTrain(const SortingHill& hill, bool force, OperationInfo* op) {
        if (op) ResetOp_(*op, EventType::kTrainReady);

        const bool no_more_incoming = (hill.GetNumberOfWagBuffer() == 0);
        const bool allow_partial = force || (no_more_incoming && ring_total_ == 0);

        // 1) полный поезд
        auto full_it = FindInOrder_([&](const TrainState& tr) {
            return tr.has_loco && tr.capacity > 0 && tr.wagons.size() >= static_cast<size_t>(tr.capacity);
        });
        if (full_it != train_order_.end()) {
            int id = *full_it;
            SendTrainById_(id);
            if (op) {
                op->success = true;
                op->train_sent = true;
                op->train_number = last_sent_train_;
                op->message = "Отправлен полный поезд " + last_sent_train_;
            }
            return true;
        }

        // 2) частичный/пустой
        if (allow_partial) {
            auto part_it = FindInOrder_([&](const TrainState& tr) {
                if (!tr.has_loco) return false;
                if (force) return true;
                return !tr.wagons.empty();
            });
            if (part_it != train_order_.end()) {
                int id = *part_it;
                SendTrainById_(id);
                if (op) {
                    op->success = true;
                    op->train_sent = true;
                    op->train_number = last_sent_train_;
                    op->message = "Отправлен поезд " + last_sent_train_;
                }
                return true;
            }
        }

        if (op) {
            op->success = false;
            op->message = "Нет поездов для отправки";
        }
        return false;
    }

    size_t RingTotal() const {
        return ring_total_;
    }

    size_t RingMax() const {
        return ring_max_;
    }

private:
    enum class TrainKind { kFreight = 0, kPass = 1, kDanger = 2, kEmpty = 3 };

    struct PathState {
        bool prepared = false;
        int train_id = -1;
    };

    struct TrainState {
        int id = 0;
        TrainKind kind = TrainKind::kFreight;
        std::string train_number;
        int path_id = -1;

        bool has_loco = false;
        int capacity = 0;
        std::vector<Wagon> wagons;
    };

private:
    std::vector<PathState> paths_;

    std::unordered_map<int, TrainState> trains_;
    std::deque<int> train_order_;

    std::array<std::deque<Wagon>, 4> ring_{};
    size_t ring_total_ = 0;
    size_t ring_max_ = 0;

    std::deque<Locomotive> free_locos_;

    int next_train_id_ = 1;
    int kind_rotation_ = 0;
    std::string last_sent_train_;

private:
    static void ResetOp_(OperationInfo& op, EventType type) {
        op = OperationInfo{};
        op.event_type = type;
    }

    static int GetLocoCapacity_(LocoType t) {
        switch (t) {
            case LocoType::kElectro16: return 16;
            case LocoType::kElectro32: return 32;
            case LocoType::kDiesel24:  return 24;
            case LocoType::kDiesel64:  return 64;
            default: return 0;
        }
    }

    static TrainKind WagonKind_(WagonType wt) {
        switch (wt) {
            case WagonType::kFreight: return TrainKind::kFreight;
            case WagonType::kPass:    return TrainKind::kPass;
            case WagonType::kDanger:  return TrainKind::kDanger;
            case WagonType::kEmpty:   return TrainKind::kEmpty;
            default:                  return TrainKind::kFreight;
        }
    }

    static int KindIndex_(TrainKind k) {
        return static_cast<int>(k);
    }

    static std::string KindSuffix_(TrainKind k) {
        switch (k) {
            case TrainKind::kFreight: return "Г";
            case TrainKind::kPass:    return "Л";
            case TrainKind::kDanger:  return "О";
            case TrainKind::kEmpty:   return "П";
            default: return "";
        }
    }

    static std::string MakeTrainNumber_(int id, TrainKind kind) {
        std::ostringstream out;
        out << std::setw(4) << std::setfill('0') << id << KindSuffix_(kind);
        return out.str();
    }

    TrainKind ChooseKindForNewTrain_() {
        // Если в кольце уже есть вагоны - планируем поезд под самый большой "хвост".
        size_t best_sz = 0;
        int best_k = -1;
        for (int k = 0; k < 4; ++k) {
            size_t sz = ring_[k].size();
            if (sz > best_sz) {
                best_sz = sz;
                best_k = k;
            }
        }
        if (best_k != -1) {
            return static_cast<TrainKind>(best_k);
        }

        // иначе - ротация
        TrainKind k = static_cast<TrainKind>(kind_rotation_ % 4);
        ++kind_rotation_;
        return k;
    }

    int FindOldestTrainWithoutLoco_() const {
        for (int id : train_order_) {
            auto it = trains_.find(id);
            if (it != trains_.end() && !it->second.has_loco) return id;
        }
        return -1;
    }

    int FindOldestTrainWithLocoAndSpace_(TrainKind kind) const {
        for (int id : train_order_) {
            auto it = trains_.find(id);
            if (it == trains_.end()) continue;
            const TrainState& tr = it->second;
            if (tr.kind != kind) continue;
            if (!tr.has_loco) continue;
            if (tr.capacity <= 0) continue;
            if (tr.wagons.size() < static_cast<size_t>(tr.capacity)) return id;
        }
        return -1;
    }

    void AttachLocoToTrain_(int train_id, const Locomotive& loco, OperationInfo* op) {
        TrainState& tr = trains_.at(train_id);
        tr.has_loco = true;
        tr.capacity = GetLocoCapacity_(loco.loco_type);

        // Выгружаем вагоны из кольца в поезд
        auto& q = ring_[KindIndex_(tr.kind)];
        while (!q.empty() && tr.wagons.size() < static_cast<size_t>(tr.capacity)) {
            tr.wagons.push_back(q.front());
            q.pop_front();
            --ring_total_;
        }

        if (op) {
            op->loco_attached = true;
            op->loco_capacity = tr.capacity;
            op->train_capacity = tr.capacity;
            op->train_wagons = tr.wagons.size();
            op->ring_total = ring_total_;
            op->ring_max = ring_max_;
        }
    }

    template <class Pred>
    auto FindInOrder_(Pred pred) -> std::deque<int>::iterator {
        for (auto it = train_order_.begin(); it != train_order_.end(); ++it) {
            auto tr_it = trains_.find(*it);
            if (tr_it == trains_.end()) continue;
            if (pred(tr_it->second)) return it;
        }
        return train_order_.end();
    }

    void FreePathForTrain_(const TrainState& tr) {
        if (tr.path_id >= 0 && tr.path_id < static_cast<int>(paths_.size())) {
            paths_[tr.path_id].train_id = -1;
            // после отправки путь снова "не подготовлен"
            paths_[tr.path_id].prepared = false;
        }
    }

    void SendTrainById_(int train_id) {
        auto it = trains_.find(train_id);
        if (it == trains_.end()) return;

        last_sent_train_ = it->second.train_number;
        FreePathForTrain_(it->second);
        trains_.erase(it);

        auto qit = std::find(train_order_.begin(), train_order_.end(), train_id);
        if (qit != train_order_.end()) train_order_.erase(qit);
    }
};
