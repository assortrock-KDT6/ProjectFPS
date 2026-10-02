#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Common/GameDatas.h"
#include "FPSLobbyGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLobbyChanged);

/** Lobby selection belongs to the server; late joiners receive the current value. */
UCLASS()
class PROJECTFPS_API AFPSLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category="Lobby") FLobbyChanged OnLobbyChanged;
    virtual void AddPlayerState(APlayerState* PlayerState) override;
    virtual void RemovePlayerState(APlayerState* PlayerState) override;
    void NotifyRosterChanged() { OnLobbyChanged.Broadcast(); }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	const FFPSPlayableMap& GetSelectedMap() const { return _SelectedMap; }
	void SetSelectedMap(const FFPSPlayableMap& Map);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(ReplicatedUsing=OnRep_SelectedMap)
	FFPSPlayableMap _SelectedMap;
    UFUNCTION() void OnRep_SelectedMap();
};
