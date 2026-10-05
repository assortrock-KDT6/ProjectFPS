#include "Component/MetaHumanOutfitBinderComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

UMetaHumanOutfitBinderComponent::UMetaHumanOutfitBinderComponent()
{
	PrimaryComponentTick.bCanEverTick=false;

	OutfitComponentNames = { TEXT("SWAT_Top"),TEXT("SWAT_Pants"),TEXT("SWAT_Boots") };
}

void UMetaHumanOutfitBinderComponent::OnRegister()
{
	Super::OnRegister();

	BindOutfit();
}

void UMetaHumanOutfitBinderComponent::BeginPlay()
{
	Super::BeginPlay();

	BindOutfit();
}

void UMetaHumanOutfitBinderComponent::BindOutfit()
{
	AActor* Owner = GetOwner();

	if(!IsValid(Owner)||Owner->HasAnyFlags(RF_ClassDefaultObject|RF_ArchetypeObject))
	{
		return;
	}

	TArray<USkeletalMeshComponent*> Meshes;
	
	Owner->GetComponents(Meshes);

	USkeletalMeshComponent* Body = nullptr;

	for(USkeletalMeshComponent * const Mesh : Meshes)
	{
		if(Mesh->GetFName() == BodyComponentName)
		{
			Body = Mesh;
			break;
		}
	}

	if (!Body)
	{
		return;
	}
	
	for(USkeletalMeshComponent* const Mesh : Meshes)
	{
		if (nullptr == Mesh)
		{
			continue;
		}

		if (Mesh == Body || !OutfitComponentNames.Contains(Mesh->GetFName()))
		{
			continue;
		}

		Mesh->SetLeaderPoseComponent(Body,true,false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetOwnerNoSee(Body->bOwnerNoSee);
		Mesh->SetOnlyOwnerSee(Body->bOnlyOwnerSee);
	}
}
