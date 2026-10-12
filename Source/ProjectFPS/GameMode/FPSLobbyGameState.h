#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Common/GameDatas.h"
#include "FPSLobbyGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLobbyChanged);

/** 
 * 로비 선택값은 서버 권한으로 관리되며, 늦게 참가한 클라이언트에도 현재 값이 복제된다. 
 */

UCLASS()
class PROJECTFPS_API AFPSLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category="Lobby") 
	FLobbyChanged _OnLobbyChanged;

private:
	UPROPERTY(ReplicatedUsing = OnRep_SelectedMap)
	FFPSPlayableMap _SelectedMap;
public:
    virtual void AddPlayerState(APlayerState* PlayerState) override;

    virtual void RemovePlayerState(APlayerState* PlayerState) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:
	void NotifyRosterChanged();
	
	const FFPSPlayableMap& GetSelectedMap() const;
	
	void SetSelectedMap(const FFPSPlayableMap& Map);

private:
    UFUNCTION() 
	void OnRep_SelectedMap();
};
