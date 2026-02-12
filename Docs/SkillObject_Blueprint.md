# SkillObject 설계도

> 이 문서는 **"실제로 코드를 짤 때 뭘 어떤 형태로 만드는가"** 를 그림으로 보여준다.

---

## 한줄 답변

> **정책(조합 블록) = 전부 UObject.** 나머지(데이터 전달, 옵션 목록 등)는 상황에 맞는 형태를 쓴다.

---

## 1. 전체 그림: 레고 비유

```
┌─────────────────────────────────────────────────────┐
│                                                     │
│   📦 DataAsset  (= 레고 조립 설명서)                  │
│   "파이어볼은 이 블록들을 끼우세요"                     │
│                                                     │
│   ┌──────────┐ ┌──────────┐ ┌──────────┐           │
│   │  Spawn   │ │ Movement │ │Detection │           │
│   │  블록    │ │  블록    │ │  블록    │  ...       │
│   └──────────┘ └──────────┘ └──────────┘           │
│       ↑             ↑            ↑                  │
│    UObject       UObject      UObject               │
│                                                     │
└──────────────────────┬──────────────────────────────┘
                       │
                       │ "설명서대로 조립!"
                       ▼
┌─────────────────────────────────────────────────────┐
│                                                     │
│   🎯 AOKSkillObject  (= 완성된 레고 작품)             │
│   "나는 파이어볼이다. 날아가서 터진다."                  │
│                                                     │
│   내 안에 복사된 블록들:                               │
│   [Spawn복사본] [Movement복사본] [Detection복사본] ... │
│                                                     │
└─────────────────────────────────────────────────────┘
```

---

## 2. 각 조각의 형태와 이유

코드에 등장하는 모든 조각을 **4가지 형태**로 나눈다:

```
형태 1: AActor        → 게임 세계에 실제로 존재하는 것 (보이고, 움직이고, 부딪힘)
형태 2: UObject       → 두뇌/규칙 담당 (보이지 않지만 판단을 내림)
형태 3: 구조체(Struct) → 택배 상자 (데이터를 담아서 전달만 함)
형태 4: 열거형(Enum)   → 메뉴판 (정해진 선택지 중 하나 고르기)
```

### 한눈에 보기

| 이름 | 형태 | 한마디 설명 | 왜 이 형태? |
|------|------|-------------|-------------|
| **AOKSkillObject** | AActor | 게임에 나타나는 스킬 본체 | 위치, 충돌, 이동이 필요하니까 |
| **DataAsset** | UPrimaryDataAsset | 에디터에서 만드는 조립 설명서 | 에셋으로 저장/관리해야 하니까 |
| **7개 정책** | UObject | 조합 블록 (끼워넣는 두뇌) | 드롭다운으로 교체 가능해야 하니까 |
| **SpawnContext** | 구조체 | "누가, 어디서, 뭘 향해" 정보 묶음 | 함수에 데이터 넘기기만 하면 되니까 |
| **SpawnResult** | 구조체 | "여기에, 이 방향으로 만들어" 결과 묶음 | 위와 같은 이유 |
| **EOKDropPattern** | 열거형 | Random / Ring / Grid / Cross | 고정된 선택지니까 |
| **EOKOrbitCenterLost** | 열거형 | 중심 사라지면? 소멸 / 직진 / 고정 | 고정된 선택지니까 |
| **EOKKnockbackOrigin** | 열거형 | 밀어내는 방향 기준점 | 고정된 선택지니까 |
| **EOKFilterTarget** | 열거형 (비트) | 적 / 아군 / 파괴물 조합 체크 | 체크박스처럼 복수 선택이니까 |
| **PMC** | 엔진 컴포넌트 | 투사체 물리 처리기 | 언리얼이 이미 만들어놓은 것 |
| **Shape** | 엔진 컴포넌트 | 감지 범위 도형 | 언리얼 충돌 시스템을 써야 하니까 |

---

## 3. 왜 정책(블록)은 전부 UObject인가?

### 만약 구조체로 만들면?

```
스킬 DataAsset 에디터 화면:

  이동 타입: [드롭다운: 직선 ▼]

  ──── 직선 전용 설정 ────
    속도: 2000
    중력: 0
  ──── 공전 전용 설정 (안 쓰는데 보임) ────
    반지름: ???
    각속도: ???
  ──── 부메랑 전용 설정 (안 쓰는데 보임) ────
    복귀속도: ???
    도착반경: ???

  → 문제: 안 쓰는 설정까지 전부 보인다!
  → 문제: 새 이동 타입 추가하면 switch문 수정해야 한다!
```

