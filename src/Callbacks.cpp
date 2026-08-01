
#include <Unreal/UObject.hpp>
#include <Unreal/UPackage.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <String/StringType.hpp>
#include <TheIsle/APlayerController.hpp>
#include <Callbacks.hpp>


UClass* CopyClass(UPackage* Package) {
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

UFunction* CopyFunction(UClass* UCustom, RC::StringType Name, RC::StringType TargetPath) {
	// Если вдруг мы уже имеем регнутую, иначе может крашнуть...
	UFunction* CreatedFunction = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.ModClassProxy:") + Name);
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


void CallsHandler::CreateHelpers() {
	// Я ебал рот, его крашит если пытаться самому лукапить пакет ебаный, ну будут через прокси искать, хули мне
	UClass* PackageSource = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.ActorPoolManager"));
	UPackage* FoundPackage = static_cast<UPackage*>(PackageSource->GetOutermost());

	UClass* UCustom = CopyClass(FoundPackage);
	FHandleCharacterDied = CopyFunction(UCustom, STR("HandleCharacterDeath"), STR("/Script/TheIsle.CharacterDiedDelegate__DelegateSignature"));
	FHandleCharacterDied->SetFuncPtr(&CharacterDiedCollector);
	FHandleActorDestroyed = CopyFunction(UCustom, STR("HandleActorDestroyed"), STR("/Script/Engine.ActorDestroyedSignature__DelegateSignature"));
	FHandleActorDestroyed->SetFuncPtr(&ActorDestroyedCollector);

	// Нет смысла делать safechecks тут, похуй лучше краш чем хуй пойми что
	CallBackSucker = UObjectGlobals::StaticFindObject<UObject*>(nullptr, nullptr, STR("/Script/TheIsle.ModDelegateProxy"));

	FStaticConstructObjectParameters UObjectParams{UCustom, FoundPackage};
	UObjectParams.Name = FName(STR("ModDelegateProxy"), FNAME_Add);
	CallBackSucker = UObjectGlobals::StaticConstructObject(UObjectParams);
}

CallsHandler::CallsHandler(RC::DataBase::DataBase* DBLink, StatisticSubsystem* TickerLink) {
	Database = DBLink;
	Ticker = TickerLink;

	LookUp = this;

	OnPlayerRespawned = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Game/TheIsle/Core/GameModes/BP_SurvivalGameMode.BP_SurvivalGameMode_C:OnPlayerRespawned"));
	OnPlayerRespawnedIDs = UObjectGlobals::RegisterHook(OnPlayerRespawned,
		[this](UnrealScriptFunctionCallableContext& Context, void* CustomData) {
			PreCharacterSpawn(Context);
		},
		[this](UnrealScriptFunctionCallableContext& Context, void* CustomData) {
			PostCharacterSpawn(Context);
		}, nullptr
	);

	// Ну хз даже как засейвится до генерации BP...
	Hook::RegisterEngineTickPreCallback(
		[this](Hook::TCallbackIterationData<void>& info, UEngine* Context, float DeltaSeconds, bool bIdleMode) {
			CreateHelpers();
		}
		, {true, true, STR("Callbacks"), STR("Initialization")}
	);
}

CallsHandler::~CallsHandler() {
	UObjectGlobals::UnregisterHook(OnPlayerRespawned, OnPlayerRespawnedIDs);
	LookUp = nullptr;
}


void CallsHandler::HandleActorDestroyed(UObject* Context, FFrame& Stack) {
	RC::Output::send<RC::LogLevel::Verbose>(STR("Noob destroyed"));
	ATIDinosaurBase* Dinosaur = *reinterpret_cast<ATIDinosaurBase**>(Stack.Locals());
	Ticker->Dinosaurs.Remove(Dinosaur);
	BindingManager.UnBindContainer(Dinosaur);
}

void CallsHandler::HandleCharacterDied(UObject* Context, FFrame& Stack) {
	RC::Output::send<RC::LogLevel::Verbose>(STR("Noob died"));
	ATIDinosaurBase* Dinosaur = *reinterpret_cast<ATIDinosaurBase**>(Stack.Locals());
	Ticker->Dinosaurs.Remove(Dinosaur);
	BindingManager.UnBindContainer(Dinosaur);
	Database->SaveDino(Dinosaur, false, true);
}


void CallsHandler::PreCharacterSpawn(UnrealScriptFunctionCallableContext& context) {}

void CallsHandler::PostCharacterSpawn(UnrealScriptFunctionCallableContext& context) {
	RC::Output::send<RC::LogLevel::Verbose>(STR("Noob spawned"));
	APlayerController* Controller = *reinterpret_cast<APlayerController**>(context.TheStack.Locals());
	APawn* Pawn = Controller->Pawn();
	if (!Pawn || !Pawn->IsA(ATIDinosaurBase::StaticClass())) return;

	ATIDinosaurBase* Dinosaur = static_cast<ATIDinosaurBase*>(Pawn);
	if (Ticker->Dinosaurs.Contains(Dinosaur)) return;// Режим спектатора тоже тригерит это, как и перезаход после hard выхода с сервера

	Ticker->Dinosaurs.Add(Dinosaur);

	// Ересь с биндами через проперти и надежда в Аллаха! Это даже близко не UE stype game dev, но я что-то обязательно придумаю
	BindingManager.Bind(
		static_cast<FMulticastDelegateProperty*>(Dinosaur->StaticClass()->FindProperty(FName(STR("OnCharacterDied"), FNAME_Find))),
		static_cast<void*>(Dinosaur->OnCharacterDied()),
		Dinosaur, CallBackSucker, FHandleCharacterDied->GetFName()
	);

	// Ересь с регистрацией хуеты что имеет глобальное хранилище, я ебал его в рот...
	BindingManager.CheckAndBind(
		static_cast<FMulticastDelegateProperty*>(Dinosaur->StaticClass()->FindProperty(FName(STR("OnDestroyed"), FNAME_Find))),
		static_cast<void*>(Dinosaur->OnDestroyed()),
		Dinosaur, CallBackSucker, FHandleCharacterDied->GetFName()
	);
}
