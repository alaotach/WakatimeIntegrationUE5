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

#define LOCTEXT_NAMESPACE "FWakatimeIntegrationModule"


IMPLEMENT_MODULE(FWakatimeIntegrationModule, WakatimeIntegration)

void FWakatimeIntegrationModule::StartupModule()
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	AssetRegistry.OnAssetAdded().AddRaw(this, &FWakatimeIntegrationModule::OnAssetAdded);
	AssetRegistry.OnAssetRemoved().AddRaw(this, &FWakatimeIntegrationModule::OnAssetRemoved);
	AssetRegistry.OnAssetRenamed().AddRaw(this, &FWakatimeIntegrationModule::OnAssetRenamed);
	FCoreUObjectDelegates::OnObjectSaved.AddRaw(this, &FWakatimeIntegrationModule::OnObjectSaved);

	TimerHandle = FTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateRaw(this, &FWakatimeIntegrationModule::OnTimerTick),
		30.0f
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

bool FWakatimeIntegrationModule::OnTimerTick(float DeltaTime)
{
	UE_LOG(LogTemp, Warning, TEXT("Waka: Timer Event"));
	SendHeartbeat();
	return true;
}

void FWakatimeIntegrationModule::SendHeartbeat()
{
	FName localLastSavedName;
	int32 localDeleteOperations = 0;
	int32 localSaveOperations = 0;
	int32 localRenameOperations = 0;
	int32 localAddOperations = 0;
	{
		FScopeLock Lock(&DataLock);
		if (!Dirty) {
			return; //don't bother copying anything
		}
		localDeleteOperations = DeleteOperations;
		localSaveOperations = SaveOperations;
		localRenameOperations = RenameOperations;
		localAddOperations = AddOperations;
		localLastSavedName = LastSavedName;
	}

	/*TSharedPtr<FJsonObject> requestBody = MakeShared<FJsonObject>();
	requestBody->SetNumberField("totalBlueprintEdits", EditsToSend);
	JsonObject->SetStringField("lastSavedObject", LastSavedToSend);*/
	UE_LOG(LogTemp, Warning, TEXT("Waka: TODO(Send Heartbeat)"));
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

#undef LOCTEXT_NAMESPACE
	
//IMPLEMENT_MODULE(FWakatimeIntegrationModule, WakatimeIntegration);