### UObject로 만들면?

```
스킬 DataAsset 에디터 화면:

  이동 정책: [드롭다운: Projectile(직선) ▼]
    └── 속도: 2000
    └── 중력: 0

  → 선택한 것의 설정만 보인다!
  → 새 이동 타입 = 파일 1개 추가. 기존 코드 수정 없음!
```

### 비교표

```
                구조체              UObject
                ──────              ───────
에디터 모습:     다 보임 (지저분)     선택한 것만 보임 (깔끔)
새 타입 추가:    switch문 수정 필요   파일 1개 추가 (끝)
런타임 상태:     가능은 한데 번거로움  자연스러움
메모리:          아주 약간 가벼움     약간 무거움 (무시해도 될 수준)

→ 결론: 메모리 차이는 미미. 에디터 편의성과 확장성이 압도적.
→ 전부 UObject로 통일!
```

---

## 4. 실제 파일 구조 그림

```
Source/OKGame/SkillObject/
│
├── OKSkillObject.h/.cpp              ← 🎯 메인 액터 (AActor)
├── OKSkillObjectDataAsset.h/.cpp     ← 📦 조립 설명서 (DataAsset)
├── OKSkillObjectTypes.h              ← 📋 구조체 + 열거형 모음
│
├── Policy/                           ← 🧩 조합 블록 모음
│   ├── OKSpawnPolicy.h               ← 스폰 블록 부모
│   ├── OKMovementPolicy.h            ← 이동 블록 부모
│   ├── OKDetectionPolicy.h           ← 감지 블록 부모
│   ├── OKFilterPolicy.h              ← 필터 블록 부모
│   ├── OKEffectPolicy.h              ← 효과 블록 부모
│   ├── OKTimingPolicy.h              ← 타이밍 블록 부모
│   ├── OKExpirePolicy.h              ← 종료 블록 부모
│   │
│   ├── Spawn/
│   │   ├── OKSpawnPolicy_Forward.h   ← "시전자 앞에서"
│   │   ├── OKSpawnPolicy_AtTarget.h  ← "지정 위치에"
│   │   ├── OKSpawnPolicy_Spread.h    ← "부채꼴로 여러 개"
│   │   └── OKSpawnPolicy_Drop.h      ← "하늘에서 떨어짐"
│   │
│   ├── Movement/
│   │   ├── OKMovementPolicy_None.h       ← "정지"
│   │   ├── OKMovementPolicy_Projectile.h ← "직선 비행"
│   │   ├── OKMovementPolicy_Homing.h     ← "유도 추적"
│   │   ├── OKMovementPolicy_Orbit.h      ← "시전자 주위 공전"
│   │   ├── OKMovementPolicy_Follow.h     ← "시전자 따라감"
│   │   ├── OKMovementPolicy_Boomerang.h  ← "갔다 돌아옴"
│   │   └── OKMovementPolicy_Wave.h       ← "물결치며 이동"
│   │
│   ├── Detection/
│   │   ├── OKDetectionPolicy_Sphere.h    ← "구체 범위"
│   │   ├── OKDetectionPolicy_Box.h       ← "상자 범위"
│   │   ├── OKDetectionPolicy_Capsule.h   ← "캡슐 범위"
│   │   └── OKDetectionPolicy_Ray.h       ← "직선 레이저"
│   │
│   ├── Filter/
│   │   ├── OKFilterPolicy_TeamBased.h    ← "적/아군/파괴물 체크"
│   │   └── OKFilterPolicy_Tag.h          ← "태그로 직접 지정"
│   │
│   ├── Effect/
│   │   ├── OKEffectPolicy_Damage.h       ← "데미지"
│   │   ├── OKEffectPolicy_Heal.h         ← "회복"
│   │   ├── OKEffectPolicy_Buff.h         ← "버프/디버프"
│   │   ├── OKEffectPolicy_Knockback.h    ← "밀어내기"
│   │   └── OKEffectPolicy_Multi.h        ← "위 효과 여러 개 동시"
│   │
│   ├── Timing/
│   │   ├── OKTimingPolicy_OnOverlap.h    ← "닿으면 바로"
│   │   ├── OKTimingPolicy_Periodic.h     ← "N초마다 반복"
│   │   ├── OKTimingPolicy_OnExpire.h     ← "사라질 때 한번"
│   │   └── OKTimingPolicy_Chain.h        ← "연쇄 점프"
│   │
│   └── Expire/
│       ├── OKExpirePolicy_Lifetime.h     ← "N초 후 소멸"
│       ├── OKExpirePolicy_OnHit.h        ← "맞으면 소멸"
│       ├── OKExpirePolicy_HitCount.h     ← "N번 맞으면 소멸"
│       ├── OKExpirePolicy_WallHit.h      ← "벽에 부딪히면 소멸"
│       ├── OKExpirePolicy_Manual.h       ← "내가 취소할 때까지"
│       └── OKExpirePolicy_Composite.h    ← "위 조건 조합 (OR/AND)"
│
└── Damage/
    └── OKDamageHelper.h/.cpp             ← 🔧 데미지 공용 도구
```

