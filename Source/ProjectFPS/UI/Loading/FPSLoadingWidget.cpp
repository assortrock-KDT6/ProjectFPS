#include "UI/Loading/FPSLoadingWidget.h"

void UFPSLoadingWidget::ApplyLoadingState(const FFPSLoadingScreenState& InState, UFPSLoadingScreenData* InStyle)
{
    const bool Changed = Style != InStyle || State.Phase != InState.Phase || State.Background != InState.Background
        || !State.MapName.EqualTo(InState.MapName) || !State.ModeName.EqualTo(InState.ModeName)
        || !State.StatusText.EqualTo(InState.StatusText) || State.ReadyPlayers != InState.ReadyPlayers
        || State.ExpectedPlayers != InState.ExpectedPlayers || State.CountdownSeconds != InState.CountdownSeconds;
    
    State = InState;
    Style = InStyle;

    if (Changed)
    {
        RefreshLoadingView();
    }
}

void UFPSLoadingWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Style)
    {
        RefreshLoadingView();
    }

}

FReply UFPSLoadingWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
    // Cover gameplay clicks without taking keyboard focus from the existing ESC menu.
    return FReply::Handled();
}
