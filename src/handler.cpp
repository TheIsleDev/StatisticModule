#include <DynamicOutput/Output.hpp>
#include <DynamicOutput/OutputDevice.hpp>

#include <Unreal/AActor.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/Core/Containers/ContainerAllocationPolicies.hpp>

#include "DBLink/database.cpp"
#include "DBLink/structures.hpp"

#include <Reflection/_include_custom.hpp>

#include "_structs.hpp"

namespace StatisticSystemComponent {
	using namespace RC::Unreal;

	static UClass* DinoClass{};
	static ATIGameModeBase* GameMode{};

	static int TicksFired{0};
	static constexpr int PerTicksFired{60};// how often we save data (every min)

// If server wont restart very often, like 20 hours and players would die like every 30 seconds
// then go for adding unordered_map and use it to store time, delay for clean supposed to be around 20 minutes
	std::unordered_set<int> DeathList{};

	auto Fire() -> void {
		if (!GameMode) {// It fires before we have gamemode, need to solve that
			GameMode = static_cast<ATIGameModeBase*>(UObjectGlobals::FindFirstOf(STR("BP_SurvivalGameMode_C")));
			if(!GameMode) return;
		}

		StatisticStructs::StatisticBatch Batch;

		TArray<ATICharacterBase*> ActivePlayers = GameMode->GetAllPlayerCharacters();
		for (ATICharacterBase* Character : ActivePlayers) {
			if (!Character || !Character->IsA(DinoClass)) continue;// Make sure it's actually dino, not a fucking damn human

			ATIDinosaurBase* Dinosaur = static_cast<ATIDinosaurBase*>(Character);
			int32 DinoID = Dinosaur->GetID();
			if (Dinosaur->GetbIsDead()) {
				if (DeathList.contains(DinoID)) continue;
				DeathList.insert(DinoID);
				DataBaseConnector::DinoDied(DinoID);
				continue;
			}

			// Make it save on their own delay, not global #FUTURE #NOLAZINES
			if (!TicksFired && !Dinosaur->GetSteamId().IsEmpty()) {
				FTIPlayerData PlayerData = UTISaveManager::GetCharacterData(Character, false);
				FString Result = UTISaveManager::PlayerDataToString(PlayerData);
				FString SteamID = Dinosaur->GetSteamId();
				DataBaseConnector::SaveDino(SteamID, DinoID, Result, true);
			}

			Batch.DinoID.push_back(std::to_string(static_cast<int>(DinoID)));
			Batch.Growth.push_back(std::to_string(Dinosaur->GetGrowth()));
			Batch.Health.push_back(std::to_string(Dinosaur->FGetHealth()));
			Batch.Stamina.push_back(std::to_string(Dinosaur->FGetStamina()));
			Batch.Hunger.push_back(std::to_string(Dinosaur->FGetHunger()));
			Batch.Thirst.push_back(std::to_string(Dinosaur->FGetThirst()));
			Batch.Oxygen.push_back(std::to_string(Dinosaur->FGetOxygen()));
			Batch.Blood.push_back(std::to_string(Dinosaur->FGetBlood()));

			FVector PlayerVector = Dinosaur->K2_GetActorLocation();
			Batch.X.push_back(std::to_string(static_cast<float>(PlayerVector.X())));
			Batch.Y.push_back(std::to_string(static_cast<float>(PlayerVector.Y())));
			Batch.Z.push_back(std::to_string(static_cast<float>(PlayerVector.Z())));
		}

		if (++TicksFired > PerTicksFired) TicksFired = 0;
		if (!Batch.Size()) return;
		DataBaseConnector::StoreStatisticBatch(Batch);
	}

	auto Initialize(StatisticSystemConfig::StatisticConfig Config) -> void {
		if (!DataBaseConnector::Initialize(Config.Database)) {
			Output::send<LogLevel::Error>(STR("DB connection failed, con string: {}"), to_wstring(Config.Database));
		} else DataBaseConnector::PrepareStatistic();

		DinoClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIDinosaurBase"));
    }

	auto Destroy() -> void {
		DataBaseConnector::Destroy();
	}
}