
// Declare local debugging if you want more logging on test run
//#define LOCAL_DEBUGGING

#include <Helpers/config_reader.hpp>

#include "CallBacks.hpp"
#include "Statistic.hpp"
#include "Subsystem.hpp"

StatisticSystem::StatisticSystem() {
	ModName = STR("Statistic");
	ModVersion = STR("1.0.2");
	ModDescription = STR("Hehe");
	ModAuthors = STR("Shiza");

	RC::ConfigLoader::LoadModConfig(&Config);
}

StatisticSystem::~StatisticSystem() {
}

//void TickFired(auto& info, UEngine* Context, float DeltaSeconds, bool bIdleMode) {
//}


void TickFired(Hook::TCallbackIterationData<void>& info, UEngine* Context, float DeltaSeconds, bool bIdleMode) {
}

void StatisticSystem::on_unreal_init() {
	static RC::DataBase::DataBase Database{Config.Database};
	static StatisticSubsystem Ticker{&Database};
	static CallsHandler Calls{&Database, &Statistic};

	Unreal::Hook::RegisterEngineTickPreCallback(TickFired, {false, true, STR("StatisticSystem"), STR("InstallHook")});
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
