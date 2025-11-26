#pragma once

#include "handler_interface.h"
#include "enums.h"
#include "common.h"

#include <memory>
#include <optional>
#include <queue>
#include <vector>

class SortingHill {
public:
    explicit SortingHill(size_t number_of_paths,
                         std::vector<std::unique_ptr<SortingHandler>> handlers);

    void AddWagon(Wagon wagon);
    bool IsWagonBuffer() const;
    size_t GetNumberOfPaths() const;
    size_t GetNumberOfWagBuffer() const;
    bool CheckEvent(EventType event) const;
    void HandleEvent(EventType event);

private:
    std::vector<std::unique_ptr<SortingHandler>> handlers_;
    const size_t number_of_paths_;
    std::queue<Wagon> wagon_buffer_;
    int train_index_ = 1;

private:
    void PopWagon();
};
