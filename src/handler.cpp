#include <string>

#include <DynamicOutput/Output.hpp>
#include <DynamicOutput/OutputDevice.hpp>

#include "Containers/Array.hpp"
#include "DBLink/database.cpp"
#include "DBLink/structures.hpp"
#include "Structs/TheIsleStructs.hpp"
#include "Structs/FunctionParamisator.hpp"

#include "_structs.hpp"

namespace StatisticSystemComponent {
	using namespace RC::Unreal;

	static UClass* GameModeBaseClass{};
	static FProperty* GameModeAllPlayers{};

	static UClass* PlayerControllerBaseClass{};
	static FProperty* PlayerControllerPawn{};

	static UClass* DinoClass{};
	static FProperty* DinoIDProp{};
	static FProperty* DinoGrowthProp{};
	static UFunction* GetHealth{};
	static UFunction* GetStamina{};
	static UFunction* GetHunger{};
	static UFunction* GetThirst{};
	static UFunction* GetOxygen{};
	static UFunction* GetBlood{};

	static UObject* GameMode{};

	auto Fire() -> void {
		if (!GameMode) {
			GameMode = UObjectGlobals::FindFirstOf(STR("BP_SurvivalGameMode_C"));
			if(!GameMode) return;
		}

		StatisticStructs::StatisticBatch Batch;

		TSet<IsleStructs::ATIPlayerController*>* ActivePlayers = GameModeAllPlayers->ContainerPtrToValuePtr<TSet<IsleStructs::ATIPlayerController*>>(GameMode);
		for (IsleStructs::ATIPlayerController* Player : *ActivePlayers) {
			IsleStructs::APawn* Pawn = *PlayerControllerPawn->ContainerPtrToValuePtr<IsleStructs::APawn*>(Player);;
			if (!Pawn || !Pawn->IsA(DinoClass)) continue;// Make sure it's actually dino, not a fucking damn human

			IsleStructs::ATIDinosaurBase* Dino = static_cast<IsleStructs::ATIDinosaurBase*>(Pawn);
			int32 DinoID = *DinoIDProp->ContainerPtrToValuePtr<int32>(Dino);

			Batch.DinoID.push_back(std::to_string(static_cast<int>(DinoID)));
			Batch.Growth.push_back(std::to_string(*DinoGrowthProp->ContainerPtrToValuePtr<float>(Dino)));

			IsleStructs::FReturnFloatParams Params{};
			Dino->ProcessEvent(GetHealth, &Params);
			Batch.Health.push_back(std::to_string(Params.ReturnValue));
			Dino->ProcessEvent(GetStamina, &Params);
			Batch.Stamina.push_back(std::to_string(Params.ReturnValue));
			Dino->ProcessEvent(GetHunger, &Params);
			Batch.Hunger.push_back(std::to_string(Params.ReturnValue));
			Dino->ProcessEvent(GetThirst, &Params);
			Batch.Thirst.push_back(std::to_string(Params.ReturnValue));
			Dino->ProcessEvent(GetOxygen, &Params);
			Batch.Oxygen.push_back(std::to_string(Params.ReturnValue));
			Dino->ProcessEvent(GetBlood, &Params);
			Batch.Blood.push_back(std::to_string(Params.ReturnValue));

			FVector PlayerVector = Dino->K2_GetActorLocation();
			Batch.X.push_back(std::to_string(static_cast<float>(PlayerVector.X())));
			Batch.Y.push_back(std::to_string(static_cast<float>(PlayerVector.Y())));
			Batch.Z.push_back(std::to_string(static_cast<float>(PlayerVector.Z())));
		}
		if (!Batch.Size()) return;

		DataBaseConnector::StoreStatisticBatch(Batch);
	}

	auto Initialize(StatisticSystemConfig::StatisticConfig Config) -> void {
		if (!DataBaseConnector::Initialize(Config.Database)) {
			Output::send<LogLevel::Error>(STR("DB connection failed, con string: {}"), to_wstring(Config.Database));
		} else DataBaseConnector::PrepareStatistic();

		GameModeBaseClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIGameModeBase"));
		GameModeAllPlayers = GameModeBaseClass->GetPropertyByNameInChain(STR("AllPlayerControllers"));

		PlayerControllerBaseClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIPlayerController"));
		PlayerControllerPawn = PlayerControllerBaseClass->GetPropertyByNameInChain(STR("Pawn"));// Dinos/Humans/Spectator

		DinoClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIDinosaurBase"));
		DinoIDProp = DinoClass->GetPropertyByNameInChain(STR("ID"));
		DinoGrowthProp = DinoClass->GetPropertyByNameInChain(STR("Growth"));
		GetHealth = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.TICharacterBase:GetHealth"));
		GetStamina = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.TICharacterBase:GetStamina"));
		GetHunger = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.TICharacterBase:GetHunger"));
		GetThirst = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.TICharacterBase:GetThirst"));
		GetOxygen = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.TICharacterBase:GetOxygen"));
		GetBlood = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.TICharacterBase:GetBlood"));
    }

