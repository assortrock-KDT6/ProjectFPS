#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Common/GameDatas.h"
#include "SessionMapCatalog.generated.h"

/** Session destinations are project data, independent of any menu widget. */
UCLASS(BlueprintType)
class PROJECTFPS_API USessionMapCatalog : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Session") TSoftObjectPtr<UWorld> LobbyLevel;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Session", meta=(TitleProperty="DisplayName")) TArray<FFPSPlayableMap> Maps;
};
