#pragma once

#include "handler_interface.h"
#include "enums.h"

class SortingReporterImpl : public SortingHandler {
public:
    ~SortingReporterImpl();

    void StartShift(const SortingHill& sorting_hill) override;
    void EndShift(const SortingHill& sorting_hill) override;
    void PreparePath(SortingHill& sorting_hill, OperationInfo& operation_info) override;
    void AllocatePathForTrain(SortingHill& sorting_hill, OperationInfo& operation_info) override;
    void HandleLocomotive(SortingHill& sorting_hill, const Locomotive& locomotive, OperationInfo& operation_info) override;
    void HandleWagon(SortingHill& sorting_hill, const Wagon& wagon, OperationInfo& operation_info) override;
    void SendTrain(SortingHill& sorting_hill, OperationInfo& operation_info) override;
};
