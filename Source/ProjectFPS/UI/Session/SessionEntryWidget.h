#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Common/GameDatas.h"
#include "SessionEntryWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSessionJoinRequested, int32, ResultIndex);

/** 검색 결과의 원본 인덱스를 보관하는 참가 행. 화면상의 행 번호로 참가하지 않는다. */
UCLASS(Abstract)
class PROJECTFPS_API USessionEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Session")
	FSessionJoinRequested _OnJoinRequested;

	UFUNCTION(BlueprintCallable, Category = "Session")
	void DisplaySession(const FFPSOnlineSessionInfo& Session);

	void SetJoinAllowed(bool Allowed);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _RoomNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _DetailsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _PlayersText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _PingText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> _JoinButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> _JoinButtonText;

private:
	int32 _ResultIndex = INDEX_NONE;
	bool _HasSpace = false;
	bool _JoinAllowed = false;

	UFUNCTION()
	void HandleJoinClicked();
};
