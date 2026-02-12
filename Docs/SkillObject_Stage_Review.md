# SkillObject 7단계 심화 설계 검토 보고서

---

## 목차

- [1. 개요](#1-개요)
- [2. 공통 설계 원칙](#2-공통-설계-원칙)
  - [2.1 정책 인스턴스 관리](#21-정책-인스턴스-관리)
  - [2.2 정책 간 데이터 흐름](#22-정책-간-데이터-흐름)
  - [2.3 AOKSkillObject 런타임 루프](#23-aokskillobject-런타임-루프)
  - [2.4 파괴 시퀀스](#24-파괴-시퀀스)
  - [2.5 타이머 전략](#25-타이머-전략)
- [3. Stage 1 — Spawn (소환)](#3-stage-1--spawn-소환)
  - [3.1 FOKSkillSpawnContext 설계](#31-fokskillspawncontext-설계)
  - [3.2 FOKSkillSpawnResult 설계](#32-fokskillspawnresult-설계)
  - [3.3 Spread: N개 독립 Actor](#33-spread-n개-독립-actor)
  - [3.4 Drop: 낙하 패턴](#34-drop-낙하-패턴)
  - [3.5 엣지 케이스](#35-엣지-케이스-spawn)
- [4. Stage 2 — Movement (이동)](#4-stage-2--movement-이동)
  - [4.1 ProjectileMovementComponent 전략](#41-projectilemovementcomponent-전략)
  - [4.2 Orbit 정책 상세](#42-orbit-정책-상세)
  - [4.3 Boomerang 정책 상세](#43-boomerang-정책-상세)
  - [4.4 Wave 정책 상세](#44-wave-정책-상세)
  - [4.5 Tick 최적화](#45-tick-최적화)
  - [4.6 정책 상태 관리](#46-정책-상태-관리)
- [5. Stage 3 — Detection (감지)](#5-stage-3--detection-감지)
  - [5.1 Shape 컴포넌트 생성 전략](#51-shape-컴포넌트-생성-전략)
  - [5.2 충돌 채널 설계](#52-충돌-채널-설계)
  - [5.3 기존 MeleeAttack과의 관계](#53-기존-meleeattack과의-관계)
  - [5.4 기존 AgentManager 활용 여부](#54-기존-agentmanager-활용-여부)
  - [5.5 Ray 정책 상세](#55-ray-정책-상세)
- [6. Stage 4 — Filter (필터)](#6-stage-4--filter-필터)
  - [6.1 팀 시스템 설계](#61-팀-시스템-설계)
  - [6.2 통합 FilterPolicy_TeamBased](#62-통합-filterpolicy_teambased)
  - [6.3 ASC 접근 표준화](#63-asc-접근-표준화)
  - [6.4 Destructible 필터](#64-destructible-필터)
- [7. Stage 5 — Effect (효과)](#7-stage-5--effect-효과)
  - [7.1 GameplayEffectSpec 생성 위치](#71-gameplayeffectspec-생성-위치)
  - [7.2 SetByCaller 파이프라인](#72-setbycaller-파이프라인)
  - [7.3 Multi 정책: Best Effort](#73-multi-정책-best-effort)
  - [7.4 Knockback 상세](#74-knockback-상세)
  - [7.5 공유 데미지 유틸리티](#75-공유-데미지-유틸리티)
- [8. Stage 6 — Timing (타이밍)](#8-stage-6--timing-타이밍)
  - [8.1 OnOverlap + Periodic 공존](#81-onoverlap--periodic-공존)
  - [8.2 Chain (연쇄) 정책](#82-chain-연쇄-정책)
  - [8.3 OnExpire 연결](#83-onexpire-연결)
- [9. Stage 7 — Expire (종료)](#9-stage-7--expire-종료)
  - [9.1 Composite 복합 조건](#91-composite-복합-조건)
  - [9.2 OnHit 소멸 → 폭발 스폰 체인](#92-onhit-소멸--폭발-스폰-체인)
  - [9.3 Manual 취소](#93-manual-취소)
  - [9.4 WallHit 처리](#94-wallhit-처리)
- [10. 단계 간 인터페이스 흐름도](#10-단계-간-인터페이스-흐름도)
- [11. 기존 코드베이스 통합 전략](#11-기존-코드베이스-통합-전략)
- [12. 검토 항목 체크리스트](#12-검토-항목-체크리스트)

---

## 1. 개요

### 1.1 문서 목적
본 문서는 `SkillObject_Design_Report.md`에서 확정된 **7단계 파이프라인 + UObject 정책 객체** 아키텍처를 기반으로, 각 단계별 **실제 구현 시 마주칠 세부 설계 문제**를 심화 검토한 결과를 기록한다.

### 1.2 검토 범위
- 7개 정책(Spawn, Movement, Detection, Filter, Effect, Timing, Expire)의 상세 설계 결정
- 정책 간 인터페이스 및 이벤트 흐름
- 기존 코드베이스(`CombatTokenManager`, `MeleeAttack`, `AgentManager`)와의 통합
- 엣지 케이스 및 주의사항

### 1.3 선행 문서
- `Docs/SkillObject_Design_Report.md` — 7단계 파이프라인 아키텍처 결정 경위

### 1.4 검토 방법
3개 그룹으로 나누어 병렬 심화 검토를 수행했다:

| 그룹 | 담당 단계 | 검토 초점 |
|------|-----------|-----------|
| **Spawn + Movement** | 1~2단계 | 스폰 컨텍스트, PMC 연동, Tick 최적화 |
| **Detection + Filter** | 3~4단계 | Shape 생성 전략, 팀 시스템, 기존 코드 관계 |
| **Effect + Timing + Expire** | 5~7단계 | GAS 통합, SetByCaller, 복합 조건, 이벤트 흐름 |

---

## 2. 공통 설계 원칙

> **이 장의 핵심**: 모든 정책에 적용되는 인스턴스 관리, 데이터 흐름, 파괴 시퀀스, 타이머 전략을 먼저 확립한다.

### 2.1 정책 인스턴스 관리

**결정: `DuplicateObject`로 전 정책 인스턴스 복사**

DataAsset의 정책 UObject는 CDO(Class Default Object) 역할을 한다. 런타임에 AOKSkillObject가 생성될 때 `DuplicateObject`로 개별 인스턴스를 복사하여 사용한다.

```
DataAsset (에디터 에셋)
  └─ UOKMovementPolicy_Orbit  (원본 = 템플릿)
       │
       │  DuplicateObject(Original, NAME_None, SkillObjectActor)
       ▼
AOKSkillObject (런타임 인스턴스)
  └─ UOKMovementPolicy_Orbit  (복사본, Outer = AOKSkillObject)
```

| 대안 | 기각 사유 |
|------|-----------|
| CDO 직접 공유 | 런타임 상태(CurrentAngle 등)를 가질 수 없음 |
| NewObject 후 프로퍼티 복사 | DuplicateObject가 이미 이를 수행하며 더 안전 |

- **Outer**: 복사 시 `NewOuter = AOKSkillObject` → 정책의 라이프사이클이 SkillObject와 동일
- **GC 안전**: Outer 체인에 의해 자동 관리

### 2.2 정책 간 데이터 흐름

**결정: AOKSkillObject를 Mediator(중재자)로 사용**

정책끼리 직접 참조하지 않는다. 모든 교류는 AOKSkillObject를 통해 간접적으로 이루어진다.

```
Detection 오버랩 발생
  → AOKSkillObject::OnDetectionOverlap(HitActors)
    → FilterPolicy->PassesFilter(Actor)  → 통과
      → TimingPolicy->OnDetected(Actor)  → 발동 결정
        → EffectPolicy->ApplyEffect(Actor)
  → ExpirePolicy->OnHit()               → 소멸 판정
```

**근거:**
- 각 정책이 다른 정책의 존재를 모르므로 독립적 교체 가능
- 디버깅 시 AOKSkillObject의 중재 함수에 브레이크포인트 하나로 전체 흐름 추적
- 기존 `AgentManager`의 `FInstancedStruct` 간접 접근 패턴과 일관됨

### 2.3 AOKSkillObject 런타임 루프

AOKSkillObject::Tick에서의 처리 순서:

```cpp
void AOKSkillObject::Tick(float DeltaTime)
{
    // 1. 소멸 판정 (먼저 체크하여 불필요한 처리 방지)
    if (bIsExpiring) return;
    if (ExpirePolicy && ExpirePolicy->ShouldExpire())
    {
        BeginExpire(EOKSkillExpireReason::PolicyCondition);
        return;
    }

    // 2. 이동 업데이트
    if (MovementPolicy)
    {
        MovementPolicy->Tick(this, DeltaTime);
    }

    // 3. 타이밍 Tick (Periodic 등)
    if (TimingPolicy)
    {
        TimingPolicy->Tick(DeltaTime);
    }

    // 4. Detection은 오버랩 이벤트 기반 (Tick 불필요)
    // → OnComponentBeginOverlap 바인딩으로 처리
}
```

### 2.4 파괴 시퀀스

소멸은 즉시 Destroy하지 않고 **2-프레임 지연 시퀀스**를 따른다:

```
[프레임 N]
  ShouldExpire() == true  또는  OnHit()/OnWallHit() 트리거
    → BeginExpire(Reason)
      → bIsExpiring = true
      → TimingPolicy_OnExpire 있으면 → EffectPolicy->ApplyEffect() (소멸 시 효과)
      → OnDestroyedSpawnDA 있으면 → 후속 SkillObject 스폰 (폭발 등)
      → 충돌/Tick 비활성화
      → SetActorHiddenInGame(true)

[프레임 N+1]
  → Destroy()
```

**근거:**
- 소멸 시 효과(Explosion 등)가 같은 프레임에서 처리되어야 시각적 연속성 확보
- 물리 엔진이 해당 프레임의 오버랩 결과를 완료한 뒤 안전하게 파괴

### 2.5 타이머 전략

**결정: Tick 기반 누적 시간 (FTimerHandle 사용하지 않음)**

```cpp
// TimingPolicy_Periodic 예시
Accumulated += DeltaTime;
if (Accumulated >= Interval)
{
    Accumulated -= Interval;  // 잔여 시간 보존
    FireEffect();
}
```

| 대안 | 기각 사유 |
|------|-----------|
| `FTimerHandle` | TimerManager에 의존, 일시정지/시간왜곡 대응 어려움 |
| `SetTimer` | 정책 UObject가 AActor가 아니므로 직접 사용 불가, 우회 필요 |

- Tick 기반은 `CustomTimeDilation`에 자연스럽게 대응
- ExpirePolicy_Lifetime과 TimingPolicy_Periodic 모두 동일 패턴 사용

---

## 3. Stage 1 — Spawn (소환)

> **이 장의 핵심**: SpawnContext/SpawnResult 구조체 설계, N개 독립 Actor 전략, Drop 패턴 열거

### 3.1 FOKSkillSpawnContext 설계

GA에서 SpawnPolicy에 전달하는 컨텍스트 구조체:

| 필드 | 타입 | 설명 |
|------|------|------|
| `Instigator` | `TWeakObjectPtr<AActor>` | 시전자. 약참조로 소멸 안전 |
| `InstigatorTransform` | `FTransform` | 시전 시점의 위치/회전 **스냅샷** |
| `AimDirection` | `FVector` | 조준 방향 (카메라/컨트롤러 기반) |
| `TargetLocation` | `TOptional<FVector>` | 지정 위치 (AtTarget, Drop용). 없으면 미사용 |
| `HomingTarget` | `TWeakObjectPtr<AActor>` | 유도 타겟 (Homing용). 없으면 미사용 |
| `World` | `UWorld*` | 스폰 시 월드 참조 |
| `InstigatorASC` | `UAbilitySystemComponent*` | GE 적용 시 Source ASC |
| `EffectSpecHandle` | `FGameplayEffectSpecHandle` | GA에서 사전 생성한 GE Spec |
| `RandomSeed` | `int32` | 결정론적 랜덤 (리플레이 대비) |

**설계 포인트:**
- `InstigatorTransform`은 **스냅샷**: 시전 후 시전자가 이동해도 스폰 위치 불변
- `TOptional<FVector>`로 불필요한 필드가 기본값과 구분됨
- `RandomSeed`는 기존 `UOKStressDataAsset`의 `SyntheticSeed` 패턴 참조

### 3.2 FOKSkillSpawnResult 설계

SpawnPolicy가 반환하는 결과 구조체:

| 필드 | 타입 | 설명 |
|------|------|------|
| `SpawnTransform` | `FTransform` | 스폰 위치/회전 |
| `InitialVelocity` | `FVector` | 초기 속도 (PMC 설정용) |
| `PerObjectHomingTarget` | `TWeakObjectPtr<AActor>` | 개별 유도 타겟 (Spread에서 분산 시) |
| `CustomFloat` | `float` | 정책 확장용 범용 값 |

SpawnPolicy의 시그니처:

```cpp
virtual TArray<FOKSkillSpawnResult> CalculateSpawnTransforms(
    const FOKSkillSpawnContext& Context) const;
```

- 반환값이 **배열**: Spread/Drop은 N개 결과 반환, Forward/AtTarget은 1개 반환
- GA가 배열 길이만큼 `SpawnActor<AOKSkillObject>()` 호출

### 3.3 Spread: N개 독립 Actor

**결정: Spread는 N개의 독립적인 AOKSkillObject를 생성**

| 대안 | 기각 사유 |
|------|-----------|
| 1개 Actor가 N개 서브 프로젝타일 관리 | 정책 파이프라인이 "1 Actor = 1 파이프라인" 전제이므로 복잡도 급증 |

- 3개 Spread → `CalculateSpawnTransforms()`이 `TArray` 3개 원소 반환
- 각 Actor가 독립적으로 Movement/Detection/Expire 실행
- 공전 방패 예시: 120도 간격 3개 Actor, 각각 `UOKMovementPolicy_Orbit` 인스턴스 보유

### 3.4 Drop: 낙하 패턴

```cpp
UENUM(BlueprintType)
enum class EOKDropPattern : uint8
{
    Random,     // 타겟 위치 주변 랜덤 산개
    Ring,       // 타겟 중심 원형 배치
    Grid,       // 격자 배치
    Cross,      // 십자 배치
};
```

- Drop 스폰 높이: `Context.TargetLocation + FVector(0, 0, DropHeight)`
- 산개 범위: `SpreadRadius` 파라미터로 제어
- RandomSeed 사용 시 동일 시드 → 동일 패턴 보장

### 3.5 엣지 케이스 (Spawn)

| 상황 | 처리 |
|------|------|
| TargetLocation이 벽 내부 | SpawnPolicy에서 NavMesh 프로젝션 or LineTrace로 보정 |
| Instigator가 스폰 전 사망 | `TWeakObjectPtr::IsValid()` 체크 후 스폰 취소 |
| Spread Count = 0 | 빈 배열 반환 → GA에서 아무것도 스폰하지 않음 |

---

## 4. Stage 2 — Movement (이동)

> **이 장의 핵심**: PMC 소유 전략, Orbit/Boomerang/Wave 세부 동작, Tick 최적화

### 4.1 ProjectileMovementComponent 전략

**결정: AOKSkillObject가 생성자에서 PMC를 미리 보유, 기본 비활성**

```cpp
// AOKSkillObject 생성자
ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("PMC"));
ProjectileMovement->bAutoActivate = false;
ProjectileMovement->SetComponentTickEnabled(false);
```

MovementPolicy가 Initialize 시:
- **Projectile/Homing** → PMC 파라미터 설정 후 `Activate()`, `SetComponentTickEnabled(true)`
- **Orbit/Boomerang/Wave/Follow** → PMC 비활성 유지, 자체 Tick 로직 사용
- **None** → PMC 비활성, Actor Tick도 비활성

**근거:**
- 동적 `NewObject<UProjectileMovementComponent>` + `RegisterComponent()`는 스폰 스파이크 유발
- 미리 보유 + 비활성은 메모리만 소비 (Tick 비용 0)
- PMC의 Homing/Gravity/Bounce 등 내장 기능을 최대 활용

### 4.2 Orbit 정책 상세

공전 정책의 핵심 설계 결정:

| 항목 | 결정 | 근거 |
|------|------|------|
| 중심점 추적 | **매 Tick 시전자 현재 위치 추적** | 정지된 중심점은 시전자 이동 시 부자연스러움 |
| 중심 소실 시 | `EOKOrbitCenterLostBehavior` 열거 | 아래 참조 |
| 공전축 | `EOKOrbitAxis` (Yaw / Pitch / Roll) | Yaw가 기본 (수평 공전) |
| PMC 사용 | 사용하지 않음 | 원운동은 PMC로 표현 불가 |

```cpp
UENUM(BlueprintType)
enum class EOKOrbitCenterLostBehavior : uint8
{
    Expire,      // 즉시 소멸
    Freefall,    // 현재 접선 방향으로 직선 이동 후 수명 소진
    FixCenter,   // 마지막 유효 위치를 고정점으로 계속 공전
};
```

런타임 상태:
- `CurrentAngle` (float) — 현재 공전 각도
- `OrbitCenter` (FVector) — 캐시된 중심점 (매 Tick 갱신 또는 고정)

### 4.3 Boomerang 정책 상세

| 항목 | 결정 |
|------|------|
| 복귀 대상 | **시전자의 현재 위치 (매 Tick 추적)** |
| 복귀 판정 | 시전자와의 거리 ≤ `ArrivalRadius` 도달 시 Expire 트리거 |
| 가속 방식 | 거리 비례 가속 → 가까워질수록 빨라짐 (자연스러운 부메랑 느낌) |
| PMC 사용 | 사용하지 않음 |

```
Phase 1 (전진): 방향 = AimDirection, 속도 = ForwardSpeed
  MaxDistance 도달 → Phase 2 전환

Phase 2 (복귀): 방향 = (시전자 위치 - 현재 위치).Normalize()
  속도 = ReturnSpeed * (1.0 + DistanceFactor)
  ArrivalRadius 도달 → Expire 트리거
```

런타임 상태: `bIsReturning` (bool), `TraveledDistance` (float)

### 4.4 Wave 정책 상세

사인파 이동의 설계:

| 항목 | 결정 |
|------|------|
| 진동 평면 | **전진 방향에 수직** (로컬 기준) |
| 진동 모드 | `EOKWaveMode` (Horizontal2D / Vertical2D / Spiral3D) |
| PMC 사용 | 사용하지 않음 (직접 위치 계산) |

```
Position(t) = StartPos + ForwardDir * Speed * t
            + PerpDir * Amplitude * sin(Frequency * t)

// Spiral3D의 경우:
+ RightDir * Amplitude * sin(Frequency * t)
+ UpDir    * Amplitude * cos(Frequency * t)
```

### 4.5 Tick 최적화

**결정: `RequiresTick()` 가상 함수로 조건부 Tick 등록**

```cpp
// MovementPolicy 베이스
virtual bool RequiresTick() const { return false; }

// Projectile/Homing → false (PMC가 자체 Tick)
// Orbit/Boomerang/Wave/Follow → true
// None → false
```

AOKSkillObject::InitializeSkillObject에서:

```cpp
bool bNeedsTick = false;
if (MovementPolicy) bNeedsTick |= MovementPolicy->RequiresTick();
if (TimingPolicy)   bNeedsTick |= TimingPolicy->RequiresTick();
if (ExpirePolicy)   bNeedsTick |= ExpirePolicy->RequiresTick();
SetActorTickEnabled(bNeedsTick);
```

- 기존 `UOKCharacterMovementComponent`의 `TRACE_CPUPROFILER_EVENT_SCOPE` 패턴을 Tick에 적용
- 정지 AoE(Movement=None, Timing=Periodic) → Periodic이 `RequiresTick() = true` 반환 → Tick 활성

### 4.6 정책 상태 관리

**결정: 상태를 가진 정책은 DuplicateObject로 이미 개별 인스턴스이므로 멤버 변수 직접 사용**

DuplicateObject 방식에서는 각 인스턴스가 독립적이므로, `CurrentAngle`, `bIsReturning` 등을 UObject 멤버 변수로 직접 보유해도 안전하다.

> **참고**: 기존 `AgentManager`의 `FInstancedStruct SpatialEntryData` 패턴은 정책 자체에 상태를 두지 않는 대안이었으나, DuplicateObject가 이미 인스턴스 분리를 보장하므로 불필요한 간접층이 됨.

---

## 5. Stage 3 — Detection (감지)

> **이 장의 핵심**: 하이브리드 Shape 생성, 전용 충돌 채널, 기존 시스템과의 역할 분리

### 5.1 Shape 컴포넌트 생성 전략

**결정: 하이브리드 방식**

| Shape | 생성 방식 | 근거 |
|-------|-----------|------|
| **Sphere** | AOKSkillObject 생성자에서 기본 보유 | 가장 빈번 (70%+ 예상), 동적 생성 비용 회피 |
| **Box / Capsule** | DetectionPolicy::Initialize에서 동적 생성 | `NewObject` + `RegisterComponent()` |

```cpp
// AOKSkillObject 생성자
DefaultSphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("DefaultDetection"));
DefaultSphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
// → DetectionPolicy_Sphere가 Radius 설정 후 활성화
// → Box/Capsule 정책은 이것을 무시하고 자체 컴포넌트 동적 생성
```

DetectionPolicy 인터페이스:

```cpp
virtual UShapeComponent* CreateDetectionComponent(AOKSkillObject* Owner);
// Sphere → Owner->DefaultSphereComponent 반환 (Radius 설정 후)
// Box    → NewObject<UBoxComponent> → RegisterComponent → 반환
```

### 5.2 충돌 채널 설계

**결정: 전용 충돌 채널 `ECC_SkillDetection` 추가**

```ini
// DefaultEngine.ini
+DefaultChannelResponses=(
    Channel=ECC_GameTraceChannel1,
    DefaultResponse=ECR_Ignore,
    Name=SkillDetection
)
```

| 오브젝트 유형 | SkillDetection 채널 응답 |
|---------------|------------------------|
| SkillObject Detection Shape | Query (Overlap) |
| 캐릭터/AI | Overlap |
| 지형/벽 | Ignore |
| 다른 SkillObject | Ignore |

- 기존 Physics/Visibility 채널과 간섭 없음
- 벽 감지는 별도 WallHit 로직 (Visibility 채널 사용)

### 5.3 기존 MeleeAttack과의 관계

| 항목 | MeleeAttack (기존) | SkillObject Detection |
|------|--------------------|-----------------------|
| **방식** | 매 프레임 CapsuleTraceMulti (Sweep) | 오버랩 이벤트 (OnBeginOverlap) |
| **용도** | 본 기반 근접 공격 (애니메이션 동기) | 투사체/AoE 범위 감지 |
| **상태** | `// TODO: 충돌 처리 로직 추가` | 신규 구축 |

**통합 방안:**
- MeleeAttack의 `// TODO` 부분에 공유 데미지 유틸리티 (`UOKDamageHelper::ApplyDamageWithGE()`) 연결
- 두 시스템이 동일한 GAS 데미지 파이프라인을 사용하되, 감지 방식만 다름
- MeleeAttack은 AnimNotify에 종속되므로 SkillObject의 DetectionPolicy로 대체하지 않음

### 5.4 기존 AgentManager 활용 여부

**결정: AgentManager를 Detection에 활용하지 않음**

| 이유 | 설명 |
|------|------|
| 용도 차이 | AgentManager는 AI 에이전트 공간 쿼리 전용 (NPC 상호작용) |
| 2D 한정 | `THierarchicalHashGrid2D`는 수직(Z축) 고려 없음 → 3D 투사체 부적합 |
| 등록 오버헤드 | 수백 개 SkillObject를 해시 그리드에 등록/해제하면 기존 에이전트 쿼리 성능 저하 |

AgentManager는 현재 용도(AI 에이전트 근접 탐색)에만 사용하고, SkillObject는 언리얼 물리 엔진의 오버랩 시스템을 활용한다.

### 5.5 Ray 정책 상세

| 항목 | 결정 |
|------|------|
| 트레이스 함수 | `UWorld::LineTraceSingleByChannel` |
| 트레이스 빈도 | **매 프레임** (기본), `TraceInterval`로 조절 가능 |
| 길이 | `RayLength` 파라미터 (고정 길이, 무한 아님) |
| 채널 | `ECC_SkillDetection` |
| 다중 적중 | 기본 Single (관통 필요 시 Multi 서브클래스 추가) |

---

## 6. Stage 4 — Filter (필터)

> **이 장의 핵심**: GameplayTag 기반 팀 시스템, 통합 비트마스크 필터, ASC 접근 표준화

### 6.1 팀 시스템 설계

**결정: GameplayTag 기반 (FGenericTeamId 사용하지 않음)**

| 대안 | 기각 사유 |
|------|-----------|
| `FGenericTeamId` | 단순 int ID, 게임 전체에 ETeamAttitude 매핑 필요, 유연성 부족 |
| 듀얼 (둘 다) | 두 시스템 동기화 부담, 의미 중복 |

GameplayTag 방식:
```
Team.Player
Team.Monster
Team.Neutral
Team.Destructible
```

- ASC에 `Team.*` LooseGameplayTag 부여
- 비교: `InstigatorTeamTag`와 `TargetTeamTag`의 관계로 적/아군 판정
- 확장 용이: `Team.Monster.Boss`, `Team.Player.Summon` 등 계층 확장 가능

### 6.2 통합 FilterPolicy_TeamBased

**결정: Enemy/Ally/All을 별도 클래스가 아닌 하나의 `FilterPolicy_TeamBased`로 통합**

```cpp
UENUM(BlueprintType, meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true"))
enum class EOKFilterTarget : uint8
{
    None         = 0,
    Enemy        = 1 << 0,   // 0x01
    Ally         = 1 << 1,   // 0x02
    Neutral      = 1 << 2,   // 0x04
    Destructible = 1 << 3,   // 0x08
    Self         = 1 << 4,   // 0x10
};
```

```cpp
UCLASS(EditInlineNew, DefaultToInstanced)
class UOKFilterPolicy_TeamBased : public UOKFilterPolicy
{
    // 비트마스크로 허용 대상 조합
    UPROPERTY(EditAnywhere, meta=(Bitmask, BitmaskEnum=EOKFilterTarget))
    int32 AllowedTargets;  // Enemy | Destructible 등 복합 가능

    UPROPERTY(EditAnywhere)
    bool bIgnoreInstigator = true;
};
```

**장점:**
- "적 + 파괴물" 복합 필터가 에디터에서 체크박스 조합으로 즉시 가능
- 별도 클래스 5개 → 1개로 축소
- `UOKFilterPolicy_Tag`는 별도 유지 (고급 사용자용 GameplayTag 직접 지정)

### 6.3 ASC 접근 표준화

**결정: `UOKAbilitySystemComponent::GetASCFromActor(AActor*)` 정적 메서드**

```cpp
static UOKAbilitySystemComponent* GetASCFromActor(AActor* Actor)
{
    // 1. IAbilitySystemInterface 시도
    // 2. FindComponentByClass 폴백
    // 3. nullptr 반환
}
```

- 기존 `CombatTokenManager`의 `FindComponentByClass<UAbilitySystemComponent>` 패턴을 표준화
- 모든 정책이 이 메서드를 통해 ASC에 접근

### 6.4 Destructible 필터

**결정: `Team.Destructible` GameplayTag + `IOKDamageable` 인터페이스 이중 체크**

```
// FilterPolicy_TeamBased::PassesFilter 내부
if (AllowedTargets & Destructible)
{
    if (Target의 ASC에 Team.Destructible 태그 있음) → 통과
    else if (Target이 IOKDamageable 구현) → 통과
}
```

- GameplayTag: ASC가 있는 파괴 가능 오브젝트 (나무 상자 등)
- `IOKDamageable`: ASC 없이도 데미지를 받을 수 있는 경량 인터페이스 (벽 장식 등)

---

## 7. Stage 5 — Effect (효과)

> **이 장의 핵심**: GE Spec 생성 위치, SetByCaller 태그 규약, Multi 정책 전략, Knockback 구현

### 7.1 GameplayEffectSpec 생성 위치

**결정: GA에서 GE Spec을 생성하여 EffectPolicy에 전달 (Method A)**

```cpp
// GA_SkillObject::ActivateAbility()

// 1. GE Spec 생성
FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(GEClass);

// 2. SetByCaller 값 설정
SpecHandle.Data->SetSetByCallerMagnitude(DamageTag, BaseDamage * Coefficient);

// 3. SpawnContext에 포함
FOKSkillSpawnContext Context;
Context.EffectSpecHandle = SpecHandle;
Context.InstigatorASC = GetAbilitySystemComponentFromActorInfo();

// 4. Spawn
SpawnPolicy->CalculateSpawnTransforms(Context);
```

| 대안 | 기각 사유 |
|------|-----------|
| EffectPolicy에서 GE Spec 생성 | EffectPolicy가 GA 컨텍스트(Level, Source ASC)에 의존하게 됨 |
| AOKSkillObject에서 생성 | Actor가 GA 정보를 보유해야 하여 결합도 증가 |

**장점:**
- GE Spec의 Source(누가 적용했는가)가 GA 시점에 명확하게 결정됨
- EffectPolicy는 전달받은 Spec을 `ApplyGameplayEffectSpecToTarget()`으로 적용만 하면 됨
- Ability Level, Source Tags 등이 자연스럽게 포함됨

### 7.2 SetByCaller 파이프라인

**태그 네이밍 규약:**

| 태그 | 용도 |
|------|------|
| `Data.Damage.Base` | 기본 데미지 수치 |
| `Data.Heal.Base` | 기본 회복 수치 |
| `Data.Knockback.Force` | 넉백 세기 |
| `Data.Duration.Value` | 버프/디버프 지속시간 |

**데미지 Coefficient 적용 위치:**

```
[GA에서]
BaseDamage = AttributeSet의 AttackPower  (또는 고정값)
Coefficient = DataAsset의 EffectPolicy_Damage.Coefficient  (예: 1.5)
최종값 = BaseDamage * Coefficient

→ SetSetByCallerMagnitude(Data.Damage.Base, 최종값)
→ GE의 Modifier에서 SetByCaller로 읽어서 IncomingDamage에 적용
→ DamageExecutionCalculation에서 Defense 등 감산
→ PostGameplayEffectExecute에서 최종 Health 차감
```

### 7.3 Multi 정책: Best Effort

**결정: 하나의 SubEffect가 실패해도 나머지는 계속 실행 (Best Effort)**

```cpp
UCLASS(EditInlineNew, DefaultToInstanced)
class UOKEffectPolicy_Multi : public UOKEffectPolicy
{
    UPROPERTY(EditAnywhere, Instanced)
    TArray<UOKEffectPolicy*> SubEffects;

    virtual void ApplyEffect(AActor* Target) override
    {
        for (auto* Sub : SubEffects)
        {
            if (Sub) Sub->ApplyEffect(Target);
            // 실패해도 continue — Best Effort
        }
    }
};
```

| 대안 | 기각 사유 |
|------|-----------|
| All-or-Nothing | GE 적용 실패 시 롤백 불가 (UE GAS 한계), 너무 복잡 |
| 순서 의존 | SubEffects 순서가 결과를 바꾸면 디자이너 실수 유발 |

- SubEffects 순서: 배열 순서대로 실행하되, 순서에 의미를 두지 않음
- 로그: 실패 시 Warning 로그 출력 (디버깅용)

### 7.4 Knockback 상세

**결정: `LaunchCharacter` 기반**

| 대안 | 기각 사유 |
|------|-----------|
| `AddImpulse` | 물리 시뮬레이션 의존, CharacterMovement와 충돌 가능 |
| 커스텀 루트모션 | 구현 복잡도 과다, 기본 넉백에는 과함 |

```cpp
UENUM(BlueprintType)
enum class EOKKnockbackOrigin : uint8
{
    SkillObject,    // SkillObject → 타겟 방향
    Instigator,     // 시전자 → 타겟 방향
    WorldUp,        // 수직 상방 (띄우기)
    Custom,         // 커스텀 방향 벡터
};
```

```cpp
// 넉백 적용
FVector Direction = (TargetLocation - Origin).GetSafeNormal();
FVector FinalForce = Direction * KnockbackForce + FVector(0, 0, UpForce);
TargetCharacter->LaunchCharacter(FinalForce, /*bXYOverride=*/true, /*bZOverride=*/true);
```

- `LaunchCharacter`는 `CharacterMovementComponent`와 자연스럽게 통합
- `bXYOverride=true`, `bZOverride=true`로 현재 속도를 덮어써 깔끔한 넉백

### 7.5 공유 데미지 유틸리티

**결정: `UOKDamageHelper` 정적 유틸리티 클래스 신설**

```cpp
// UOKDamageHelper (UBlueprintFunctionLibrary 상속)

static bool ApplyDamageWithGE(
    UAbilitySystemComponent* SourceASC,
    AActor* Target,
    const FGameplayEffectSpecHandle& SpecHandle)
{
    // 1. Target에서 ASC 획득 (GetASCFromActor)
    // 2. ASC 있으면 → ApplyGameplayEffectSpecToTarget
    // 3. ASC 없고 IOKDamageable이면 → 인터페이스 호출
    // 4. 결과 반환
}
```

- **SkillObject의 EffectPolicy**와 **MeleeAttack의 TODO 부분**이 모두 이 유틸리티를 호출
- 데미지 파이프라인의 단일 진입점 역할

---

## 8. Stage 6 — Timing (타이밍)

> **이 장의 핵심**: OnOverlap과 Periodic 공존, Chain 연쇄의 "같은 Actor 점프" 전략, OnExpire 연결

### 8.1 OnOverlap + Periodic 공존

**시나리오**: 독구름에 진입하면 즉시 1회 피해 + 이후 0.5초마다 지속 피해

**결정: Periodic 정책에 `bFireOnFirstTick` 옵션 추가**

```cpp
UPROPERTY(EditAnywhere)
bool bFireOnFirstTick = false;  // true면 진입 즉시 1회 + 이후 주기적

UPROPERTY(EditAnywhere)
float Interval = 0.5f;
```

- `bFireOnFirstTick = true`이면 OnOverlap 시 즉시 ApplyEffect → 이후 Interval마다 반복
- 별도 OnOverlap + Periodic 정책 조합보다 단순하고 직관적
- 퇴장 후 재진입 시 Accumulated 리셋

### 8.2 Chain (연쇄) 정책

**핵심 결정: 새 Actor를 스폰하지 않고, 같은 AOKSkillObject가 타겟을 옮겨가며 "점프"**

```
1회차: Actor → 타겟 A 적중 → Effect 적용
  → TimingPolicy_Chain::OnHit()
    → SearchRadius 내 다음 타겟 B 탐색 (SphereOverlap)
    → 현재 위치를 타겟 B 위치로 이동
    → ChainCount++
    → Effect 적용 (DamageDecay 적용)

2회차: Actor → 타겟 B 적중 → ...

MaxChain 도달 또는 다음 타겟 없음 → Expire
```

| 대안 | 기각 사유 |
|------|-----------|
| 매 점프마다 새 Actor 스폰 | 스폰 비용, 5회 연쇄 = 5개 Actor, GC 부담 |

**데미지 감쇠:**

```cpp
UPROPERTY(EditAnywhere)
float DamageDecayPerChain = 0.8f;  // 매 점프마다 80%로 감소
// 3회차 데미지 = 원래 * 0.8 * 0.8 = 0.64배
```

- 다음 타겟 탐색 주체: **TimingPolicy_Chain**이 `SphereOverlapMultiByChannel` 수행
- 이미 적중한 타겟은 `AlreadyHitSet`에 기록하여 제외
- 탐색 결과를 FilterPolicy에 통과시킨 후 가장 가까운 대상 선택

### 8.3 OnExpire 연결

**결정: AOKSkillObject가 직접 호출 (Method B)**

```cpp
// AOKSkillObject::BeginExpire(Reason)
{
    // OnExpire 타이밍이 있으면 효과 발동
    if (TimingPolicy && TimingPolicy->IsA<UOKTimingPolicy_OnExpire>())
    {
        // 범위 내 모든 타겟에게 효과 적용
        TArray<AActor*> Targets = GatherDetectedActors();
        for (AActor* Target : Targets)
        {
            if (FilterPolicy->PassesFilter(Target))
            {
                EffectPolicy->ApplyEffect(Target);
            }
        }
    }
}
```

| 대안 | 기각 사유 |
|------|-----------|
| 델리게이트 (`OnExpired` 이벤트) | 간접 호출 추가, 바인딩 관리 부담, 디버깅 어려움 |

- 직접 호출이 명시적이고 추적 가능
- AOKSkillObject가 이미 중재자(Mediator)이므로 자연스러운 역할

---

## 9. Stage 7 — Expire (종료)

> **이 장의 핵심**: 복합 조건 OR/AND, OnHit→폭발 체인, Manual 취소, WallHit 처리

### 9.1 Composite 복합 조건

**결정: 기본 OR 로직, AND는 별도 서브클래스**

```cpp
// OR: 하나라도 충족되면 소멸
UCLASS(EditInlineNew, DefaultToInstanced)
class UOKExpirePolicy_Composite : public UOKExpirePolicy
{
    UPROPERTY(EditAnywhere, Instanced)
    TArray<UOKExpirePolicy*> SubPolicies;

    virtual bool ShouldExpire() const override
    {
        for (auto* Sub : SubPolicies)
        {
            if (Sub && Sub->ShouldExpire()) return true;  // Short-circuit
        }
        return false;
    }
};
```

- **OR** (기본): "3초 경과 OR 적중 시" → 먼저 충족되는 조건으로 소멸
- **AND** (별도 `UOKExpirePolicy_CompositeAnd`): "적중 AND 3초 경과" → 모든 조건 동시 충족
- Short-circuit 평가로 성능 최적화 (첫 번째 true에서 즉시 반환)
- SubPolicies도 DuplicateObject로 개별 인스턴스화

### 9.2 OnHit 소멸 → 폭발 스폰 체인

**결정: 후속 스폰 책임은 AOKSkillObject 레벨에 부여 (ExpirePolicy가 아님)**

```cpp
// AOKSkillObject에 직접 보유
UPROPERTY(EditAnywhere)
TSoftObjectPtr<UOKSkillObjectDataAsset> OnDestroyedSpawnDA;
```

```
BeginExpire(Reason)
{
    // 1. 소멸 시 효과 (있으면)
    ...

    // 2. 후속 SkillObject 스폰 (있으면)
    if (OnDestroyedSpawnDA.IsValid())
    {
        SpawnSubSkillObject(OnDestroyedSpawnDA.Get(), GetActorTransform());
    }

    // 3. 비활성화 + 다음 프레임 Destroy
    ...
}
```

**근거:**
- ExpirePolicy는 "언제 끝나는가"만 판단 — 스폰은 다른 관심사
- DataAsset 참조로 에디터에서 드롭다운 연결 가능
- 파이어볼 → DA_Fireball_Explosion, 메테오 → DA_Meteor_Explosion 등 직관적 체인

### 9.3 Manual 취소

**결정: GA에서 `TWeakObjectPtr<AOKSkillObject>` 보유, EndAbility 시 Expire 트리거**

```cpp
// GA_SkillObject
TWeakObjectPtr<AOKSkillObject> SpawnedSkillObject;

void ActivateAbility()
{
    AOKSkillObject* Obj = SpawnSkillObject(...);
    SpawnedSkillObject = Obj;
}

void EndAbility()
{
    if (SpawnedSkillObject.IsValid())
    {
        SpawnedSkillObject->BeginExpire(EOKSkillExpireReason::ManualCancel);
    }
}
```

- `ExpirePolicy_Manual`의 `ShouldExpire()`는 항상 `false` → GA가 외부에서 트리거
- 시전자 사망 시 GA 자동 종료 → EndAbility → 자동 Expire

### 9.4 WallHit 처리

**조건부 구현:**

| PMC 사용 여부 | 처리 방식 |
|---------------|-----------|
| PMC 활성 (Projectile, Homing) | `PMC::OnProjectileStop` 델리게이트 바인딩 |
| PMC 비활성 (Orbit, Wave 등) | 매 Tick `SweepTrace`로 벽 감지 |

```cpp
// PMC 사용 시 — AOKSkillObject::InitializeSkillObject 에서 바인딩
ProjectileMovement->OnProjectileStop.AddDynamic(
    this, &AOKSkillObject::OnProjectileStop);

void AOKSkillObject::OnProjectileStop(const FHitResult& Hit)
{
    if (ExpirePolicy) ExpirePolicy->OnWallHit();
}
```

```cpp
// PMC 미사용 시 — MovementPolicy Tick에서 직접 체크
FHitResult Hit;
bool bHit = World->SweepSingleByChannel(
    Hit, CurrentPos, NextPos, FQuat::Identity,
    ECC_Visibility, FCollisionShape::MakeSphere(SmallRadius));
if (bHit)
{
    Owner->BeginExpire(EOKSkillExpireReason::WallHit);
}
```

---

## 10. 단계 간 인터페이스 흐름도

전체 파이프라인의 단계 간 인터페이스를 정리한다:

```
┌─────────────────────────────────────────────────────────────────┐
│                     GA_SkillObject                               │
│                                                                  │
│  1. GE Spec 생성 (SetByCaller 설정)                               │
│  2. FOKSkillSpawnContext 구성                                      │
│  3. SpawnPolicy->CalculateSpawnTransforms(Context)                │
│     → TArray<FOKSkillSpawnResult>                                │
│  4. 결과 개수만큼 SpawnActor<AOKSkillObject>()                      │
│  5. 각 Actor에 InitializeSkillObject(DataAsset, Context) 호출      │
│                                                                  │
└───────────────────────────┬──────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│              AOKSkillObject::InitializeSkillObject                │
│                                                                  │
│  1. DataAsset에서 7개 정책 DuplicateObject                         │
│  2. MovementPolicy->Initialize(this)    → PMC 설정/활성화          │
│  3. DetectionPolicy->CreateDetection()  → Shape 컴포넌트 반환      │
│  4. Shape에 OnBeginOverlap 바인딩                                 │
│  5. FilterPolicy->Initialize(TeamTag, Instigator)                 │
│  6. EffectPolicy->Initialize(ASC, SpecHandle)                    │
│  7. TimingPolicy->Initialize(this)                                │
│  8. ExpirePolicy->Initialize(this)                                │
│  9. SetActorTickEnabled(RequiresTick 판정)                        │
│                                                                  │
└───────────────────────────┬──────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                    런타임 루프 (매 Tick)                            │
│                                                                  │
│  ┌─ ExpirePolicy->ShouldExpire() ──→ true ─→ BeginExpire() ─┐  │
│  │                                                            │  │
│  │ (false)                                                    │  │
│  ▼                                                            │  │
│  MovementPolicy->Tick()     위치 갱신                          │  │
│  TimingPolicy->Tick()       Periodic 카운트                    │  │
│                                                                │  │
│  ┌─ OnBeginOverlap 이벤트 ────────────────────────┐           │  │
│  │  HitActor 수신                                  │           │  │
│  │  → FilterPolicy->PassesFilter(Actor)            │           │  │
│  │    → (통과) TimingPolicy->OnDetected(Actor)     │           │  │
│  │      → (발동) EffectPolicy->ApplyEffect(Actor)  │           │  │
│  │  → ExpirePolicy->OnHit()                        │           │  │
│  └─────────────────────────────────────────────────┘           │  │
│                                                                │  │
│  ┌─ PMC::OnProjectileStop ─────────────────────────┐          │  │
│  │  → ExpirePolicy->OnWallHit()                     │          │  │
│  └──────────────────────────────────────────────────┘          │  │
│                                                                │  │
│  ┌─ BeginExpire(Reason) ─────────────────────────────────────┘  │
│  │  1. bIsExpiring = true                                       │
│  │  2. TimingPolicy_OnExpire → 범위 내 타겟에 Effect 적용        │
│  │  3. OnDestroyedSpawnDA → 후속 SkillObject 스폰               │
│  │  4. 충돌/Tick 비활성화, Hidden                                │
│  │  5. 다음 프레임 Destroy()                                    │
│  └──────────────────────────────────────────────────────────────│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 11. 기존 코드베이스 통합 전략

### 11.1 CombatTokenManager 통합

| 항목 | 현재 상태 | 통합 방안 |
|------|-----------|-----------|
| ASC 접근 | `FindComponentByClass<UASC>` | `UOKAbilitySystemComponent::GetASCFromActor()` 로 표준화 |
| 사망 이벤트 | `Event.Monster.Death` 태그 수신 | 그대로 유지, AttributeSet의 `PostGameplayEffectExecute`에서 발행 |
| 팀 구분 | 없음 (몬스터만 관리) | `Team.*` GameplayTag 도입 후 호환 유지 |

### 11.2 MeleeAttack 통합

| 항목 | 현재 상태 | 통합 방안 |
|------|-----------|-----------|
| 히트 검출 | 본 기반 CapsuleTraceMulti | 그대로 유지 (AnimNotify에 종속) |
| 데미지 적용 | `// TODO: 충돌 처리 로직 추가` | `UOKDamageHelper::ApplyDamageWithGE()` 호출 |
| GE Spec | 없음 | MeleeAttack용 GA에서 Spec 생성 후 전달 |

### 11.3 AgentManager 통합

| 항목 | 현재 상태 | 통합 방안 |
|------|-----------|-----------|
| 공간 쿼리 | `THierarchicalHashGrid2D` | SkillObject에서 사용하지 않음 (독립 유지) |
| `FInstancedStruct` | 에이전트 데이터 저장 | 패턴만 참조, SkillObject는 DuplicateObject 방식 사용 |

### 11.4 신규 의존성

| 파일 | 추가 내용 |
|------|-----------|
| `OKGame.Build.cs` | (이미 GAS 모듈 포함, 추가 불필요) |
| `DefaultEngine.ini` | `ECC_SkillDetection` 충돌 채널 정의 |
| 신규 `OKTeamTypes.h` | `Team.*` GameplayTag 네이티브 태그 정의 |
| 신규 `OKDamageHelper.h` | 공유 데미지 적용 유틸리티 |

---

## 12. 검토 항목 체크리스트

각 설계 결정의 상태를 추적한다:

### Spawn
- [x] `FOKSkillSpawnContext` 구조체 필드 정의
- [x] `FOKSkillSpawnResult` 구조체 필드 정의
- [x] Spread = N개 독립 Actor 결정
- [x] Drop 패턴 열거형 (`EOKDropPattern`) 정의
- [x] 스폰 실패 (벽 내부, Instigator 사망) 처리 정의

### Movement
- [x] PMC 소유 전략: AOKSkillObject 생성자에서 기본 비활성
- [x] Orbit 중심 소실 행동 (`EOKOrbitCenterLostBehavior`)
- [x] Boomerang 복귀 방식: 시전자 현재 위치 추적 + 거리 비례 가속
- [x] Wave 진동 모드 (`EOKWaveMode`)
- [x] Tick 최적화: `RequiresTick()` 가상 함수
- [x] 정책 상태: DuplicateObject 인스턴스의 멤버 변수

### Detection
- [x] 하이브리드 Shape 생성 (Sphere 기본 + Box/Capsule 동적)
- [x] 전용 충돌 채널 `ECC_SkillDetection`
- [x] MeleeAttack과의 역할 분리 명확화
- [x] AgentManager 비활용 결정 및 근거
- [x] Ray 정책: 매 프레임 SingleTrace, 고정 길이

### Filter
- [x] 팀 시스템: GameplayTag 기반 (`Team.*`)
- [x] 통합 `FilterPolicy_TeamBased` + 비트마스크
- [x] ASC 접근 표준화: `GetASCFromActor()`
- [x] Destructible: GameplayTag + IOKDamageable 이중 체크

### Effect
- [x] GE Spec 생성 위치: GA에서 생성, EffectPolicy에 전달
- [x] SetByCaller 태그 네이밍: `Data.Damage.Base` 등
- [x] Multi 정책: Best Effort (실패 무시)
- [x] Knockback: `LaunchCharacter` + `EOKKnockbackOrigin`
- [x] 공유 유틸리티: `UOKDamageHelper::ApplyDamageWithGE()`

### Timing
- [x] OnOverlap + Periodic 공존: `bFireOnFirstTick` 옵션
- [x] Chain: 같은 Actor 점프, `DamageDecayPerChain`
- [x] OnExpire: AOKSkillObject에서 직접 호출

### Expire
- [x] Composite: 기본 OR, AND는 별도 서브클래스
- [x] 폭발 스폰: `OnDestroyedSpawnDA` (AOKSkillObject 레벨)
- [x] Manual: GA의 EndAbility 연동
- [x] WallHit: PMC OnProjectileStop / SweepTrace 조건부

### 공통
- [x] 인스턴스 관리: DuplicateObject, Outer = AOKSkillObject
- [x] 데이터 흐름: AOKSkillObject를 Mediator로 사용
- [x] 파괴 시퀀스: 2-프레임 지연 (효과 → 숨김 → Destroy)
- [x] 타이머 전략: Tick 기반 누적 시간

---

> **다음 단계**: 본 문서의 설계 결정을 바탕으로 구현을 진행한다. 구현 순서는 `SkillObject_Design_Report.md`의 Phase별 계획을 따른다.
