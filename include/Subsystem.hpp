
#pragma once

#include <Database/Database.hpp>
#include <TheIsle/ATIDinosaurBase.hpp>


using namespace RC::Unreal;
using namespace RC::Unreal::TheIsle;

class StatisticSubsystem {
private:
	/// How often it fires
	int TicksFired = 0;
	static constexpr int TickRate{10};

	/// Array with active dinosaurs for data gathering
	RC::DataBase::DataBase* Database{};

public:
	TArray<ATIDinosaurBase*> Dinosaurs{};

    StatisticSubsystem(RC::DataBase::DataBase* Database);
	~StatisticSubsystem();

	void Tick(float DeltaSeconds, bool bIdleMode);
};
