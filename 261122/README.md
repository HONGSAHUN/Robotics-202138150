# 261122 실습 정리

## 실습 과정 및 결과

어제 만든 `1_pubsub` 패키지에 노드 3개를 추가해서, 키보드로 W/A/S/D를 입력하면 좌표가 바뀌는 실습을 했습니다. `publisher`가 키 입력을 보내고, `processor`가 좌표를 계산해서 다시 보내고, `subscriber`가 현재 좌표를 출력합니다.

다음으로 `custom_interfaces` 패키지에 메시지와 서비스를 직접 정의하고, `2_srvmsg` 패키지에서 서버 / 클라이언트를 작성했습니다. 덧셈 서비스(`add_ints`)를 먼저 만들고, 이걸 바탕으로 현재 위치와 거리를 보내면 이동 후 위치를 돌려주는 `move_robot`을 만들었습니다.

## 파일 구조

```text
261122/
├── README.md
├── 1_pubsub/
│   ├── 1_pubsub/
│   │   ├── publisher.py            # 키 입력 → /demo_topic
│   │   ├── processor.py            # 좌표 계산 → /position_topic
│   │   └── subscriber.py           # 좌표 출력
│   ├── launch/
│   │   └── my_location.launch.py   # 세 노드 실행
│   ├── package.xml
│   └── setup.py
├── custom_interfaces/
│   ├── msg/
│   │   ├── Num.msg
│   │   └── RobotPose.msg
│   ├── srv/
│   │   ├── AddTwoInts.srv
│   │   └── MoveRobot.srv
│   ├── CMakeLists.txt
│   └── package.xml
└── 2_srvmsg/
    ├── 2_srvmsg/
    │   ├── add_ints_server.py
    │   ├── add_ints_client.py
    │   ├── move_robot_server.py
    │   └── move_robot_client.py
    ├── package.xml
    └── setup.py
```

## 실습 환경 및 의존성

- Ubuntu 24.04 (Noble)
- ROS2 Jazzy
- colcon
- rclpy, std_msgs
- custom_interfaces (ament_cmake, rosidl_default_generators)
- launch, gnome-terminal

## 구현 결과

### 1. 키 입력으로 좌표 바꾸기

```text
publisher → /demo_topic → processor → /position_topic → subscriber
```

- 시작은 (0, 0)
- W: y + 1, S: y − 1, D: x + 1, A: x − 1

#### processor.py

```python
def callback(self, msg):
    key_input = msg.data.lower()

    if key_input == 'w':
        self.y += 1
    elif key_input == 's':
        self.y -= 1
    elif key_input == 'a':
        self.x -= 1
    elif key_input == 'd':
        self.x += 1

    result = String()
    result.data = f'({self.x}, {self.y})'
    self.publisher_.publish(result)
```

`processor`는 구독도 하고 발행도 합니다. `lower()`로 대문자도 받게 했고, 다른 키가 들어오면 좌표는 그대로입니다. W를 누르면 (0, 1), 이어서 D를 누르면 (1, 1)처럼 좌표가 누적됩니다.

### 2. launch 파일

```python
publisher = ExecuteProcess(
    cmd=[
        'gnome-terminal', '--', 'bash', '-c',
        'source ~/robot_navigation/colcon_ws/install/setup.bash; '
        'ros2 run 1_pubsub publisher; '
        'exec bash'
    ],
    output='screen'
)
```

`publisher`는 키 입력을 받아야 해서 노드마다 새 터미널을 띄웠습니다. `OnProcessStart`로 publisher → processor → subscriber 순서로 실행되게 했습니다.

### 3. 커스텀 인터페이스

```text
# Num.msg
int64 my_num

# RobotPose.msg
float64 x
float64 y
float64 theta

# AddTwoInts.srv
custom_interfaces/Num a
custom_interfaces/Num b
---
custom_interfaces/Num sum

# MoveRobot.srv
RobotPose current_pose
float64 distance
---
RobotPose result_pose
bool success
```

### 4. add_ints

```python
response.sum.my_num = request.a.my_num + request.b.my_num
```

`a`, `b`, `sum`이 숫자가 아니라 `Num` 메시지라서 안쪽 `my_num`까지 들어가야 합니다.

### 5. move_robot

