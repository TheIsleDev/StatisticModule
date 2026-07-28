
#pragma once

#include <Helpers/delegates.hpp>

#include "Subsystem.hpp"


using namespace RC::Unreal;
using namespace RC::Unreal::TheIsle;

class CallsHandler {
private:
	UObject* CallBackSucker{};
	DelegateManager BindingManager{};
	UFunction* FHandleCharacterDied{};
	UFunction* FHandleActorDestroyed{};

	UFunction* FOnPlayerRespawned{};
	std::pair<int, int> FOnPlayerRespawnedIDs{};

	StatisticSubsystem* Ticker{};
	RC::DataBase::DataBase* Database{};

public:
    CallsHandler(RC::DataBase::DataBase* Database, StatisticSubsystem* Ticker);
	~CallsHandler();

	// Кастомные функции для UEшки
	void HandleActorDestroyed(UObject* Context, FFrame& Stack, void* Result);
	void HandleCharacterDied(UObject* Context, FFrame& Stack, void* Result);

	// Вызывается на контроллере когда он получает actor под контроль, почему так? хз.
	void PreCharacterSpawn(UnrealScriptFunctionCallableContext& context, void*);
	void PostCharacterSpawn(UnrealScriptFunctionCallableContext& context, void*);
};
