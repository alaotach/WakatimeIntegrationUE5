// Copyright Epic Games, Inc. All Rights Reserved.

#include "WakatimeIntegration.h"
#include "Modules/ModuleManager.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "CoreUObject/Public/UObject/UObjectGlobals.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Containers/Ticker.h"
#include <chrono>
#include "WakatimeSettings.h"
#include "ISettingsModule.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/App.h"
#include "Misc/EngineVersion.h"
#include "HAL/PlatformProcess.h"

#define LOCTEXT_NAMESPACE "FWakatimeIntegrationModule"


IMPLEMENT_MODULE(FWakatimeIntegrationModule, WakatimeIntegration)

void FWakatimeIntegrationModule::StartupModule()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings")) {
		SettingsModule->RegisterSettings("Editor", "Plugins", "Wakatime_Settings",
			NSLOCTEXT("WakatimeIntegration", "WakatimeSettingsDisplayName", "Wakatime Integration"),
			NSLOCTEXT("WakatimeIntegration", "WakatimeSettingsDescription", "Settings for Wakatime Integration plugin"),
			GetMutableDefault<UWakatimeSettings>());
	}

	const UWakatimeSettings* Settings = GetDefault<UWakatimeSettings>();
	const float TimerDuration = Settings->WakatimeInterval;

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	AssetRegistry.OnAssetAdded().AddRaw(this, &FWakatimeIntegrationModule::OnAssetAdded);
	AssetRegistry.OnAssetRemoved().AddRaw(this, &FWakatimeIntegrationModule::OnAssetRemoved);
	AssetRegistry.OnAssetRenamed().AddRaw(this, &FWakatimeIntegrationModule::OnAssetRenamed);
	FCoreUObjectDelegates::OnObjectSaved.AddRaw(this, &FWakatimeIntegrationModule::OnObjectSaved);

	TimerHandle = FTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateRaw(this, &FWakatimeIntegrationModule::OnTimerTick),
		TimerDuration
	);

	UE_LOG(LogTemp, Warning, TEXT("Wakatime Integration Startup"));
}

void FWakatimeIntegrationModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetRegistry"))
	{
		IAssetRegistry& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		AssetRegistry.OnAssetAdded().RemoveAll(this);
		AssetRegistry.OnAssetRemoved().RemoveAll(this);
		AssetRegistry.OnAssetRenamed().RemoveAll(this);

		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
		{
			SettingsModule->UnregisterSettings("Editor", "Plugins", "WakatimeIntegration");
		}
	}

	//FKismetEditorUtilities::OnBlueprintCompiled.RemoveAll(this);
	FCoreUObjectDelegates::OnObjectSaved.RemoveAll(this);

	FTicker::GetCoreTicker().RemoveTicker(TimerHandle);

	UE_LOG(LogTemp, Warning, TEXT("Wakatime Integration Shutdown"));
}

void FWakatimeIntegrationModule::OnAssetAdded(const FAssetData& AssetData)
{
	int64 now = GetCurrentTime();
	if ((now - LastAssetPushTime) < SaveDebounce) {
		UE_LOG(LogTemp, Warning, TEXT("Waka: Skipping Add Event"));
		return;
	}
	LastAssetPushTime = now;

	Dirty = true;
	AddOperations++;
	UE_LOG(LogTemp, Warning, TEXT("Waka: Asset Added"));
}

void FWakatimeIntegrationModule::OnAssetRemoved(const FAssetData& AssetData)
{
	int64 now = GetCurrentTime();
	if ((now - LastAssetPushTime) < SaveDebounce) {
		UE_LOG(LogTemp, Warning, TEXT("Waka: Skipping Remove Event"));
		return;
	}
	LastAssetPushTime = now;

	Dirty = true;
	DeleteOperations++;
	UE_LOG(LogTemp, Warning, TEXT("Waka: Asset Removed"));
}

void FWakatimeIntegrationModule::OnAssetRenamed(const FAssetData& AssetData, const FString& OldPath)
{
	int64 now = GetCurrentTime();
	if ((now - LastAssetPushTime) < SaveDebounce) {
		UE_LOG(LogTemp, Warning, TEXT("Waka: Skipping Remove Event"));
		return;
	}
	LastAssetPushTime = now;

	Dirty = true;
	RenameOperations++;
	UE_LOG(LogTemp, Warning, TEXT("Waka: Asset Renamed"));
}

void FWakatimeIntegrationModule::OnObjectSaved(UObject* SavedObject)
{
	int64 now = GetCurrentTime();
	if ((now - LastAssetPushTime) < SaveDebounce) {
		UE_LOG(LogTemp, Warning, TEXT("Waka: Skipping Save Event"));
		return;
	}
	LastAssetPushTime = now;

	SaveOperations++;
	Dirty = true;
	LastSavedName = SavedObject->GetFName();
	UE_LOG(LogTemp, Warning, TEXT("Waka: Object Saved"));
}

FString GetCurrentOSName()
{
#if PLATFORM_WINDOWS
	return TEXT("windows"); // a lot of these are unrealistic, but imagine if you did get UE4 on an xbox live
#elif PLATFORM_XBOXONE
	return TEXT("xboxone");
#elif PLATFORM_MAC
	return TEXT("darwin");
#elif PLATFORM_IOS
	return TEXT("ios");
#elif PLATFORM_LINUX
	return TEXT("linux");
#elif PLATFORM_ANDROID
	return TEXT("android");
#else
	return TEXT("unknown");
#endif
}