현재 방향 θ로 distance만큼 직진했을 때 위치를 계산합니다.

- x' = x + d·cosθ
- y' = y + d·sinθ
- θ' = θ (회전은 안 함)

```python
response.result_pose.x = request.current_pose.x + request.distance * math.cos(request.current_pose.theta * (pi/180))
response.result_pose.y = request.current_pose.y + request.distance * math.sin(request.current_pose.theta * (pi/180))
response.result_pose.theta = request.current_pose.theta
response.success = True
```

이동량만 돌려주면 안 되고 현재 x, y에 더해야 해서 클라이언트가 현재 위치를 같이 보냅니다. 강사님은 라디안을 기본으로 쓴다고 하셨는데, 입력은 도가 편해서 도로 받고 서버에서 라디안으로 바꿨습니다.

| 입력 (x y θ d) | 결과 (x', y') |
|---|---|
| 0 0 30 1 | (0.866, 0.5) |
| 0 0 90 1 | (0, 1) |
| 1 2 0 3 | (4, 2) |

90도를 넣으면 x가 `6.12e-17`처럼 나오는데, 0에 아주 가까운 값이고 실수 계산 오차입니다. `:.2f`로 출력하면 `0.00`으로 나옵니다.

## 실행 방법

### 빌드

1. 세 폴더를 `colcon_ws/src/` 안에 넣습니다.
2. `custom_interfaces`를 먼저 빌드하고, 나머지를 빌드합니다.

```bash
cd ~/robot_navigation/colcon_ws
colcon build --packages-select custom_interfaces
colcon build --packages-select 1_pubsub 2_srvmsg
source install/setup.bash
```

### launch

1. 아래 명령을 실행하면 터미널 3개가 열립니다.

```bash
ros2 launch 1_pubsub my_location.launch.py
```

2. publisher 터미널에서 `w`, `a`, `s`, `d`를 입력하고 Enter를 누릅니다.
3. subscriber 터미널에서 좌표가 바뀌는지 확인합니다.

launch 파일 안에 경로가 `~/robot_navigation/colcon_ws`로 들어가 있어서, 경로가 다르면 바꿔야 합니다.

### add_ints

```bash
ros2 run 2_srvmsg add_ints_server
ros2 run 2_srvmsg add_ints_client 3 5
```

서버를 먼저 켜고, 다른 터미널에서 클라이언트에 숫자 2개를 넣습니다.

### move_robot

```bash
ros2 run 2_srvmsg move_robot_server
ros2 run 2_srvmsg move_robot_client 0 0 30 1
```

클라이언트에는 `x y theta(도) distance` 순서로 숫자 4개를 넣습니다.

## 배운 이론 정리

### 노드 역할 나누기

- 입력(publisher) / 계산(processor) / 출력(subscriber)
- Python 안에서 함수로 넘기는 게 아니라 **노드끼리 메시지로** 주고받기
- 노드 하나가 Publisher이면서 Subscriber일 수도 있음
- 같은 주제 실행 파일은 새 패키지 말고 기존 패키지에 추가
- **로그 중요!** 로그만 보고 무슨 일이 있었는지 알 수 있어야 디버깅 가능
  - 의미 없는 로그는 감점 요소

#### 타이머 vs input()

- 타이머: 정해진 주기마다 콜백 실행
- `input()`: Enter 칠 때까지 그 줄에서 멈춤
- 어제 talker 코드를 그대로 쓰면 1초마다가 아니라 입력할 때마다 발행됨

#### 문자열 vs String 메시지

- 키보드로 받은 건 Python 문자열 → 바로 publish 못 함
- `msg = String()` → `msg.data = 입력값` → `publish(msg)`
- `String`과 `data`는 메시지 정의에 있는 타입, 필드

#### 수정이 반영 안 될 때

- 저장했는지
- 실제로 colcon이 빌드하는 파일을 고쳤는지
- 루트에서 빌드했는지, 빌드 성공했는지
- `source install/setup.bash` 했는지
- `ros2 run`은 `install/`에 있는 걸 실행 → 고치면 다시 빌드

### Launch 파일

- 여러 노드를 명령 하나로 실행
- `launch/` 폴더는 **setup.py 있는 바깥 폴더**에
- 파일 이름은 `~.launch.py`
- `generate_launch_description()` → `LaunchDescription` 반환
  - 함수 이름 바꾸면 안 됨
