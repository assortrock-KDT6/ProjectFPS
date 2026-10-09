#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TitlePlayerController.generated.h"

class UTitleScreenWidget;

UCLASS()
class PROJECTFPS_API ATitlePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATitlePlayerController();

	UFUNCTION(BlueprintCallable, Category = "Title")
	void StartFromTitle();

	UFUNCTION(BlueprintCallable, Category = "Title")
	void QuitFromTitle();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Title")
	TSoftObjectPtr<UWorld> LobbyLevel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Title")
	TSubclassOf<UTitleScreenWidget> TitleWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Title")
	bool bLeavingTitle = false;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Title|Camera")
	FName TitleCameraTag = TEXT("TitleMainCamera");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Title|UI")
	int32 TitleWidgetZOrder = 20;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Title|Transition", meta = (ClampMin = "0.0", Units = "s"))
	float TitleFadeDuration = 0.35f;

	// Travel always waits for the fade, even if this delay is shorter.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Title|Transition", meta = (ClampMin = "0.0", Units = "s"))
	float TitleTravelDelay = 0.4f;

private:
	void InitializeTitleCamera(UWorld& World);
	bool InitializeTitleWidget();
	void PauseTitleSequences(UWorld& World);
	void FinishTravel();

	UPROPERTY(Transient)
	TObjectPtr<UTitleScreenWidget> TitleWidget;

	// Keep the destination selected at click time throughout the fade.
	UPROPERTY(Transient)
	TSoftObjectPtr<UWorld> PendingLobbyLevel;

	FTimerHandle TravelTimer;
};

