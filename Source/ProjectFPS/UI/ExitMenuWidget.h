#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ExitMenuWidget.generated.h"

class UFPSOnlineSessionSubsystem;

USTRUCT(BlueprintType)
struct FExitMenuStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Menu") 
	bool Busy = true;

	UPROPERTY(BlueprintReadOnly, Category = "Menu") 
	bool CanReturnToLobby = false;

	UPROPERTY(BlueprintReadOnly, Category = "Menu") 
	bool Host = false;

	UPROPERTY(BlueprintReadOnly, Category = "Menu") 
	bool ExitInProgress = false;
};

/** Session commands only. WBP_ExitMenu owns controls, confirmation and presentation. */
UCLASS(Abstract)
class PROJECTFPS_API UExitMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Menu")
	UFPSOnlineSessionSubsystem* GetSessionSubsystem() const;

	UFUNCTION(BlueprintPure, Category = "Menu")
	FExitMenuStatus GetExitStatus() const;

	UFUNCTION(BlueprintCallable, Category = "Menu")
	bool TryExit(bool QuitApplication);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Menu")
	void HandleExitCleanupCompleted(bool WasSuccessful, const FString& ErrorMessage);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Menu")
	void HandleBack();
};