- `ExecuteProcess`: bash 명령 실행 → 그 안에서 `ros2 run`
- `OnProcessStart`: 앞 프로세스가 시작되면 다음 실행
  - 프로세스 시작 ≠ 노드 준비 완료

#### setup.py data_files

```python
('share/' + package_name + '/launch', ['launch/my_location.launch.py'])
```

- (설치 위치, 파일 목록)
- 소스 폴더에 있는 것만으로는 못 찾음 → 설치 결과에 들어가야 `ros2 launch`로 실행 가능
- `console_scripts` = 실행 파일 등록, `data_files` = 자료 파일 등록

#### run vs launch

- `ros2 run 패키지 실행이름` → setup.py에 등록한 이름
- `ros2 launch 패키지 파일이름` → launch 파일 이름
- 출력이 두 번씩 나오면 전에 켜 둔 노드가 남아 있는지 확인

### Service

- **Client**가 요청 → **Server**가 처리해서 응답
- 토픽은 계속 흘려보내고, 서비스는 요청 한 번에 응답 한 번
- e.g. 현재 위치 조회, 초기화, 다른 장치에 계산 요청
- turtlesim의 reset, kill, spawn도 서비스
- Action은 오래 걸리는 작업 + 중간 피드백 + 취소 (이번엔 구현 X)

### 커스텀 인터페이스

- 기존 String, 숫자로 표현하기 불편한 데이터를 직접 정의
- 한 번 만들면 여러 패키지에서 같이 씀
- `.msg`: `타입 이름` 한 줄씩
- `.srv`: e.g. 메뉴판. 무슨 주문을 받고 뭘 주는지
  - `---` 위는 **Request**, 아래는 **Response**
  - 실제 처리는 Python 서버에서
- 다른 메시지를 필드로 쓸 수 있음
  - `custom_interfaces/Num a`, `RobotPose current_pose`
- 파일 이름은 대문자로 시작

#### 인터페이스 패키지

- 노드 패키지(`2_srvmsg`, ament_python)랑 인터페이스 패키지(`custom_interfaces`)를 따로 만듦
- 인터페이스 패키지는 **ament_cmake**
  - ament_python으로 만들면 CMakeLists.txt가 없어서 인터페이스가 안 만들어짐
- `CMakeLists.txt`: `rosidl_generate_interfaces()`에 msg, srv 파일 등록
  - 새로 만들면 여기에 추가해야 함
- `package.xml`: rosidl_default_generators, rosidl_default_runtime
- 빌드하면 Python에서 import 가능
  - `from custom_interfaces.srv import MoveRobot`

### 서버 / 클라이언트

#### 서버

- `create_service(타입, 서비스 이름, 콜백)`
- 콜백에 `request`, `response`가 들어옴 → `response` 채워서 return
- 서버를 먼저 켜야 함

#### 클라이언트

- `create_client(타입, 서비스 이름)` → 서버랑 **타입, 이름 둘 다 같아야** 연결
- `wait_for_service()`: 서버 뜰 때까지 대기
- `call_async()`: 요청 보내고 **Future** 받음
  - 비동기라고 결과가 바로 오는 게 아님
- `spin_until_future_complete()`: 결과 올 때까지 기다림

#### 서비스 이름

- `/move_robot`: 절대 이름
- `move_robot`: 상대 이름 → 네임스페이스가 앞에 붙음

#### sys.argv

- `ros2 run 2_srvmsg move_robot_client 0 0 30 1`
- `sys.argv[0]`은 실행 파일 이름 → 입력값은 `[1]`부터
- 전부 문자열이라 `int()`, `float()`로 변환
- 숫자를 안 넣으면 `IndexError` → ROS 문제가 아니라 없는 값을 꺼낸 것
- `float64` 필드에는 `float`만 들어감

#### 중첩 필드

- `current_pose`는 메시지 → `request.current_pose.x`
- `distance`는 그냥 float64 → `request.distance`

### 2D 이동

- Pose = 위치 (x, y) + 방향 θ
- θ 방향으로 d만큼 직진
  - x' = x + d·cosθ
  - y' = y + d·sinθ
- 삼각함수는 **라디안**
  - rad = deg × π / 180
