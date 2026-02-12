# UOKNavMeshGenerateComponent 기술 분석 보고서

---

## 1. 개요

`UOKNavMeshGenerateComponent`는 **물리적 스태틱 메쉬 지오메트리 없이 NavMesh 영역 속성을 수정**하는 UE5 커스텀 컴포넌트이다.

`UBoxComponent`를 상속하여 보이지 않는 박스 볼륨을 정의하고, `GetNavigationData()`를 오버라이드하여 해당 볼륨 내의 NavMesh 폴리곤에 원하는 Area Class를 적용한다. 이를 통해 렌더링, 물리, 콜리전 없이도 AI 경로 탐색의 비용과 접근성을 영역별로 제어할 수 있다.

**파일 위치:**
- 헤더: `Source/OKGame/Components/OKNavMeshGenerateComponent.h`
- 소스: `Source/OKGame/Components/OKNavMeshGenerateComponent.cpp`

---

## 2. 작동 원리 (HOW)

### 2.1 UE5 Navigation Discovery 파이프라인

UE5의 네비게이션 시스템은 아래 단계를 거쳐 컴포넌트로부터 NavMesh 데이터를 수집한다:

```
[1] UPrimitiveComponent 월드에 등록
         │
         ▼
[2] CanEverAffectNavigation() == true 확인
         │
         ▼
[3] NavigationSystem이 내부 목록에 컴포넌트 등록
         │
         ▼
[4] NavMesh 빌드/업데이트 시 IsNavigationRelevant() 호출
         │
         ▼
[5] GetNavigationData(FNavigationRelevantData&) 호출
         │
         ▼
[6] FNavigationRelevantData에 Modifier 또는 Geometry 데이터 주입
         │
         ▼
[7] Recast 엔진이 타일별로 NavMesh 생성/수정
```

`UOKNavMeshGenerateComponent`는 `UBoxComponent → UShapeComponent → UPrimitiveComponent` 상속 체인을 통해 이 파이프라인에 자연스럽게 진입한다.

### 2.2 GetNavigationData() 오버라이드 — 핵심 동작

```cpp
// OKNavMeshGenerateComponent.cpp:31-41
void UOKNavMeshGenerateComponent::GetNavigationData(FNavigationRelevantData& Data) const
{
    if (AreaClass)
    {
        if (AreaClass != FNavigationSystem::GetDefaultWalkableArea() || AreaClassToReplace)
        {
            Data.Modifiers.CreateAreaModifiers(this, AreaClass, AreaClassToReplace);
        }
    }
}
```

**단계별 동작:**

1. **`AreaClass` 유효성 확인**: nullptr이 아닌지 체크
2. **조건 필터링**: 아래 두 조건 중 하나라도 만족할 때만 모디파이어 생성
   - `AreaClass`가 기본 Walkable Area가 **아님** (의미 있는 변경이 있음)
   - `AreaClassToReplace`가 **설정됨** (선택적 교체가 필요함)
3. **`CreateAreaModifiers()` 호출**: 핵심 함수

### 2.3 CreateAreaModifiers()의 내부 메커니즘

`Data.Modifiers.CreateAreaModifiers(this, AreaClass, AreaClassToReplace)` 호출 시 UE5 엔진 내부에서 다음이 발생한다:

1. `this`(UBoxComponent)의 **월드 공간 바운딩 박스**를 획득
2. 박스의 **8개 꼭짓점**을 월드 좌표로 변환
3. 꼭짓점들로 **Convex Volume**(볼록 다면체)을 구성
4. `FAreaNavModifier`를 생성:
   - **볼륨**: 위에서 구성한 Convex Volume
   - **적용할 Area**: `AreaClass`
   - **교체 대상 Area**: `AreaClassToReplace` (선택적)
5. `FCompositeNavModifier`에 이 모디파이어를 추가

Recast NavMesh 빌드 시:
- 각 타일에서 모디파이어의 Convex Volume과 교차하는 폴리곤을 찾음
- 교차 폴리곤을 볼륨 경계에서 분할
- 볼륨 내부 폴리곤의 Area Class 플래그를 `AreaClass`로 변경
- `AreaClassToReplace`가 설정된 경우, 해당 Area의 폴리곤**만** 수정

### 2.4 Geometry 기반 vs Modifier 기반 비교

| 구분 | Geometry 기반 (Static Mesh) | Modifier 기반 (이 컴포넌트) |
|------|---------------------------|---------------------------|
| **원리** | 콜리전 삼각형 → Voxel 하이트필드 → NavMesh 폴리곤 생성 | 기존 NavMesh 폴리곤의 Area Class를 변경 |
| **역할** | 걸을 수 있는 **표면 자체**를 정의 | 이미 존재하는 표면의 **속성**을 수정 |
| **지오메트리** | 필수 (메쉬/콜리전) | 불필요 (수학적 볼륨만 사용) |
| **렌더링** | 보통 있음 | 없음 (보이지 않는 볼륨) |
| **물리 상호작용** | 있음 (콜리전) | 없음 (NoCollision) |
| **성능 비용** | 복셀화 + 폴리곤 생성 | 모디파이어 적용만 |

### 2.5 생성자 설계 의도

```cpp
// OKNavMeshGenerateComponent.cpp:11-19
UOKNavMeshGenerateComponent::UOKNavMeshGenerateComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , AreaClass(UNavArea_Default::StaticClass())
{
    PrimaryComponentTick.bCanEverTick = false;
    SetGenerateOverlapEvents(false);
    CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
    BodyInstance.SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}
```

| 설정 | 값 | 의도 |
|------|-----|------|
| `bCanEverTick` | `false` | 매 프레임 업데이트 불필요. 선언적(declarative) 컴포넌트 — NavSystem이 필요 시 쿼리 |
| `GenerateOverlapEvents` | `false` | 물리적 오버랩 이벤트 비활성화 — 순수 네비게이션 용도 |
| `CanCharacterStepUpOn` | `ECB_No` | 캐릭터가 이 컴포넌트 위에 올라설 수 없음 |
| `CollisionProfile` | `NoCollision` | 모든 물리 시뮬레이션에서 제외 |
| `AreaClass` 기본값 | `UNavArea_Default` | 기본 상태에서는 효과 없음 (조건문에서 스킵) |

