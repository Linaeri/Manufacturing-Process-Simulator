# 생산 패널 계산 공식 / Production Panel Formulas

이 문서는 프로그램에 구현된 생산·마모·품질·재고·에너지·비용 계산을 정리합니다. 수치와 공식은 프로그램용 단순 모델이며, 실제 제조 설비의 검증된 물리 모델은 아닙니다.

This document describes the production, wear, quality, inventory, energy and cost calculations implemented in the program. These are simplified program models, not validated physical models of manufacturing equipment.

---

## 1. 기본 설정 / Default Settings

| 항목 / Item | 기본값 및 범위 / Default and Range |
|---|---|
| 기계 수 / Machines | 3 |
| 초기 원자재 / Initial stock | 150장 / sheets |
| 양품 생산 목표 / Good-product target | 500개 / units |
| 초기 상태 / Initial state | Pause, 모든 기계 활성화 / Paused, all machines enabled |
| 전체 배율 `G` / Global multiplier | 기본 1, 범위 0.25–4, 변경 단위 0.25 / Default 1, range 0.25–4, step 0.25 |
| 개별 속도 `v` / Machine speed | 기본 1, 범위 0.5–2, 변경 단위 0.25 / Default 1, range 0.5–2, step 0.25 |
| 프레스 기본 주기 `D` / Stamping cycle | 2초 / seconds |
| 성형 기본 주기 `D` / Forming cycle | 3초 / seconds |

전체 배율 `G`는 내부 시간의 진행 속도를 조절합니다. 개별 속도 `v`는 해당 기계의 생산 속도, 불량 확률 및 전력에 영향을 줍니다.

The global multiplier `G` controls internal time progression. Machine speed `v` affects the individual machine’s throughput, defect probability and power.

## 2. 생산 시간과 생산량 / Production Time and Throughput

```text
내부 시간 증가 = 실제 경과 시간 × G
Internal time increment = real elapsed time × G

작업 진행률 증가 = 실제 경과 시간 × G × v / D
Progress increment = real elapsed time × G × v / D

제품당 소요 시간 ≈ D / (G × v)초
Time per product ≈ D / (G × v) seconds

기계당 이론 생산량 = 60 × G × v / D개/분
Theoretical machine throughput = 60 × G × v / D units/minute

전체 이론 생산량 = 사용 가능한 기계의 생산량 합계
Total theoretical throughput = sum of eligible machine throughput
```

- 사용 가능한 기계는 활성화 상태이고 마모가 200 미만인 기계입니다.
- 표시되는 생산능력은 설정 기반 이론값이며 Pause 또는 재고 부족을 반영한 실제 실적이 아닙니다.
- 프레스 3대가 모두 `G = 1`, `v = 1`이면 기계당 30개/분, 전체 90개/분입니다.

- Eligible machines are enabled and have wear below 200.
- Displayed capacity is theoretical; it does not represent actual output during Pause or stock shortages.
- Three stamping machines at `G = 1`, `v = 1` provide 30 units/minute each, or 90 units/minute combined.

## 3. 마모와 정비 / Wear and Maintenance

```text
제품 1개 완료 → 마모 +1, 작업 횟수 +1
Each completed product → wear +1, cycle count +1

마모 ≥ 150 → 정비 알림
Wear ≥ 150 → maintenance warning

마모 ≥ 200 → 해당 기계 정지
Wear ≥ 200 → machine stops

Service → 해당 기계 마모 = 0
Service → machine wear = 0
```

속도가 높아지면 같은 시간에 더 많은 제품을 생산하므로 마모가 더 빨리 쌓입니다. 단, **제품 1개당 마모 증가량은 속도와 관계없이 +1**입니다.

Service는 마모만 초기화하며 작업 횟수, 생산 기록 및 진행 중인 작업은 유지합니다. 정비 비용과 대기 시간은 없습니다.

Higher speed produces more items per minute, causing faster wear accumulation over time. However, **wear per product remains +1 at every speed**.

Service resets only wear. Cycle counts, production records and work in progress are preserved. Maintenance has no cost or waiting period.

## 4. 불량 확률 / Defect Probability

제품 완성 후 마모를 먼저 1 증가시킨 다음, 해당 마모값으로 불량 확률을 계산합니다.

Wear increases by 1 at completion before the defect probability is calculated.

```text
p = 0.025 + max(0, v − 1) × 0.15 + wear × 0.0003

불량 확률(%) = 2.5 + max(0, v − 1) × 15 + wear × 0.03
Defect probability (%) = 2.5 + max(0, v − 1) × 15 + wear × 0.03
```

