
// Declare local debugging if you want more logging on test run
//#define LOCAL_DEBUGGING

#include <TheIsleHelpers/ConfigReader.hpp>

#include "Statistic.hpp"


StatisticSystem::StatisticSystem() {
	ModName = STR("Statistic");
	ModVersion = STR("1.0.2");
	ModDescription = STR("Hehe");
	ModAuthors = STR("Shiza");

	RC::ConfigLoader::LoadModConfig(&Config);
}

StatisticSystem::~StatisticSystem() {
}

void StatisticSystem::on_unreal_init() {
	static RC::DataBase::DataBase DatabaseLink{Config.Database};
	Database = &DatabaseLink;
	static StatisticSubsystem Ticker{Database};
	TickingStatistic = &Ticker;
	static CallsHandler Calls{Database, TickingStatistic};
	Callbacks = &Calls;
}


#define MOD_API __declspec(dllexport)
extern "C" {
	MOD_API RC::CppUserModBase* start_mod() {
		return new StatisticSystem();
	}

	MOD_API void uninstall_mod(RC::CppUserModBase* mod) {
		delete mod;
	}
}
