# 261114 실습 정리

## 실습 과정 및 결과

GPIO의 디지털 출력, 저항을 이용한 전류 제한, 공통 GND를 적용한 LED 점멸 실습입니다. Uno D7에 저항과 LED를 연결한 뒤, 핀을 출력으로 설정하고 HIGH와 LOW 사이에 각각 1초 지연을 넣었습니다. 회로와 실행 코드는 `Pasted image.png`에 저장했습니다.

코드상 점등과 소등은 각각 1초이며, 한 주기는 약 2초입니다.

## 파일 구조

```text
261114/
├── README.md
├── swanky_blad_waasa1.ino   # D7 LED 점멸 코드
└── Pasted image.png         # Tinkercad 회로 및 코드 화면
```

![LED 점멸 실습](./Pasted%20image.png)

## 실행 방법

### swanky_blad_waasa1.ino

1. Tinkercad Circuits에서 Arduino Uno R3, 브레드보드, LED와 저항을 배치합니다.
2. 이미지처럼 D7에서 저항과 LED를 거쳐 GND로 연결합니다.
3. Code → Text에서 `swanky_blad_waasa1.ino` 내용을 붙여 넣습니다.
4. Start Simulation을 누릅니다.

`setup()`에서 D7을 출력으로 설정하고, `loop()`에서 HIGH와 LOW를 1초 간격으로 반복합니다. LED는 1초 켜지고 1초 꺼지는 동작을 반복합니다.

## 배운 이론 정리

Python, C#, C++ 등 쓸 예정 → 변수 네이밍 잘 하기!

### 목차

1. 임베디드 시스템 기반 저수준 제어
2. ROS2 기초 및 고수준 판단
3. SLAM & Navigation
4. RTOS 기반 고수준 - 저수준 연동

우리는 기본적으로 ROS2 바탕으로 시뮬레이션 돌릴 예정.

### 로봇 구동 시스템

- 단순 모터 제어 X, 센서 데이터 처리, 위치 추정, 경로 계획, 모터 제어 동시 수행
- 각 기능은 요구되는 연산량과 **제어 주기**가 다르다.
  - 따라서, high-level, low-level로 역할 분리
    - **high-level**: 복잡한 연산과 로봇의 판단/명령을 담당
    - **low-level**: 실제 하드웨어를 빠르고 일정한 주기로 제어

**고수준 SBC / ROS2**

- Perception
- Planning
- Navigation

↕ **Communication**

**저수준 MCU / RTOS**

- Motor
- Encoder
- Control

#### High-Level Decision-Making

- 센서 데이터 처리 및 주변 환경 인식
- Localization / Mapping / SLAM
- Path Planning & Navigation
- 카메라, LiDAR 등 대용량 센서 데이터 처리
- ROS2 기반의 여러 노드 실행
- 높은 연산 성능과 메모리 요구

#### High-Level 하드웨어 SBC

- 젯슨, 라즈베리파이 등등
- Single Board Computer

#### Low-Level Control

- 모터 PWM 출력
  - Pulse Width Modulation
- Encoder 데이터 처리
- PID Control
- 각종 센서 데이터 수집 및 전처리 (IMU, Ultrasonic 등)
  - Inertial Measurement Unit
- GPIO, ADC/DAC, UART, I2C, SPI 등 직접적인 하드웨어 제어
- **짧고 일정한 주기의 제어 수행**

#### Low-Level 하드웨어 MCU

- 아두이노 보드에 쓰이는 MCU, ESP32 등
  - 수업에서 쓰는 ESP32는 와이파이, 블루투스 같은 **무선 통신 가능**
  - ESP32 - R32 → 보드 이름과 핀맵 한 번 확인해보기
- Microcontroller Unit (그냥 마이크로컨트롤러라고 부른다)
- e.g. 근육 쪽에서 즉각적인 반응을 담당하는 것에 비유

#### High-Level / Low-Level 분리 이유

- High-level은 많은 연산량 필요
- Low-level은 빠르고 일정한 제어 주기가 중요
- SBC에서는 여러 프로세스가 동시에 실행되어 제어 주기가 변할 수 있음
- SBC는 Linux 기반으로 다양한 소프트웨어 및 고성능 연산 환경을 제공
- MCU + RTOS는 모터 제어와 같은 주기적 작업을 안정적으로 수행하도록 구성 가능
- 역할을 분리하면 각 하드웨어의 장점을 활용할 수 있음

#### High-Level / Low-Level 비교 정리

| 구분 | High-Level | Low-Level |
|---|---|---|
| 주요 역할 | 판단 / 계획 | 직접 제어 |
| 연산량 | 높음 | 상대적으로 낮음 |
| 중요 요소 | 처리 성능 | 실시간성 |
| Hardware | SBC | MCU |
| OS | **Linux** | **RTOS (개발 PC에서는 Linux 사용)** |
| Software | ROS2 | Firmware / micro-ROS |
| 주요 개발 언어 | **Python** / C++ **(다른 언어도 사용 가능)** | **C / C++ (하드웨어 제어에 주로 사용)** |

### Linux

- **높은 개발 자유도**
  - 오픈소스 기반으로 시스템 내부 구조 및 커널 수준까지 설정 가능
- **강력한 패키지 관리 - apt**
  - 소프트웨어 설치, 의존성 관리 및 업데이트 용이
- ROS2 중심의 개발 생태계
  - 주요 로봇 소프트웨어, 라이브러리 및 개발 문서가 Ubuntu 환경을 중심으로 제공