| 개별 속도 / Speed | 완료 후 마모 / Wear After Completion | 불량 확률 / Defect Probability |
|---|---:|---:|
| 0.5× | 100 | 5.5% |
| 1× | 100 | 5.5% |
| 1.5× | 100 | 13.0% |
| 2× | 100 | 20.5% |
| 2× | 200 | 23.5% |

- 속도 1배 이하에서는 속도로 인한 불량 가산점이 없습니다.
- 전체 배율 `G`는 불량 확률에 직접 영향을 주지 않습니다.
- 실제 불량 판정에는 난수를 사용하므로 관측된 불량률은 계산 확률과 다를 수 있습니다.

- Speeds at or below 1× receive no additional speed penalty.
- The global multiplier `G` does not directly affect defect probability.
- Outcomes use random values, so observed defect rates can differ from the calculated probability.

### 난수 판정 / Random Decision

```text
초기 난수 상태 / Initial random state = 42

다음 상태 = 이전 상태 × 1664525 + 1013904223
Next state = previous state × 1664525 + 1013904223

불량 조건: (상태 % 10000) < p × 10000
Defective when: (state % 10000) < p × 10000
```

난수 상태는 C의 `unsigned` 정수 범위에서 순환합니다. 초기 상태와 작업 완료 순서가 같으면 같은 판정 순서가 나옵니다.

The state wraps within the C `unsigned` integer range. Identical initial conditions and completion order produce the same outcome sequence.

## 5. 전력과 에너지 / Power and Energy

```text
작업 중 전력 = 4 + 4 × v² kW
Working power = 4 + 4 × v² kW

에너지 증가량 = 실제 경과 시간(초) × G × (4 + 4 × v²) / 3600 kWh
Energy increment = real elapsed seconds × G × (4 + 4 × v²) / 3600 kWh

전체 에너지 = 모든 기계의 작업 중 에너지 누적 합계
Total energy = accumulated working energy across all machines
```

| 개별 속도 / Speed | 작업 중 전력 / Working Power |
|---|---:|
| 0.5× | 5 kW |
| 1× | 8 kW |
| 1.5× | 13 kW |
| 2× | 20 kW |

속도를 1배에서 2배로 높이면 생산 속도는 2배, 작업 중 전력은 2.5배가 됩니다. Pause, 개별 Stop, 고장 정지 및 원자재 대기 중에는 에너지를 누적하지 않습니다.

Increasing speed from 1× to 2× doubles throughput and multiplies working power by 2.5. No energy accumulates during Pause, individual Stop, fault shutdown or material waiting.

## 6. 재고와 잔여 시간 / Inventory and Stock Duration

```text
새 작업 시작 → 재고 −1장
New job starts → stock −1 sheet

소비 원자재 = 누적 공급량 − 현재 재고
Consumed sheets = total supplied − current stock

Restock → 현재 재고 +100, 누적 공급량 +100
Restock → stock +100, total supplied +100

예상 재고 잔여 시간(분) = 현재 재고 / 전체 이론 생산량
Estimated stock duration (minutes) = stock / total theoretical throughput

재고 비율(%) = 현재 재고 / 누적 공급량 × 100
Stock percentage (%) = stock / total supplied × 100
```

- 원자재는 완료 시점이 아닌 작업 시작 시점에 차감합니다.
- 양품과 불량품 모두 원자재를 소비합니다.
- 재고가 0이어도 진행 중인 작업은 완료합니다.
- 생산능력이 0이면 `No active machines`를 표시합니다.
- 재입고 후 활성화된 기계는 Play 상태에서 자동으로 작업을 재개합니다.

- Material is deducted when work starts, rather than when it finishes.
- Both good and defective products consume material.
- Existing jobs can finish after stock reaches zero.
- Zero configured capacity displays `No active machines`.
- Enabled machines resume automatically after restocking while Play is active.

## 7. 비용·매출·이익 / Costs, Revenue and Profit

```text
재료비 = 소비 원자재 × €2.50
Material cost = consumed sheets × €2.50

전기료 = 누적 에너지(kWh) × €0.22
Electricity cost = accumulated energy (kWh) × €0.22

총비용 = 재료비 + 전기료
Total cost = material cost + electricity cost

프레스 매출 = 양품 수 × €7.50
Stamping revenue = good products × €7.50

성형 매출 = 양품 수 × €9.00
Forming revenue = good products × €9.00

예상 이익 = 매출 − 총비용
Estimated profit = revenue − total cost
```

불량품은 매출을 발생시키지 않습니다. 재입고 자체에는 비용을 부과하지 않고 원자재가 작업에 투입될 때 재료비를 반영합니다. 인건비, 정비비, 간접비 및 폐기 비용은 제외합니다.

Defective products generate no revenue. Restocking has no immediate charge; material cost is recognized when sheets enter production. Labor, maintenance, overhead and disposal costs are excluded.

