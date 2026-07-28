
#include <TheIsle/APlayerController.hpp>

#include "CallBacks.hpp"


CallsHandler::CallsHandler(RC::DataBase::DataBase* DBLink, StatisticSubsystem* TickerLink) {
	Database = DBLink;
	Ticker = TickerLink;
}

CallsHandler::~CallsHandler() {
	UObjectGlobals::UnregisterHook(FOnPlayerRespawned, FOnPlayerRespawnedIDs);
}


void CallsHandler::HandleActorDestroyed(UObject* Context, FFrame& Stack, void* Result) {
	RC::Output::send<RC::LogLevel::Verbose>(STR("Noob destroyed"));
	ATIDinosaurBase* Dinosaur = *reinterpret_cast<ATIDinosaurBase**>(Stack.Locals());
	Ticker->Dinosaurs.Remove(Dinosaur);
	BindingManager.UnBindContainer(Dinosaur);
}

void CallsHandler::HandleCharacterDied(UObject* Context, FFrame& Stack, void* Result) {
	RC::Output::send<RC::LogLevel::Verbose>(STR("Noob died"));
	ATIDinosaurBase* Dinosaur = *reinterpret_cast<ATIDinosaurBase**>(Stack.Locals());
	Ticker->Dinosaurs.Remove(Dinosaur);
	BindingManager.UnBindContainer(Dinosaur);
	Database->SaveDino(Dinosaur, false, true);
}


void CallsHandler::PreCharacterSpawn(UnrealScriptFunctionCallableContext& context, void*) {}

void CallsHandler::PostCharacterSpawn(UnrealScriptFunctionCallableContext& context, void*) {
	RC::Output::send<RC::LogLevel::Verbose>(STR("Noob spawned"));
	APlayerController* Controller = *reinterpret_cast<APlayerController**>(context.TheStack.Locals());
	APawn* Pawn = Controller->Pawn();
	if (!Pawn || !Pawn->IsA(ATIDinosaurBase::StaticClass())) return;

	ATIDinosaurBase* Dinosaur = static_cast<ATIDinosaurBase*>(Pawn);
	if (Ticker->Dinosaurs.Contains(Dinosaur)) return;// Режим спектатора тоже тригерит это, как и перезаход после hard выхода с сервера

	Ticker->Dinosaurs.Add(Dinosaur);

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