---

## 5. 각 형태별 실제 모습

### 형태 1: AActor — 게임에 나타나는 것

```
AOKSkillObject (= 파이어볼, 힐링 서클, 유도 미사일 등)
┌──────────────────────────────────────────┐
│                                          │
│  가지고 태어나는 것 (생성자에서 만듦):      │
│  ├── 투사체 물리 (PMC) ← 기본 꺼져있음    │
│  └── 구체 감지 범위 (Sphere) ← 기본 꺼져있음│
│                                          │
│  조립 후 가지는 것 (초기화 시 복사):        │
│  ├── SpawnPolicy 복사본                   │
│  ├── MovementPolicy 복사본                │
│  ├── DetectionPolicy 복사본               │
│  ├── FilterPolicy 복사본                  │
│  ├── EffectPolicy 복사본                  │
│  ├── TimingPolicy 복사본                  │
│  └── ExpirePolicy 복사본                  │
│                                          │
│  혼자 가진 설정:                           │
│  └── 소멸 시 스폰할 DataAsset (폭발 등)    │
│                                          │
└──────────────────────────────────────────┘
```

### 형태 2: UObject — 두뇌/규칙 블록

7개 정책 블록 모두 같은 패턴:

```
[부모] UOKMovementPolicy          ← 추상. "이동 블록이란 이런 것이다"
  │
  ├── [자식] _None                ← "안 움직여"
  ├── [자식] _Projectile          ← "직선으로 날아가" + 속도, 중력
  ├── [자식] _Homing              ← "쫓아가"          + 속도, 회전력
  ├── [자식] _Orbit               ← "빙글빙글 돌아"    + 반지름, 각속도
  └── ...

에디터에서 보이는 모습:

  이동 정책: [ Projectile ▼ ]     ← 드롭다운으로 선택
    ├── Speed: 2000               ← 선택한 타입의 설정만 보임
    └── GravityScale: 0.0
```

**핵심**: 부모 1개 + 자식 여러 개. 에디터에서 드롭다운으로 바꾸면 설정도 바뀜.

### 형태 3: 구조체 — 데이터 택배 상자

```
FOKSkillSpawnContext (GA → SpawnPolicy로 전달)
┌──────────────────────────────┐
│  시전자: 누구?                │
│  시전 위치: 어디?              │
│  조준 방향: 어디 보고?         │
│  타겟 위치: 어디 찍었어? (선택) │
│  유도 타겟: 누굴 쫓아? (선택)  │
│  데미지 정보: 얼마나 아파?     │
│  랜덤 시드: 매번 같은 패턴?    │
└──────────────────────────────┘

FOKSkillSpawnResult (SpawnPolicy → GA로 반환)
┌──────────────────────────────┐
│  스폰 위치/방향: 여기에 만들어  │
│  초기 속도: 이 속도로 출발      │
│  개별 유도 타겟: 이놈 쫓아 (선택)│
└──────────────────────────────┘
```

**왜 구조체?** → 함수 호출할 때 데이터를 묶어서 넘기기만 하면 된다. 판단을 내리거나, 상태를 기억하거나, 에디터에서 교체할 필요가 없다.

### 형태 4: 열거형 — 메뉴판

