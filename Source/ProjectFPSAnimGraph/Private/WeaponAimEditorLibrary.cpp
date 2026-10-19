#include "WeaponAimEditorLibrary.h"

#include "AnimGraphNode_WeaponAim.h"
#include "AnimGraphNode_WeaponGrip.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_LocalToComponentSpace.h"
#include "AnimGraphNode_ComponentToLocalSpace.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphSchema.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "ScopedTransaction.h"

namespace WeaponAimEditor
{
	template<typename T>
	T* AddNode(UEdGraph* Graph, int32 X, int32 Y)
	{
		FGraphNodeCreator<T> Creator(*Graph);
		T* Node = Creator.CreateNode();
		Node->NodePosX = X;
		Node->NodePosY = Y;
		Creator.Finalize();
		return Node;
	}

	UEdGraphPin* FindPosePin(UEdGraphNode* Node, EEdGraphPinDirection Direction)
	{
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->Direction == Direction && Pin->PinType.PinCategory == TEXT("struct")
				&& (Pin->PinType.PinSubCategoryObject == FPoseLink::StaticStruct()
					|| Pin->PinType.PinSubCategoryObject == FComponentSpacePoseLink::StaticStruct()))
			{
				return Pin;
			}
		}

		return nullptr;
	}
}

bool UWeaponAimEditorLibrary::InstallQuinnWeaponAimNode()
{
	// 수정 대상을 고정하여 캐릭터 BP와 다른 애니메이션 에셋은 건드리지 않는다.
	UAnimBlueprint* Blueprint = LoadObject<UAnimBlueprint>(nullptr,
		TEXT("/Game/Characters/Heroes/Mannequin/Rig/ABP_Quinn_PostProcess.ABP_Quinn_PostProcess"));
	if (false == IsValid(Blueprint))
	{
		return false;
	}

	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	for (UEdGraph* Graph : Graphs)
	{
		if (Graph->GetFName() != TEXT("AnimGraph"))
		{
			continue;
		}

		UAnimGraphNode_Root* Root = nullptr;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (true == Node->IsA<UAnimGraphNode_WeaponAim>())
			{
				return true;
			}

			UAnimGraphNode_Root* Candidate = Cast<UAnimGraphNode_Root>(Node);
			if (nullptr != Candidate)
			{
				Root = Candidate;
			}
		}

		UEdGraphPin* Input = nullptr != Root ? WeaponAimEditor::FindPosePin(Root, EGPD_Input) : nullptr;
		if (nullptr == Input || Input->LinkedTo.Num() != 1)
		{
			return false;
		}

		const FScopedTransaction Transaction(NSLOCTEXT("ProjectFPS", "InstallWeaponAim", "총구 조준 보정 노드 연결"));
		Blueprint->Modify();
		Graph->Modify();
		Root->Modify();
		UEdGraphPin* Source = Input->LinkedTo[0];

		UAnimGraphNode_LocalToComponentSpace* ToComponent = WeaponAimEditor::AddNode<UAnimGraphNode_LocalToComponentSpace>(
			Graph, Root->NodePosX, Root->NodePosY);
		UAnimGraphNode_WeaponAim* Aim = WeaponAimEditor::AddNode<UAnimGraphNode_WeaponAim>(
			Graph, Root->NodePosX + 250, Root->NodePosY);
		UAnimGraphNode_ComponentToLocalSpace* ToLocal = WeaponAimEditor::AddNode<UAnimGraphNode_ComponentToLocalSpace>(
			Graph, Root->NodePosX + 550, Root->NodePosY);

		const UEdGraphSchema* Schema = Graph->GetSchema();
		Schema->BreakSinglePinLink(Source, Input);
		const bool Connected =
			Schema->TryCreateConnection(Source, WeaponAimEditor::FindPosePin(ToComponent, EGPD_Input))
			&& Schema->TryCreateConnection(WeaponAimEditor::FindPosePin(ToComponent, EGPD_Output), WeaponAimEditor::FindPosePin(Aim, EGPD_Input))
			&& Schema->TryCreateConnection(WeaponAimEditor::FindPosePin(Aim, EGPD_Output), WeaponAimEditor::FindPosePin(ToLocal, EGPD_Input))
			&& Schema->TryCreateConnection(WeaponAimEditor::FindPosePin(ToLocal, EGPD_Output), Input);

		if (false == Connected)
		{
			Graph->RemoveNode(ToComponent);
			Graph->RemoveNode(Aim);
			Graph->RemoveNode(ToLocal);
			Schema->TryCreateConnection(Source, Input);
			return false;
		}

		Root->NodePosX += 800;
		Blueprint->ParentClass = UAnimInstance::StaticClass();
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		return Blueprint->Status != BS_Error;
	}

	return false;
}

