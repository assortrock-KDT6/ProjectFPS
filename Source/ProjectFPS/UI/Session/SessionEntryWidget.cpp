#include "UI/Session/SessionEntryWidget.h"

void USessionEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetJoinAllowed(_JoinAllowed);
}

void USessionEntryWidget::DisplaySession(const FFPSOnlineSessionInfo& Session)
{
    SessionInfo = Session;
    HasSpace = Session._MaxPlayers > 0 && Session._CurrentPlayers < Session._MaxPlayers;
    SetJoinAllowed(_JoinAllowed);
}

void USessionEntryWidget::SetJoinAllowed(bool Allowed)
{
    _JoinAllowed = Allowed;
    CanJoin = Allowed && HasSpace && SessionInfo._ResultIndex != INDEX_NONE;
    RefreshEntry();
}

void USessionEntryWidget::RequestJoin()
{
    if (CanJoin) { _OnJoinRequested.Broadcast(SessionInfo._ResultIndex); }
}
