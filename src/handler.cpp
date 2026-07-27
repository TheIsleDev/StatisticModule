#include <DynamicOutput/Output.hpp>
#include <DynamicOutput/OutputDevice.hpp>

#include <Unreal/AActor.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/Core/Containers/ContainerAllocationPolicies.hpp>
#include "CoreUObject/UObject/Class.hpp"
#include "FArchive.hpp"

#include <DBLink/database.cpp>
#include <DBLink/structures.hpp>
#include <Reflection/_include_custom.hpp>

#include "_structs.hpp"

// Declare local debugging if you want more logging on test run
//#define LOCAL_DEBUGGING
namespace StatisticSystemComponent {
	using namespace RC::Unreal;

	ATIGameModeBase* GameMode{};
	TArray<ATIDinosaurBase*> Dinosaurs{};

	// Fucking pulley for da shit callback?
	UObject* CallBackSucker{};
	DelegateManager BindingManager{};
	UFunction* FHandleCharacterDeath{};

	UFunction* FOnPlayerRespawned{};
	std::pair<int, int> FOnPlayerRespawnedIDs{};

	UFunction* FAttemptDestroy{};
	std::pair<int, int> FAttemptDestroyIDs{};

	UFunction* FAutosave{};
	std::pair<int, int> FAutosaveIDs{};