bool UWeaponAimEditorLibrary::InstallWeaponGripNode(UAnimBlueprint* Blueprint)
{
	if (false == IsValid(Blueprint))
	{
		return false;
	}

	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	for (UEdGraph* Graph : Graphs)
	{
		if (Graph->GetFName() != TEXT("AnimGraph"))
		{
			continue;
		}

		UAnimGraphNode_Root* Root = nullptr;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (true == Node->IsA<UAnimGraphNode_WeaponGrip>())
			{
				return true;
			}

			UAnimGraphNode_Root* Candidate = Cast<UAnimGraphNode_Root>(Node);
			if (nullptr != Candidate)
			{
				Root = Candidate;
			}
		}

		UEdGraphPin* Input = nullptr != Root ? WeaponAimEditor::FindPosePin(Root, EGPD_Input) : nullptr;
		if (nullptr == Input || Input->LinkedTo.Num() != 1)
		{
			return false;
		}

		const FScopedTransaction Transaction(NSLOCTEXT("ProjectFPS", "InstallWeaponGrip", "무기 양손 IK 노드 연결"));
		Blueprint->Modify();
		Graph->Modify();
		Root->Modify();
		UEdGraphPin* Source = Input->LinkedTo[0];

		UAnimGraphNode_LocalToComponentSpace* ToComponent = WeaponAimEditor::AddNode<UAnimGraphNode_LocalToComponentSpace>(
			Graph, Root->NodePosX, Root->NodePosY);
		UAnimGraphNode_WeaponGrip* Aim = WeaponAimEditor::AddNode<UAnimGraphNode_WeaponGrip>(
			Graph, Root->NodePosX + 250, Root->NodePosY);
		UAnimGraphNode_ComponentToLocalSpace* ToLocal = WeaponAimEditor::AddNode<UAnimGraphNode_ComponentToLocalSpace>(
			Graph, Root->NodePosX + 550, Root->NodePosY);

		const UEdGraphSchema* Schema = Graph->GetSchema();
		Schema->BreakSinglePinLink(Source, Input);
		const bool Connected =
			Schema->TryCreateConnection(Source, WeaponAimEditor::FindPosePin(ToComponent, EGPD_Input))
			&& Schema->TryCreateConnection(WeaponAimEditor::FindPosePin(ToComponent, EGPD_Output), WeaponAimEditor::FindPosePin(Aim, EGPD_Input))
			&& Schema->TryCreateConnection(WeaponAimEditor::FindPosePin(Aim, EGPD_Output), WeaponAimEditor::FindPosePin(ToLocal, EGPD_Input))
			&& Schema->TryCreateConnection(WeaponAimEditor::FindPosePin(ToLocal, EGPD_Output), Input);

		if (false == Connected)
		{
			Graph->RemoveNode(ToComponent);
			Graph->RemoveNode(Aim);
			Graph->RemoveNode(ToLocal);
			Schema->TryCreateConnection(Source, Input);
			return false;
		}

		Root->NodePosX += 800;
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		return Blueprint->Status != BS_Error;
	}

	return false;
}



