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
	static constexpr int PerTicksFired{100};// how often we save data (every min)

	auto Fire() -> void {
		if (!GameMode) {// It fires before we have gamemode, need to solve that
			GameMode = static_cast<ATIGameModeBase*>(UObjectGlobals::FindFirstOf(STR("BP_SurvivalGameMode_C")));
			if(!GameMode) return;
		}

		StatisticStructs::StatisticBatch Batch;

		TArray<ATICharacterBase*> ActivePlayers = GameMode->GetAllPlayerCharacters();
		for (ATICharacterBase* Character : ActivePlayers) {
			if (!Character || !Character->IsA(DinoClass)) continue;// Make sure it's actually dino, not a fucking damn human

			ATIDinosaurBase* Dino = static_cast<ATIDinosaurBase*>(Character);
			int32 DinoID = Dino->GetID();
			if (Dino->GetbIsDead()) {
				DataBaseConnector::DinoDied(DinoID);
				continue;
			}

			if (!TicksFired && Dino->GetSteamId().IsEmpty()) {
				FTIPlayerData PlayerData = UTISaveManager::GetCharacterData(Character, false);
				FString Result = UTISaveManager::PlayerDataToString(PlayerData);
				FString SteamID = Dino->GetSteamId();
				DataBaseConnector::SaveDino(SteamID, DinoID, Result, true);
			}

			Batch.DinoID.push_back(std::to_string(static_cast<int>(DinoID)));
			Batch.Growth.push_back(std::to_string(Dino->GetGrowth()));

			Batch.Health.push_back(std::to_string(Dino->GetHealth()));
			Batch.Stamina.push_back(std::to_string(Dino->GetStamina()));
			Batch.Hunger.push_back(std::to_string(Dino->GetHunger()));
			Batch.Thirst.push_back(std::to_string(Dino->GetThirst()));
			Batch.Oxygen.push_back(std::to_string(Dino->GetOxygen()));
			Batch.Blood.push_back(std::to_string(Dino->GetBlood()));

			FVector PlayerVector = Dino->K2_GetActorLocation();
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