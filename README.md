# OKGame

Unreal Engine 5.7 project targeting Windows. The core gameplay code focuses on agent steering and local avoidance using a spatial hash grid.

## 주요 기능
- `AOKAgent` 기반 에이전트 이동: 목표 추종 + 분리(Separation) + 예측 회피(Predictive Avoidance) 조합
- `UOKAgentManager` 월드 서브시스템: 에이전트 등록/해제, 공간 해시 그리드 검색
- 기본 맵: `/Game/Map/LV-Test`

## 코드 구조
- `Source/OKGame/Agent/OKAgent.*`
  - 에이전트 이동 및 회피 로직
  - `SetMoveTarget`, `SetDesiredSpeed`로 이동 목표/속도 설정
  - 디버그 드로잉(`bDrawDebug`)
- `Source/OKGame/Agent/OKAgentManager.*`
  - `FAgentHandle`로 에이전트 관리
  - `THierarchicalHashGrid2D` 기반 인접 에이전트 검색
  - `FindCloseAgentRange`로 탐색 범위 내 에이전트 목록 반환

## 동작 개요
1. `AOKAgent::BeginPlay`에서 `UOKAgentManager`에 등록
2. `Tick`에서 인접 에이전트를 조회하고 조향/회피력을 합산
3. 속도/가속도 클램프 후 위치 및 회전 업데이트
4. 이동 업데이트를 `UOKAgentManager::AgentMoveUpdate`에 전달

## 설정 참고
- 렌더러 설정: Ray Tracing, Substrate 활성화 (`Config/DefaultEngine.ini`)
- 입력: Enhanced Input 사용 (`Config/DefaultInput.ini`)

## 실행 방법
1. Unreal Editor에서 `OKGame.uproject` 열기
2. `LV-Test` 맵을 열고 플레이 실행

## 개발 메모
- 환경 회피(`CalculateEnvironmentAvoidanceForce`)는 틀만 존재하며 레이캐스트/오버랩 기반 구현이 필요합니다.
- `UOKAgentManager`의 `Tick`은 구현되어 있지만 `IsTickable()`이 `false`이므로 디버그 그리드 드로잉이 필요하면 활성화해야 합니다.
