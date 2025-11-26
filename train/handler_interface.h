#pragma once

#include "common.h"

class SortingHill;

class SortingHandler {
public:
    virtual ~SortingHandler() = default;

    /* Начало смены. */
    virtual void StartShift(const SortingHill& sorting_hill) = 0;

    /* Окончание смены. */
    virtual void EndShift(const SortingHill& sorting_hill) = 0;

    /* Запрос на подготовку пути. */
    virtual void PreparePath(SortingHill& sorting_hill, OperationInfo& operation_info) = 0;

    /* Запрос на размещение поезда на пути. */
    virtual void AllocatePathForTrain(SortingHill& sorting_hill, OperationInfo& operation_info) = 0;

    /* Обработчик поступающих локомотивов. */
    virtual void HandleLocomotive(SortingHill& sorting_hill, const Locomotive& locomotive, OperationInfo& operation_info) = 0;

    /* Обработчик поступающих вагонов. */
    virtual void HandleWagon(SortingHill& sorting_hill, const Wagon& wagon_info, OperationInfo& operation_info) = 0;

    /* Запрос на отправку готового поезда. */
    virtual void SendTrain(SortingHill& sorting_hill, OperationInfo& operation_info) = 0;
};
