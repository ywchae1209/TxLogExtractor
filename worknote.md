1. redo block iterator
   * fixed sized block iterator
   * decode: file-head (1st block)
   * decode: redo-head (2nd block)
   * decode: block-head (2nd block ~)
   * decode: basic endian

2. redo record iterator
    * decode: start-offset ~> record-offsets
      * 1:N = 1-block n-record
      * N:1 = multi-block record
      
   ```
   [ CAVEAT ]
   
   1. block' offset(start-record-offset) may not correct.
   2. validate with VLD is not effetive.
   
   * some assumption.
       - record header's 16byte is not splited over block.
       - record size < 32MB 
   ```
   
## Note 레코드 좌표찾기가 어려운 이유

### 원인 1)

블록의 offset은 그 지점부터 시작하는 Record를 알려 주지만,

- 시작하는 Record가 없는 경우, 블록의 offset은 갱신하지 않음.
- 따라서 `블록 offset에서 과연 유효한 Record가 시작하는 건지 불확실`


### 원인 2)

멀티블록 레코드

- 시작 블록내 마지막 레코드부터 여러 블록에 걸친 레코드 (A라 하자)
- 중간의 블록들에 offset이 있어서 그로부터 시작하는 레코드(B라 하자)가 있는 경우,

- `레코드A, 레코드B의 선택문제` 발생.

### 원인 3)

멀티레코드 블록 (한 블록에 여러 Record존재)

- 블록내의 `두번째 이후 Record의 유효성 판정이 불확실함.`
- 블록내 마지막 Record가 여러 블록에 걸친 Record의 시작인 경우,
  문제원인 2)와 복합상황.

### 원인 4)

void 레코드 ( 오라클이 void로 선언한 쓰레기블록 )

- void 레코드를 무시하면, `유효한 Record를  누락했을 지 파악하기 어려움.`

### 원인 5)

Log Writing Group 정보의 불확실

- LWN 정보에 쓰여진 레코드의 low-scn과 next-scn이 있음.
- LWN의 scn 범위는 레코드 유효성 검증에 가장 강력한 실마리.

- 그러나, `LWN scn범위를 초과한 Record들`이 상당수 존재.
  ( 초과범위는 random : 1900 정도의 오차도 발견됨. )

