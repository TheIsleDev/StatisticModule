#include <DynamicOutput/Output.hpp>
#include <DynamicOutput/OutputDevice.hpp>

#include <Unreal/AActor.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UPackage.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/Core/Containers/ContainerAllocationPolicies.hpp>
#include "CoreUObject/UObject/Class.hpp"
#include "FArchive.hpp"

#include <DBLink/database.cpp>
#include <DBLink/structures.hpp>
#include <Reflection/_include_custom.hpp>

namespace StatisticSystemComponent {
	using namespace RC::Unreal;

	ATIGameModeBase* GameMode{};
	TArray<ATIDinosaurBase*> Dinosaurs{};

	// Fucking pulley for da shit callback?
	UObject* CallBackSucker{};
	DelegateManager BindingManager{};
	UFunction* FHandleCharacterDied{};
	UFunction* FHandleActorDestroyed{};

	UFunction* FOnPlayerRespawned{};
	std::pair<int, int> FOnPlayerRespawnedIDs{};

	// Наша кастомная функция, для игры это поинтер на память в библиотеку
	auto HandleActorDestroyed(UObject* Context, FFrame& Stack, void* Result) -> void {
		RC::Output::send<RC::LogLevel::Verbose>(STR("Noob destroyed"));
		ATIDinosaurBase* Dinosaur = *reinterpret_cast<ATIDinosaurBase**>(Stack.Locals());
		Dinosaurs.Remove(Dinosaur);
		BindingManager.UnBindContainer(Dinosaur);
	}


	// Наша кастомная функция, для игры это поинтер на память в библиотеку
	auto HandleCharacterDied(UObject* Context, FFrame& Stack, void* Result) -> void {
		RC::Output::send<RC::LogLevel::Verbose>(STR("Noob died"));
		ATIDinosaurBase* Dinosaur = *reinterpret_cast<ATIDinosaurBase**>(Stack.Locals());
		Dinosaurs.Remove(Dinosaur);
		BindingManager.UnBindContainer(Dinosaur);
		DataBaseConnector::SaveDino(Dinosaur, false, true);
	}

	auto PreCharacterSpawn(UnrealScriptFunctionCallableContext& context, void*) -> void {}

	// Вызывается на контроллере когда он получает actor под контроль, почему так? хз.
	auto PostCharacterSpawn(UnrealScriptFunctionCallableContext& context, void*) -> void {
		RC::Output::send<RC::LogLevel::Verbose>(STR("Noob spawned"));
		APlayerController* Controller = *reinterpret_cast<APlayerController**>(context.TheStack.Locals());
		APawn* Pawn = Controller->GetPawn();
		if (!Pawn || !Pawn->IsA(ATIDinosaurBase::StaticClass())) return;

		ATIDinosaurBase* Dinosaur = static_cast<ATIDinosaurBase*>(Pawn);
		if (Dinosaurs.Contains(Dinosaur)) return;// Режим спектатора тоже тригерит это, как и перезаход после hard выхода с сервера

		Dinosaurs.Add(Dinosaur);

		// Ересь с биндами через проперти и надежда в Аллаха! Это даже близко не UE stype game dev, но я что-то обязательно придумаю.
		BindingManager.Bind(
			static_cast<FMulticastDelegateProperty*>(Dinosaur->StaticClass()->FindProperty(FName(STR("OnCharacterDied"), FNAME_Find))),
			static_cast<void*>(Dinosaur->GetOnCharacterDied()),
			Dinosaur, CallBackSucker, FHandleCharacterDied->GetFName());

		BindingManager.Bind(
			static_cast<FMulticastDelegateProperty*>(Dinosaur->StaticClass()->FindProperty(FName(STR("OnDestroyed"), FNAME_Find))),
			static_cast<void*>(Dinosaur->GetOnDestroyed()),
			Dinosaur, CallBackSucker, FHandleCharacterDied->GetFName());
	}


