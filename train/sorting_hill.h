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

    void AddWagon(const Wagon& wagon);
    bool IsWagonBuffer() const;
    size_t GetNumberOfPaths() const;
    size_t GetNumberOfWagBuffer() const;

    bool CheckEvent(EventType event) const;
    void HandleEvent(EventType event);

    bool IsShiftEnding() const;

    // Метрики для отчёта
    size_t GetPreparedPathsCount() const;
    size_t GetPlannedTrainsCount() const;
    size_t GetArrivedLocosCount() const;
    size_t GetProcessedWagonsCount() const;
    size_t GetSentTrainsCount() const;

    size_t GetRingTotal() const;
    size_t GetRingMax() const;

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

    void ApplyRingDrainForTrain_(size_t prev_ring_total, const OperationInfo& operation_info);
    void ApplyPreparePath_(const OperationInfo& operation_info);
    void ApplyTrainPlanned_(const OperationInfo& operation_info);
    void ApplyLocoArrived_(const OperationInfo& operation_info);
    void ApplyWagonArrived_(const OperationInfo& operation_info);
    void ApplyTrainReady_(const OperationInfo& operation_info);

    void FreePath_(int path_id);

    static int WagonTypeIndex_(WagonType type);
    static WagonType TrainNumberToWagonType_(const std::string& train_number);
};
