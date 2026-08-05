
// Declare local debugging if you want more logging on test run
//#define LOCAL_DEBUGGING

#include <TheIsleHelpers/ConfigReader.hpp>
#include <Statistic.hpp>


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
	Database = std::make_unique<RC::DataBase::DataBase>(Config.Database);
	Database->PrepareStatistic();
	TickingStatistic = std::make_unique<StatisticSubsystem>(Database.get());
	Callbacks = std::make_unique<CallsHandler>(Database.get(), TickingStatistic.get());
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