이 컴포넌트는 **물리적으로 존재하지 않는 순수 네비게이션 메타데이터 볼륨**이다.

### 2.6 에디터 시간 NavArea 델리게이트 추적

```cpp
// OKNavMeshGenerateComponent.cpp:43-53
void UOKNavMeshGenerateComponent::PostInitProperties()
{
    Super::PostInitProperties();
#if WITH_EDITOR
    if (GIsEditor && !HasAnyFlags(RF_ClassDefaultObject))
    {
        OnNavAreaRegisteredDelegateHandle = UNavigationSystemBase::OnNavAreaRegisteredDelegate()
            .AddUObject(this, &UOKNavMeshGenerateComponent::OnNavAreaRegistered);
        OnNavAreaUnregisteredDelegateHandle = UNavigationSystemBase::OnNavAreaUnregisteredDelegate()
            .AddUObject(this, &UOKNavMeshGenerateComponent::OnNavAreaUnregistered);
    }
#endif
}
```

**동작:**
- 에디터에서 NavArea 클래스가 **등록/해제**될 때 (예: 새 커스텀 NavArea 블루프린트 생성/삭제)
- `FNavigationSystem::UpdateActorData(*GetOwner())`를 호출하여 NavMesh를 즉시 갱신
- `#if WITH_EDITOR` 가드로 **에디터 전용** — 쉬핑 빌드에서는 이 오버헤드 없음
- `!HasAnyFlags(RF_ClassDefaultObject)`로 CDO(Class Default Object)에서는 바인딩하지 않음

**`BeginDestroy()`**에서 델리게이트를 해제하여 메모리 누수를 방지한다.

---

## 3. 왜 이렇게 해야 하는가 (WHY)

### 3.1 기본 UE5 NavMesh의 한계

기본적으로 UE5는 NavMesh를 다음에서만 생성한다:
- **Static Mesh** (콜리전 포함)
- **Landscape**
- **BSP Brush**

이 방식의 근본 가정: **걸을 수 있는 표면 = 물리적 지오메트리가 존재**

이 가정이 성립하지 않는 상황:

| 문제 상황 | 설명 |
|----------|------|
| **영역별 이동 비용 조절** | 같은 바닥 위에서도 특정 구역의 이동 비용을 높여 AI가 우회하게 하고 싶을 때 |
| **동적 경로 변경** | 게임 중 특정 영역의 네비게이션 속성을 변경해야 할 때 |
| **선택적 Area 교체** | 특정 Area 타입만 골라서 다른 타입으로 바꾸고 싶을 때 |
| **프로토타이핑** | 최소한의 지오메트리로 네비게이션 동작을 테스트할 때 |

### 3.2 ANavModifierVolume vs 커스텀 컴포넌트

UE5에는 이미 `ANavModifierVolume` 액터가 존재하지만, 커스텀 컴포넌트를 만든 이유가 있다:

| 비교 항목 | ANavModifierVolume | UOKNavMeshGenerateComponent |
|----------|-------------------|---------------------------|
| **유형** | 독립 **액터** | **컴포넌트** (다른 액터에 부착) |
| **배치** | 별도로 월드에 배치 필요 | 기존 게임플레이 액터에 AddComponent |
| **생명주기** | 독립적으로 관리 | 소유 액터와 함께 자동 생성/파괴 |
| **프로그래밍 제어** | 별도 액터 참조 필요 | `GetOwner()`를 통해 소유 액터에서 직접 제어 |
| **동적 스폰** | 액터 스폰 필요 | 컴포넌트 추가만으로 충분 |
| **조합성** | 낮음 (1 액터 = 1 볼륨) | 높음 (1 액터에 여러 컴포넌트 부착 가능) |

### 3.3 컴포넌트 기반의 조합성 이점

- **적 스포너**에 부착: 스폰 지점 주변의 네비게이션 비용 자동 조절
- **트리거 존**에 부착: 영역 진입/이탈에 따른 동적 네비게이션 변경
- **전투 영역 액터**에 부착: 전투 상태에 따른 AI 경로 유도
- **프로시저럴 액터**에 부착: 런타임 생성 오브젝트에 네비게이션 속성 자동 포함

---

## 4. 설정 변경 및 활용 방안 (WHAT)

### 4.1 AreaClass 설정 옵션

`AreaClass` 프로퍼티(`TSubclassOf<UNavArea>`)로 볼륨 내 NavMesh 폴리곤의 Area 타입을 지정한다.

| NavArea 클래스 | 기본 비용 | 효과 | 활용 예 |
|---------------|----------|------|---------|
| `UNavArea_Default` | 1.0 | 기본 보행 영역 | 이 컴포넌트에서는 효과 없음 (조건문 스킵) |
| `UNavArea_Obstacle` | 매우 높음 | 영역을 장애물로 표시 | AI 진입 금지 구역 |
| `UNavArea_Null` | 통과 불가 | 경로 탐색에서 완전 제외 | 절대 진입 불가 영역 |
| **커스텀 UNavArea** | 사용자 정의 | `DefaultCost`, `FixedAreaEnteringCost` 자유 설정 | 고비용 회피 구역, 저비용 선호 경로 |

**커스텀 NavArea 생성 시 설정 가능한 값:**
- `DefaultCost`: 해당 Area를 통과하는 비용 (기본 1.0, 높을수록 우회 유도)
- `FixedAreaEnteringCost`: 해당 Area에 진입하는 순간 추가되는 고정 비용
- `DrawColor`: 에디터에서의 시각화 색상

### 4.2 AreaClassToReplace 선택적 교체

| AreaClassToReplace 값 | 동작 |
|----------------------|------|
| `nullptr` (기본값) | 볼륨 내 **모든** NavMesh 폴리곤에 `AreaClass` 적용 |
| 특정 NavArea 클래스 | 해당 Area 타입의 폴리곤**만** `AreaClass`로 교체 |