	auto GenerateClass(UPackage* Package) -> UClass* {
		// Санити чек)))
		UClass* UCustom = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.ModClassProxy"));
		if (UCustom) {
			return UCustom;
		}

		// Начало создания кастомного класса для того что бы у нас функция была приватная и легче было выгружать мод без перезапуска сервера
		// TODO: Перенести это все в кастомный модуль что упростит работу, хотя хз что будет быстрее, кастомный мод лоадер или это
		// В кастомном модлоадере я смогу обращаться проще к другой функции да и там схема будет немного другой, по этому не думаю что стоит тратить на это время щас
		FStaticConstructObjectParameters UClassParams{UClass::StaticClass(), Package};
		UClassParams.Name = FName(STR("ModClassProxy"), FNAME_Add);
		UClassParams.SetFlags = UObject::StaticClass()->GetObjectFlags();
		UClassParams.Template = UObject::StaticClass();// Копируем основу

		// Регестрирует наш чертов класс
		UCustom = static_cast<UClass*>(UObjectGlobals::StaticConstructObject(UClassParams));

		// Ересь? Да!
		UCustom->GetClassConstructor() = UObject::StaticClass()->GetClassConstructor();
		UCustom->GetClassWithin() = UObject::StaticClass()->GetClassWithin();
		UCustom->GetClassCastFlags() = UObject::StaticClass()->GetClassCastFlags();
		UCustom->GetChildProperties() = UObject::StaticClass()->GetChildProperties();
		UCustom->GetPropertiesSize() = UObject::StaticClass()->GetPropertiesSize();
		return UCustom;
	}

	auto GenerateFunction1(UClass* UCustom) -> void {
		// Если вдруг мы уже имеем регнутую, иначе может крашнуть...
		FHandleCharacterDied = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.ModClassProxy:HandleCharacterDeath"));
		if (FHandleCharacterDied) {
			FHandleCharacterDied->SetFuncPtr(&HandleCharacterDied);
			return;
		}

		// Пример для копирования значений что бы не регать разную хуету, заебусь блять регать все значения сам каждый раз
		UFunction* FCharacterDiedDelegate = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.CharacterDiedDelegate__DelegateSignature"));

		// То бля, во бля... короче функция так же как и кастомный класс
		FStaticConstructObjectParameters FParams{UFunction::StaticClass(), UCustom};
		FParams.Name = FName(STR("HandleCharacterDied"), FNAME_Add);
		FParams.SetFlags = UFunction::StaticClass()->GetObjectFlags();
		FParams.Template = FCharacterDiedDelegate;// Копирует всякие приколы с базы

		// Регестрирует нашу функцию
		FHandleCharacterDied = static_cast<UFunction*>(UObjectGlobals::StaticConstructObject(FParams));

		// Копируем базовые праметры, это у нас деменшены функции в памяти и дименшены параметров что передаются в неё
		FHandleCharacterDied->GetNumParms() = FCharacterDiedDelegate->GetNumParms();
		FHandleCharacterDied->GetParmsSize() = FCharacterDiedDelegate->GetParmsSize();
		FHandleCharacterDied->GetChildProperties() = FCharacterDiedDelegate->GetChildProperties();
		FHandleCharacterDied->GetPropertiesSize() = FCharacterDiedDelegate->GetPropertiesSize();

		// Добавляем наши флаги
		FHandleCharacterDied->GetFunctionFlags() = (FCharacterDiedDelegate->GetFunctionFlags() | FUNC_Native | FUNC_Public) & ~(FUNC_Delegate | FUNC_MulticastDelegate);

		// Делаем наш ебаный указатель, это вызов wrapper для доступа к памяти самой функции
		FHandleCharacterDied->SetFuncPtr(&HandleCharacterDied);
		UCustom->GetFuncMap().Add(FHandleCharacterDied->GetFName(), TObjectPtr<UFunction>(FHandleCharacterDied));
	}

