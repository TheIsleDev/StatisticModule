#include <Mod/CppUserModBase.hpp>

#include <Config/config_reader.hpp>

#include "_structs.hpp"
#include "handler.cpp"

class StatisticSystem : public RC::CppUserModBase {
private:
	StatisticSystemConfig::StatisticConfig Config;

	int TicksFired{0};
	static constexpr int PerTicksFired{720};// 120 per sec, 30 game ticks

public:
	StatisticSystem() : CppUserModBase() {
		ModName = STR("Statistic");
		ModVersion = STR("1.0");
		ModDescription = STR("Hehe");
		ModAuthors = STR("Shiza");
	}

	~StatisticSystem() override {
		StatisticSystemComponent::Destroy();
	}

	auto on_unreal_init() -> void override {
		ModConfigReader::LoadModConfig(&Config);

		StatisticSystemComponent::Initialize(Config);
	}

	auto on_update() -> void override {
		if (++TicksFired < PerTicksFired) return;
		TicksFired = 0;

		StatisticSystemComponent::Fire();
	}
};

#define MOD_API __declspec(dllexport)
extern "C" {
	MOD_API RC::CppUserModBase* start_mod() {
		return new StatisticSystem();
	}

	MOD_API void uninstall_mod(RC::CppUserModBase* mod) {
		delete mod;
	}
}