**활용 예:**
- `AreaClassToReplace = NavArea_Default`, `AreaClass = CustomHighCost`
  → 기본 보행 영역만 고비용으로 변경, 이미 장애물인 곳은 유지
- `AreaClassToReplace = CustomHighCost`, `AreaClass = NavArea_Default`
  → 고비용 영역을 다시 기본 비용으로 복원

### 4.3 박스 크기/위치 조절

`UBoxComponent` 상속이므로 다음 방법으로 볼륨을 조절할 수 있다:

```cpp
// 코드에서 박스 크기 설정 (Half Extent)
NavMeshComp->SetBoxExtent(FVector(500.f, 500.f, 200.f));

// 위치 조절
NavMeshComp->SetWorldLocation(FVector(1000.f, 0.f, 0.f));

// 변경 후 NavMesh 갱신 알림
FNavigationSystem::UpdateActorData(*GetOwner());
```

- **비균일 크기**: X, Y, Z 각각 다르게 설정 가능 (직육면체)
- **회전**: 소유 액터의 Rotation이 볼륨에 반영됨
- **스케일**: 액터 스케일도 볼륨 크기에 영향

### 4.4 실전 활용 패턴

#### 패턴 A: AI 회피 구역 (Soft Avoidance)

```
설정:
- AreaClass = 커스텀 NavArea (DefaultCost = 10.0)
- AreaClassToReplace = nullptr
- BoxExtent = 위험 영역 크기

효과:
- AI가 가능하면 이 영역을 우회
- 다른 경로가 없으면 통과 가능 (Soft Block)
- 위험 지역, 함정 주변에 적합
```

#### 패턴 B: 동적 장벽 (Dynamic Barrier)

```
설정:
- AreaClass = UNavArea_Null
- 런타임에 SetBoxExtent()로 크기 조절
- 변경 후 FNavigationSystem::UpdateActorData() 호출

효과:
- 해당 영역을 경로 탐색에서 완전 제외
- 문/게이트 열림/닫힘에 따른 동적 경로 차단/개방
```

#### 패턴 C: 전투 링 구성 (OKCombatToken 시스템 연동)

```
구성:
- 플레이어 주변에 동심원 형태로 복수 컴포넌트 배치
  - 내부 링 (반경 300): AreaClass = 저비용 Area → 근접 공격 유도
  - 외부 링 (반경 800): AreaClass = 고비용 Area → 중거리 배회 억제

동적 조절:
- OKCombatTokenManager의 전투 페이즈에 따라 박스 크기 변경
- 공격 페이즈: 내부 링 확대 → 더 많은 AI가 접근
- 방어 페이즈: 외부 링 확대 → AI 분산 유도
```

#### 패턴 D: OKAgent 시스템과의 전략적 조합

```
두 레벨의 AI 이동 제어:

[전략적 레벨] UOKNavMeshGenerateComponent
  → 경로 탐색(Pathfinding) 단계에서 영역 선호도 결정
  → "어디로 갈 것인가"를 제어

[전술적 레벨] OKAgent (분리력 + 예측 회피)
  → 프레임 단위 로컬 충돌 회피
  → "어떻게 갈 것인가"를 제어

조합 효과:
  → 전략적으로 올바른 경로를 선택하면서도
  → 전술적으로 다른 에이전트와 충돌 없이 이동
```

---

## 5. 주의사항 및 제한

| 항목 | 설명 |
|------|------|
| **기존 지오메트리 필수** | 이 컴포넌트는 기존 NavMesh 폴리곤을 **수정**하는 것이지, 빈 공간에 새 NavMesh를 **생성**하지 않는다. 바닥 지오메트리(Static Mesh, Landscape 등)가 반드시 있어야 효과 발생 |
| **타일 재생성 비용** | 런타임에 `UpdateActorData()` 호출 시 해당 NavMesh 타일이 재빌드됨. 큰 볼륨이 여러 타일에 걸치면 비용 증가. `ARecastNavMesh`의 타일 크기 고려 필요 |
| **Z축 제한** | NavMesh는 본질적으로 2D 표면 투영. 3D 볼륨이지만 다층 건물에서는 박스 높이를 신중하게 설정해야 의도하지 않은 층에 영향을 미치지 않음 |
| **기본값 무효과** | 생성자에서 `AreaClass = UNavArea_Default`로 설정. 별도의 AreaClass를 지정하지 않으면 아무 효과 없음 |
| **모디파이어 우선순위** | 여러 Modifier 볼륨이 겹칠 때, NavMesh 빌드 시 처리 순서에 따라 결과가 달라질 수 있음 |

---

## 6. 요약

`UOKNavMeshGenerateComponent`는 UE5 Navigation 파이프라인의 `GetNavigationData()` 가상 함수를 오버라이드하여, **UBoxComponent의 수학적 바운딩 박스를 Modifier Volume으로 활용**하는 경량 컴포넌트이다.

**핵심 메커니즘:** `FCompositeNavModifier::CreateAreaModifiers()`가 BoxComponent의 월드 공간 꼭짓점으로 Convex Volume을 구성하고, 이 볼륨과 교차하는 기존 NavMesh 폴리곤의 Area Class를 변경한다.

**존재 이유:** 기본 UE5 NavMesh는 물리적 지오메트리에서만 생성되어 영역별 비용 조절이나 동적 변경이 어렵다. 이 컴포넌트는 기존 액터에 부착 가능한 컴포넌트 형태로, 보이지 않는 볼륨을 통해 NavMesh 속성을 유연하게 제어할 수 있게 한다.

**활용 가치:** AI 회피 구역, 동적 장벽, 전투 링 구성 등 다양한 패턴으로 활용 가능하며, OKAgent의 로컬 회피 시스템과 조합하면 전략적 경로 선택 + 전술적 충돌 회피의 이중 구조를 구현할 수 있다.

---
---

