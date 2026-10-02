#include "UI/Session/SessionMapCardWidget.h"
#include "Engine/Texture2D.h"

void USessionMapCardWidget::DisplayMap(const FFPSPlayableMap& InMap, int32 InIndex, bool InSelected)
{
    Map = InMap;
    MapIndex = InIndex;
    Selected = InSelected;
    ThumbnailTexture = Map.Thumbnail.LoadSynchronous();
    RefreshCard();
}

void USessionMapCardWidget::SetSelected(bool InSelected)
{
    Selected = InSelected;
    RefreshCard();
}

void USessionMapCardWidget::RequestSelect()
{
    if (MapIndex != INDEX_NONE && !Map.Level.IsNull())
    { 
        OnMapSelected.Broadcast(MapIndex); 
    }
}
