#include <DynamicOutput/Output.hpp>
#include <DynamicOutput/OutputDevice.hpp>

#include <Unreal/CoreUObject/UObject/UnrealType.hpp>

#include "DBLink/database.cpp"
#include "DBLink/structures.hpp"
#include "Structs/TheIsleStructs.hpp"
#include "Structs/FunctionParamisator.hpp"

#include <Reflection/_include_custom.hpp>

#include "_structs.hpp"

namespace StatisticSystemComponent {
	using namespace RC::Unreal;

	static UClass* GameModeBaseClass{};
	static FProperty* GameModeAllPlayers{};

	static UClass* PlayerControllerBaseClass{};
	static FProperty* PlayerControllerPawn{};

	static UClass* DinoClass{};
	static FProperty* DinoIDProp{};
	static FProperty* DinobIsDeadProp{};
	static FProperty* DinoGrowthProp{};

	static UObject* GameMode{};

	static int TicksFired{0};
	static constexpr int PerTicksFired{10};// how often we save data (every min)

	auto Fire() -> void {
		if (!GameMode) {
			GameMode = UObjectGlobals::FindFirstOf(STR("BP_SurvivalGameMode_C"));
			if(!GameMode) return;
		}

		StatisticStructs::StatisticBatch Batch;

		TArray<ATICharacterBase*>* ActivePlayers = GameModeAllPlayers->ContainerPtrToValuePtr<TArray<ATICharacterBase*>>(GameMode);
		for (ATICharacterBase* Character : *ActivePlayers) {
			if (!Character || !Character->IsA(DinoClass)) continue;// Make sure it's actually dino, not a fucking damn human

			ATIDinosaurBase* Dino = static_cast<ATIDinosaurBase*>(Character);
			int32 DinoID = *DinoIDProp->ContainerPtrToValuePtr<int32>(Dino);
			if (*DinobIsDeadProp->ContainerPtrToValuePtr<bool>(Dino)) {
				DataBaseConnector::DinoDied(DinoID);
				continue;
			}

			if (!TicksFired) {
				//save here in future
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

		GameModeBaseClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIGameModeBase"));
		GameModeAllPlayers = GameModeBaseClass->GetPropertyByNameInChain(STR("AllPlayerCharacters"));

		PlayerControllerBaseClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIPlayerController"));
		PlayerControllerPawn = PlayerControllerBaseClass->GetPropertyByNameInChain(STR("Pawn"));// Dinos/Humans/Spectator

		DinoClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIDinosaurBase"));
		DinoIDProp = DinoClass->GetPropertyByNameInChain(STR("ID"));
		DinoGrowthProp = DinoClass->GetPropertyByNameInChain(STR("Growth"));
		DinobIsDeadProp = DinoClass->GetPropertyByNameInChain(STR("bIsDead"));
    }

	auto Destroy() -> void {
		DataBaseConnector::Destroy();
	}
}