#include <DynamicOutput/Output.hpp>
#include <DynamicOutput/OutputDevice.hpp>

#include <Unreal/AActor.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/Core/Containers/ContainerAllocationPolicies.hpp>

#include <DBLink/database.cpp>
#include <DBLink/structures.hpp>
#include <Reflection/_include_custom.hpp>

#include "_structs.hpp"

// Declare local debugging if you want more logging on test run
//#define LOCAL_DEBUGGING
namespace StatisticSystemComponent {
	using namespace RC::Unreal;

	static ATIGameModeBase* GameMode{};

	static int TicksFired{0};
	static constexpr int PerTicksFired{60};// how often we save data (every min)

	auto Fire() -> void {
		if (!GameMode) {// It fires before we have gamemode, need to solve that
			GameMode = static_cast<ATIGameModeBase*>(UObjectGlobals::FindFirstOf(STR("BP_SurvivalGameMode_C")));
			if(!GameMode) return;
		}

		StatisticStructs::StatisticBatch Batch;

		TArray<ATICharacterBase*> ActivePlayers = GameMode->GetAllPlayerCharacters();
		for (ATICharacterBase* Character : ActivePlayers) {
			if (!Character || !Character->IsA(ATIDinosaurBase::StaticClass())) continue;

			ATIDinosaurBase* Dinosaur = static_cast<ATIDinosaurBase*>(Character);
			if (Dinosaur->GetbIsDead()) {
				DataBaseConnector::SaveDino(Dinosaur, false, true);// Make it later do detour for smth
				continue;
			}

			// Make it save on their own delay, not global #FUTURE #NOLAZINES
			if (!TicksFired && !Dinosaur->GetSteamId().IsEmpty()) {
				DataBaseConnector::SaveDino(Dinosaur, true, false);
			}

			Batch.SteamID.push_back(RC::to_string(*Dinosaur->GetSteamId()));
			Batch.Class.push_back(RC::to_string(Dinosaur->GetClassPrivate()->GetPathName()));
			Batch.DinoID.push_back(std::to_string(Dinosaur->GetID()));

			Batch.Growth.push_back(std::to_string(Dinosaur->GetGrowth()));
			Batch.Health.push_back(std::to_string(Dinosaur->FGetHealth()));
			Batch.Stamina.push_back(std::to_string(Dinosaur->FGetStamina()));
			Batch.Hunger.push_back(std::to_string(Dinosaur->FGetHunger()));
			Batch.Thirst.push_back(std::to_string(Dinosaur->FGetThirst()));
			Batch.Oxygen.push_back(std::to_string(Dinosaur->FGetOxygen()));
			Batch.Blood.push_back(std::to_string(Dinosaur->FGetBlood()));

			FVector PlayerVector = Dinosaur->K2_GetActorLocation();
			Batch.X.push_back(std::to_string(PlayerVector.X()));
			Batch.Y.push_back(std::to_string(PlayerVector.Y()));
			Batch.Z.push_back(std::to_string(PlayerVector.Z()));
		}

		if (++TicksFired > PerTicksFired) TicksFired = 0;
		if (!Batch.Size()) return;
		DataBaseConnector::StoreStatisticBatch(Batch);
	}

	auto Initialize(StatisticSystemConfig::StatisticConfig Config) -> void {
		if (DataBaseConnector::Initialize(Config.Database)) DataBaseConnector::PrepareStatistic();
#ifdef LOCAL_DEBUGGING
		else RC::Output::send<RC::LogLevel::Error>(STR("DB connection failed, con string: {}"), RC::to_wstring(Config.Database));
#endif
	}

	auto Destroy() -> void {
		DataBaseConnector::Destroy();
	}
}