# 부록: FNavigationRelevantData 엔진 전수 조사

> UE 5.7 엔진 소스 코드 기반 전수 조사 결과

---

## 부록 A: FNavigationRelevantData 전체 필드 레퍼런스

**소스 위치:** `Engine/Source/Runtime/Engine/Classes/AI/Navigation/NavigationRelevantData.h` (Lines 39-176)

`FNavigationRelevantData`는 네비게이션 시스템이 개별 컴포넌트/액터로부터 수집하는 **모든 네비게이션 관련 데이터를 담는 컨테이너**이다. `FNavigationOctree`의 각 노드(`FNavigationOctreeElement`)가 이 구조체의 인스턴스를 보관한다.

### A.1 전체 멤버 변수 목록

| # | 필드 | 타입 | 설명 | GetNavigationData에서 수정 가능 |
|---|------|------|------|-------------------------------|
| 1 | **`CollisionData`** | `TNavStatArray<uint8>` | 내보낸 콜리전 지오메트리. Recast가 `FRecastGeometryCache`로 해석 | △ 직접 수정은 드묾. `FRecastGeometryExport`가 `DoCustomNavigableGeometryExport()` 경로로 채움 |
| 2 | **`VoxelData`** | `TNavStatArray<uint8>` | 캐시된 복셀 데이터. Recast가 `FRecastVoxelCache`로 해석 | × 엔진 내부 캐시 전용 |
| 3 | **`Bounds`** | `FBox` | 지오메트리 바운딩 박스 (언리얼 좌표계) | △ 지오메트리 익스포트 시 자동 계산. 직접 설정 가능하나 비권장 |
| 4 | **`NavDataPerInstanceTransformDelegate`** | `FNavDataPerInstanceTransformDelegate` | 인스턴스별 트랜스폼을 제공하는 델리게이트 | ○ `UInstancedStaticMeshComponent`가 설정 |
| 5 | **`ShouldUseGeometryDelegate`** | `FFilterNavDataDelegate` | 특정 `FNavDataConfig`에 대해 이 지오메트리를 사용할지 필터링 | ○ 커스텀 필터 바인딩 가능 |
| 6 | **`Modifiers`** | **`FCompositeNavModifier`** | **Area, 오프메쉬 링크 등 모디파이어 집합체** | **◎ 모든 GetNavigationData 오버라이드의 주요 수정 대상** |
| 7 | **`SourceElement`** | `TSharedRef<const FNavigationElement>` | 이 데이터의 원본 네비게이션 엘리먼트 참조 | × 읽기 전용 (생성자에서 설정) |
| 8 | `bPendingLazyGeometryGathering` | `uint32 : 1` | 지오메트리의 지연 수집 대기 플래그 | × 엔진(FNavigationOctree)이 관리 |
| 9 | `bPendingLazyModifiersGathering` | `uint32 : 1` | 모디파이어의 지연 수집 대기 플래그 | × 엔진이 관리 |
| 10 | `bPendingChildLazyModifiersGathering` | `uint32 : 1` | 자식 엘리먼트 모디파이어의 지연 수집 대기 | × 엔진이 관리 |
| 11 | `bSupportsGatheringGeometrySlices` | `uint32 : 1` | 지오메트리 슬라이스 단위 수집 지원 여부 | ○ 설정 가능 (대형 Landscape 등에 사용) |
| 12 | `bShouldSkipDirtyAreaOnAddOrRemove` | `uint32 : 1` | 옥트리 추가/제거 시 Dirty Area 마킹 생략 | ○ 설정 가능 (성능 최적화) |
| 13 | `bLoadedData` | `uint32 : 1` | 레벨 로딩에서 온 데이터인지 여부 | × 엔진이 관리 |

> **범례:** ◎ = 주요 수정 대상, ○ = 수정 가능, △ = 가능하나 일반적이지 않음, × = 수정 불가/비권장

### A.2 핵심 발견: Modifiers가 유일한 주요 수정 대상

엔진 전체에서 `GetNavigationData()`를 오버라이드하는 **모든 클래스가 `Data.Modifiers`를 수정**한다. `CollisionData`, `VoxelData`, `Bounds`는 별도의 지오메트리 익스포트 경로(`DoCustomNavigableGeometryExport` → `FRecastGeometryExport`)를 통해 채워지며, `GetNavigationData()` 내에서 직접 수정하는 경우는 없다.

### A.3 주요 쿼리 메서드

| 메서드 | 반환 | 설명 |
|--------|------|------|
| `HasGeometry()` | `bool` | `CollisionData`가 비어있지 않은지 |
| `HasModifiers()` | `bool` | `Modifiers`에 Area 또는 링크가 있는지 |
| `HasDynamicModifiers()` | `bool` | 동적(per-instance) 모디파이어가 있는지 |
| `IsEmpty()` | `bool` | 지오메트리도 모디파이어도 없는지 |
| `GetModifierForAgent(NavAgent)` | `FCompositeNavModifier` | 특정 에이전트에 맞는 모디파이어 반환 |
| `IsMatchingFilter(Filter)` | `bool` | `FNavigationRelevantDataFilter` 조건 충족 여부 |
| `GetDirtyFlag()` | `ENavigationDirtyFlag` | 지오메트리/모디파이어 중 어떤 것이 변경되었는지 |
| `HasPerInstanceTransforms()` | `bool` | 인스턴스별 트랜스폼 델리게이트가 설정되었는지 |

---

## 부록 B: GetNavigationData() 엔진 전수 조사

엔진 전체에서 `GetNavigationData()`를 오버라이드하는 클래스는 **10개** (+ 우리 프로젝트 1개 = 총 11개)이다.

### B.1 기본 인터페이스 선언

```
파일: Engine/Source/Runtime/Engine/Classes/AI/Navigation/NavRelevantInterface.h

class INavRelevantInterface
{
    virtual void GetNavigationData(FNavigationRelevantData& Data) const {}
};
```

기본 구현은 **빈 함수**이다. 모든 오버라이드는 이 인터페이스를 구현한다.

