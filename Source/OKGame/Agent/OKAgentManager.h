// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HierarchicalHashGrid2D.h"
#include "StructUtils/InstancedStruct.h"
#include "Subsystems/WorldSubsystem.h"
#include "OKAgentManager.generated.h"

class AOKAgent;

#pragma region Agent HashGrid
USTRUCT(BlueprintType)
struct OKGAME_API FAgentHandle
{
    GENERATED_BODY()
public:
    FAgentHandle() : ID(0) {}

    bool IsValid() const { return ID != 0; }
    void Invalidate() { ID = 0; }

    friend FString LexToString(const FAgentHandle Handle)
    {
        return FString::Printf(TEXT("Agent_0x%llu"), Handle.ID);
    }
 
    bool operator==(const FAgentHandle Other) const { return ID == Other.ID; }
    bool operator!=(const FAgentHandle Other) const { return !(*this == Other); }
    bool operator<(const FAgentHandle Other) const { return ID < Other.ID; }

    friend uint32 GetTypeHash(const FAgentHandle Handle)
    {
        return CityHash32(reinterpret_cast<const char*>(&Handle.ID), sizeof Handle.ID);
    }

private:
    // Only the UOKAgentManager can set the ID
    friend class UOKAgentManager;

    explicit FAgentHandle(const uint64 InID) : ID(InID) {}
    
    UPROPERTY(VisibleAnywhere, Category = Agent)
    uint64 ID;

public:
    static const FAgentHandle Invalid;
};

using FAgentHashGrid2D = THierarchicalHashGrid2D<2, 4, FAgentHandle>;

USTRUCT()
struct OKGAME_API FAgentSpatialEntryData
{
    GENERATED_BODY()
    virtual ~FAgentSpatialEntryData() = default;
};

USTRUCT()
struct OKGAME_API FAgentHashGridEntryData : public FAgentSpatialEntryData
{
    GENERATED_BODY()

    FAgentHashGrid2D::FCellLocation CellLoc;
};

USTRUCT()
struct OKGAME_API FAgentRuntimeData
{
    GENERATED_BODY()

public:
    FAgentRuntimeData() = default;

    explicit FAgentRuntimeData(AOKAgent* InAgent, const FAgentHandle InHandle)
        : AgentObject(InAgent)
        , Handle(InHandle)
    {
        SpatialEntryData.InitializeAs<FAgentHashGridEntryData>();
    }

    UPROPERTY(VisibleAnywhere, Category = Agent)
    TWeakObjectPtr<AOKAgent> AgentObject;

    UPROPERTY(VisibleAnywhere, Category = Agent)
    FAgentHandle Handle;

    UPROPERTY(VisibleAnywhere, Category = Agent)
    FTransform Transform;

    UPROPERTY(VisibleAnywhere, Category = Agent)
    FBox Bounds;

    UPROPERTY()
    FInstancedStruct SpatialEntryData;
};
#pragma endregion Agent HashGrid

UCLASS()
class OKGAME_API UOKAgentManager : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

    // FTickableGameObject implementation Begin
    virtual TStatId GetStatId() const override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return false; }
    virtual bool IsTickableInEditor() const override { return false; }
    // FTickableGameObject implementation End

	FAgentHandle RegisterAgent(AOKAgent* Agent);
	bool UnregisterAgent(AOKAgent* Agent);
    void AgentMoveUpdate(const AOKAgent* Agent);

    UFUNCTION(BlueprintCallable, Category = "Agent", meta = (DisplayName = "Find Agent In Range"))
    TArray<AOKAgent*> FindCloseAgentRange(const AOKAgent* Agent, const float Range = 400.0f) const;

private:
    UPROPERTY(Transient)
    TMap<FAgentHandle, FAgentRuntimeData> RuntimeAgents;

    uint64 NextHandleID = 1;

    FAgentHashGrid2D AgentGrid;
};