- 광범위한 활용 분야
  - 서버, 클라우드, 임베디드 시스템 등 현대 IT 인프라 전반을 뒷받침
  - 안드로이드는 Linux 커널 기반, GitHub 같은 서비스의 서버 인프라에도 Linux 활용

### Linux CLI

#### GUI vs CLI

**CLI**: 사용자가 명령어(Command)를 입력하여 컴퓨터와 상호작용하는 방식

- GUI - Graphical User Interface
  - 아이콘, 창, 마우스로 조작
- CLI - Command Line Interface
  - 명령어 입력으로 조작
  - GUI로 할 수 없는 게 너무 많기 때문에 사용.

#### 터미널 프롬프트의 이해

**`user_name@host_name:~/parent_directory/current_directory$`**

- **user_name**: 현재 로그인한 사용자 이름
- **host_name**: 현재 접속한 컴퓨터(호스트) 이름
- @: 사용자와 컴퓨터를 구분하는 표시
- **~**: **사용자의 home 디렉터리(폴더)**
  - **`~` = `$HOME`**, Ubuntu 일반 사용자라면 보통 `/home/사용자이름`
- /: 폴더와 폴더를 구분하는 표시
  - **맨 처음이 /로 시작하면 절대경로**, 현재 디렉터리에서 시작하면 상대경로
  - **`~/…`는 홈 기준 경로** → 셸에서 홈의 절대경로로 확장됨
- parent_directory: 상위(부모) 디렉터리
- current_directory: 현재 디렉터리

#### 기초 중요 명령어

- **실행 권한: `chmod +x`**
  - `#!/usr/bin/python3`
  - 어떤 파이썬 인터프리터를 사용해서 실행할 거냐?
- **`echo $USER`**
  - $: 변수 값을 꺼내는 기호
  - USER: 환경변수
- **`sudo apt update`**
  - **새로운 버전이 나왔는지 확인할 수 있도록 패키지 목록 갱신**
  - 패키지 목록(인덱스)을 갱신하고, 설치는 하지 않는다.
- **`sudo apt upgrade`**
  - **설치된 패키지를 이용 가능한 새 버전으로 업그레이드**

### MCU

- 특정 제어 작업을 수행하는 소형 컴퓨팅 장치
- CPU, Memory, 주변장치 등을 하나로 통합

#### ESP32

- 저비용, 저전력 MCU 제품군
- 수업에서 사용하는 계열은 Wi-Fi, Bluetooth 무선 통신 지원
- GPIO, PWM, ADC, UART, I2C, SPI 등 지원
- **ESP32 칩 - SoC**: CPU, 메모리 및 주변장치 등을 통합한 반도체
- **ESP32 모듈**: ESP32 칩과 Flash, 무선 통신 관련 부품 등을 통합
- **ESP32 개발 보드**: 모듈에 전원 회로, USB 통신 회로, 핀 헤더 등을 추가한 보드

### 임베디드 시스템

- 특정 기능을 수행하도록 설계된 시스템
- 수업에서는 MCU 기반으로 동작
- 센서 및 액추에이터와 직접 연결하여 물리적 환경과 상호작용
- MCU 기반 시스템은 보통 제한된 컴퓨팅 자원 (CPU, Memory 등)을 사용
- 실시간 처리 및 제어에 적합하도록 설계

### 임베디드 시스템 - 기초 용어

| 주제 | 핵심 용어 |
|---|---|
| 전기적 기초 | Voltage, Current, VCC, GND |
| 신호의 종류 | Analog / Digital |
| 디지털 신호 해석 | HIGH / LOW, Logic Level (3.3V / 5V) |
| MCU 입출력 | GPIO, Input / Output |
| 디지털 출력 응용 | PWM, Duty Cycle, Frequency |
| 아날로그 입출력 | ADC / DAC |
| 외부 장치 통신 | UART, I2C, SPI |

#### Input / Output

- 전기는 왜 들어와야 하나?
  - 센서 정보가 ESP의 input pin으로 들어온다.
  - output pin은 모터 드라이버, LED 등등에 제어 신호를 보낸다.

#### High / Low

- 디지털 신호의 0과 1에 해당하는 논리 상태
- Low는 GND 부근, High는 해당 보드의 논리 전압 부근
  - 입력은 정해진 전압 범위로 HIGH / LOW를 판정한다.
  - **수업의 ESP32는 3.3V, Arduino Uno R3는 5V 논리 환경**

#### Digital / Analog

- Digital: 이산적인 현상
- Analog: 연속적인 현상
- **ADC**를 통해 아날로그 센서의 전압을 ESP가 처리할 수 있는 숫자로 변환한다.
  - **8비트: 0–255 / Uno ADC는 10비트: 0–1023 / ESP32 기본 ADC 반환값은 12비트: 0–4095**
  - 실제 측정 전압 범위는 보드와 ADC 설정에 따라 다름.
- DAC: 디지털 값을 아날로그 출력으로 변환

#### 전기적 기초

- 전압은 두 지점의 전위차, 전류는 전하의 흐름 → 저항에서는 **V = IR**
- LED는 극성이 있고, 직렬 저항으로 전류 제한
- **Common Ground**: MCU와 센서·구동부가 같은 기준으로 신호를 읽도록 GND 연결
- 브레드보드: 내부에서 연결된 구멍을 이용해 납땜 없이 회로 구성
