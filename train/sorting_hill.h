#pragma once

#include "handler_interface.h"
#include "enums.h"
#include "common.h"

#include <array>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

class SortingHill {
public:
    explicit SortingHill(size_t number_of_paths,
                         std::vector<std::unique_ptr<SortingHandler>> handlers);

    void AddWagon(Wagon wagon);
    bool IsWagonBuffer() const;
    size_t GetNumberOfPaths() const;
    size_t GetNumberOfWagBuffer() const;

    // Проверка возможности события. Использует внутреннее состояние станции,
    // которое обновляется по OperationInfo после каждого события.
    bool CheckEvent(EventType event) const;

    void HandleEvent(EventType event);

    // true, если сейчас выполняется завершение смены
    bool IsShiftEnding() const {
        return shift_ending_;
    }

    // Метрики для отчёта
    size_t GetPreparedPathsCount() const {
        return prepared_paths_count_;
    }

    size_t GetPlannedTrainsCount() const {
        return planned_trains_count_;
    }

    size_t GetArrivedLocosCount() const {
        return arrived_locos_count_;
    }

    size_t GetProcessedWagonsCount() const {
        return processed_wagons_count_;
    }

    size_t GetSentTrainsCount() const {
        return sent_trains_count_;
    }

    size_t GetRingTotal() const {
        return ring_total_;
    }

    size_t GetRingMax() const {
        return ring_max_;
    }
    
    // Пропущенные вагоны - те, что не попали ни в один отправленный поезд к окончанию смены.
    // Пропущенные вагоны = оставшиеся во входном буфере + оставшиеся на кольцевом пути.
    size_t GetMissedWagons(WagonType type) const;
    size_t GetRingWagons(WagonType type) const;
    size_t GetBufferWagonsLeft(WagonType type) const;

private:
    struct PathMeta {
        bool prepared = false;
        bool occupied = false;
        std::string train_number;
    };

    struct TrainMeta {
        bool has_loco = false;
        int capacity = 0;
        int wagons = 0;
        int path_id = -1;
    };

private:
    std::vector<std::unique_ptr<SortingHandler>> handlers_;
    const size_t number_of_paths_;

    std::queue<Wagon> wagon_buffer_;

    // Зеркальная модель станции (для CheckEvent и отчёта)
    std::vector<PathMeta> paths_;
    std::unordered_map<std::string, TrainMeta> trains_;

    bool shift_ending_ = false;

    size_t ring_total_ = 0;
    size_t ring_max_ = 0;

    std::array<size_t, 4> ring_by_type_{};
    std::array<size_t, 4> wagon_buffer_initial_by_type_{};
    std::array<size_t, 4> wagon_buffer_processed_by_type_{};

    size_t prepared_paths_count_ = 0;
    size_t planned_trains_count_ = 0;
    size_t arrived_locos_count_ = 0;
    size_t processed_wagons_count_ = 0;
    size_t sent_trains_count_ = 0;

private:
    void PopWagon();

    void ResetShiftState_();
    void ApplyOperationInfo_(const OperationInfo& operation_info);

    static int WagonTypeIndex_(WagonType type);
    static WagonType TrainNumberToWagonType_(const std::string& train_number);
};
