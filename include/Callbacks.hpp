
#pragma once

#include <TheIsleHelpers/Delegates.hpp>
#include <Subsystem.hpp>


using namespace RC::Unreal;
using namespace RC::Unreal::TheIsle;

class CallsHandler {
private:
	UObject* CallBackSucker{};
	DelegateManager BindingManager{};
	UFunction* FHandleCharacterDied{};
	UFunction* FHandleActorDestroyed{};

	UFunction* OnPlayerRespawned{};
	std::pair<int, int> OnPlayerRespawnedIDs{};

	StatisticSubsystem* Ticker{};
	RC::DataBase::DataBase* Database{};

public:
    CallsHandler(RC::DataBase::DataBase* Database, StatisticSubsystem* Ticker);
	~CallsHandler();

	// Калбэк инициализации к моменту создания BP хуйни.
	void CreateHelpers();

	// Кастомные функции для UEшки
	void HandleActorDestroyed(UObject* Context, FFrame& Stack);
	void HandleCharacterDied(UObject* Context, FFrame& Stack);

	// Вызывается на контроллере когда он получает actor под контроль, почему так? хз.
	void PreCharacterSpawn(UnrealScriptFunctionCallableContext& context);
	void PostCharacterSpawn(UnrealScriptFunctionCallableContext& context);
};
