
#include "Hooks/Hooks.hpp"
#include <Unreal/UObject.hpp>
#include <Unreal/UPackage.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <String/StringType.hpp>
#include <TheIsle/APlayerController.hpp>
#include <Callbacks.hpp>
#include <cstddef>
#define LOCAL_DEBUGGING


UClass* CopyClass(UPackage* Package, const RC::StringType ClassName) {
	// Санити чек)))
	UClass* UCustom = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.") + ClassName);
	if (UCustom) {
		return UCustom;
	}

	// Начало создания кастомного класса для того что бы у нас функция была приватная и легче было выгружать мод без перезапуска сервера
	// TODO: Перенести это все в кастомный модуль что упростит работу, хотя хз что будет быстрее, кастомный мод лоадер или это
	// В кастомном модлоадере я смогу обращаться проще к другой функции да и там схема будет немного другой, по этому не думаю что стоит тратить на это время щас
	FStaticConstructObjectParameters UClassParams{UClass::StaticClass(), Package};
	UClassParams.Name = FName(ClassName, FNAME_Add);
	UClassParams.SetFlags = static_cast<EObjectFlags>(UObject::StaticClass()->GetObjectFlags() | RF_MarkAsNative | RF_MarkAsRootSet);
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

UFunction* CopyFunction(UClass* UCustom, const RC::StringType ClassName, const RC::StringType Name, const RC::StringType TargetPath) {
	// Если вдруг мы уже имеем регнутую, иначе может крашнуть...
	UFunction* CreatedFunction = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.") + ClassName + STR(":") + Name);
	if (CreatedFunction) {
		return CreatedFunction;
	}

	// Пример для копирования значений что бы не регать разную хуету, заебусь блять регать все значения сам каждый раз
	UFunction* ActorDestroyedSignature = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, TargetPath);

	// То бля, во бля... короче функция так же как и кастомный класс
	FStaticConstructObjectParameters FParams{UFunction::StaticClass(), UCustom};
	FParams.Name = FName(Name, FNAME_Add);
	FParams.SetFlags = UFunction::StaticClass()->GetObjectFlags();
	FParams.Template = ActorDestroyedSignature;// Копирует всякие приколы с базы

	// Регестрирует нашу функцию
	CreatedFunction = static_cast<UFunction*>(UObjectGlobals::StaticConstructObject(FParams));

	// Копируем базовые праметры, это у нас деменшены функции в памяти и дименшены параметров что передаются в неё
	CreatedFunction->GetNumParms() = ActorDestroyedSignature->GetNumParms();
	CreatedFunction->GetParmsSize() = ActorDestroyedSignature->GetParmsSize();
	CreatedFunction->GetChildProperties() = ActorDestroyedSignature->GetChildProperties();
	CreatedFunction->GetPropertiesSize() = ActorDestroyedSignature->GetPropertiesSize();

	// Добавляем наши флаги
	CreatedFunction->GetFunctionFlags() = (ActorDestroyedSignature->GetFunctionFlags() | FUNC_Native | FUNC_Public) & ~(FUNC_Delegate | FUNC_MulticastDelegate);

	// Делаем наш ебаный указатель, это вызов wrapper для доступа к памяти самой функции
	UCustom->GetFuncMap().Add(CreatedFunction->GetFName(), TObjectPtr<UFunction>(CreatedFunction));
	return CreatedFunction;
}


CallsHandler* LookUp{};

// Ересь с калбэками
void ActorDestroyedCollector(UObject* Context, FFrame& Stack, void* Result) {
	LookUp->HandleActorDestroyed(Context, Stack);
}

void CharacterDiedCollector(UObject* Context, FFrame& Stack, void* Result) {
	LookUp->HandleCharacterDied(Context, Stack);
}


bool CallsHandler::CreateHelpers() {
	static int TicksFired = 0;
	static constexpr int TickRate{300};

	if (++TicksFired < TickRate) return false;
	TicksFired = 0;

	static const RC::StringType ClassName = STR("ModClassProxy");

	if (!OnPlayerRespawned) {
		OnPlayerRespawned = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Game/TheIsle/Core/GameModes/BP_SurvivalGameMode.BP_SurvivalGameMode_C:OnPlayerRespawned"));
		if (!OnPlayerRespawned) return false;

		OnPlayerRespawnedIDs = UObjectGlobals::RegisterHook(OnPlayerRespawned,
			[this](UnrealScriptFunctionCallableContext& Context, void* CustomData) {
				PreCharacterSpawn(Context);
			},
			[this](UnrealScriptFunctionCallableContext& Context, void* CustomData) {
				PostCharacterSpawn(Context);
			}, nullptr
		);
	}

	// Я ебал рот, его крашит если пытаться самому лукапить пакет ебаный, ну будут через прокси искать, хули мне
	UClass* PackageSource = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.ActorPoolManager"));
	if (!PackageSource) return false;

	UPackage* FoundPackage = static_cast<UPackage*>(PackageSource->GetOutermost());
	if (!FoundPackage) return false;

	if (!CustomClass) {
		CustomClass = CopyClass(FoundPackage, ClassName);
		if (!CustomClass) return false;
	}
	FUObjectItem* ClassInternalItem = CustomClass->GetObjectItem();
	ClassInternalItem->SetGCKeep();
	ClassInternalItem->SetRootSet();

	if (!FHandleCharacterDied) {
		FHandleCharacterDied = CopyFunction(CustomClass, ClassName, STR("HandleCharacterDeath"), STR("/Script/TheIsle.CharacterDiedDelegate__DelegateSignature"));
		if (!FHandleCharacterDied) return false;
		FHandleCharacterDied->SetFuncPtr(&CharacterDiedCollector);
	}

	if (!FHandleActorDestroyed) {
		FHandleActorDestroyed = CopyFunction(CustomClass, ClassName, STR("HandleActorDestroyed"), STR("/Script/Engine.ActorDestroyedSignature__DelegateSignature"));
		if (!FHandleActorDestroyed) return false;
		FHandleActorDestroyed->SetFuncPtr(&ActorDestroyedCollector);
	}

	if (!CallBackSucker) {
		CallBackSucker = UObjectGlobals::StaticFindObject<UObject*>(nullptr, nullptr, STR("/Script/TheIsle.ModDelegateProxy"));
		if (!CallBackSucker) {
			FStaticConstructObjectParameters UObjectParams{CustomClass, FoundPackage};
			UObjectParams.SetFlags = RF_MarkAsRootSet;
			UObjectParams.Name = FName(STR("ModDelegateProxy"), FNAME_Add);
			CallBackSucker = UObjectGlobals::StaticConstructObject(UObjectParams);
			if (!CallBackSucker) return false;
		}
		FUObjectItem* InternalItem = CallBackSucker->GetObjectItem();
		InternalItem->SetGCKeep();
		InternalItem->SetRootSet();
	}

	return true;
}