```
EOKDropPattern     →  "낙하 패턴 뭐로 할래?"
  ├── Random            랜덤으로 흩뿌리기
  ├── Ring              원형으로 배치
  ├── Grid              격자로 배치
  └── Cross             십자로 배치

EOKKnockbackOrigin →  "밀어내는 기준점?"
  ├── SkillObject       스킬에서 밀기
  ├── Instigator        시전자에서 밀기
  ├── WorldUp           위로 띄우기
  └── Custom            내가 정한 방향

EOKFilterTarget    →  "누구한테 적용? (복수 선택 가능)"
  ├── ☐ Enemy           적
  ├── ☐ Ally            아군
  ├── ☐ Neutral         중립
  ├── ☐ Destructible    파괴물
  └── ☐ Self            나 자신
```

---

## 6. 형태를 고르는 기준 (앞으로 새로 만들 때도 적용)

```
질문 1: 게임 세계에 나타나야 해? (위치, 충돌, 보이기)
  → Yes → AActor
  → No  → 다음 질문

질문 2: 에디터에서 드롭다운으로 교체해야 해?
  → Yes → UObject (EditInlineNew + DefaultToInstanced)
  → No  → 다음 질문

질문 3: 고정된 선택지 중 하나를 고르는 거야?
  → Yes → 열거형 (Enum)
  → No  → 구조체 (Struct)
```

그림으로 보면:

```
         게임에 나타남?
            │
     ┌──Yes─┤──No──┐
     │              │
  AActor      드롭다운 교체?
                    │
             ┌──Yes─┤──No──┐
             │              │
          UObject     고정 선택지?
                            │
                     ┌──Yes─┤──No──┐
                     │              │
                   Enum          Struct
```

---

## 7. 조립 흐름: 처음부터 끝까지

```
[에디터에서]

  1. DataAsset 만들기 (= 조립 설명서 작성)
     ┌─────────────────────────────┐
     │ DA_Fireball                 │
     │                             │
     │ Spawn:     Forward(100)     │ ← 드롭다운 선택
     │ Movement:  Projectile(2000) │ ← 드롭다운 선택
     │ Detection: Sphere(30)       │ ← 드롭다운 선택
     │ Filter:    Enemy            │ ← 체크박스
     │ Effect:    Damage(1.5배)    │ ← 드롭다운 선택
     │ Timing:    OnOverlap        │ ← 드롭다운 선택
     │ Expire:    OnHit            │ ← 드롭다운 선택
     │                             │
     │ 소멸시 스폰: DA_Explosion    │ ← 에셋 참조
     └─────────────────────────────┘


[게임 실행 중]

  2. 어빌리티가 발동됨
     │
     │ SpawnContext 구조체에 정보 담기
     │ (누가 쐈는지, 어디 보고 있는지, 데미지 얼마인지)
     │
     ▼
  3. SpawnPolicy가 위치 계산
     │
     │ "시전자 앞 100cm에 만들어"
     │ → SpawnResult 구조체로 반환
     │
     ▼
  4. AOKSkillObject 생성!
     │
     │ DataAsset의 7개 블록을 복사(DuplicateObject)
     │ 각 블록 초기화
     │ Tick 필요 여부 판단 후 켜기/끄기
     │
     ▼
  5. 매 프레임 동작
     │
     │ [소멸 체크] → 아직 살아있나?
     │ [이동]     → Movement가 위치 갱신
     │ [감지]     → 누가 범위에 들어오면 이벤트
     │ [필터]     → 적인가? 아군인가?
     │ [타이밍]   → 지금 때려야 하나?
     │ [효과]     → 데미지 적용!
     │ [소멸]     → 조건 충족 → 사라짐
     │
     ▼
  6. 소멸 처리
     │
     │ 소멸 시 효과 있으면 → 적용
     │ 후속 스폰 있으면 → DA_Explosion 스폰
     │ 숨기고 → 다음 프레임에 제거
```

---

## 8. 자주 헷갈리는 것들

### Q: "구조체가 더 가볍지 않나?"

```
구조체:  메모리 약간 적음.  하지만 에디터에서 드롭다운 교체 불가.
UObject: 메모리 약간 많음.  에디터에서 드롭다운으로 자유롭게 교체.

파이어볼 1개에 정책 7개 × UObject 1개 = 약 7KB 추가.
동시에 파이어볼 100개 떠도 700KB.
RAM 16GB 기준 0.004%.

→ 의미 없는 차이. 편의성이 훨씬 중요.
```

### Q: "7개 전부 UObject? 진짜 전부?"

