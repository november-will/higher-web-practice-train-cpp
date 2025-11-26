#include "sorting_reporter.h"
#include "sorting_hill.h"

#include <iostream>

using namespace std::literals;

SortingReporterImpl::~SortingReporterImpl() {}

void SortingReporterImpl::StartShift(const SortingHill& sorting_hill) {
    std::cout << "Начало рабочей смены"s << std::endl;
    std::cout << "Очередь вагонов: "s << sorting_hill.GetNumberOfWagBuffer() << std::endl;
}

void SortingReporterImpl::EndShift(const SortingHill& sorting_hill) {
    std::cout << "Рабочая смена окончена"s << std::endl;
    std::cout << std::endl;
    std::cout << "===== ОТЧЁТ О СМЕНЕ ====="s << std::endl;
    std::cout << "Подготовлено путей:                    "s << -1 << std::endl;
    std::cout << "Запланировано поездов:                 "s << -1 << std::endl;
    std::cout << "Прибыло локомотивов:                   "s << -1 << std::endl;
    std::cout << "Обработано вагонов (с повторами):      "s << -1 << std::endl;
    std::cout << "Отправлено поездов:                    "s << -1 << std::endl;
    std::cout << "Осталось вагонов в буфере:             "s << -1 << std::endl;
    std::cout << "=========================="s << std::endl;
}


void SortingReporterImpl::PreparePath(SortingHill& sorting_hill, OperationInfo& operation_info) {

}

void SortingReporterImpl::AllocatePathForTrain(SortingHill& sorting_hill, OperationInfo& operation_info) {

}

void SortingReporterImpl::HandleLocomotive(SortingHill& sorting_hill, const Locomotive& locomotive, OperationInfo& operation_info) {

}

void SortingReporterImpl::HandleWagon(SortingHill& sorting_hill, const Wagon& wagon, OperationInfo& operation_info) {

}

void SortingReporterImpl::SendTrain(SortingHill& sorting_hill, OperationInfo& operation_info) {

}