CallsHandler::CallsHandler(RC::DataBase::DataBase* DBLink, StatisticSubsystem* TickerLink) {
	Database = DBLink;
	Ticker = TickerLink;

	LookUp = this;

	// Ну хз даже как засейвится до генерации BP...
	Hook::RegisterEngineTickPreCallback(
		[this](Hook::TCallbackIterationData<void>& info, UEngine* Context, float DeltaSeconds, bool bIdleMode) {
			if (!CreateHelpers()) return;

			info.RemoveSelf();
		}
		, {false, true, STR("Statistic"), STR("CallbacksInitialize")}
	);
}

CallsHandler::~CallsHandler() {
	if (OnPlayerRespawned) UObjectGlobals::UnregisterHook(OnPlayerRespawned, OnPlayerRespawnedIDs);
	LookUp = nullptr;
}


void CallsHandler::HandleActorDestroyed(UObject* Context, FFrame& Stack) {
#ifdef LOCAL_DEBUGGING
	RC::Output::send<RC::LogLevel::Verbose>(STR("Dino GC'ed"));
#endif
	ATIDinosaurBase* Dinosaur = *reinterpret_cast<ATIDinosaurBase**>(Stack.Locals());
	Ticker->Dinosaurs.Remove(Dinosaur);
	BindingManager.UnBindContainer(Dinosaur);
}

void CallsHandler::HandleCharacterDied(UObject* Context, FFrame& Stack) {
#ifdef LOCAL_DEBUGGING
	RC::Output::send<RC::LogLevel::Verbose>(STR("Dino died"));
#endif
	ATIDinosaurBase* Dinosaur = *reinterpret_cast<ATIDinosaurBase**>(Stack.Locals());
	Ticker->Dinosaurs.Remove(Dinosaur);
	BindingManager.UnBindContainer(Dinosaur);
	Database->SaveDino(Dinosaur, false, true);
}


void CallsHandler::PreCharacterSpawn(UnrealScriptFunctionCallableContext& context) {}

void CallsHandler::PostCharacterSpawn(UnrealScriptFunctionCallableContext& context) {
#ifdef LOCAL_DEBUGGING
	RC::Output::send<RC::LogLevel::Verbose>(STR("Dino spawned"));
#endif
	APlayerController* Controller = *reinterpret_cast<APlayerController**>(context.TheStack.Locals());
	APawn* Pawn = Controller->Pawn();
	if (!Pawn || !Pawn->IsA(ATIDinosaurBase::StaticClass())) return;

	ATIDinosaurBase* Dinosaur = static_cast<ATIDinosaurBase*>(Pawn);
	if (Ticker->Dinosaurs.Contains(Dinosaur)) return;// Режим спектатора тоже тригерит это, как и перезаход после hard выхода с сервера

	Ticker->Dinosaurs.Add(Dinosaur);

	// Ересь с биндами через проперти и надежда в Аллаха! Это даже близко не UE stype game dev, но я что-то обязательно придумаю
	FMulticastInlineDelegateProperty* POnCharacterDied = static_cast<FMulticastInlineDelegateProperty*>(Dinosaur->StaticClass()->FindProperty(FName(STR("OnCharacterDied"), FNAME_Find)));
	BindingManager.Bind(
		POnCharacterDied, POnCharacterDied->ContainerPtrToValuePtr<void>(Dinosaur),
		Dinosaur, CallBackSucker, FHandleCharacterDied->GetFName()
	);

	// Ересь с регистрацией хуеты что имеет глобальное хранилище, я ебал его в рот...
	FMulticastSparseDelegateProperty* POnDestroyed = static_cast<FMulticastSparseDelegateProperty*>(Dinosaur->StaticClass()->FindProperty(FName(STR("OnDestroyed"), FNAME_Find)));
	BindingManager.Bind(
		POnDestroyed, POnDestroyed->ContainerPtrToValuePtr<void>(Dinosaur),
		Dinosaur, CallBackSucker, FHandleCharacterDied->GetFName()
	);
}