bool FWakatimeIntegrationModule::OnTimerTick(float DeltaTime)
{
	UE_LOG(LogTemp, Warning, TEXT("Waka: Timer Event"));
	SendHeartbeat();
	return true;
}

void FWakatimeIntegrationModule::SendHeartbeat()
{
	UE_LOG(LogTemp, Warning, TEXT("waka sanity check"));
	FName localLastSavedName = TEXT("None");
	int32 localDeleteOperations = 0;
	int32 localSaveOperations = 0;
	int32 localRenameOperations = 0;
	int32 localAddOperations = 0;
	bool localDirty = false;
	{
		FScopeLock Lock(&DataLock);
		localDirty = Dirty;
		if (!localDirty) {
			return; //don't bother copying anything
		}
		localDeleteOperations = DeleteOperations;
		localSaveOperations = SaveOperations;
		localRenameOperations = RenameOperations;
		localAddOperations = AddOperations;
		localLastSavedName = LastSavedName;

		DeleteOperations = 0;
		SaveOperations = 0;
		RenameOperations = 0;
		AddOperations = 0;
		Dirty = false;
	}
	UE_LOG(LogTemp, Warning, TEXT("waka copy success"));
	if (!localDirty) {
		return;
	}
	const UWakatimeSettings* Settings = GetDefault<UWakatimeSettings>();
	if (!Settings) {
		UE_LOG(LogTemp, Error, TEXT("Waka: Settings are invalid, cannot send heartbeat."));
		return;
	}

	FString Endpoint = Settings->WakatimeEndpoint;
	if (Endpoint.EndsWith(TEXT("/"))) //remove trailing slash
	{
		Endpoint.RemoveAt(Endpoint.Len() - 1);
	}

	FString EntityName = TEXT("None");
	if (localLastSavedName.IsValid())
	{
		EntityName = localLastSavedName.ToString();
	}
	UE_LOG(LogTemp, Warning, TEXT("waka s time"));
	int64 localTime = GetCurrentTime();

	UE_LOG(LogTemp, Warning, TEXT("waka s enginever"));
	FString EngineVersionString = FEngineVersion::Current().ToString(EVersionComponent::Patch);

	UE_LOG(LogTemp, Warning, TEXT("waka s projname"));
	FString ProjectName = FApp::GetProjectName();

	UE_LOG(LogTemp, Warning, TEXT("waka s compname"));
	FString ComputerName = FPlatformProcess::ComputerName();

	UE_LOG(LogTemp, Warning, TEXT("waka s osname"));
	FString OSName = GetCurrentOSName();


	UE_LOG(LogTemp, Warning, TEXT("waka formatting part"));
	FString TargetURL = Endpoint + TEXT("/users/current/heartbeats");
	FString Body = FString::Printf(
		TEXT( //language as UnrealEngine because you could be making shaders, doing blueprints, c++, really anything
			"{\"type\": \"file\", \"time\" : %lld, \"project\": \"%s\", \"entity\": \"%s\", "
			"\"language\": \"UnrealEngine\", \"plugin\": \"UnrealEngine\", \"is_write\": false, "
			"\"user_agent\": \"unreal_engine/%s\", \"machine_name_id\": \"%s\", "
			"\"line_additions\": %d, \"line_deletions\": %d, \"operating_system\": \"%s\"}"
		),
		localTime, *ProjectName, *EntityName, *EngineVersionString,
		*ComputerName, localAddOperations, localDeleteOperations, *OSName);
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Body);
	UE_LOG(LogTemp, Warning, TEXT("waka formatting part done"));

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(TargetURL);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("UnrealEngine4"));

	FString RawBearerToken = Settings->WakatimeBearerToken.TrimStartAndEnd();
	FString AuthToken = FString::Printf(TEXT("Bearer %s"), *RawBearerToken);
	UE_LOG(LogTemp, Warning, TEXT("waka auth: %s"), *AuthToken);
	Request->SetHeader(TEXT("Authorization"), AuthToken);

	Request->SetContentAsString(Body);

	Request->OnProcessRequestComplete().BindRaw(this, &FWakatimeIntegrationModule::OnHttpResponse);

	Request->ProcessRequest();

	UE_LOG(LogTemp, Warning, TEXT("Waka: Sent Heatbeat"));
}

int64 FWakatimeIntegrationModule::GetCurrentTime()
{
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

	std::chrono::system_clock::duration duration = now.time_since_epoch();

	std::chrono::seconds seconds_duration =
		std::chrono::duration_cast<std::chrono::seconds>(duration);

	int64_t seconds_since_epoch = seconds_duration.count();

	return seconds_since_epoch; //why is it so complicated to get a timestamp bruh
}

void FWakatimeIntegrationModule::OnHttpResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("waka: heartbeat failed to connect or received no response"));
		return;
	}

	int32 ResponseCode = Response->GetResponseCode();
	FString ResponseString = Response->GetContentAsString();

	if (ResponseCode >= 200 && ResponseCode < 300) {
		UE_LOG(LogTemp, Log, TEXT("waka: heartbeat successful. code: %d"), ResponseCode);
	}
	else if (ResponseCode == 401)
	{
		UE_LOG(LogTemp, Error, TEXT("waka: heartbeat failed due to invalid api token (401). response: %s"), *ResponseString);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("waka: heartbeat failed. code: %d. response: %s"), ResponseCode, *ResponseString);
	}
}

#undef LOCTEXT_NAMESPACE
	
//IMPLEMENT_MODULE(FWakatimeIntegrationModule, WakatimeIntegration);