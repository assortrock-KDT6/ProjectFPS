// Fill out your copyright notice in the Description page of Project Settings.


#include "Common/GameDefines.h"

namespace FCharacterStateUtils
{
	const TCHAR* FCharacterStateUtils::ToString(EFPSOnlineOperationState Type)
	{
		switch (Type)
		{
		case EFPSOnlineOperationState::Idle:
		{
			return TEXT("Idle");
		}
		case EFPSOnlineOperationState::Destroying:
		{
			return TEXT("Destroying");
		}
		case EFPSOnlineOperationState::Creating:
		{
			return TEXT("Creating");
		}
		case EFPSOnlineOperationState::Finding:
		{
			return TEXT("Finding");
		}
		case EFPSOnlineOperationState::Joining:
		{
			return TEXT("Joining");
		}
		case EFPSOnlineOperationState::Starting:
		{
			return TEXT("Starting");
		}
		case EFPSOnlineOperationState::Ending:
		{
			return TEXT("Ending");
		}
		}
		return TEXT("");
	}

	const TCHAR* FCharacterStateUtils::ToString(EFPSOnlineConnectionState Type)
	{
		switch (Type)
		{
		case EFPSOnlineConnectionState::None:
		{
			return TEXT("None");
		}
		case EFPSOnlineConnectionState::Hosting:
		{
			return TEXT("Hosting");
		}
		case EFPSOnlineConnectionState::Joined:
		{
			return TEXT("Joined");
		}
		case EFPSOnlineConnectionState::CleanupFailed:
		{
			return TEXT("CleanupFailed");
		}
		}
		return TEXT("");
	}

	const TCHAR* FCharacterStateUtils::ToString(EFPSOnlineTravelState Type)
	{
		switch (Type)
		{
		case EFPSOnlineTravelState::None:
		{
			return TEXT("None");
		}
		case EFPSOnlineTravelState::Traveling:
		{
			return TEXT("Traveling");
		}
		}
		return TEXT("");
	}
}
