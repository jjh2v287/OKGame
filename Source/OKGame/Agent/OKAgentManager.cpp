// Fill out your copyright notice in the Description page of Project Settings.

#include "OKAgentManager.h"

#include "DrawDebugHelpers.h"
#include "OKAgent.h"

const FAgentHandle FAgentHandle::Invalid = FAgentHandle(0);

void UOKAgentManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UOKAgentManager::Deinitialize()
{
	Super::Deinitialize();
}

TStatId UOKAgentManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(OKAgentManager, STATGROUP_Tickables);
}

void UOKAgentManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const TSet<FAgentHashGrid2D::FCell>& AllCells = AgentGrid.GetCells();
	for (const FAgentHashGrid2D::FCell& Cell : AllCells)
	{
		FBox CellBounds = AgentGrid.CalcCellBounds(FAgentHashGrid2D::FCellLocation(Cell.X, Cell.Y, Cell.Level));
		DrawDebugBox(GetWorld(), CellBounds.GetCenter(), CellBounds.GetExtent(), GColorList.GetFColorByIndex(Cell.Level), false, -1.0f, SDPG_Foreground);
	}
}

FAgentHandle UOKAgentManager::RegisterAgent(AOKAgent* Agent)
{
	FAgentHandle NewHandle = FAgentHandle(NextHandleID++);
	if (!NewHandle.IsValid())
	{
		return FAgentHandle::Invalid;
	}
	
	if (RuntimeAgents.Contains(Agent->GetAgentHandle()))
	{
		return Agent->GetAgentHandle();
	}

	FAgentRuntimeData& RuntimeData = RuntimeAgents.Emplace(NewHandle, FAgentRuntimeData(Agent, NewHandle));

	FVector Origin, Extent;
	Agent->GetActorBounds(false, Origin, Extent);
	const FBox Box = FBox(Origin - Extent, Origin + Extent);
	RuntimeData.Bounds = Box;
	RuntimeData.Transform = Agent->GetActorTransform();

	if (FAgentHashGridEntryData* GridEntryData = RuntimeData.SpatialEntryData.GetMutablePtr<FAgentHashGridEntryData>())
	{
		GridEntryData->CellLoc = AgentGrid.Add(NewHandle, RuntimeData.Bounds);
	}
	else
	{
		RuntimeAgents.Remove(NewHandle);
		return FAgentHandle::Invalid;
	}

	return NewHandle;
}

bool UOKAgentManager::UnregisterAgent(AOKAgent* Agent)
{
	if (!IsValid(Agent))
	{
		return false;
	}

	FAgentRuntimeData RuntimeData;
	if (RuntimeAgents.RemoveAndCopyValue(Agent->GetAgentHandle(), RuntimeData))
	{
		if (const FAgentHashGridEntryData* GridEntryData = RuntimeData.SpatialEntryData.GetPtr<FAgentHashGridEntryData>())
		{
			AgentGrid.Remove(Agent->GetAgentHandle(), GridEntryData->CellLoc);
		}
		Agent->SetAgentHandle(FAgentHandle::Invalid);
		return true;
	}

	return false;
}

void UOKAgentManager::AgentMoveUpdate(const AOKAgent* Agent)
{
	if (Agent->GetAgentHandle() == FAgentHandle::Invalid)
	{
		return;
	}
	
	if (!RuntimeAgents.Contains(Agent->GetAgentHandle()))
	{
		return;
	}

	FVector Origin, Extent;
	Agent->GetActorBounds(false, Origin, Extent);
	const FBox Box = FBox(Origin - Extent, Origin + Extent);

	FAgentRuntimeData* RuntimeData = RuntimeAgents.Find(Agent->GetAgentHandle());
	if (FAgentHashGridEntryData* GridEntryData = RuntimeData->SpatialEntryData.GetMutablePtr<FAgentHashGridEntryData>())
	{
		GridEntryData->CellLoc = AgentGrid.Move(Agent->GetAgentHandle(), GridEntryData->CellLoc, Box);
	}
}

TArray<AOKAgent*> UOKAgentManager::FindCloseAgentRange(const AOKAgent* Agent, const float Range) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AOKAgent_FindCloseAgentRange);
	TArray<AOKAgent*> Agents;

	const FVector Location = Agent->GetActorLocation();
	const FVector SearchExtent(Range);
	const FBox QueryBox = FBox(Location - SearchExtent, Location + SearchExtent);

	TArray<FAgentHandle> FoundHandlesInGrid;
	AgentGrid.Query(QueryBox, FoundHandlesInGrid);

	for (const FAgentHandle& Handle : FoundHandlesInGrid)
	{
		const FAgentRuntimeData* RuntimeData = RuntimeAgents.Find(Handle);
		if (RuntimeData && RuntimeData->AgentObject.IsValid())
		{
			if (RuntimeData->AgentObject.Get() != Agent)
			{
				Agents.Emplace(RuntimeData->AgentObject.Get());
			}
		}
	}

	return Agents;
}
