// Copyright Epic Games, Inc. All Rights Reserved.

#include "WakatimeIntegration.h"

#define LOCTEXT_NAMESPACE "FWakatimeIntegrationModule"

void FWakatimeIntegrationModule::StartupModule()
{
	UE_LOG(LogTemp, Warning, TEXT("Wakatime Integration Startup"));
}

void FWakatimeIntegrationModule::ShutdownModule()
{
	UE_LOG(LogTemp, Warning, TEXT("Wakatime Integration Shutdown"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FWakatimeIntegrationModule, WakatimeIntegration)