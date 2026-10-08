#pragma once

#include "CoreMinimal.h"
#include "Misc/Base64.h"
#include "OnlineSessionSettings.h"

namespace FPSSessionDisplayName
{
	inline const FName LegacyKey(TEXT("FPSDISPLAYNAME"));
	inline const FName EncodedKey(TEXT("FPSDISPLAYNAMEB64V1"));

	inline void Write(FOnlineSessionSettings& Settings, const FString& DisplayName)
	{
		// Steam OSS sends UTF-8 but reads custom string settings as ANSI. Base64
		// keeps the wire value ASCII; the FString overload encodes UTF-8 bytes.
		Settings.Set(EncodedKey, FBase64::Encode(DisplayName), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		// Keep the original key for clients created before the encoded format.
		Settings.Set(LegacyKey, DisplayName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	}

	inline FString Read(const FOnlineSessionSettings& Settings)
	{
		FString EncodedName;
		FString DisplayName;
		if (Settings.Get(EncodedKey, EncodedName) && !EncodedName.IsEmpty()
			&& FBase64::Decode(EncodedName, DisplayName) && !DisplayName.IsEmpty()
			&& FBase64::Encode(DisplayName) == EncodedName)
		{
			return DisplayName;
		}

		// Old rooms do not have the versioned key. Never guess whether their
		// plain names are Base64, and do not expose invalid encoded data in UI.
		DisplayName.Reset();
		Settings.Get(LegacyKey, DisplayName);
		return DisplayName;
	}
}
