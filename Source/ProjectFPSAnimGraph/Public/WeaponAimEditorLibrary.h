#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WeaponAimEditorLibrary.generated.h"

class UAnimBlueprint;

// 후처리 그래프 전환을 재현하기 위한 에디터 전용 도구다. BP_Player는 접근하지 않는다.
UCLASS()
class PROJECTFPSANIMGRAPH_API UWeaponAimEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Weapon Aim|Editor")
	static bool InstallQuinnWeaponAimNode();

	UFUNCTION(BlueprintCallable, Category = "Weapon Grip|Editor")
	static bool InstallWeaponGripNode(UAnimBlueprint* Blueprint);

	UFUNCTION(BlueprintCallable, Category = "Weapon Grip|Editor")
	static bool OrderWeaponGripBeforeAim(UAnimBlueprint* Blueprint);
};
