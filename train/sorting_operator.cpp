#include "sorting_operator.h"

SortingOperatorImpl::~SortingOperatorImpl() {}

void SortingOperatorImpl::StartShift(const SortingHill&) {}

void SortingOperatorImpl::EndShift(const SortingHill&) {}

void SortingOperatorImpl::PreparePath(SortingHill& sorting_hill, OperationInfo& operation_info) {

}

void SortingOperatorImpl::AllocatePathForTrain(SortingHill& sorting_hill, OperationInfo& operation_info) {

}

void SortingOperatorImpl::HandleLocomotive(SortingHill& sorting_hill, const Locomotive& locomotive, OperationInfo& operation_info) {

}

void SortingOperatorImpl::HandleWagon(SortingHill& sorting_hill, const Wagon& wagon, OperationInfo& operation_info) {

}

void SortingOperatorImpl::SendTrain(SortingHill& sorting_hill, OperationInfo& operation_info) {

}
