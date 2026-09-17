6.3.1.1. KDO: IRP, ORP

|  index | element(vector)    |                                desc                                |
|:------:|--------------------|:------------------------------------------------------------------:|
| 1      | Ktudb              | Transaction 대상 지시자                                            |
| 2      | Ktubl \| Ktubu     | Transaction 조작 대상 지정                                         |
| 3      | KTB                | Transaction Block                                                  |
| 4      | KDO_IRP \| KDO_ORP | 데이터 조작 대상 지시 • op = 0x22 (IRP) • op = 0x26 (ORP)          |
| 5 ~ n  | Byte               | Insert, Overwrite의 Column data  • n = [4] KDO의 column count(cc)  |
| 6      | Supplemental       | Row의 추가 정보 • SUP의 column count(cc)에 따라 하위 Elements 구성 |
| 7      | Int16 * cc         | Supplemental column의 ID vector                                    |
| 8      | Int16 * cc         | Supplemental column의 length vector                                |
| 9 ~ cc | Byte               | Supplemental column data                                           |

6.3.1.2. KDO: URP

|  index  | element(vector)  |                                desc                                |
|:-------:|------------------|:------------------------------------------------------------------:|
| 1       | Ktudb            | Transaction 대상 지시자                                            |
| 2       | Ktubl \| Ktubu   | Trnsaction 조작 대상 지정                                          |
| 3       | KTB              | Transaction Block                                                  |
| 4       | KDO_URP          | 데이터 조작 대상 지시 • op = 0x25                                  |
| 5       | Int16 * n        | Update column의 ID vector • n = [4] KDO_URP의 nnew                 |
| 6 ~ n   | Byte             | Update column datas • n = [4] KDO_URP의 nnew                       |
| 7       | Supplemental     | Row의 추가 정보 • SUP의 column count(sc)에 따라 하위 Elements 구성 |
| 8       | Int16 * sc       | Supplemental column의 ID vector                                    |
| 9       | Int16 * sc       | Supplemental column의 length vector                                |
| 10 ~ sc | Byte             | Supplemental column data                                           |

6.3.1.3. KDO: DRP, LKR, MFC, CFA, LMN

|  index | element(vector)                                     |                                                      desc                                                      |
|:------:|-----------------------------------------------------|:--------------------------------------------------------------------------------------------------------------:|
| 1      | Ktudb                                               | Transaction 대상 지시자                                                                                        |
| 2      | Ktubl \| Ktubu                                      | Transaction 조작 대상 지정                                                                                     |
| 3      | KTB                                                 | Transaction Block                                                                                              |
| 4      | KDO_DRP \| KDO_LKR \| KDO_MFC \| KDO_CFA \| KDO_LMN | 데이터 조작 대상 지시 • op  = 0x23 (DRP), • op = 0x24 (LKR) • op = 0x27(MFC) • op = 0x28(CFA) • op = 0x30(LMN) |
| 5      | Supplemental                                        | Row의 추가 정보 • SUP의 column count(sc)에 따라 하위 Elements 구성                                             |
| 6      | Int16 * sc                                          | Supplemental column의 ID vector                                                                                |
| 7      | Int16 * sc                                          | Supplemental column의 length vector                                                                            |
| 8 ~ sc | Byte                                                | Supplemental column datas                                                                                      |


6.3.1.4. KDO: QMI

|  index  | element(vector) |                                               desc                                              |
|:-------:|-----------------|:-----------------------------------------------------------------------------------------------:|
| 1       | Ktudb           | Transaction 대상 지시자                                                                         |
| 2       | Ktubl \| Ktubu  | Trnsaction 조작 대상 지정                                                                       |
| 3       | KTB             | Transaction Block                                                                               |
| 4       | KDO_QMI         | 데이터 조작 대상 지시 • op = 0x2b • nrow = row의 개수 • KDO_QMI의 slot는 int16 * nrow 만큼 저장 |
| 5       | Int16 * nrow    | Insert rows의 length vector                                                                     |
| 6       | Byte * nrow     | Inset rows의 data vector • row internal format으로 저장                                         |
| 7       | Supplemental    | Row의 추가 정보 • SUP의 column count(sc)에 따라 하위 Elements 구성                              |
| 8       | Int16 * sc      | Supplemental column의 ID vector                                                                 |
| 9       | Int16 * sc      | Supplemental column의 length vector                                                             |
| 10 ~ sc | Byte            | Supplemental column data                                                                        |


6.3.1.5. KDO: QMD

|  index | element(vector)  |                                               desc                                               |
|:------:|------------------|:------------------------------------------------------------------------------------------------:|
| 1      | Ktudb            | Transaction 대상 지시자                                                                          |
| 2      | Ktubl \| Ktubu | Trnsaction 조작 대상 지정                                                                       |
| 3      | A_KTB            | Transaction Block                                                                                |
| 4      | KDO_QMD          | 데이터 조작 대상 지시 • op = 0x2c • nnrow = row의 개수 • KDO_QMD의 slot는 int16 * nrow 만큼 저장 |
| 5      | Supplemental     | Row의 추가 정보 • SUP의 column count(sc)에 따라 하위 Elements 구성                               |
| 6      | Int16 * sc       | Supplemental column의 ID vector                                                                  |
| 7      | Int16 * sc       | Supplemental column의 length vector                                                              |
| 8 ~ sc | Byte             | Supplemental column datas                                                                        |
