// Copyright Dale Grinsell 2024. All Rights Reserved. 

#include "MS_Utils.h"
#include "MetasoundNodeRegistrationMacro.h"

#define LOCTEXT_NAMESPACE "FMS_UtilsModule"

void FMS_UtilsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FMetasoundFrontendRegistryContainer::Get()->RegisterPendingNodes();
}

void FMS_UtilsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMS_UtilsModule, MS_Utils)