### B.2 전체 오버라이드 목록 및 수정 내용

#### ① UPrimitiveComponent (기본 구현)

```
파일: Engine/Source/Runtime/Engine/Private/Components/PrimitiveComponent.cpp:3454
수정 필드: Data.Modifiers
```

- `bFillCollisionUnderneathForNavmesh` 또는 `bRasterizeAsFilledConvexVolume` 플래그가 설정된 경우에만 모디파이어 생성
- 대부분의 PrimitiveComponent에서는 실질적으로 아무 것도 하지 않음

#### ② UStaticMeshComponent

```
파일: Engine/Source/Runtime/Engine/Private/Components/StaticMeshComponent.cpp:3261
수정 필드: Data.Modifiers
```

- `Super::GetNavigationData(Data)` 호출 (UPrimitiveComponent)
- StaticMesh에 `UNavCollisionBase`가 있고 `ShouldExportAsObstacle()`이 true면:
  - `NavCollision->GetNavigationModifier(Data.Modifiers, ComponentTransform)` 호출
  - 메쉬의 NavCollision 데이터를 모디파이어로 추가

#### ③ UShapeComponent

```
파일: Engine/Source/Runtime/Engine/Private/Components/ShapeComponent.cpp:173
수정 필드: Data.Modifiers
```

```cpp
void UShapeComponent::GetNavigationData(FNavigationRelevantData& Data) const
{
    Super::GetNavigationData(Data);
    if (bDynamicObstacle)
    {
        Data.Modifiers.CreateAreaModifiers(this, GetDesiredAreaClass());
    }
}
```

- `bDynamicObstacle == true`일 때만 모디파이어 생성
- `UOKNavMeshGenerateComponent`의 부모 클래스인 `UBoxComponent`가 이 클래스를 상속

#### ④ UInstancedStaticMeshComponent

```
파일: Engine/Source/Runtime/Engine/Private/Components/InstancedStaticMeshComponentHelper.h:87-105
수정 필드: Data.Modifiers + Data.NavDataPerInstanceTransformDelegate
```

- `Data.Modifiers.MarkAsPerInstanceModifier()` 호출
- `NavCollision->GetNavigationModifier(Data.Modifiers, FTransform::Identity)` 호출
- **유일하게 `NavDataPerInstanceTransformDelegate`를 설정**하는 클래스
- 각 인스턴스별로 독립적인 네비게이션 데이터를 제공

#### ⑤ UNavModifierComponent

```
파일: Engine/Source/Runtime/NavigationSystem/Private/NavModifierComponent.cpp:239
수정 필드: Data.Modifiers (Areas + NavMeshResolution)
```

```cpp
void UNavModifierComponent::GetNavigationData(FNavigationRelevantData& Data) const
{
    for (int32 Idx = 0; Idx < ComponentBounds.Num(); Idx++)
    {
        Data.Modifiers.Add(CreateAreaModifier(
            ComponentBounds[Idx].Box, ComponentBounds[Idx].Quat,
            AreaClass, AreaClassToReplace, bIncludeAgentHeight));
    }
    Data.Modifiers.SetNavMeshResolution(NavMeshResolution);
}
```

- **복수 바운드** 지원 (`ComponentBounds` 배열)
- **NavMeshResolution** 설정 가능
- **bIncludeAgentHeight** 옵션 지원

#### ⑥ ANavModifierVolume

```
파일: Engine/Source/Runtime/NavigationSystem/Private/NavModifierVolume.cpp:112
수정 필드: Data.Modifiers (Areas + FillCollision + NavMeshResolution)
```

- 브러시 컴포넌트에서 Area 모디파이어 생성
- `bMaskFillCollisionUnderneathForNavmesh` 설정 시 추가 모디파이어 생성
- `NavMeshResolution` 설정 지원
- `UOKNavMeshGenerateComponent`와 가장 유사한 엔진 구현

#### ⑦ USplineNavModifierComponent

```
파일: Engine/Source/Runtime/NavigationSystem/Private/SplineNavModifierComponent.cpp:61
수정 필드: Data.Modifiers
```

- 스플라인을 따라 **Convex Hull 볼륨들**을 생성
- 각 스플라인 세그먼트마다 `FAreaNavModifier`를 생성하여 추가
- 곡선 경로를 따라 네비게이션 영역을 수정할 때 사용

#### ⑧ UNavLinkComponent

```
파일: Engine/Source/Runtime/NavigationSystem/Private/NavLinkComponent.cpp:44
수정 필드: Data.Modifiers (SimpleLinks)
```

```cpp
void UNavLinkComponent::GetNavigationData(FNavigationRelevantData& Data) const
{
    NavigationHelper::ProcessNavLinkAndAppend(&Data.Modifiers,
        NavigationHelper::FNavLinkOwnerData(*this), Links);
}
```

- **오프메쉬 링크**(점프, 낙하 등 비연속 이동)를 네비게이션 데이터에 추가
- `FSimpleLinkNavModifier`를 통해 `Modifiers.SimpleLinks`에 추가

#### ⑨ UNavLinkCustomComponent

```
파일: Engine/Source/Runtime/NavigationSystem/Private/NavLinkCustomComponent.cpp:216
수정 필드: Data.Modifiers (SimpleLinks + Areas)
```

- 커스텀 링크 모디파이어 추가
- `bCreateBoxObstacle == true`이면 링크 주변에 **장애물 박스**도 추가
- 링크와 영역 모디파이어를 동시에 사용하는 유일한 클래스

#### ⑩ ANavLinkProxy

```
파일: Engine/Source/Runtime/AIModule/Private/Navigation/NavLinkProxy.cpp:248
수정 필드: Data.Modifiers (SimpleLinks)
```

- **포인트 링크**와 **세그먼트 링크** 두 가지 타입을 모두 처리
- `ProcessNavLinkAndAppend` + `ProcessNavLinkSegmentAndAppend` 호출

#### ⑪ UOKNavMeshGenerateComponent (우리 프로젝트)

