#include "sorting_operator.h"

#include <iostream>

using namespace std::literals;

SortingOperatorImpl::~SortingOperatorImpl() {
}

void SortingOperatorImpl::StartShift(const SortingHill& sorting_hill) {
    runtime_.StartShift(sorting_hill.GetNumberOfPaths());
}

void SortingOperatorImpl::EndShift(const SortingHill&) {
    runtime_.EndShift();
}

void SortingOperatorImpl::PreparePath(SortingHill&, OperationInfo& operation_info) {
    runtime_.PreparePath(&operation_info);
}

void SortingOperatorImpl::AllocatePathForTrain(SortingHill&, OperationInfo& operation_info) {
    runtime_.AllocateTrain(&operation_info);
}

void SortingOperatorImpl::HandleLocomotive(SortingHill&, const Locomotive& locomotive, OperationInfo& operation_info) {
    runtime_.HandleLocomotive(locomotive, &operation_info);
}

void SortingOperatorImpl::HandleWagon(SortingHill&, const Wagon& wagon, OperationInfo& operation_info) {
    runtime_.HandleWagon(wagon, &operation_info);
}

void SortingOperatorImpl::SendTrain(SortingHill& sorting_hill, OperationInfo& operation_info) {
    runtime_.SendTrain(sorting_hill, /*force=*/sorting_hill.IsShiftEnding(), &operation_info);
}
