#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WakatimeSettings.generated.h"

UCLASS(Config="wakatime", GlobalUserConfig)
class UWakatimeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWakatimeSettings();

	UPROPERTY(Config, EditAnywhere, Category="Wakatime Integration", meta = (DisplayName = "Wakatime Token: "))
	FString WakatimeBearerToken;

	UPROPERTY(Config, EditAnywhere, Category = "Wakatime Integration", meta = (DisplayName = "Heartbeat Interval (s): ", ClampMin = "10", ClampMax = "120"))
	int32 WakatimeInterval;

	UPROPERTY(Config, EditAnywhere, Category = "Wakatime Integration", meta = (DisplayName = "API Endpoint URL", Tooltip = "Something like this, no trailing slash: https://wakahost.example.com/api/waka/v1"))
	FString WakatimeEndpoint;

	virtual FName GetContainerName() const override { return TEXT("Editor"); }
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("Wakatime_Settings"); }
};
