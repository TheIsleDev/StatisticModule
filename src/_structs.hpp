#pragma once

#include <string>

#include <Unreal/FText.hpp>
#include <Unreal/AActor.hpp>
#include <Helpers/String.hpp>
#include <Unreal/FString.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/NameTypes.hpp>
#include <Unreal/AGameModeBase.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include "Engine/UDataTable.hpp"

namespace StatisticSystemConfig {
	struct StatisticConfig {
		std::string Database;
	};
}

namespace ModFunctions {
	using namespace RC::Unreal;
	struct BaseForFuncGetter {
		UFunction* Function{};
		uint32 BufferSize{};

		auto Initialize() -> void {
			BufferSize = Function->GetParmsSize();
			InitializeSub();
		}

		protected:
			virtual void InitializeSub() {}
	};

	struct SimplyReturnValue : public BaseForFuncGetter {
		FProperty* ReturnValue{};

		void InitializeSub() override {
			for (FProperty* Prop : TFieldRange<FProperty>(Function, EFieldIterationFlags::IncludeDeprecated)) {
				auto Name = Prop->GetName();
				if (Name == STR("ReturnValue")) ReturnValue = Prop;
			}
		}
	};
}