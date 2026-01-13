#include <gtest/gtest.h>

#include "station_runtime.h"
#include "sorting_hill.h"
#include "sorting_operator.h"
#include "handler_interface.h"
#include "common.h"

#include <memory>
#include <string>
#include <vector>

static bool EndsWith(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static SortingHill MakeHill(size_t paths) {
    std::vector<std::unique_ptr<SortingHandler>> handlers;
    return SortingHill(paths, std::move(handlers));
}

static Wagon W(int num, WagonType t) {
    return Wagon{num, t};
}

static Locomotive L(LocoType t) {
    return Locomotive{t};
}

TEST(StationRuntime, AllocateRequiresPreparedPath) {
    auto hill = MakeHill(2);
    StationRuntime rt;
    rt.StartShift(hill.GetNumberOfPaths());

    OperationInfo op;
    bool ok = rt.AllocateTrain(&op);

    EXPECT_FALSE(ok);
    EXPECT_FALSE(op.success);
}

TEST(StationRuntime, PrepareThenAllocate_GeneratesTrainNumber) {
    auto hill = MakeHill(2);
    StationRuntime rt;
    rt.StartShift(hill.GetNumberOfPaths());

    OperationInfo p;
    ASSERT_TRUE(rt.PreparePath(&p));
    ASSERT_TRUE(p.success);
    ASSERT_TRUE(p.path_id.has_value());
    EXPECT_EQ(*p.path_id, 0);

    OperationInfo a;
    ASSERT_TRUE(rt.AllocateTrain(&a));
    ASSERT_TRUE(a.success);
    ASSERT_TRUE(a.train_number.has_value());

    // На пустом кольце первый поезд по ротации = грузовой -> "Г"
    EXPECT_TRUE(EndsWith(*a.train_number, "Г"));
    EXPECT_EQ(a.train_number->substr(0, 4), "0001");
}

TEST(StationRuntime, WagonGoesToRingWhenNoTrainWithLoco) {
    auto hill = MakeHill(1);
    StationRuntime rt;
    rt.StartShift(hill.GetNumberOfPaths());

    OperationInfo op;
    ASSERT_TRUE(rt.HandleWagon(W(1, WagonType::kFreight), &op));
    ASSERT_TRUE(op.success);
    ASSERT_TRUE(op.wagon_to_ring.has_value());
    EXPECT_TRUE(*op.wagon_to_ring);

    EXPECT_EQ(rt.RingTotal(), 1u);
    EXPECT_EQ(rt.RingMax(), 1u);
}

TEST(StationRuntime, AllocateChoosesLargestRingKind) {
    auto hill = MakeHill(2);
    StationRuntime rt;
    rt.StartShift(hill.GetNumberOfPaths());

    // Без поездов все вагоны пойдут в кольцо
    rt.HandleWagon(W(10, WagonType::kDanger), nullptr);
    rt.HandleWagon(W(11, WagonType::kDanger), nullptr);
    rt.HandleWagon(W(12, WagonType::kDanger), nullptr);
    rt.HandleWagon(W(20, WagonType::kPass), nullptr);

    EXPECT_EQ(rt.RingTotal(), 4u);

    ASSERT_TRUE(rt.PreparePath(nullptr));

    OperationInfo a;
    ASSERT_TRUE(rt.AllocateTrain(&a));
    ASSERT_TRUE(a.train_number.has_value());

    // В кольце больше всего опасных -> поезд "О"
    EXPECT_TRUE(EndsWith(*a.train_number, "О"));
    EXPECT_EQ(a.train_number->substr(0, 4), "0001");
}

TEST(StationRuntime, LocoAttachesAndDrainsRingThenPartialSendWhenNoIncoming) {
    auto hill = MakeHill(1);
    StationRuntime rt;
    rt.StartShift(hill.GetNumberOfPaths());

    // 5 грузовых вагонов -> в кольцо
    for (int i = 0; i < 5; ++i) rt.HandleWagon(W(100 + i, WagonType::kFreight), nullptr);
    EXPECT_EQ(rt.RingTotal(), 5u);

    ASSERT_TRUE(rt.PreparePath(nullptr));

    OperationInfo a;
    ASSERT_TRUE(rt.AllocateTrain(&a));
    ASSERT_TRUE(a.train_number.has_value());
    EXPECT_TRUE(EndsWith(*a.train_number, "Г"));

    // Прицепляем локомотив -> выгрузит из кольца в поезд (место 16, вагонов 5)
    OperationInfo l;
    ASSERT_TRUE(rt.HandleLocomotive(L(LocoType::kElectro16), &l));
    ASSERT_TRUE(l.success);
    EXPECT_EQ(rt.RingTotal(), 0u);

    // Входящих вагонов нет (hill пустой), кольцо пустое => разрешено отправить неполный
    OperationInfo s;
    ASSERT_TRUE(rt.SendTrain(hill, /*force=*/false, &s));
    ASSERT_TRUE(s.success);
    ASSERT_TRUE(s.train_number.has_value());
    EXPECT_EQ(*s.train_number, "0001Г");
}

TEST(StationRuntime, PartialNotSentWhenIncomingWagonsExist) {
    auto hill = MakeHill(1);
    StationRuntime rt;
    rt.StartShift(hill.GetNumberOfPaths());

    ASSERT_TRUE(rt.PreparePath(nullptr));
    ASSERT_TRUE(rt.AllocateTrain(nullptr));
    ASSERT_TRUE(rt.HandleLocomotive(L(LocoType::kElectro16), nullptr));

    // 1 вагон в поезд
    OperationInfo w;
    ASSERT_TRUE(rt.HandleWagon(W(1, WagonType::kFreight), &w));
    ASSERT_TRUE(w.success);
    ASSERT_TRUE(w.wagon_to_ring.has_value());
    EXPECT_FALSE(*w.wagon_to_ring);

    // Имитируем, что ещё есть входящие вагоны
    hill.AddWagon(W(999, WagonType::kFreight));

    OperationInfo s;
    bool sent = rt.SendTrain(hill, /*force=*/false, &s);

    // Не полный (1/16), а incoming есть -> отправлять нельзя
    EXPECT_FALSE(sent);
    EXPECT_FALSE(s.success);
}

TEST(StationRuntime, FullTrainSentEvenWhenIncomingExists) {
    auto hill = MakeHill(1);
    StationRuntime rt;
    rt.StartShift(hill.GetNumberOfPaths());

    ASSERT_TRUE(rt.PreparePath(nullptr));
    ASSERT_TRUE(rt.AllocateTrain(nullptr));
    ASSERT_TRUE(rt.HandleLocomotive(L(LocoType::kElectro16), nullptr));

    // Заполняем до capacity=16
    for (int i = 0; i < 16; ++i) {
        OperationInfo w;
        ASSERT_TRUE(rt.HandleWagon(W(1000 + i, WagonType::kFreight), &w));
        ASSERT_TRUE(w.success);
        ASSERT_TRUE(w.wagon_to_ring.has_value());
        EXPECT_FALSE(*w.wagon_to_ring);
    }

    // Имитируем, что входящие ещё есть
    hill.AddWagon(W(555, WagonType::kFreight));

    OperationInfo s;
    ASSERT_TRUE(rt.SendTrain(hill, /*force=*/false, &s));
    ASSERT_TRUE(s.success);
    ASSERT_TRUE(s.train_number.has_value());
    EXPECT_EQ(*s.train_number, "0001Г");
}

TEST(StationRuntime, ReservedLocoUsedOnAllocateTrain) {
    auto hill = MakeHill(1);
    StationRuntime rt;
    rt.StartShift(hill.GetNumberOfPaths());

    // Локомотив пришёл до планирования -> в резерв
    OperationInfo l;
    ASSERT_TRUE(rt.HandleLocomotive(L(LocoType::kElectro16), &l));
    ASSERT_TRUE(l.success);
    EXPECT_TRUE(l.loco_reserved);

    ASSERT_TRUE(rt.PreparePath(nullptr));

    OperationInfo a;
    ASSERT_TRUE(rt.AllocateTrain(&a));
    ASSERT_TRUE(a.success);

    // Локо должен автоматически прицепиться => вагон пойдёт в поезд
    OperationInfo w;
    ASSERT_TRUE(rt.HandleWagon(W(42, WagonType::kFreight), &w));
    ASSERT_TRUE(w.success);
    ASSERT_TRUE(w.wagon_to_ring.has_value());
    EXPECT_FALSE(*w.wagon_to_ring);
}

TEST(StationRuntime, PreparePathOnceUntilTrainSentThenPrepareAgain) {
    auto hill = MakeHill(1);
    StationRuntime rt;
    rt.StartShift(hill.GetNumberOfPaths());

    OperationInfo p1;
    ASSERT_TRUE(rt.PreparePath(&p1));
    ASSERT_TRUE(p1.success);

    OperationInfo p2;
    EXPECT_FALSE(rt.PreparePath(&p2)); // единственный путь уже prepared
    EXPECT_FALSE(p2.success);

    ASSERT_TRUE(rt.AllocateTrain(nullptr));
    ASSERT_TRUE(rt.HandleLocomotive(L(LocoType::kElectro16), nullptr));

    // Заполняем до capacity=16
    for (int i = 0; i < 16; ++i) rt.HandleWagon(W(100 + i, WagonType::kFreight), nullptr);

    OperationInfo s;
    ASSERT_TRUE(rt.SendTrain(hill, /*force=*/false, &s));
    ASSERT_TRUE(s.success);

    // После отправки prepared сброшен -> можно снова подготовить
    OperationInfo p3;
    ASSERT_TRUE(rt.PreparePath(&p3));
    ASSERT_TRUE(p3.success);
}


TEST(StationRuntime, EndShiftDoesNotCrash) {
    auto hill = MakeHill(2);
    StationRuntime rt;
    rt.StartShift(hill.GetNumberOfPaths());

    ASSERT_TRUE(rt.PreparePath(nullptr));
    ASSERT_TRUE(rt.AllocateTrain(nullptr));
    ASSERT_TRUE(rt.HandleLocomotive(L(LocoType::kElectro16), nullptr));

    // просто проверка, что EndShift безопасен
    rt.EndShift();
    SUCCEED();
}


TEST(SortingHill, ShiftEndForcesSendingAndFreesPaths) {
    using namespace std::literals;

    std::vector<std::unique_ptr<SortingHandler>> handlers;
    handlers.push_back(std::make_unique<SortingOperatorImpl>());
    SortingHill hill(/*number_of_paths=*/1, std::move(handlers));

    hill.HandleEvent(EventType::kShiftStarted);

    // Подготовка пути и планирование поезда
    ASSERT_TRUE(hill.CheckEvent(EventType::kPreparePath));
    hill.HandleEvent(EventType::kPreparePath);

    ASSERT_TRUE(hill.CheckEvent(EventType::kTrainPlanned));
    hill.HandleEvent(EventType::kTrainPlanned);

    // Прицепляем локомотив (тип локомотива выбирается случайно, это не важно)
    ASSERT_TRUE(hill.CheckEvent(EventType::kLocoArrived));
    hill.HandleEvent(EventType::kLocoArrived);

    // Поезд пустой (вагонов 0) - в обычном режиме отправлять нельзя
    EXPECT_FALSE(hill.CheckEvent(EventType::kTrainReady));

    // Конец смены должен освободить пути (разрешено отправлять локомотив без вагонов)
    hill.HandleEvent(EventType::kShiftEnded);

    EXPECT_EQ(hill.GetSentTrainsCount(), 1u);

    // После конца смены путь должен быть свободен и доступен к подготовке
    EXPECT_TRUE(hill.CheckEvent(EventType::kPreparePath));
}