```
파일: Source/OKGame/Components/OKNavMeshGenerateComponent.cpp:31
수정 필드: Data.Modifiers (Areas)
```

- `Data.Modifiers.CreateAreaModifiers(this, AreaClass, AreaClassToReplace)` 호출
- BoxComponent의 바운드를 Convex Volume으로 사용
- 엔진의 `UShapeComponent`와 `ANavModifierVolume`의 패턴을 결합한 구현

### B.3 패턴 분석

```
GetNavigationData() 오버라이드에서 수정하는 필드 분포:

Data.Modifiers         ████████████████████████ 11/11 (100%)
  └─ Areas             ████████████████████     9/11 (82%)
  └─ SimpleLinks       ████                     3/11 (27%)
  └─ CustomLinks       █                        1/11 (9%)

Data.NavDataPerInstance ██                       1/11 (9%)
Data.CollisionData                               0/11 (0%)
Data.VoxelData                                   0/11 (0%)
Data.Bounds                                      0/11 (0%)
```

**결론:** `GetNavigationData()`의 사실상 유일한 목적은 **`Data.Modifiers`를 채우는 것**이다.

---

## 부록 C: FCompositeNavModifier / FAreaNavModifier 전체 API

### C.1 FCompositeNavModifier

**소스 위치:** `Engine/Source/Runtime/Engine/Public/AI/NavigationModifier.h` (Lines 278-404)

Navigation Modifier의 **집합체(Composite)**. 세 종류의 모디파이어를 담는다:

#### 세 가지 모디파이어 타입

| 타입 | 저장 배열 | 설명 | 사용처 |
|------|----------|------|--------|
| `FAreaNavModifier` | `Areas` | **영역 모디파이어** — 볼륨 내 NavMesh 폴리곤의 Area Class 변경 | NavModifierVolume, ShapeComponent, 우리 컴포넌트 등 |
| `FSimpleLinkNavModifier` | `SimpleLinks` | **단순 오프메쉬 링크** — 두 점 사이의 비연속 이동 경로 | NavLinkComponent, NavLinkProxy |
| `FCustomLinkNavModifier` | `CustomLinks` | **커스텀 오프메쉬 링크** — 프로그래밍 가능한 이동 경로 | NavLinkCustomComponent |

#### 주요 플래그

| 플래그 | 타입 | 설명 | 설정 메서드 |
|--------|------|------|-----------|
| `bFillCollisionUnderneathForNavmesh` | `uint32:1` | NavMesh 아래 콜리전을 채워서 **바닥이 없어도 NavMesh 생성** | `SetFillCollisionUnderneathForNavmesh(bool)` |
| `bMaskFillCollisionUnderneathForNavmesh` | `uint32:1` | FillCollision의 효과를 **마스킹**(무효화) | `SetMaskFillCollisionUnderneathForNavmesh(bool)` |
| `bRasterizeAsFilledConvexVolume` | `uint32:1` | Convex 볼륨을 **채워진 형태로 래스터화** (내부를 솔리드로 취급) | `SetRasterizeAsFilledConvexVolume(bool)` |
| `NavMeshResolution` | `ENavigationDataResolution` | NavMesh 해상도 힌트 (`Default`, `Low`, `Invalid`) | `SetNavMeshResolution(ENavigationDataResolution)` |
| `bIsPerInstanceModifier` | `uint32:1` | 인스턴스별 모디파이어 여부 | `MarkAsPerInstanceModifier()` |
| `bHasPotentialLinks` | `uint32:1` | 잠재적 링크 보유 여부 | `MarkPotentialLinks()` |
| `bAdjustHeight` | `uint32:1` | 높이 조정 필요 여부 | 자동 설정 |
| `bHasLowAreaModifiers` | `uint32:1` | 저해상도 Area 모디파이어 보유 | 자동 설정 |

#### 주요 메서드

| 메서드 | 설명 |
|--------|------|
| `CreateAreaModifiers(PrimComp, AreaClass, AreaClassToReplace)` | PrimitiveComponent의 바운드로 Area 모디파이어 생성 |
| `CreateAreaModifiers(CollisionShape, LocalToWorld, AreaClass, bIncludeAgentHeight)` | CollisionShape으로 Area 모디파이어 생성 |
| `Add(FAreaNavModifier)` | Area 모디파이어 직접 추가 |
| `Add(FSimpleLinkNavModifier)` | 단순 링크 추가 |
| `Add(FCustomLinkNavModifier)` | 커스텀 링크 추가 |
| `Add(FCompositeNavModifier)` | 다른 Composite의 모든 모디파이어를 병합 |
| `GetInstantiatedMetaModifier(NavAgent, Owner)` | 특정 에이전트에 맞게 인스턴스화된 모디파이어 반환 |
| `Reset()` / `Empty()` / `Shrink()` | 초기화/비우기/메모리 축소 |

### C.2 FAreaNavModifier

**소스 위치:** `Engine/Source/Runtime/Engine/Public/AI/NavigationModifier.h` (Lines 100-171)

개별 **영역 모디파이어**. 특정 볼륨 내의 NavMesh 폴리곤에 Area Class를 적용한다.

#### Shape 타입 (ENavigationShapeType)

| Shape | 설명 | 생성자 시그니처 |
|-------|------|----------------|
| **Cylinder** | 원기둥 볼륨 | `FAreaNavModifier(float Radius, float Height, const FTransform& LocalToWorld, TSubclassOf<UNavAreaBase> AreaClass)` |
| **Box** | 박스 볼륨 | `FAreaNavModifier(const FVector& Extent, const FTransform& LocalToWorld, TSubclassOf<UNavAreaBase> AreaClass)` |
| **Box (FBox)** | FBox에서 생성 | `FAreaNavModifier(const FBox& Box, const FTransform& LocalToWorld, TSubclassOf<UNavAreaBase> AreaClass)` |
| **Convex** | 임의의 볼록 다면체 | `FAreaNavModifier(const TArray<FVector>& Points, ENavigationCoordSystem::Type CoordType, const FTransform& LocalToWorld, TSubclassOf<UNavAreaBase> AreaClass)` |
| **InstancedConvex** | 인스턴스별 Convex | `InitializePerInstanceConvex(Points, FirstIndex, LastIndex, AreaClass)` |