bool UWeaponAimEditorLibrary::OrderWeaponGripBeforeAim(UAnimBlueprint* Blueprint)
{
	if (false == IsValid(Blueprint))
	{
		return false;
	}
	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	for (UEdGraph* Graph : Graphs)
	{
		if (Graph->GetFName() != TEXT("AnimGraph"))
		{
			continue;
		}
		UAnimGraphNode_WeaponAim* Aim = nullptr;
		UAnimGraphNode_WeaponGrip* Grip = nullptr;
		UAnimGraphNode_Root* Root = nullptr;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (true == Node->IsA<UAnimGraphNode_WeaponAim>())
			{
				Aim = Cast<UAnimGraphNode_WeaponAim>(Node);
			}
			if (true == Node->IsA<UAnimGraphNode_WeaponGrip>())
			{
				Grip = Cast<UAnimGraphNode_WeaponGrip>(Node);
			}
			if (true == Node->IsA<UAnimGraphNode_Root>())
			{
				Root = Cast<UAnimGraphNode_Root>(Node);
			}
		}
		if (nullptr == Aim || nullptr == Grip || nullptr == Root)
		{
			return false;
		}
		// 출력에서 역순으로 검사한다. 이미 총구 보정이 마지막이면 다시 교환하지 않는다.
		UEdGraphNode* Cursor = Root;
		for (int32 Index = 0; Index < 16; ++Index)
		{
			if (Cursor == Aim)
			{
				return true;
			}
			if (Cursor == Grip)
			{
				break;
			}
			UEdGraphPin* Pin = WeaponAimEditor::FindPosePin(Cursor, EGPD_Input);
			if (nullptr == Pin || Pin->LinkedTo.Num() != 1)
			{
				return false;
			}
			Cursor = Pin->LinkedTo[0]->GetOwningNode();
		}
		UEdGraphPin* AimIn = WeaponAimEditor::FindPosePin(Aim, EGPD_Input);
		UEdGraphPin* AimOut = WeaponAimEditor::FindPosePin(Aim, EGPD_Output);
		UEdGraphPin* GripIn = WeaponAimEditor::FindPosePin(Grip, EGPD_Input);
		UEdGraphPin* GripOut = WeaponAimEditor::FindPosePin(Grip, EGPD_Output);
		if (AimIn->LinkedTo.Num() != 1 || AimOut->LinkedTo.Num() != 1
			|| GripIn->LinkedTo.Num() != 1 || GripOut->LinkedTo.Num() != 1)
		{
			return false;
		}
		UEdGraphPin* BeforeAim = AimIn->LinkedTo[0];
		UEdGraphPin* AfterAim = AimOut->LinkedTo[0];
		UEdGraphPin* BeforeGrip = GripIn->LinkedTo[0];
		UEdGraphPin* AfterGrip = GripOut->LinkedTo[0];
		Blueprint->Modify();
		Graph->Modify();
		const UEdGraphSchema* Schema = Graph->GetSchema();
		for (UEdGraphPin* Pin : {AimIn, AimOut, GripIn, GripOut})
		{
			Schema->BreakPinLinks(*Pin, true);
		}
		const bool Connected = Schema->TryCreateConnection(BeforeAim, GripIn)
			&& Schema->TryCreateConnection(GripOut, AfterAim)
			&& Schema->TryCreateConnection(BeforeGrip, AimIn)
			&& Schema->TryCreateConnection(AimOut, AfterGrip);
		if (false == Connected)
		{
			for (UEdGraphPin* Pin : {AimIn, AimOut, GripIn, GripOut})
			{
				Schema->BreakPinLinks(*Pin, true);
			}
			Schema->TryCreateConnection(BeforeAim, AimIn);
			Schema->TryCreateConnection(AimOut, AfterAim);
			Schema->TryCreateConnection(BeforeGrip, GripIn);
			Schema->TryCreateConnection(GripOut, AfterGrip);
			return false;
		}
		Swap(Aim->NodePosX, Grip->NodePosX);
		Swap(Aim->NodePosY, Grip->NodePosY);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		return Blueprint->Status != BS_Error;
	}
	return false;
}