## 8. 생산 지표와 목표 / Production Metrics and Targets

```text
총생산량 = 양품 수 + 불량 수
Total produced = good products + defective products

성공률(%) = 양품 수 / 총생산량 × 100
Success rate (%) = good products / total produced × 100

실제 불량률(%) = 불량 수 / 총생산량 × 100
Observed defect rate (%) = defective products / total produced × 100

목표 달성률(%) = 양품 수 / 생산 목표 × 100
Target completion (%) = good products / target × 100

목표 달성 조건: 양품 수 ≥ 목표
Target achieved when: good products ≥ target
```

- 생산량이 0이면 성공률은 0%입니다.
- 목표는 100개 단위로 조정하며 최솟값은 100개입니다.
- 목표 달성 후에도 생산은 계속됩니다.
- 달성률 숫자는 100%를 넘을 수 있지만 진행 막대는 100%까지만 표시합니다.

- Success rate is 0% before any products are completed.
- Targets change in steps of 100, with a minimum of 100.
- Production continues after reaching the target.
- Completion percentage can exceed 100%; the progress bar is capped at 100%.

## 9. 알림 조건 / Alert Conditions

| 알림 / Alert | 조건 / Condition |
|---|---|
| 원자재 소진 / Out of materials | 재고 = 0 / Stock = 0 |
| 원자재 부족 / Low stock | 재고 1–30장 / Stock = 1–30 sheets |
| 정비 필요 / Maintenance required | 한 대라도 마모 ≥ 150 / Any machine wear ≥ 150 |
| 기계 정지 / Machine stopped | 한 대라도 Stop 또는 마모 ≥ 200 / Any machine disabled or wear ≥ 200 |
| 높은 불량률 / High defect rate | 총생산량 ≥ 10, 실제 불량률 > 12% / Total produced ≥ 10 and observed defect rate > 12% |
| 높은 에너지 사용 / High energy use | Play 상태, 한 대라도 속도 > 1.5배 / Play active and any machine speed > 1.5× |

높은 에너지 알림은 실제 소비 전력 대신 속도 설정을 검사합니다. 따라서 Stop 상태인 기계도 속도가 기준을 넘으면 알림 조건에 포함될 수 있습니다.

The energy warning checks configured speed rather than actual power consumption. A stopped machine can therefore satisfy the warning condition.

## 10. 제어와 초기화 / Controls and Reset

| 동작 / Action | 결과 / Result |
|---|---|
| Play | 생산·시간·에너지 갱신 시작 / Starts production, time and energy updates |
| Pause | 갱신 중지, 작업 상태 보존 / Freezes updates and preserves work |
| 기계 Stop / Machine Stop | 해당 기계만 정지, 진행 상태 보존 / Stops that machine and preserves progress |
| 기계 Start / Machine Start | 보존한 작업 재개 / Resumes preserved work |
| Service | 해당 기계 마모만 0으로 초기화 / Resets only that machine’s wear |
| Reset | 생산 상태를 초기 기본값으로 복원 / Restores production defaults |

Reset은 재고 150장, 목표 500개, 모든 속도 1배, 모든 기계 활성화, Pause 상태로 복원합니다. 생산량, 마모, 에너지, 경과 시간 및 기록도 초기화합니다.

Reset restores 150 sheets, a 500-unit target, all speeds at 1×, all machines enabled and the paused state. Output, wear, energy, elapsed time and records are cleared.

공정 변경은 Pause 상태이며 생산량이 0이고 원자재가 사용되지 않았을 때만 가능합니다.

Process changes are allowed only while paused, with zero completed products and no consumed material.

## 11. 계산 및 기록 방식 / Calculation and Record Handling

- 한 프레임의 경과 시간을 최대 0.02초씩 나누어 계산합니다.
- 실제 완료 시점은 시간 단위 분할 때문에 이론 소요 시간과 조금 차이 날 수 있습니다.
- 최근 1,000개 제품 기록을 보관하고, 이후에는 오래된 기록부터 덮어씁니다.
- 누적 생산량은 기록 보관 한도와 관계없이 유지됩니다.
- CSV에는 현재 누적 지표와 보관 중인 최근 제품 기록을 저장합니다.
- 생산 계산은 3D 화면과 독립적이며, 줌·회전 기능을 제거해도 계속 작동합니다.

- Frame time is divided into calculation slices of at most 0.02 seconds.
- Completion times may differ slightly from theoretical durations because of these discrete steps.
- The latest 1,000 product records are retained; older records are overwritten.
- Cumulative production totals remain independent of the record limit.
- CSV export contains current cumulative metrics and retained product records.
- Production calculations are independent of the 3D view and continue to work without zoom or rotation.
