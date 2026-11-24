# D3D12 Renderer 포트폴리오 계획 초안

C++과 HLSL로 Windows용 Direct3D 12 렌더러를 구현한다. 직접 만든 기둥과 바닥으로 테스트 장면을 구성하고, 카메라로 둘러보며 조명과 그림자를 확인할 수 있게 한다. 동일한 장면에서 프레임 동기화 방식과 그리기 방식을 바꿔 CPU 대기 시간과 명령 기록 시간이 어떻게 달라지는지 측정한다.

펄어비스 게임 엔진 프로그래머 지원을 위한 포트폴리오다. 2026년 11월 24일부터 하루 6시간을 확보한다. **12월 18일 제출을 목표로 150시간을 배정하고, 12월 19~20일의 12시간은 예비 시간으로 둔다.** 12월 20일은 개인 목표일이다. 공식 공고는 채용 완료 시 마감하는 상시채용이며 관련 경력 2년 이상을 필수로 명시한다. [펄어비스 공식 채용 공고](https://www.pearlabyss.com/ko-KR/Company/Careers/detail?_jobOpeningNo=6)

## 개발 방식과 코드 구조

저장소 README의 원칙대로 렌더러의 설계, 구현, 디버깅은 작성자가 직접 한다. AI는 개념 설명, 코드 읽기, 디버깅 질문과 리뷰에 활용한다. Microsoft 샘플에서 API 호출 순서를 학습하고, 새 솔루션에서 디바이스·큐·스왑 체인·RTV를 직접 구성한다. Win32 창 생성이나 메시지 처리 등 참고 코드의 일부를 가져올 경우 저작권 표시와 MIT 라이선스를 유지하고 README에 재사용 범위를 적는다. [Microsoft DirectX Graphics Samples](https://github.com/microsoft/DirectX-Graphics-Samples)

| 구성 | 책임 |
|---|---|
| `Win32Application` | 창 생성, 메시지 루프, 입력과 시간 처리. `HWND`를 렌더러 초기화에 전달한다. |
| `D3D12Renderer` | 디바이스, 큐, 스왑 체인과 렌더링 리소스를 관리한다. `Initialize`, `Render`, `Shutdown`을 통해 프레임을 기록하고 제출한다. |
| `Scene` | 카메라, 방향광, 기둥의 위치·회전·색상 등 CPU 쪽 장면 데이터를 관리한다. |
| `FrameResource` | 프레임별 Command Allocator, 상수 및 인스턴스 데이터 영역, 마지막 제출의 Fence 값을 보관한다. 렌더러 내부 구조체로 시작한다. |

창과 렌더러만으로 배경색을 표시하는 것이 첫 단계다. 카메라와 여러 물체가 생길 때 `Scene`을 추가하고, 여러 프레임을 처리할 때 `FrameResource`를 도입한다. 렌더러는 해당 프레임의 Fence 완료를 확인한 뒤 Allocator를 재설정하고 GPU가 읽는 버퍼 영역을 갱신한다. [Microsoft Command Allocator 문서](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandallocator-reset)

## 샘플 학습과 구현 일정

하루 6시간은 샘플 읽기 1시간 30분, 구현 3시간 30분, 검증과 기록 1시간을 기본으로 배분한다.

| 기간 | 학습할 코드 | 구현과 완료 기준 |
|---|---|---|
| **11/24~26 · 18시간** | `HelloWindow`의 `OnRender`, `PopulateCommandList`와 `HelloTriangle`의 `LoadAssets`, `VSMain`. 명령 기록 → 큐 제출 → `Present` 흐름을 확인한다. | 새 솔루션에서 배경색, 삼각형, 인덱스 버퍼를 쓴 사각형 순서로 띄운다. |
| **11/27~30 · 24시간** | `HelloConstBuffers`에서 CBV 생성과 갱신을 읽고, `D3D12DynamicIndexing`에서 깊이 버퍼 생성과 바인딩을 확인한다. | 월드·뷰·투영 변환을 사용해 바닥과 기둥을 배치한다. 카메라 조작, 깊이 테스트, 방향광을 적용하고 그리기 순서를 바꿔도 앞쪽 물체가 뒤쪽 물체를 가리는지 확인한다. |
| **12/1~4 · 24시간** | `HelloTriangle::WaitForPreviousFrame`과 `HelloFrameBuffering::MoveToNextFrame`을 비교한다. | 프레임 리소스 두 개를 두고 매 프레임 완료를 기다리는 방식과 재사용 시 대기하는 방식을 구현한다. 두 모드의 화면이 같고 Debug Layer에서 해당 오류가 발생하지 않는지 확인한다. |
| **12/5~8 · 24시간** | `D3D12DynamicIndexing`의 반복 draw와 `DrawIndexedInstanced`의 `InstanceCount`, `StartInstanceLocation`을 읽는다. | 인스턴스별 위치·색상을 버퍼에 담는다. 같은 메시, 셰이더, 인스턴스 데이터로 기둥을 한 개씩 그리는 경로와 한 번에 그리는 경로를 구현한다. 두 경로에서 같은 화면이 나오고 기둥의 draw call 수가 달라지는지 확인한다. |
| **12/9~13 · 30시간** | `D3D12Multithreading`에서 그림자 텍스처의 DSV·SRV, 상태 전환, HLSL 깊이 비교 부분을 읽는다. | 방향광 하나의 깊이 패스와 카메라 패스를 연결한다. 기둥 하나가 바닥에 그림자를 드리우면 전체 장면에 적용하고 켜기·끄기를 추가한다. |
| **12/14~15 · 12시간** | Release 빌드에서 두 성능 비교를 반복하고, 가능하면 PIX 타임라인을 확보한다. | 측정 조건, 결과, 해석을 표에 정리한다. |
| **12/16~18 · 18시간** | 제출물과 재현 절차를 정리한다. | 새 폴더에 클론해 빌드·실행하고, README·1분 데모 영상·측정 비교표를 준비해 제출한다. |
| **12/19~20 · 예비 12시간** | 빌드나 제출 링크에서 발견된 문제를 수정한다. | 최종 제출물에 접근하고 실행할 수 있는지 확인한다. |

12월 11일까지 기둥 하나의 그림자가 제대로 표시되면 전체 장면에 적용한다. 그 시점에 그림자가 불안정하면 기본 방향광 장면으로 제출 범위를 확정하고, 인스턴싱 비교와 빌드 재현성에 시간을 배정한다.

## 측정과 제출 기준

- **프레임 동기화**는 매 프레임 대기와 프레임 리소스 재사용 시 대기를 비교한다. CPU가 Fence를 기다린 시간과 프레임 시간을 기록한다.
- **인스턴싱**은 한 개씩 그리기와 한 번에 그리기를 비교한다. 기둥의 draw call 수와 CPU 명령 기록 시간을 기록한다.
- 두 실험 모두 같은 GPU, 장면, 카메라, 해상도, VSync, 그림자 설정을 사용한다. 한 번에 비교 대상 하나만 바꾸고, 워밍업 후 반복 측정한다. 성능 차이가 작으면 관측한 수치와 조건을 그대로 기록한다. [Microsoft PIX 타이밍 캡처](https://learn.microsoft.com/en-us/windows/win32/direct3dtools/pix/articles/timing-captures/pix-timing-captures)
- README에는 스크린샷, 빌드·실행 방법, 샘플 재사용 범위와 직접 구현한 부분, 프레임 동기화 흐름, 성능 비교표를 담는다. 데모 영상은 장면·그림자, draw call 변화, 측정 결과 순으로 보여준다. 블로그에는 구현 중 겪은 문제 하나를 증상 → 원인 → 수정 → 검증 순으로 정리한다.

[Direct3D 12 Hello World 샘플 안내](https://learn.microsoft.com/en-us/samples/microsoft/directx-graphics-samples/d3d12-hello-world-samples-win32/) · [DrawIndexedInstanced API 문서](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-drawindexedinstanced)