#### Apply Mode (ENavigationAreaMode)

| Mode | 설명 | 활용 |
|------|------|------|
| **Apply** | 볼륨 내 **모든** 폴리곤에 AreaClass 적용 | 기본 모드. 영역 전체를 특정 Area로 칠할 때 |
| **Replace** | `AreaClassToReplace`와 **매칭되는 폴리곤만** 교체 | 선택적 교체. 특정 Area만 변경할 때 |
| **ApplyInLowPass** | 저해상도 패스에서 Apply | 멀리 있는 타일에서 적용 |
| **ReplaceInLowPass** | 저해상도 패스에서 Replace | 멀리 있는 타일에서 교체 |

#### 주요 멤버 변수

| 변수 | 타입 | 설명 |
|------|------|------|
| `Cost` | `float` | 네비게이션 모디파이어 정렬용 비용. < 0이면 미설정 |
| `FixedCost` | `float` | 고정 비용 |
| `AreaClassOb` | `TWeakObjectPtr<UClass>` | 적용할 Area 클래스 |
| `ReplaceAreaClassOb` | `TWeakObjectPtr<UClass>` | 교체 대상 Area 클래스 |
| `Bounds` | `FBox` | 모디파이어의 바운딩 박스 |
| `Points` | `TArray<FVector>` | Convex 볼륨의 꼭짓점들 |
| `ShapeType` | `ENavigationShapeType::Type` | 형태 타입 |
| `ApplyMode` | `ENavigationAreaMode::Type` | 적용 모드 |
| `bExpandTopByCellHeight` | `uint8:1` | 상단을 셀 높이만큼 확장 |
| `bIncludeAgentHeight` | `uint8:1` | 에이전트 높이 포함 여부 |
| `bIsLowAreaModifier` | `uint8:1` | 저해상도 모디파이어 여부 |

### C.3 FSimpleLinkNavModifier (오프메쉬 링크)

오프메쉬 링크는 NavMesh 폴리곤이 물리적으로 연결되지 않은 두 지점을 AI가 이동할 수 있도록 연결한다. 점프, 낙하, 사다리, 텔레포트 등에 사용된다.

| 필드 | 설명 |
|------|------|
| `Links` | `TArray<FNavigationLink>` — 포인트 투 포인트 링크 배열 |
| `SegmentLinks` | `TArray<FNavigationSegmentLink>` — 세그먼트(선분) 링크 배열 |
| `LocalToWorld` | 링크의 로컬→월드 트랜스폼 |
| `UserId` | 사용자 정의 ID |

### C.4 FNavigableGeometryExport 인터페이스

`GetNavigationData()`와는 별도로, 콜리전 지오메트리를 NavMesh에 제공하는 경로이다:

| 메서드 | 설명 |
|--------|------|
| `ExportChaosTriMesh()` | Chaos 물리 삼각형 메쉬 익스포트 |
| `ExportChaosConvexMesh()` | Chaos 볼록 메쉬 익스포트 |
| `ExportChaosHeightField()` | Chaos 하이트필드 익스포트 |
| `ExportRigidBodySetup()` | 리지드바디 셋업 익스포트 |
| `ExportCustomMesh()` | 커스텀 메쉬 (버텍스/인덱스 버퍼 직접 제공) |
| `AddNavModifiers()` | 모디파이어 추가 |

이 인터페이스는 `DoCustomNavigableGeometryExport()`를 통해 호출되며, `FNavigationRelevantData.CollisionData`를 채우는 데 사용된다.

---

## 부록 D: 엔진 호출 메커니즘 (FNavigationOctree 파이프라인)

### D.1 전체 데이터 흐름

```
[1] 컴포넌트가 월드에 등록
         │
         ▼
[2] FNavigationElement 생성
    ├─ NavigationDataExportDelegate에 람다 바인딩:
    │    NavRelevantInterface->GetNavigationData(Data)
    ├─ CustomGeometryExportDelegate에 바인딩:
    │    NavRelevantInterface->DoCustomNavigableGeometryExport(GeomExport)
    └─ GeometrySliceExportDelegate에 바인딩 (선택적)
         │
         ▼
[3] FNavigationOctree에 노드 추가 (AddNode)
    ├─ FNavigationOctreeElement 생성
    │    └─ FNavigationRelevantData 인스턴스 포함
    │
    ├─ [즉시 수집 모드]
    │    NavigationDataExportDelegate.Execute(Element, *Data)
    │    → GetNavigationData() 호출됨
    │    → Data.Modifiers가 채워짐
    │
    └─ [지연 수집 모드]
         bPendingLazyModifiersGathering = true
         bPendingLazyGeometryGathering = true
         → 나중에 DemandLazyDataGathering() 호출 시 수집
              │
              ▼
[4] NavMesh 타일 빌드 요청 시
    ├─ FNavigationOctree에서 해당 타일 범위의 노드 쿼리
    ├─ 각 노드의 FNavigationRelevantData를 수집
    │    ├─ CollisionData → Recast 지오메트리 입력
    │    └─ Modifiers → Area/링크 수정자
    │
    ├─ Recast가 지오메트리를 복셀화 → 하이트필드 → 폴리곤 생성
    └─ Modifiers의 Area가 폴리곤에 적용
         │
         ▼
[5] 완성된 NavMesh 타일이 ARecastNavMesh에 등록
    → AI 경로 탐색에 사용 가능
```

### D.2 핵심: 델리게이트 기반 호출

엔진은 `GetNavigationData()`를 **직접 가상 함수 호출하지 않는다**. 대신:

1. `FNavigationElement` 생성 시 `NavigationDataExportDelegate`에 람다를 바인딩:
   ```cpp
   // NavigationElement.cpp:97
   NavigationDataExportDelegate.BindWeakLambda(NavRelevantInterface,
       [NavRelevantInterface](const FNavigationElement&,
                              FNavigationRelevantData& OutData)
       {
           NavRelevantInterface->GetNavigationData(OutData);
       });
   ```

2. `FNavigationOctree`가 필요 시 델리게이트를 실행:
   ```cpp
   // NavigationOctree.cpp:240-262
   SourceElement.NavigationDataExportDelegate.ExecuteIfBound(
       SourceElement, *OctreeElement.Data);
   ```

이 패턴의 장점:
- **지연 평가(Lazy Evaluation)** 지원 — 실제로 데이터가 필요할 때까지 수집을 미룸
- **약한 참조(Weak Reference)** — `BindWeakLambda`로 오브젝트 수명 안전성 보장
- **유연한 데이터 소스** — 가상 함수뿐 아니라 커스텀 델리게이트도 바인딩 가능

### D.3 FNavigationOctree의 역할

`FNavigationOctree`는 `TOctree2` 기반의 **공간 분할 자료구조**로, 모든 네비게이션 관련 데이터를 공간적으로 관리한다.

| 속성 | 값 |
|------|-----|
| 최대 리프 엘리먼트 수 | 16 |
| 최소 포함 엘리먼트 수/노드 | 7 |
| 최대 깊이 | 12 |
| 지오메트리 저장 모드 | `SkipNavGeometry` / `StoreNavGeometry` |

**주요 메서드:**

| 메서드 | 설명 |
|--------|------|
| `AddNode(Bounds, Element)` | 네비게이션 엘리먼트를 옥트리에 추가 |
| `RemoveNode(Id)` | 노드 제거 |
| `UpdateNode(Id, NewBounds)` | 바운드 업데이트 |
| `GetDataForID(Id)` | 특정 노드의 `FNavigationRelevantData` 읽기 접근 |
| `GetMutableDataForID(Id)` | 특정 노드의 `FNavigationRelevantData` 쓰기 접근 |
| `DemandLazyDataGathering(Data)` | 지연된 데이터 수집을 즉시 실행 |

### D.4 FNavigationRelevantDataFilter

옥트리 쿼리 시 특정 조건의 데이터만 필터링하는 구조체:

| 필드 | 설명 |
|------|------|
| `bIncludeGeometry` | 지오메트리가 있는 노드만 통과 |
| `bIncludeOffmeshLinks` | 오프메쉬 링크 모디파이어가 있는 노드만 통과 |
| `bIncludeAreas` | Area 모디파이어가 있는 노드만 통과 |
| `bIncludeMetaAreas` | 메타 Area 모디파이어가 있는 노드만 통과 |
| `bExcludeLoadedData` | 레벨 로딩 데이터 제외 (월드 파티션 동적 모드 전용) |

---

## 부록 E: UOKNavMeshGenerateComponent 확장 가능성

엔진 전수 조사를 통해 발견한, 우리 컴포넌트에 추가로 활용 가능한 `FNavigationRelevantData`/`FCompositeNavModifier` 기능들:

### E.1 현재 사용하는 기능

| 기능 | 사용 여부 | 코드 |
|------|----------|------|
| Area 모디파이어 (CreateAreaModifiers) | ✅ 사용 | `Data.Modifiers.CreateAreaModifiers(this, AreaClass, AreaClassToReplace)` |

### E.2 추가 활용 가능한 기능

| # | 기능 | 설명 | 추가 방법 |
|---|------|------|----------|
| 1 | **NavMeshResolution** | 볼륨 내 NavMesh 해상도를 `Default` 또는 `Low`로 설정 | `Data.Modifiers.SetNavMeshResolution(ENavigationDataResolution::Low)` |
| 2 | **FillCollisionUnderneath** | 바닥 지오메트리 없이도 NavMesh 생성 가능 | `Data.Modifiers.SetFillCollisionUnderneathForNavmesh(true)` |
| 3 | **MaskFillCollision** | 다른 FillCollision 효과를 마스킹 | `Data.Modifiers.SetMaskFillCollisionUnderneathForNavmesh(true)` |
| 4 | **RasterizeAsFilledConvexVolume** | 볼륨을 솔리드로 래스터화 | `Data.Modifiers.SetRasterizeAsFilledConvexVolume(true)` |
| 5 | **오프메쉬 링크 추가** | 점프/낙하 등 비연속 경로 정의 | `NavigationHelper::ProcessNavLinkAndAppend(&Data.Modifiers, ...)` |
| 6 | **커스텀 Shape** | Box 대신 Cylinder나 Convex 사용 | `FAreaNavModifier(Radius, Height, Transform, AreaClass)` 직접 생성 |
| 7 | **IncludeAgentHeight** | 에이전트 높이를 볼륨에 포함 | `FAreaNavModifier.SetIncludeAgentHeight(true)` |
| 8 | **ExpandTopByCellHeight** | 상단을 셀 높이만큼 확장 | `FAreaNavModifier.SetExpandTopByCellHeight(true)` |
| 9 | **ShouldUseGeometryDelegate** | 특정 NavAgent에 대해서만 효과 적용 | `Data.ShouldUseGeometryDelegate.BindLambda(...)` |
| 10 | **bShouldSkipDirtyAreaOnAddOrRemove** | Dirty Area 마킹 생략으로 성능 최적화 | `Data.bShouldSkipDirtyAreaOnAddOrRemove = true` |

### E.3 특히 주목할 기능: FillCollisionUnderneathForNavmesh

이 플래그는 **바닥 지오메트리가 없어도 NavMesh를 생성**할 수 있게 한다. 현재 보고서 본문 5장의 "기존 지오메트리 필수" 제한을 극복하는 방법이다.

```cpp
// 이 한 줄 추가로 바닥 없이도 NavMesh 생성 가능
Data.Modifiers.SetFillCollisionUnderneathForNavmesh(true);
```

`ANavModifierVolume`이 이 기능을 이미 사용하고 있어, 검증된 엔진 기능이다.
