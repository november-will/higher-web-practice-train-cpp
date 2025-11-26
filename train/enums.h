#pragma once

enum class EventType {
    kShiftStarted, // начало работ
    kPreparePath,  // готовьте путь
    kTrainPlanned, // формируйте состав
    kLocoArrived,  // подача локомотива
    kWagonArrived, // вагон на сортировку
    kTrainReady,   // отправляйте поезд
    kShiftEnded,   // окончание работ
};

enum class TrainType {
    kFreight, // Г
    kPass,    // Л
    kDanger,  // О
};

enum class WagonType {
    kFreight, // Г
    kPass,    // Л
    kDanger,  // О
    kEmpty,   // П
};

enum class LocoType {
    kElectro16, // ЭВЛ-16
    kElectro32, // ЭВЛ-32
    kDiesel24,  // ДЛ-24
    kDiesel64,  // ТДЛ-64
};
