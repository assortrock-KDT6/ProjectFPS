#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Common/GameDatas.h"
#include "SessionEntryWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSessionJoinRequested, int32, ResultIndex);

/** Keeps the original result identity and validates join requests; presentation is Blueprint-owned. */
UCLASS(Abstract)
class PROJECTFPS_API USessionEntryWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable, Category="Session") FSessionJoinRequested _OnJoinRequested;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Session") FFPSOnlineSessionInfo SessionInfo;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Session") bool HasSpace = false;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Session") bool CanJoin = false;
    UFUNCTION(BlueprintCallable, Category="Session") void DisplaySession(const FFPSOnlineSessionInfo& Session);
    UFUNCTION(BlueprintCallable, Category="Session") void SetJoinAllowed(bool Allowed);
    UFUNCTION(BlueprintCallable, Category="Session") void RequestJoin();
protected:
    UFUNCTION(BlueprintImplementableEvent, Category="Session|View") void RefreshEntry();
    virtual void NativeConstruct() override;
private:
    bool _JoinAllowed = false;
};
