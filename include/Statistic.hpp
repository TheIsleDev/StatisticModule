
#pragma once

#include <string>

#include <Mod/CppUserModBase.hpp>
#include <Database/Database.hpp>

#include "Callbacks.hpp"
#include "Subsystem.hpp"


struct StatisticConfig {
	std::string Database;
};

class StatisticSystem : public RC::CppUserModBase {
private:
	StatisticConfig Config{};
	RC::DataBase::DataBase* Database{};

public:
	StatisticSubsystem* TickingStatistic{};
	CallsHandler* Callbacks{};

    StatisticSystem();
	~StatisticSystem() override;

	void on_unreal_init() override;
};
