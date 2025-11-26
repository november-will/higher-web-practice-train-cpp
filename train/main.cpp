#include "sorting_hill.h"
#include "enums.h"
#include "random.h"
#include "sorting_operator.h"
#include "sorting_reporter.h"
#include "common.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

int main() {
    using namespace std::literals;

    size_t number_of_paths = RandomGen::GetInRange(2, 15);
    std::vector<std::unique_ptr<SortingHandler>> handlers;
    handlers.push_back(std::make_unique<SortingOperatorImpl>());
    handlers.push_back(std::make_unique<SortingReporterImpl>());

    SortingHill sorting_hill(number_of_paths, std::move(handlers));

    int wagon_left = RandomGen::GetInRange(1024, 4095);
    for (int i = 0; i < wagon_left; ++i) {
        int wagon_num = RandomGen::GetInRange(0, 99999999);
        WagonType wagon_type = RandomGen::GetRandomElem<WagonType>(kWagonType);
        sorting_hill.AddWagon({wagon_num, wagon_type});
    }

    sorting_hill.HandleEvent(EventType::kShiftStarted);

    while (sorting_hill.IsWagonBuffer()) {
        try {
            auto next_event = RandomGen::GetRandomElem<EventType>(kEventsBalanced);
            if (sorting_hill.CheckEvent(next_event)) {
                std::cout << "Команда дежурного: "s << next_event << std::endl;
                sorting_hill.HandleEvent(next_event);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        } catch (const std::out_of_range& error_message) {
            std::cerr << "Произошла ошибка обработки: "s << error_message.what() << std::endl;
        } catch (const std::exception& exc) {
            std::cerr << "Общая ошибка: "s << exc.what() << std::endl;
        }
    }
    sorting_hill.HandleEvent(EventType::kShiftEnded);
}