	int TicksFired{0};
	static constexpr int PerTicksFired{60};// how often we save data (every min)
	auto Fire() -> void {
		if (!GameMode) {// It fires before we have gamemode, need to solve that
			GameMode = static_cast<ATIGameModeBase*>(UObjectGlobals::FindFirstOf(STR("BP_SurvivalGameMode_C")));
			if(!GameMode) return;
		}

		StatisticStructs::StatisticBatch Batch;
		for (ATIDinosaurBase* Dinosaur : Dinosaurs) {
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


	auto HandleCharacterDeath(UObject* Context, FFrame& Stack, void* Result) -> void {
		RC::Output::send<RC::LogLevel::Verbose>(STR("Noob died"));
		ATIDinosaurBase* Dinosaur = *reinterpret_cast<ATIDinosaurBase**>(Stack.Locals());
		Dinosaurs.Remove(Dinosaur);
		BindingManager.UnBind(Dinosaur->GetOnCharacterDied());
		DataBaseConnector::SaveDino(Dinosaur, false, true);
	}

	auto PreCharacterSpawn(UnrealScriptFunctionCallableContext& context, void*) -> void {}

	auto PostCharacterSpawn(UnrealScriptFunctionCallableContext& context, void*) -> void {
		RC::Output::send<RC::LogLevel::Verbose>(STR("Noob spawned"));
		APlayerController* Controller = *reinterpret_cast<APlayerController**>(context.TheStack.Locals());
		APawn* Pawn = Controller->GetPawn();
		if (!Pawn || !Pawn->IsA(ATIDinosaurBase::StaticClass())) return;

		ATIDinosaurBase* Dinosaur = static_cast<ATIDinosaurBase*>(Pawn);
		Dinosaurs.Add(Dinosaur);
		BindingManager.Bind(Dinosaur->GetOnCharacterDied(), CallBackSucker, FHandleCharacterDeath->GetFName());
	}


	auto PreCharacterDestroy(UnrealScriptFunctionCallableContext& context, void*) -> void {
		RC::Output::send<RC::LogLevel::Verbose>(STR("Noob destroyed"));
		if (!context.Context->IsA(ATIDinosaurBase::StaticClass())) return;

		ATIDinosaurBase* Dinosaur = static_cast<ATIDinosaurBase*>(context.Context);
		Dinosaurs.Remove(Dinosaur);
	}

	auto PostCharacterDestroy(UnrealScriptFunctionCallableContext& context, void*) -> void {}


	auto PreAutosave(UnrealScriptFunctionCallableContext& context, void*) -> void {
		RC::Output::send<RC::LogLevel::Verbose>(STR("Noob saved"));
		if (!context.Context->IsA(ATIDinosaurBase::StaticClass())) return;

		ATIDinosaurBase* Dinosaur = static_cast<ATIDinosaurBase*>(context.Context);
		DataBaseConnector::SaveDino(Dinosaur, true, false);
	}

	auto PostAutosave(UnrealScriptFunctionCallableContext& context, void*) -> void {}


	auto Initialize(StatisticSystemConfig::StatisticConfig Config) -> void {
		if (DataBaseConnector::Initialize(Config.Database)) DataBaseConnector::PrepareStatistic();
#ifdef LOCAL_DEBUGGING
		else RC::Output::send<RC::LogLevel::Error>(STR("DB connection failed, con string: {}"), RC::to_wstring(Config.Database));
#endif

		FOnPlayerRespawned = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Game/TheIsle/Core/GameModes/BP_SurvivalGameMode.BP_SurvivalGameMode_C:OnPlayerRespawned"));
		FOnPlayerRespawnedIDs = UObjectGlobals::RegisterHook(FOnPlayerRespawned, &PreCharacterSpawn, &PostCharacterSpawn, nullptr);

		FAttemptDestroy = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.TICharacterBase:AttemptDestroy"));
		FAttemptDestroyIDs = UObjectGlobals::RegisterHook(FAttemptDestroy, &PreCharacterDestroy, &PostCharacterDestroy, nullptr);

		FAutosave = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.TICharacterBase:Autosave"));
		FAutosaveIDs = UObjectGlobals::RegisterHook(FAutosave, &PreAutosave, &PostAutosave, nullptr);

		UClass* ObjectClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/CoreUObject.Object"));
		FStaticConstructObjectParameters OParams{ObjectClass};
		OParams.Name  = FName(STR("ModDelegateProxy"), FNAME_Add);
		CallBackSucker = UObjectGlobals::StaticConstructObject(OParams);

		UFunction* Signature = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.CharacterDiedDelegate__DelegateSignature"));
		UClass* FunctionClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/CoreUObject.Function"));
		FStaticConstructObjectParameters FParams{FunctionClass, ObjectClass};
		FParams.Name = FName(STR("HandleCharacterDeath"), FNAME_Add);
		FParams.Template = Signature;

		FHandleCharacterDeath = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/CoreUObject.Object:HandleCharacterDeath"));
		if (FHandleCharacterDeath) {
			FHandleCharacterDeath->SetFuncPtr(&HandleCharacterDeath);
			return;
		}

		FHandleCharacterDeath = static_cast<UFunction*>(UObjectGlobals::StaticConstructObject(FParams));

		FHandleCharacterDeath->GetNumParms() = Signature->GetNumParms();
		FHandleCharacterDeath->GetParmsSize() = Signature->GetParmsSize();
		FHandleCharacterDeath->GetChildProperties() = Signature->GetChildProperties();
		FHandleCharacterDeath->GetPropertiesSize() = Signature->GetPropertiesSize();
		FHandleCharacterDeath->GetFunctionFlags() = (Signature->GetFunctionFlags() | FUNC_Native | FUNC_Public) & ~(FUNC_Delegate | FUNC_MulticastDelegate);

		FHandleCharacterDeath->SetFuncPtr(&HandleCharacterDeath);
		ObjectClass->GetFuncMap().Add(FHandleCharacterDeath->GetFName(), TObjectPtr<UFunction>(FHandleCharacterDeath));
	}

	auto Destroy() -> void {
		DataBaseConnector::Destroy();
		BindingManager.Destroy();
		UObjectGlobals::UnregisterHook(FOnPlayerRespawned, FOnPlayerRespawnedIDs);
		UObjectGlobals::UnregisterHook(FAttemptDestroy, FAttemptDestroyIDs);
		UObjectGlobals::UnregisterHook(FAutosave, FAutosaveIDs);
	}
}
