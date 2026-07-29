
#pragma once

#include <string>

#include <Mod/CppUserModBase.hpp>
#include <Database/Database.hpp>

#include "CallBacks.hpp"
#include "Subsystem.hpp"

struct StatisticConfig {
	std::string Database;
};

class StatisticSystem : public RC::CppUserModBase {
private:
	StatisticConfig Config{};
	Hook::GlobalCallbackId FireCallBackID{};
	RC::DataBase::DataBase* Database{};

public:
	StatisticSubsystem* TickingStatistic{};
	CallsHandler* Callbacks{};

    StatisticSystem();
	~StatisticSystem() override;

	void on_unreal_init() override;
};