```
"정책 블록" 7개만 UObject.
나머지는 상황에 맞는 형태:

  ┌─────────────────┬──────────┐
  │ 이것은           │ 형태     │
  ├─────────────────┼──────────┤
  │ 정책 블록 7개    │ UObject  │  ← 교체가 핵심
  │ 메인 액터       │ AActor   │  ← 게임에 존재
  │ 데이터 전달     │ 구조체   │  ← 값만 넘김
  │ 선택지 목록     │ 열거형   │  ← 고정된 옵션
  │ 물리/감지       │ 컴포넌트 │  ← 엔진 기본 제공
  └─────────────────┴──────────┘

→ "전부 UObject"는 "정책 블록을 전부 UObject로 통일"이라는 뜻.
→ 실제 프로젝트에는 4가지 형태가 다 나옴.
```

### Q: "새로운 이동 방식을 추가하려면?"

```
1. OKMovementPolicy_Zigzag.h 파일 1개 만들기
2. UOKMovementPolicy 상속받기
3. Initialize()와 Tick() 채우기
4. 끝!

기존 파일 수정: 0개
에디터에서 드롭다운에 자동으로 나타남.
```

### Q: "구조체가 적합한 경우는?"

```
"만들고 → 전달하고 → 버린다" = 구조체

예시:
  FOKSkillSpawnContext  (GA → SpawnPolicy로 전달하고 끝)
  FOKSkillSpawnResult   (SpawnPolicy → GA로 반환하고 끝)

이런 것들은 에디터에서 교체할 필요도 없고,
상태를 기억할 필요도 없고,
그냥 정보를 묶어서 전달만 하면 된다.
```

---

## 9. 최종 정리: 뭘 어떤 형태로?

| 역할 | 이름 | 형태 | 이유 (한마디) |
|------|------|------|--------------|
| 스킬 본체 | `AOKSkillObject` | AActor | 게임에서 보이고, 움직이고, 부딪히니까 |
| 조립 설명서 | `UOKSkillObjectDataAsset` | DataAsset | 에셋으로 저장해서 에디터에서 관리 |
| 스폰 블록 | `UOKSpawnPolicy_*` | UObject | 드롭다운 교체 |
| 이동 블록 | `UOKMovementPolicy_*` | UObject | 드롭다운 교체 |
| 감지 블록 | `UOKDetectionPolicy_*` | UObject | 드롭다운 교체 |
| 필터 블록 | `UOKFilterPolicy_*` | UObject | 드롭다운 교체 |
| 효과 블록 | `UOKEffectPolicy_*` | UObject | 드롭다운 교체 |
| 타이밍 블록 | `UOKTimingPolicy_*` | UObject | 드롭다운 교체 |
| 종료 블록 | `UOKExpirePolicy_*` | UObject | 드롭다운 교체 |
| 스폰 입력 정보 | `FOKSkillSpawnContext` | 구조체 | 전달만 하면 되니까 |
| 스폰 결과 정보 | `FOKSkillSpawnResult` | 구조체 | 전달만 하면 되니까 |
| 낙하 패턴 선택 | `EOKDropPattern` | 열거형 | 고정된 4개 중 택1 |
| 공전 중심 소실 | `EOKOrbitCenterLostBehavior` | 열거형 | 고정된 3개 중 택1 |
| 밀어내기 기준 | `EOKKnockbackOrigin` | 열거형 | 고정된 4개 중 택1 |
| 필터 대상 조합 | `EOKFilterTarget` | 열거형 (비트) | 체크박스 복수 선택 |
| 사인파 모드 | `EOKWaveMode` | 열거형 | 고정된 3개 중 택1 |
| 소멸 이유 | `EOKSkillExpireReason` | 열거형 | 디버깅용, 왜 사라졌는지 |
| 투사체 물리 | `UProjectileMovementComponent` | 엔진 컴포넌트 | 언리얼이 이미 만들어놓은 것 |
| 감지 범위 도형 | `USphereComponent` 등 | 엔진 컴포넌트 | 언리얼 충돌 시스템 |
| 데미지 공용 도구 | `UOKDamageHelper` | 유틸 클래스 | 여러 곳에서 같은 함수 호출 |
| 팀 태그 | `Team.Player` 등 | GameplayTag | GAS 시스템과 연동 |

---

> **한줄 결론**: 교체하는 것 = UObject, 전달하는 것 = 구조체, 고르는 것 = 열거형, 나타나는 것 = Actor.
