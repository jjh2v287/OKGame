# SkillObject 시스템 설계 보고서

---

## 목차

- [1. 개요](#1-개요)
- [2. 논의 과정](#2-논의-과정)
  - [2.1 현재 코드베이스 분석](#21-현재-코드베이스-분석)
  - [2.2 초기 설계안: 단일 클래스 + Config Struct](#22-초기-설계안-단일-클래스--config-struct)
  - [2.3 핵심 전환: "단계 분리" 사고](#23-핵심-전환-단계-분리-사고)
  - [2.4 구현 방식 비교](#24-구현-방식-비교)
  - [2.5 최종 결정: 전부 UObject 정책 객체](#25-최종-결정-전부-uobject-정책-객체)
- [3. 최종 아키텍처](#3-최종-아키텍처)
  - [3.1 전체 구조](#31-전체-구조)
  - [3.2 7단계 정책 슬롯](#32-7단계-정책-슬롯)
  - [3.3 정책 베이스 클래스](#33-정책-베이스-클래스)
  - [3.4 각 단계별 서브클래스](#34-각-단계별-서브클래스)
- [4. 선행 조건: GAS 기반 레이어](#4-선행-조건-gas-기반-레이어)
- [5. 예시 스킬 조합](#5-예시-스킬-조합)
- [6. 구현 계획](#6-구현-계획)
- [7. 검증 방법](#7-검증-방법)
- [8. 결정 이력 요약](#8-결정-이력-요약)

---

## 1. 개요

### 1.1 문서 목적
본 문서는 OKGame 프로젝트의 **SkillObject(스킬 오브젝트) 시스템** 설계 과정을 기록한다. 문제 정의부터 최종 아키텍처 결정까지의 논의 흐름, 검토한 대안들, 각 결정의 근거를 포함한다.

### 1.2 프로젝트 배경
- **엔진**: Unreal Engine 5.7
- **장르**: 액션 게임 (싱글플레이어)
- **현재 상태**: GAS(Gameplay Ability System) 모듈이 링크되어 있으나 거의 미사용. CombatToken(AI 공격 턴 관리)과 MeleeAttack AnimNotifyState(본 기반 히트 검출)만 존재하며, 데미지 처리 로직은 미구현(`// TODO`) 상태.

### 1.3 해결하려는 문제
액션 게임에서 스킬은 매우 다양한 형태를 가진다:

| 차원 | 예시 |
|------|------|
| **이동 방식** | 직선 투사체, 유도 미사일, 정지 AoE, 시전자 추적, 공전, 부메랑 |
| **소환 방식** | 시전자 앞, 지정 위치, 부채꼴 다수, 상공 낙하 |
| **피격자 검출** | 플레이어만, 몬스터만, 모두, 파괴 가능 오브젝트 |
| **데미지 유형** | 단일 즉시, 범위(AoE), 도트(DoT), 연쇄, 소멸 시 폭발 |
| **종료 조건** | 시간 경과, 적중 즉시, N회 적중, 벽 충돌 |

이 조합의 수가 방대하기 때문에, 스킬마다 새 클래스를 만드는 방식은 유지보수 비용이 급격히 증가한다. **어떻게 하면 최소한의 코드로 최대한 다양한 스킬을 만들 수 있는가**가 핵심 과제이다.

> **TL;DR — 핵심 결론**
> 1. 스킬 오브젝트의 생애를 **7단계**(Spawn / Movement / Detection / Filter / Effect / Timing / Expire)로 분리
> 2. 각 단계를 **UObject 정책 객체**(`EditInlineNew` + `DefaultToInstanced`)로 구현
> 3. **DataAsset** 1개에 7개 정책 슬롯을 조합하여 새 스킬을 C++ 수정 없이 생성

---

## 2. 논의 과정

> **이 장의 핵심**: 코드베이스 분석 → 초기안(Config Struct) → 단계 분리 아이디어 → 3가지 구현 방식 비교 → UObject 정책 객체 채택

### 2.1 현재 코드베이스 분석

프로젝트 소스 코드를 전수 탐색하여 다음을 파악했다:

**존재하는 시스템:**
- `UUKCombatTokenManager` (GameInstanceSubsystem) — AI 몬스터의 공격 턴 관리. 토큰 풀 방식으로 동시 공격 수를 제어.
- `UOKAnimNotifyState_MeleeAttack` — 애니메이션 몽타주 중 본 위치 기반 캡슐 스윕으로 히트 검출. 프레임 보간으로 정확도 확보.
- `UOKAgentManager` — 2D 계층적 해시 그리드로 에이전트 근접 탐색. 대규모 NPC 관리용.
- `UOKCharacterMovementComponent` — 캐릭터 무브먼트 확장. 본 물리 업데이트 최적화.
- `UOKStressDataAsset` — `UPrimaryDataAsset` 활용 패턴이 이미 존재.

**부재하는 시스템:**
- AttributeSet (체력, 공격력 등 속성 없음)
- GameplayAbility / GameplayEffect 클래스
- 투사체 / 스킬 액터
- 팀/진영 시스템
- 데미지 파이프라인

**핵심 발견:**
- GAS 모듈(`GameplayAbilities`, `GameplayTags`, `GameplayTasks`)이 `OKGame.Build.cs`에 이미 의존성으로 등록되어 있음
- CombatTokenManager가 이미 `UAbilitySystemComponent`를 통해 `Event.Monster.Death` 태그로 사망 이벤트를 수신하고 있음 → GAS 인프라의 부분적 활용이 이루어지는 중
- MeleeAttack의 히트 처리 부분이 `// TODO: 충돌 처리 로직 추가`로 비어 있음 → 데미지 파이프라인을 구축하면 이 부분도 해결됨

### 2.2 초기 설계안: 단일 클래스 + Config Struct

첫 번째 접근은 **하나의 AOKSkillObject 클래스**에 모든 설정을 Config Struct들로 전달하는 방식이었다.

```
UOKSkillObjectDataAsset
 ├─ FOKSkillMovementConfig    (enum + 파라미터)
 ├─ FOKDetectionShapeConfig   (enum + 파라미터)
 ├─ FOKTargetFilterConfig     (enum + 파라미터)
 ├─ FOKDamageConfig           (enum + 파라미터)
 └─ ...
```

AOKSkillObject 내부에서 enum에 따라 `switch`문으로 분기하는 구조.

**검토 결과:**
- **장점**: 단순하고 가벼움. 코드 추적이 용이.
- **단점**: 새 이동 타입 추가 시 switch문 수정 필요 (OCP 위반). Config Struct가 모든 타입의 파라미터를 다 보유해야 함 (Orbit 전용 필드가 Projectile에서도 노출됨). 복잡한 행동 구현 시 switch 분기가 비대해짐.

### 2.3 핵심 전환: "단계 분리" 사고

논의 과정에서 **스킬 오브젝트의 생애를 독립된 단계로 분리**하는 아이디어가 도출되었다.

핵심 질문: *"스킬 오브젝트가 태어나서 사라질 때까지, 어떤 단계를 거치는가?"*

분석 결과, 모든 스킬 오브젝트는 다음 **7단계**를 거친다:

```
[Spawn] → [Movement] → [Detection] → [Filter] → [Effect] → [Timing] → [Expire]
 어디서      어떻게       뭘 감지?     누구한테?    뭘 적용?    언제?      끝나면?
 소환?       이동?                                                        어떻게?
```

각 단계를 구체적으로 나열하면:

#### 1) Spawn (소환 방식)
> 어디서, 어떤 형태로 생성되는가

- 시전자 앞에서
- 지정 위치에
- 타겟 위치에
- 하늘에서 낙하
- 부채꼴로 다수 스폰

#### 2) Movement (이동 방식)
> 스폰 후 어떻게 움직이는가

- 정지
- 직선 비행
- 유도 추적
- 시전자 추적
- 공전
- 부메랑

#### 3) Detection (감지 방식)
> 어떻게 대상을 찾는가

- 구체 오버랩
- 박스 오버랩
- 캡슐 오버랩
- 레이캐스트 (직선)

#### 4) Filter (필터링)
> 감지된 대상 중 누구에게 적용하는가

- 적만
- 아군만
- 모두
- 파괴 가능 오브젝트
- 커스텀 태그

#### 5) Effect (효과 적용)
> 대상에게 무엇을 하는가

- 즉시 데미지 1회
- 주기적 데미지
- 힐
- 버프/디버프 부여
- 넉백

#### 6) Timing (적용 타이밍)
> 언제 Effect를 발동하는가

- 오버랩 진입 시
- 주기적 (0.5초마다)
- 소멸 시
- 연쇄 점프

#### 7) Expire (종료 조건)
> 언제 사라지는가

- 시간 경과
- 적중 즉시
- N회 적중 후
- 시전자가 취소
- 벽 충돌 시

---

이렇게 7단계로 분리하면, **파이어볼**은:

> `Spawn(시전자 앞)` + `Movement(직선)` + `Detection(구체)` + `Filter(적만)` + `Effect(데미지)` + `Timing(오버랩 시)` + `Expire(적중 즉시 → 폭발 스폰)`

**힐링 서클**은:

> `Spawn(지정 위치)` + `Movement(정지)` + `Detection(구체)` + `Filter(아군만)` + `Effect(힐)` + `Timing(주기적 1초)` + `Expire(8초 후)`

각 단계는 **서로 독립적**이다. 이동 방식이 바뀌어도 감지 방식에는 영향이 없고, 필터링이 바뀌어도 효과 적용 로직은 동일하다. 이 독립성이 조합의 핵심이다.

### 2.4 구현 방식 비교

7단계를 코드로 어떻게 표현할지 3가지 방식을 비교했다:

| 기준 | A: Struct + Enum | B: ActorComponent | C: UObject 정책 객체 |
|------|------------------|--------------------|-----------------------|
| **코드 예시** | `switch(Config.Type)` | `UOKMovementComponent_X` | `UOKMovementPolicy_X : UObject` |
| **새 타입 추가** | switch문 수정 필요 (OCP 위반) | 새 컴포넌트 1개 | 새 UObject 1개 (기존 수정 없음) |
| **에디터 표시** | 모든 파라미터 노출 | 컴포넌트로 가시적 | 드롭다운 → 해당 파라미터만 인라인 |
| **오버헤드** | 최소 | 컴포넌트 등록/해제 비용 | UObject 생성 비용 (경미) |
| **Tick 불필요 단계** | 문제 없음 | Tick 없는 로직에도 컴포넌트 오버헤드 | 문제 없음 |
| **파라미터 혼재** | 모든 타입 파라미터가 struct에 공존 | 각 컴포넌트가 자체 파라미터 | 각 정책이 자체 파라미터 |
| **상태 관리** | struct 내부 | 컴포넌트 자체 상태 | 정책 자체 상태 |
| **클래스 수** | 적음 | 중간 | 많지만 각각 작고 단순 |

> **하이브리드 검토**: 복잡한 단계(Movement, Filter, Effect)만 UObject 정책으로, 단순한 단계(Spawn, Detection, Timing, Expire)는 Struct로 하는 절충안도 검토했으나, 두 가지 패턴 혼재로 일관성이 저하되어 기각.

### 2.5 최종 결정: 전부 UObject 정책 객체

> **결정**: 7단계 전부 UObject 정책 객체 방식으로 통일.

**근거:**
1. **일관성**: 7개 단계가 동일한 패턴을 따르므로, 한 단계의 사용법을 배우면 나머지도 동일. 학습 곡선이 평평해짐.
2. **확장성**: 어떤 단계에 새 옵션이 필요하든 항상 동일한 방법(새 UObject 서브클래스 1개)으로 대응. 기존 코드 수정 불필요.
3. **에디터 UX**: `EditInlineNew` + `DefaultToInstanced` 조합으로 DataAsset 편집 시 드롭다운에서 타입을 선택하면 해당 타입의 파라미터만 인라인으로 표시됨. 디자이너 친화적.
4. **단순함 예측**: 처음에는 Struct로 충분해 보이는 단계(예: Expire)도, 게임 개발이 진행되면 복잡해질 가능성이 높음 (예: "3초 후 소멸하되 적중 시에도 소멸" 같은 복합 조건). UObject로 시작하면 나중에 마이그레이션 비용이 없음.

**기각된 대안 요약:**

| 방식 | 기각 사유 |
|------|-----------|
| Struct + Enum | OCP 위반, switch문 관리 부담, 파라미터 혼재 |
| ActorComponent | 불필요한 오버헤드 (7단계 중 Tick 필요한 건 2개뿐) |
| 하이브리드 | 두 가지 패턴 혼재로 일관성 저하 |

---

## 3. 최종 아키텍처

> **이 장의 핵심**: DataAsset에 7개 정책 슬롯 → 드롭다운 선택으로 스킬 조합 → 새 스킬 = DataAsset 1개, 새 옵션 = UObject 1개

### 3.1 전체 구조

```
GameplayAbility (GA_SkillObject)
    │
    │  ActivateAbility()
    │    1. GE Spec Handle 생성
    │    2. SpawnPolicy->CalculateSpawnTransforms()
    │    3. SpawnSkillObject()
    │
    └──→ AOKSkillObject::InitializeSkillObject()
              │
              ├─ MovementPolicy->Initialize()
              ├─ DetectionPolicy->Initialize()     → Shape 컴포넌트 생성
              ├─ FilterPolicy->Initialize()
              ├─ TimingPolicy->Initialize()
              ├─ EffectPolicy->Initialize()
              └─ ExpirePolicy->Initialize()

         런타임 루프:
              MovementPolicy->Tick()            → 위치 업데이트
              Detection 오버랩 감지             → TimingPolicy에 전달
              TimingPolicy 발동 결정            → FilterPolicy로 필터링
              FilterPolicy 통과 시              → EffectPolicy 적용
              ExpirePolicy 조건 충족 시         → 소멸 처리
```

### 3.2 7단계 정책 슬롯

DataAsset에서 7개 슬롯에 원하는 정책을 끼워넣어 조합한다:

```
UOKSkillObjectDataAsset
 ├─ UOKSpawnPolicy*          ← 어디서 소환?
 ├─ UOKMovementPolicy*       ← 어떻게 이동?
 ├─ UOKDetectionPolicy*      ← 뭘 감지?
 ├─ UOKFilterPolicy*         ← 누구한테?
 ├─ UOKEffectPolicy*         ← 뭘 적용?
 ├─ UOKTimingPolicy*         ← 언제 발동?
 └─ UOKExpirePolicy*         ← 언제 사라짐?
```

- **새 스킬 추가** = DataAsset 1개 생성 + 7개 드롭다운 선택 (C++ 수정 없음)
- **새 옵션 추가** = 해당 단계의 UObject 서브클래스 1개 추가 (다른 코드 수정 없음)

### 3.3 정책 베이스 클래스

7개 정책은 모두 `EditInlineNew`, `DefaultToInstanced` UObject로 선언한다.

| 정책 | 역할 | 핵심 인터페이스 |
|------|------|-----------------|
| **SpawnPolicy** | 스폰 위치/회전 계산 | `CalculateSpawnTransforms(Context) → TArray<FTransform>` |
| **MovementPolicy** | 매 프레임 위치 갱신 | `Initialize(Owner)`, `Tick(Owner, DeltaTime)` |
| **DetectionPolicy** | 충돌 감지 Shape 생성 | `CreateDetectionComponent(Owner) → UShapeComponent*` |
| **FilterPolicy** | 유효 타겟 판정 | `Initialize(TeamTag, Instigator)`, `PassesFilter(Target) → bool` |
| **EffectPolicy** | GE 등 효과 적용 | `Initialize(ASC, SpecHandle)`, `ApplyEffect(Target)` |
| **TimingPolicy** | 발동 시점 결정 | `Initialize(Owner)`, `OnDetected(Target)`, `Tick(DeltaTime)` |
| **ExpirePolicy** | 소멸 조건 판정 | `Initialize(Owner)`, `ShouldExpire() → bool`, `OnHit()`, `OnWallHit()` |

<details>
<summary><b>상세 코드 (클릭하여 펼치기)</b></summary>

```cpp
// Spawn: 스폰 위치/회전을 계산하여 Transform 배열 반환
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class UOKSpawnPolicy : public UObject
{
    virtual TArray<FTransform> CalculateSpawnTransforms(const FOKSkillSpawnContext& Context) const;
};

// Movement: 매 프레임 SkillObject의 위치를 갱신
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class UOKMovementPolicy : public UObject
{
    virtual void Initialize(AOKSkillObject* Owner);
    virtual void Tick(AOKSkillObject* Owner, float DeltaTime);
};

// Detection: 충돌 감지용 Shape 컴포넌트를 생성
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class UOKDetectionPolicy : public UObject
{
    virtual UShapeComponent* CreateDetectionComponent(AOKSkillObject* Owner);
};

// Filter: 감지된 액터가 유효한 타겟인지 판정
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class UOKFilterPolicy : public UObject
{
    virtual void Initialize(const FGameplayTag& InstigatorTeamTag, AActor* Instigator);
    virtual bool PassesFilter(AActor* Target) const;
};

// Effect: 유효 타겟에게 GameplayEffect 등을 적용
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class UOKEffectPolicy : public UObject
{
    virtual void Initialize(UAbilitySystemComponent* InstigatorASC,
        const FGameplayEffectSpecHandle& SpecHandle);
    virtual void ApplyEffect(AActor* Target);
};

// Timing: 언제 Effect를 발동할지 결정 (오버랩 시, 주기적, 소멸 시 등)
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class UOKTimingPolicy : public UObject
{
    virtual void Initialize(AOKSkillObject* Owner);
    virtual void OnDetected(AActor* Target);
    virtual void Tick(float DeltaTime);
};

// Expire: SkillObject의 소멸 조건 판정
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class UOKExpirePolicy : public UObject
{
    virtual void Initialize(AOKSkillObject* Owner);
    virtual bool ShouldExpire() const;
    virtual void OnHit();
    virtual void OnWallHit();
};
```

</details>

### 3.4 각 단계별 서브클래스

#### Spawn 정책
| 클래스 | 동작 | 주요 파라미터 |
|--------|------|---------------|
| `UOKSpawnPolicy_Forward` | 시전자 앞 오프셋 위치 | Offset(FVector) |
| `UOKSpawnPolicy_AtTarget` | 지정 타겟/위치에 스폰 | (런타임 타겟 위치) |
| `UOKSpawnPolicy_Spread` | 부채꼴로 N개 스폰 | Count, SpreadAngle |
| `UOKSpawnPolicy_Drop` | 상공에서 낙하 | DropHeight, Count |

#### Movement 정책
| 클래스 | 동작 | 주요 파라미터 |
|--------|------|---------------|
| `UOKMovementPolicy_None` | 정지 | — |
| `UOKMovementPolicy_Projectile` | 직선 비행 | Speed, GravityScale |
| `UOKMovementPolicy_Homing` | 유도 추적 | Speed, HomingAccel |
| `UOKMovementPolicy_Orbit` | 시전자 중심 공전 | Radius, DegreesPerSec |
| `UOKMovementPolicy_Follow` | 시전자 추적 | Offset, InterpSpeed |
| `UOKMovementPolicy_Boomerang` | 전진 후 복귀 | Speed, MaxDist, ReturnSpeed |
| `UOKMovementPolicy_Wave` | 사인파 이동 | Speed, Amplitude, Frequency |

> Projectile/Homing은 내부에서 엔진의 `UProjectileMovementComponent`를 활용한다.

#### Detection 정책
| 클래스 | 동작 | 주요 파라미터 |
|--------|------|---------------|
| `UOKDetectionPolicy_Sphere` | 구체 오버랩 | Radius |
| `UOKDetectionPolicy_Box` | 박스 오버랩 | Extent(FVector) |
| `UOKDetectionPolicy_Capsule` | 캡슐 오버랩 | Radius, HalfHeight |
| `UOKDetectionPolicy_Ray` | 레이캐스트 | Length, TraceInterval |

#### Filter 정책
| 클래스 | 동작 | 주요 파라미터 |
|--------|------|---------------|
| `UOKFilterPolicy_Enemy` | 인스티게이터의 적만 | bIgnoreInstigator |
| `UOKFilterPolicy_Ally` | 인스티게이터의 아군만 | bIgnoreInstigator |
| `UOKFilterPolicy_All` | 모든 캐릭터 | bIgnoreInstigator |
| `UOKFilterPolicy_Tag` | 커스텀 태그 기반 | RequiredTags, BlockedTags |
| `UOKFilterPolicy_Destructible` | 파괴 가능 오브젝트 | — |

#### Effect 정책
| 클래스 | 동작 | 주요 파라미터 |
|--------|------|---------------|
| `UOKEffectPolicy_Damage` | GE로 데미지 적용 | GE Class, SetByCallerTag, Coefficient |
| `UOKEffectPolicy_Heal` | GE로 회복 적용 | GE Class, Coefficient |
| `UOKEffectPolicy_Buff` | GE로 버프/디버프 부여 | GE Class |
| `UOKEffectPolicy_Knockback` | 넉백 + GE 적용 | KnockbackForce, GE Class |
| `UOKEffectPolicy_Multi` | 위 효과 복합 적용 | TArray\<UOKEffectPolicy*\> |

#### Timing 정책
| 클래스 | 동작 | 주요 파라미터 |
|--------|------|---------------|
| `UOKTimingPolicy_OnOverlap` | 오버랩 진입 시 1회 | MaxHitPerTarget |
| `UOKTimingPolicy_Periodic` | 주기적 반복 | Interval |
| `UOKTimingPolicy_OnExpire` | 소멸 시 1회 | — |
| `UOKTimingPolicy_Chain` | 연쇄 점프 | MaxChain, SearchRadius |

#### Expire 정책
| 클래스 | 동작 | 주요 파라미터 |
|--------|------|---------------|
| `UOKExpirePolicy_Lifetime` | N초 후 소멸 | Duration |
| `UOKExpirePolicy_OnHit` | 적중 즉시 소멸 | — |
| `UOKExpirePolicy_HitCount` | N회 적중 후 소멸 | MaxHits |
| `UOKExpirePolicy_WallHit` | 벽 충돌 시 소멸 | — |
| `UOKExpirePolicy_Manual` | 시전자가 취소할 때까지 | — |
| `UOKExpirePolicy_Composite` | 복합 조건 (OR) | TArray\<UOKExpirePolicy*\> |

---

## 4. 선행 조건: GAS 기반 레이어

> **이 장의 핵심**: SkillObject가 동작하려면 AttributeSet, ASC, 팀 시스템 등 GAS 인프라가 먼저 필요하다.

### 4.1 AttributeSet (`UOKAttributeSet`)
- Health, MaxHealth, AttackPower, Defense 어트리뷰트
- IncomingDamage, IncomingHeal 메타 어트리뷰트 (데미지 파이프라인 중간값)
- `PostGameplayEffectExecute`에서 최종 Health 차감/회복 및 Death 이벤트 발행

### 4.2 AbilitySystemComponent (`UOKAbilitySystemComponent`)
- `static GetASCFromActor(AActor*)` 유틸리티 (기존 CombatToken의 `FindComponentByClass` 패턴 표준화)
- `GetTeamTag()` 헬퍼 (Filter 정책에서 사용)

### 4.3 기본 GameplayAbility (`UOKGameplayAbility`)
- 공통 유틸리티 메서드 제공

### 4.4 팀/진영 시스템 (`OKTeamTypes`)
- `Team.Player`, `Team.Monster`, `Team.Neutral`, `Team.Destructible` GameplayTag 정의
- `FGenericTeamId` 매핑 (AI Controller 호환)
- `IsEnemy()`, `IsAlly()` 판정 유틸리티

---

## 5. 예시 스킬 조합

> **이 장의 핵심**: 7개 정책 슬롯만으로 파이어볼, 힐링 서클, 유도 화살, 독구름, 공전 방패, 메테오, 체인 라이트닝을 모두 표현 가능

| 스킬 | Spawn | Movement | Detection | Filter | Effect | Timing | Expire |
|------|-------|----------|-----------|--------|--------|--------|--------|
| **파이어볼** | Forward(100) | Projectile(2000) | Sphere(30) | Enemy | Damage(GE_Fire, 1.5) | OnOverlap(1) | OnHit → 폭발 스폰 |
| **힐링 서클** | AtTarget | None | Sphere(300) | Ally | Heal(GE_Heal, 0.3) | Periodic(1.0) | Lifetime(8s) |
| **유도 화살** | Forward(50) | Homing(800, 5000) | Sphere(20) | Enemy | Damage(GE_Arrow, 1.0) | OnOverlap | OnHit |
| **독구름** | AtTarget | None | Sphere(200) | Enemy | Damage(GE_Poison, 0.2) | Periodic(0.5) | Lifetime(6s) |
| **공전 방패** | Spread(3, 120) | Orbit(150, 180/s) | Sphere(40) | Enemy | Damage(GE_Shield, 0.5) | OnOverlap(1) | Lifetime(12s) |
| **메테오** | Drop(1000, 5) | Projectile(3000, G=1.0) | Sphere(200) | Enemy | Damage(GE_Meteor, 3.0) | OnOverlap | WallHit → 폭발 스폰 |
| **체인 라이트닝** | Forward(0) | Projectile(5000) | Sphere(20) | Enemy | Damage(GE_Lightning, 1.0) | Chain(5, R=500) | OnHit(연쇄 후) |

### 상세 설명

<details>
<summary><b>파이어볼 — 투사체 → 적중 시 폭발</b></summary>

적중 시 소멸하며 `DA_Fireball_Explosion`(정지 + 구체 R=300 + AoE 데미지)을 추가 스폰.

</details>

<details>
<summary><b>힐링 서클 — 정지 AoE, 주기적 아군 회복</b></summary>

지면 지정 위치에 8초간 유지. 1초 간격으로 반경 내 아군에게 회복 적용.

</details>

<details>
<summary><b>유도 화살 — 호밍 단일 타겟</b></summary>

시전자 앞에서 출발, 타겟을 향해 가속 유도. 적중 즉시 소멸.

</details>

<details>
<summary><b>독구름 — DoT</b></summary>

지정 위치에 6초간 정지. 0.5초 간격으로 반경 내 적에게 독 데미지.

</details>

<details>
<summary><b>공전 방패 — 시전자 주위 공전, 접촉 데미지</b></summary>

120도 간격으로 3개 스폰. 반경 150에서 180도/s로 공전. 12초간 유지.

</details>

<details>
<summary><b>메테오 — 상공 낙하 AoE</b></summary>

상공 1000에서 5개 낙하. 중력 적용 투사체. 지면 충돌 시 폭발 스폰.

</details>

<details>
<summary><b>체인 라이트닝 — 연쇄</b></summary>

초고속 투사체. 적중 시 반경 500 내 최대 5회 연쇄 점프 후 소멸.

</details>

---

## 6. 구현 계획

> **이 장의 핵심**: Phase 1(GAS 기반) → Phase 2(정책 베이스) → Phase 3(서브클래스) → Phase 4(핵심 액터)

### 6.1 Phase별 구현 순서

| Phase | 내용 | 산출물 |
|-------|------|--------|
| **Phase 1** | GAS 기반 레이어 | AttributeSet, ASC, 기본 Ability, 팀 시스템 |
| **Phase 2** | 정책 베이스 + DataAsset | 7개 정책 베이스 클래스, DataAsset 클래스 |
| **Phase 3** | 정책 서브클래스 구현 | 약 30개 정책 서브클래스 |
| **Phase 4** | 핵심 액터 + 어빌리티 | AOKSkillObject, GA_SkillObject, DamageExecution |

### 6.2 파일 구조

```
Source/OKGame/
├── GAS/
│   ├── OKAbilitySystemComponent.h/.cpp
│   ├── OKAttributeSet.h/.cpp
│   ├── OKGameplayAbility.h/.cpp
│   └── OKGameplayAbility_SkillObject.h/.cpp
├── Team/
│   └── OKTeamTypes.h/.cpp
├── SkillObject/
│   ├── OKSkillObject.h/.cpp
│   ├── OKSkillObjectDataAsset.h/.cpp
│   ├── OKSkillObjectTypes.h
│   ├── Policy/
│   │   ├── OKSkillPolicyBase.h
│   │   ├── Spawn/    (4개 서브클래스)
│   │   ├── Movement/ (7개 서브클래스)
│   │   ├── Detection/(4개 서브클래스)
│   │   ├── Filter/   (5개 서브클래스)
│   │   ├── Effect/   (5개 서브클래스)
│   │   ├── Timing/   (4개 서브클래스)
│   │   └── Expire/   (6개 서브클래스)
│   └── Damage/
│       └── OKDamageExecutionCalculation.h/.cpp
```

---

## 7. 검증 방법

> **이 장의 핵심**: 컴파일 → 에디터 → 런타임 → 조합 순서로 단계적 검증

1. **컴파일 테스트**: 각 Phase 완료 시 빌드 확인
2. **에디터 테스트**: DataAsset 생성 → 7개 드롭다운에서 정책 선택 → 인라인 파라미터 편집 동작 확인
3. **런타임 테스트**: LV-Test 맵에서 GA_SkillObject 활성화 → 스폰/이동/감지/데미지 동작 확인
4. **조합 테스트**: 동일 Movement에 다른 Effect를 조합하여 정상 동작 확인

---

## 8. 결정 이력 요약

| 결정 | 선택 | 근거 |
|------|------|------|
| 네트워크 | 싱글플레이어 전용 | 리플리케이션 오버헤드 제거 |
| 단계 분리 | 7단계 (Spawn/Movement/Detection/Filter/Effect/Timing/Expire) | 스킬 생애 주기의 자연스러운 분절점 |
| 구현 방식 | 전부 UObject 정책 객체 | 일관성, 확장성, 에디터 UX |
| 데이터 관리 | DataAsset (UPrimaryDataAsset) | 프로젝트 기존 패턴과 일치, 에셋 관리 용이 |
| 데미지 처리 | GAS (GameplayEffect + SetByCaller) | 기존 GAS 의존성 활용, CombatToken과 호환 |
