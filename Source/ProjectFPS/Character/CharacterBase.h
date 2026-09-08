// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EngineMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Component/Ability/FPSAbilitySystemComponent.h"
#include "CharacterBase.generated.h"

/*
 *		[캐릭터 기본 베이스]
 *		
 *		1) 캐릭터는 플레이어와 몬스터를 포함한다.
 *		2) 캐릭터는 기본적으로 서버에 존재하는 몬스터와 플레이어를 뜻한다.
 *		2-1) NPC의 종류가 상점만 있을 경우 Pawn 혹은 Actor를 사용하여 구현하도록 한다. [ 최적화 ]
 *		3) 각자 고유 아이디를 가지고 있다.
 *		4) 캐릭터 클래스는 시각적으로 보이는 요소들을 담당한다. (애니메이션, 움직임)
 */

UCLASS()
class PROJECTFPS_API ACharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// 캐릭터의 고유 아이디
	UPROPERTY(BlueprintReadWrite)
	FName	_Id = FName(TEXT(""));

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS | AbilitySystem")
	TObjectPtr<UFPSAbilitySystemComponent> _AbilitySystemComponent;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS | AbilitySystem")
	EGameplayEffectReplicationMode _AbilitySystemComponentReplicationMode = EGameplayEffectReplicationMode::Mixed;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void PossessedBy(AController* Newcontroller) override;

	// 플레이어가 서버에 접속하거나 만들어짐.
	virtual void OnRep_PlayerState() override;
public:
	// 캐릭터의 고유 아이디를 가져온다.
	const FName& GetId() const;

	// 캐릭터의 고유 아이디를 설정한다.
	void SetId(const FName& Id);

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const ;

};
