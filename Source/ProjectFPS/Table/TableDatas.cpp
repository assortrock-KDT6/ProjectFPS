// Fill out your copyright notice in the Description page of Project Settings.


#include "Table/TableDatas.h"
#include "Components/MeshComponent.h"

void FItemData::ApplyTint(UMeshComponent* Mesh) const
{
	if (false == _bUseTint || nullptr == Mesh)
	{
		return;
	}

	// 무기·수류탄 머티리얼(M_Weapon 계열)이 공통으로 가진 색 파라미터
	static const FName TintParameterName(TEXT("Team.WeaponTint"));

	// 머티리얼마다 동적 인스턴스를 만들어서 색만 바꾼다. (원본 머티리얼 에셋은 그대로)
	Mesh->SetVectorParameterValueOnMaterials(TintParameterName, FVector(_TintColor));
}
