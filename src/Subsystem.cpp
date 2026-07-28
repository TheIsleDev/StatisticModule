
#include <DynamicOutput/Output.hpp>
#include <DynamicOutput/OutputDevice.hpp>

#include "Subsystem.hpp"


StatisticSubsystem::StatisticSubsystem(RC::DataBase::DataBase* DBLink) {
	Database = DBLink;
}

StatisticSubsystem::~StatisticSubsystem() {
}


void StatisticSubsystem::Tick(float DeltaSeconds, bool bIdleMode) {
	RC::Output::send<RC::LogLevel::Verbose>(STR("Noobs num: {}"), Dinosaurs.Num());

	if (bIdleMode) return;

	if (++TicksFired < TickRate) return;
	TicksFired = 0;

	RC::DataBase::StatisticBatch Batch;
	for (ATIDinosaurBase* Dinosaur : Dinosaurs) {
		Batch.SteamID.push_back(RC::to_string(*Dinosaur->SteamId()));
		Batch.Class.push_back(RC::to_string(Dinosaur->GetClassPrivate()->GetPathName()));
		Batch.DinoID.push_back(std::to_string(Dinosaur->ID()));

		Batch.Growth.push_back(std::to_string(Dinosaur->Growth()));
		Batch.Health.push_back(std::to_string(Dinosaur->GetHealth()));
		Batch.Stamina.push_back(std::to_string(Dinosaur->GetStamina()));
		Batch.Hunger.push_back(std::to_string(Dinosaur->GetHunger()));
		Batch.Thirst.push_back(std::to_string(Dinosaur->GetThirst()));
		Batch.Oxygen.push_back(std::to_string(Dinosaur->GetOxygen()));
		Batch.Blood.push_back(std::to_string(Dinosaur->GetBlood()));

		FVector PlayerVector = Dinosaur->K2_GetActorLocation();
		Batch.X.push_back(std::to_string(PlayerVector.X()));
		Batch.Y.push_back(std::to_string(PlayerVector.Y()));
		Batch.Z.push_back(std::to_string(PlayerVector.Z()));
	}

	if (!Batch.Size()) return;
	Database->StoreStatisticBatch(Batch);
}
