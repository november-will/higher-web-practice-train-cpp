#include "sorting_reporter.h"
#include "sorting_hill.h"

#include <iostream>

using namespace std::literals;

SortingReporterImpl::~SortingReporterImpl() {
}

void SortingReporterImpl::StartShift(const SortingHill& sorting_hill) {
    std::cout << "Начало рабочей смены"s << std::endl;
    std::cout << "Очередь вагонов: "s << sorting_hill.GetNumberOfWagBuffer() << std::endl;
}

void SortingReporterImpl::EndShift(const SortingHill& sorting_hill) {
    std::cout << "Рабочая смена окончена"s << std::endl;
    std::cout << std::endl;
    std::cout << "===== ОТЧЁТ О СМЕНЕ ====="s << std::endl;
    std::cout << "Подготовлено путей:                    "s << sorting_hill.GetPreparedPathsCount() << std::endl;
    std::cout << "Запланировано поездов:                 "s << sorting_hill.GetPlannedTrainsCount() << std::endl;
    std::cout << "Прибыло локомотивов:                   "s << sorting_hill.GetArrivedLocosCount() << std::endl;
    std::cout << "Обработано вагонов (с повторами):      "s << sorting_hill.GetProcessedWagonsCount() << std::endl;
    std::cout << "Отправлено поездов:                    "s << sorting_hill.GetSentTrainsCount() << std::endl;
    std::cout << "Осталось вагонов в буфере:             "s
              << (sorting_hill.GetNumberOfWagBuffer() + sorting_hill.GetRingTotal()) << std::endl;
    std::cout << "Макс. заполнение кольцевого пути:      "s << sorting_hill.GetRingMax() << std::endl;
    std::cout << "Пропущено вагонов (Г):                 "s << sorting_hill.GetMissedWagons(WagonType::kFreight) << std::endl;
    std::cout << "Пропущено вагонов (Л):                 "s << sorting_hill.GetMissedWagons(WagonType::kPass) << std::endl;
    std::cout << "Пропущено вагонов (О):                 "s << sorting_hill.GetMissedWagons(WagonType::kDanger) << std::endl;
    std::cout << "Пропущено вагонов (П):                 "s << sorting_hill.GetMissedWagons(WagonType::kEmpty) << std::endl;
    std::cout << "=========================="s << std::endl;
}

// Репортёр не влияет на работу станции: он лишь печатает отчёт в конце смены.
// Остальные события игнорируются.
void SortingReporterImpl::PreparePath(SortingHill&, OperationInfo&) {
}

void SortingReporterImpl::AllocatePathForTrain(SortingHill&, OperationInfo&) {
}

void SortingReporterImpl::HandleLocomotive(SortingHill&, const Locomotive&, OperationInfo&) {
}

void SortingReporterImpl::HandleWagon(SortingHill&, const Wagon&, OperationInfo&) {
}

void SortingReporterImpl::SendTrain(SortingHill&, OperationInfo&) {
}