	auto Destroy() -> void {
		DataBaseConnector::Destroy();
		//destroy hooks here
	}
}
/*
#include <string>

#include <DynamicOutput/Output.hpp>
#include <DynamicOutput/OutputDevice.hpp>

#include "Containers/Array.hpp"
#include "DBLink/database.cpp"
#include "DBLink/structures.hpp"
#include "Structs/TheIsleStructs.hpp"
#include "Structs/FunctionParamisator.hpp"

#include "_structs.hpp"

namespace StatisticSystemComponent {
	using namespace RC::Unreal;

	static UClass* GameModeBaseClass{};
	static FProperty* GameModeAllPlayers{};

	static UClass* PlayerControllerBaseClass{};
	static FProperty* PlayerControllerPawn{};

	static UClass* DinoClass{};
	static FProperty* DinoIDProp{};
	static FProperty* DinoGrowthProp{};

	static ModFunctions::SimplyReturnValue GetHealth{};
	static ModFunctions::SimplyReturnValue GetStamina{};
	static ModFunctions::SimplyReturnValue GetHunger{};
	static ModFunctions::SimplyReturnValue GetThirst{};
	static ModFunctions::SimplyReturnValue GetOxygen{};
	static ModFunctions::SimplyReturnValue GetBlood{};

	static UObject* GameMode{};

	auto Fire() -> void {
		if (!GameMode) {
			GameMode = UObjectGlobals::FindFirstOf(STR("BP_SurvivalGameMode_C"));
			if(!GameMode) return;
		}

		StatisticStructs::StatisticBatch Batch;

		TSet<IsleStructs::ATIPlayerController*>* ActivePlayers = GameModeAllPlayers->ContainerPtrToValuePtr<TSet<IsleStructs::ATIPlayerController*>>(GameMode);
		for (IsleStructs::ATIPlayerController* Player : *ActivePlayers) {
			IsleStructs::APawn* Pawn = *PlayerControllerPawn->ContainerPtrToValuePtr<IsleStructs::APawn*>(Player);;
			if (!Pawn || !Pawn->IsA(DinoClass)) continue;// Make sure it's actually dino, not a fucking damn human

			IsleStructs::ATIDinosaurBase* Dino = static_cast<IsleStructs::ATIDinosaurBase*>(Pawn);
			int32 DinoID = *DinoIDProp->ContainerPtrToValuePtr<int32>(Dino);

			Batch.DinoID.push_back(std::to_string(static_cast<int>(DinoID)));
			Batch.Growth.push_back(std::to_string(*DinoGrowthProp->ContainerPtrToValuePtr<float>(Dino)));

			StructsParams::FProcessEventParams HealthParams(GetHealth.Function, GetHealth.BufferSize);
			Output::send(STR("4"));
			Dino->ProcessEvent(GetHealth.Function, HealthParams.Data());
			Output::send(STR("5"));
			Batch.Health.push_back(std::to_string(*HealthParams.GetAddress<float>(GetHealth.ReturnValue)));
			StructsParams::FProcessEventParams StaminaParams(GetStamina.Function, GetStamina.BufferSize);
			Dino->ProcessEvent(GetStamina.Function, StaminaParams.Data());
			Batch.Stamina.push_back(std::to_string(*StaminaParams.GetAddress<float>(GetStamina.ReturnValue)));
			StructsParams::FProcessEventParams StaminaParams(GetHunger.Function, GetHunger.BufferSize);
			Dino->ProcessEvent(GetHunger.Function, StaminaParams.Data());
			Batch.Hunger.push_back(std::to_string(*StaminaParams.GetAddress<float>(GetHunger.ReturnValue)));
			StructsParams::FProcessEventParams StaminaParams(GetThirst.Function, GetThirst.BufferSize);
			Dino->ProcessEvent(GetThirst.Function, StaminaParams.Data());
			Batch.Thirst.push_back(std::to_string(*StaminaParams.GetAddress<float>(GetThirst.ReturnValue)));
			StructsParams::FProcessEventParams StaminaParams(GetOxygen.Function, GetOxygen.BufferSize);
			Dino->ProcessEvent(GetOxygen.Function, StaminaParams.Data());
			Batch.Oxygen.push_back(std::to_string(*StaminaParams.GetAddress<float>(GetOxygen.ReturnValue)));
			StructsParams::FProcessEventParams StaminaParams(GetBlood.Function, GetBlood.BufferSize);
			Dino->ProcessEvent(GetBlood.Function, StaminaParams.Data());
			Batch.Blood.push_back(std::to_string(*StaminaParams.GetAddress<float>(GetBlood.ReturnValue)));
		}
		if (!Batch.Size()) return;

		DataBaseConnector::StoreStatisticBatch(Batch);
	}

	auto Initialize(StatisticSystemConfig::StatisticConfig Config) -> void {
		if (!DataBaseConnector::Initialize(Config.Database)) {
			Output::send<LogLevel::Error>(STR("DB connection failed, con string: {}"), to_wstring(Config.Database));
		} else DataBaseConnector::PrepareStatistic();

		GameModeBaseClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIGameModeBase"));
		GameModeAllPlayers = GameModeBaseClass->GetPropertyByNameInChain(STR("AllPlayerControllers"));

		PlayerControllerBaseClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIPlayerController"));
		PlayerControllerPawn = PlayerControllerBaseClass->GetPropertyByNameInChain(STR("Pawn"));// Dinos/Humans/Spectator

		DinoClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIDinosaurBase"));
		DinoIDProp = DinoClass->GetPropertyByNameInChain(STR("ID"));
		DinoGrowthProp = DinoClass->GetPropertyByNameInChain(STR("Growth"));
		GetHealth.Function = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.TICharacterBase:GetHealth"));
		GetHealth.Initialize();
		GetStamina.Function = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.TICharacterBase:GetStamina"));
		GetStamina.Initialize();
		GetHunger.Function = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.TICharacterBase:GetHunger"));
		GetHunger.Initialize();
		GetThirst.Function = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.TICharacterBase:GetThirst"));
		GetThirst.Initialize();
		GetOxygen.Function = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.TICharacterBase:GetOxygen"));
		GetOxygen.Initialize();
		GetBlood.Function = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.TICharacterBase:GetBlood"));
		GetBlood.Initialize();
    }

	auto Destroy() -> void {
		DataBaseConnector::Destroy();
		//destroy hooks here
	}
}
*/