	auto GenerateFunction2(UClass* UCustom) -> void {
		// Если вдруг мы уже имеем регнутую, иначе может крашнуть...
		FHandleActorDestroyed = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.ModClassProxy:HandleActorDestroyed"));
		if (FHandleActorDestroyed) {
			FHandleActorDestroyed->SetFuncPtr(&HandleActorDestroyed);
			return;
		}

		// Пример для копирования значений что бы не регать разную хуету, заебусь блять регать все значения сам каждый раз
		UFunction* ActorDestroyedSignature = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/Engine.ActorDestroyedSignature__DelegateSignature"));

		// То бля, во бля... короче функция так же как и кастомный класс
		FStaticConstructObjectParameters FParams{UFunction::StaticClass(), UCustom};
		FParams.Name = FName(STR("HandleActorDestroyed"), FNAME_Add);
		FParams.SetFlags = UFunction::StaticClass()->GetObjectFlags();
		FParams.Template = ActorDestroyedSignature;// Копирует всякие приколы с базы

		// Регестрирует нашу функцию
		FHandleActorDestroyed = static_cast<UFunction*>(UObjectGlobals::StaticConstructObject(FParams));

		// Копируем базовые праметры, это у нас деменшены функции в памяти и дименшены параметров что передаются в неё
		FHandleActorDestroyed->GetNumParms() = ActorDestroyedSignature->GetNumParms();
		FHandleActorDestroyed->GetParmsSize() = ActorDestroyedSignature->GetParmsSize();
		FHandleActorDestroyed->GetChildProperties() = ActorDestroyedSignature->GetChildProperties();
		FHandleActorDestroyed->GetPropertiesSize() = ActorDestroyedSignature->GetPropertiesSize();

		// Добавляем наши флаги
		FHandleActorDestroyed->GetFunctionFlags() = (ActorDestroyedSignature->GetFunctionFlags() | FUNC_Native | FUNC_Public) & ~(FUNC_Delegate | FUNC_MulticastDelegate);

		// Делаем наш ебаный указатель, это вызов wrapper для доступа к памяти самой функции
		FHandleActorDestroyed->SetFuncPtr(&HandleActorDestroyed);
		UCustom->GetFuncMap().Add(FHandleActorDestroyed->GetFName(), TObjectPtr<UFunction>(FHandleActorDestroyed));
	}

	auto Initialize(StatisticConfig Config) -> void {
		if (DataBaseConnector::Initialize(Config.Database)) DataBaseConnector::PrepareStatistic();
		else LOG_DEBUG(STR("DB connection failed, con string: {}"), RC::to_wstring(Config.Database));

		FOnPlayerRespawned = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Game/TheIsle/Core/GameModes/BP_SurvivalGameMode.BP_SurvivalGameMode_C:OnPlayerRespawned"));
		FOnPlayerRespawnedIDs = UObjectGlobals::RegisterHook(FOnPlayerRespawned, &PreCharacterSpawn, &PostCharacterSpawn, nullptr);

		// Я ебал рот, его крашит если пытаться самому лукапить пакет ебаный, ну будут через прокси искать, хули мне
		UClass* PackageSource = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.ActorPoolManager"));
		UPackage* FoundPackage = static_cast<UPackage*>(PackageSource->GetOutermost());

		UClass* UCustom = GenerateClass(FoundPackage);
		GenerateFunction1(UCustom);
		GenerateFunction2(UCustom);

		CallBackSucker = UObjectGlobals::StaticFindObject<UObject*>(nullptr, nullptr, STR("/Script/TheIsle.ModDelegateProxy"));
		if (CallBackSucker) return;

		// Создаем наш объект
		FStaticConstructObjectParameters UObjectParams{UCustom, FoundPackage};
		UObjectParams.Name = FName(STR("ModDelegateProxy"), FNAME_Add);
		CallBackSucker = UObjectGlobals::StaticConstructObject(UObjectParams);
	}

	auto Destroy() -> void {
		DataBaseConnector::Destroy();
		// Самое важное что бы игра не пыталась обращаться к модификации, т.к. будут ошибки с памятью и хард краш
		BindingManager.Destroy();
		UObjectGlobals::UnregisterHook(FOnPlayerRespawned, FOnPlayerRespawnedIDs);
	}
}
