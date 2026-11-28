/*
 * Project01 - LiDAR 기반 장애물 회피 주행
 *
 * 보드  : ESP32 (D1 R32 계열)
 * 센서  : LiDAR (UART, 230400 baud, 8N1, 47바이트 패킷) -> 261121 lidar_test.ino와 같은 방식
 * 구동  : DC 모터 4개 + 모터 드라이버 (IN1/IN2)       -> 261121 motor_test.ino와 같은 핀
 *
 * 동작 요약
 *   1) LiDAR 패킷을 계속 읽어서 0~359도 거리 배열(distances)을 갱신한다.
 *   2) 50ms마다 로봇 기준 영역(정면 / 왼쪽 앞 / 오른쪽 앞 / 뒤)의 거리를 계산한다.
 *   3) 상태(FORWARD / TURN / BACKUP)에 따라 바퀴 속도를 정한다.
 *        FORWARD : 앞이 비어 있으면 직진, 대각선 앞이 가까우면 반대쪽으로 살짝 꺾음
 *        TURN    : 앞이 막히면 더 열린 쪽으로 제자리 회전, 앞이 충분히 열리면 다시 직진
 *        BACKUP  : 앞·좌·우가 모두 막혔거나 회전이 너무 오래 걸리면 잠깐 후진
 */


// ============================================================
// 1. 핀 설정 (motor_test.ino와 동일)
// ============================================================

// Front Right
#define FR_IN1 26
#define FR_IN2 25

// Front Left
#define FL_IN1 19
#define FL_IN2 27

// Rear Right
#define RR_IN1 23
#define RR_IN2 12

// Rear Left
#define RL_IN1 0
#define RL_IN2 15

// LiDAR -> ESP32 RX
#define LIDAR_RX 16


// ============================================================
// 2. 튜닝 파라미터 (실제 로봇에서 바꿔 가며 맞추는 값)
// ============================================================

// 바퀴마다 배선/장착 방향이 달라서 "IN1=HIGH"가 전진이 아닐 수 있다.
// MOTOR_CHECK 모드로 확인한 뒤, 거꾸로 도는 바퀴만 true로 바꾼다.
const bool FR_REVERSED = false;
const bool FL_REVERSED = false;
const bool RR_REVERSED = false;
const bool RL_REVERSED = false;

// 1로 바꾸면 setup()에서 전진 -> 좌회전 -> 우회전 -> 후진을 한 번씩 해 보고 멈춘다.
#define MOTOR_CHECK 0

// LiDAR 장착 방향: 샘플 코드 기준 Back 0°, Left 90°, Front 180°, Right 270°
const int FRONT_DEG = 180;

// 거리 기준 [mm]
const int STOP_DIST    = 300;  // 정면이 이보다 가까우면 멈추고 회전 시작
const int CLEAR_DIST   = 400;  // 회전 중 정면이 이보다 멀어지면 다시 직진 (STOP_DIST보다 크게 = 히스테리시스)
const int SIDE_DIST    = 250;  // 대각선 앞이 이보다 가까우면 반대쪽으로 살짝 꺾음
const int MIN_VALID_MM = 60;   // 이보다 가까운 값은 로봇 몸체/전선이 찍힌 것으로 보고 무시

// 속도 (PWM 0~255)
const int FORWARD_SPEED = 140;
const int TURN_SPEED    = 170;
const int BACKUP_SPEED  = 130;
const int VEER_DELTA    = 60;  // 살짝 꺾을 때 한쪽 바퀴를 줄이는 양

// 시간 [ms]
const unsigned long CONTROL_PERIOD_MS = 50;    // 판단 주기
const unsigned long TURN_TIMEOUT_MS   = 3000;  // 이 시간 동안 회전해도 앞이 안 열리면 후진
const unsigned long BACKUP_TIME_MS    = 500;   // 후진 시간
const unsigned long LIDAR_TIMEOUT_MS  = 500;   // 이 시간 동안 패킷이 없으면 정지
const unsigned long DEBUG_PERIOD_MS   = 200;   // 시리얼 출력 주기

const int NO_DATA = 9999;  // 영역 안에 유효한 측정값이 하나도 없을 때 돌려주는 값


// ============================================================
// 3. 전역 변수
// ============================================================

uint8_t packet[47];
uint16_t distances[360] = {0};   // LiDAR 원래 각도 기준 거리 [mm], 0 = 유효하지 않음

float previousAngle = -1.0;
bool scanReady = false;          // 한 바퀴 이상 받았는지
unsigned long lastPacketMs = 0;

enum State { FORWARD, TURN, BACKUP };
State state = FORWARD;
int turnDir = 1;                 // +1 = 왼쪽(반시계), -1 = 오른쪽(시계)
unsigned long stateStartMs = 0;

unsigned long lastControlMs = 0;
unsigned long lastDebugMs = 0;

// 디버그 출력용으로 마지막 계산값을 저장
int dbgFront, dbgFrontL, dbgFrontR, dbgLeft, dbgRight, dbgBack;


// ============================================================
// 4. 모터 제어
// ============================================================

// 바퀴 하나를 speed(-255 ~ 255)로 돌린다. 양수 = 전진, 음수 = 후진
// 속도 조절을 위해 digitalWrite 대신 analogWrite(PWM)를 쓴다.
void setWheel(int in1, int in2, int speed, bool reversed) {
    if (reversed) {
        speed = -speed;
    }
    speed = constrain(speed, -255, 255);

    if (speed > 0) {          // IN1 = PWM, IN2 = LOW  -> 한 방향
        analogWrite(in1, speed);
        analogWrite(in2, 0);
    } else if (speed < 0) {   // IN1 = LOW, IN2 = PWM  -> 반대 방향
        analogWrite(in1, 0);
        analogWrite(in2, -speed);
    } else {                  // LOW, LOW -> 출력 끔
        analogWrite(in1, 0);
        analogWrite(in2, 0);
    }
}

// 왼쪽 바퀴 2개, 오른쪽 바퀴 2개를 묶어서 구동 (차동 구동)
//   drive(+, +) 전진 / drive(-, -) 후진
//   drive(-, +) 제자리 좌회전 / drive(+, -) 제자리 우회전
void drive(int leftSpeed, int rightSpeed) {
    setWheel(FL_IN1, FL_IN2, leftSpeed,  FL_REVERSED);
    setWheel(RL_IN1, RL_IN2, leftSpeed,  RL_REVERSED);
    setWheel(FR_IN1, FR_IN2, rightSpeed, FR_REVERSED);
    setWheel(RR_IN1, RR_IN2, rightSpeed, RR_REVERSED);
}

void stopMotors() {
    drive(0, 0);
}

// 바퀴 방향 확인용 (MOTOR_CHECK 1일 때만 실행)
void motorCheck() {
    Serial.println("[CHECK] forward");     drive(FORWARD_SPEED, FORWARD_SPEED);   delay(1000);
    stopMotors(); delay(500);
    Serial.println("[CHECK] turn left");   drive(-TURN_SPEED, TURN_SPEED);        delay(1000);
    stopMotors(); delay(500);
    Serial.println("[CHECK] turn right");  drive(TURN_SPEED, -TURN_SPEED);        delay(1000);
    stopMotors(); delay(500);
    Serial.println("[CHECK] backward");    drive(-BACKUP_SPEED, -BACKUP_SPEED);   delay(1000);
    stopMotors();
    Serial.println("[CHECK] done");
}


// ============================================================
// 5. LiDAR 수신 (lidar_test.ino를 함수로 정리)
// ============================================================

uint16_t readUInt16(uint8_t low, uint8_t high) {
    return low | (high << 8);
}

// 1바이트를 읽는다. 일정 시간 안에 안 들어오면 false (선이 빠져도 멈춰 있지 않도록)
bool readByte(uint8_t &out) {
    unsigned long start = micros();
    while (!Serial2.available()) {
        if (micros() - start > 2000) {
            return false;
        }
    }
    out = Serial2.read();
    return true;
}

// 패킷 하나를 읽어 distances[]를 갱신한다. 성공하면 true
bool readLidarPacket() {
    uint8_t b;

    // 헤더 0x54, 0x2C 확인
    if (!readByte(b) || b != 0x54) return false;
    if (!readByte(b) || b != 0x2C) return false;

    packet[0] = 0x54;
    packet[1] = 0x2C;

    for (int i = 2; i < 47; i++) {
        if (!readByte(packet[i])) return false;
    }

    // 각도는 0.01도 단위
    uint16_t startRaw = readUInt16(packet[4], packet[5]);
    uint16_t endRaw   = readUInt16(packet[42], packet[43]);

    // 360도를 넘는 각도는 깨진 패킷으로 보고 버린다
    if (startRaw >= 36000 || endRaw >= 36000) return false;

    float startAngle = startRaw / 100.0;
    float endAngle   = endRaw / 100.0;
    if (endAngle < startAngle) {
        endAngle += 360.0;
    }
    float angleStep = (endAngle - startAngle) / 11.0;

    for (int i = 0; i < 12; i++) {
        int index = 6 + i * 3;
        uint16_t distance = readUInt16(packet[index], packet[index + 1]);

        float angle = startAngle + angleStep * i;
        if (angle >= 360.0) {
            angle -= 360.0;
        }

        // 360 -> 0으로 넘어가면 한 바퀴 완료
        if (previousAngle > 300.0 && angle < 60.0) {
            scanReady = true;
        }
        previousAngle = angle;

        int degree = (int)(angle + 0.5);
        if (degree >= 360) {
            degree = 0;
        }

        // 샘플 코드와 다른 점: 0도 그대로 저장한다.
        // 0을 건너뛰면 예전 값이 계속 남아서, 장애물이 사라진 방향도 막힌 것으로 보일 수 있다.
        // 대신 계산할 때 0(과 MIN_VALID_MM 미만)은 무시한다.
        distances[degree] = distance;
    }

    return true;
}


// ============================================================
// 6. 거리 계산 (로봇 기준 각도)
// ============================================================
//
// 로봇 기준 각도(rel): 정면 0°, 왼쪽 +90°, 오른쪽 -90°, 뒤 ±180°
//   LiDAR 원래 각도 = FRONT_DEG - rel
//   예) rel = +90(왼쪽)  -> 180 - 90  = 90°  (샘플 코드의 Left와 일치)
//       rel = -90(오른쪽) -> 180 + 90 = 270° (샘플 코드의 Right와 일치)

uint16_t distanceAt(int rel) {
    int raw = ((FRONT_DEG - rel) % 360 + 360) % 360;
    return distances[raw];
}

bool isValid(uint16_t d) {
    return d >= MIN_VALID_MM;   // 0(측정 실패)도 여기서 걸러진다
}

// fromRel ~ toRel 범위에서 가장 가까운 거리 (충돌 위험 판단용)
int sectorMin(int fromRel, int toRel) {
    int result = NO_DATA;
    for (int rel = fromRel; rel <= toRel; rel++) {
        uint16_t d = distanceAt(rel);
        if (isValid(d) && d < result) {
            result = d;
        }
    }
    return result;
}

// fromRel ~ toRel 범위의 평균 거리 (어느 쪽이 더 열려 있는지 비교용)
int sectorAvg(int fromRel, int toRel) {
    long sum = 0;
    int count = 0;
    for (int rel = fromRel; rel <= toRel; rel++) {
        uint16_t d = distanceAt(rel);
        if (isValid(d)) {
            sum += d;
            count++;
        }
    }
    if (count == 0) {
        return NO_DATA;
    }
    return sum / count;
}


// ============================================================
// 7. 판단 (상태 머신)
// ============================================================

// 상태를 바꿀 때는 먼저 정지한다.
// (그렇지 않으면 새 상태의 명령이 나가기 전까지 이전 명령, 예를 들어 직진이 50ms 더 유지된다)
void changeState(State next) {
    if (next != state) {
        stopMotors();
    }
    state = next;
    stateStartMs = millis();
}

// 왼쪽/오른쪽 중 평균 거리가 더 먼(= 더 열린) 쪽을 고른다
int chooseTurnDir(int leftSpace, int rightSpace) {
    return (leftSpace >= rightSpace) ? 1 : -1;
}

void controlStep() {
    // 각 영역 거리 계산
    int front  = sectorMin(-25, 25);     // 정면 50°
    int frontL = sectorMin(26, 70);      // 왼쪽 대각선 앞
    int frontR = sectorMin(-70, -26);    // 오른쪽 대각선 앞
    int left   = sectorAvg(30, 100);     // 왼쪽이 얼마나 열려 있는지
    int right  = sectorAvg(-100, -30);   // 오른쪽이 얼마나 열려 있는지
    int back   = min(sectorMin(160, 180), sectorMin(-180, -160));

    dbgFront = front;  dbgFrontL = frontL;  dbgFrontR = frontR;
    dbgLeft = left;    dbgRight = right;    dbgBack = back;

    unsigned long elapsed = millis() - stateStartMs;

    switch (state) {

    case FORWARD:
        if (front < STOP_DIST) {
            // 앞이 막힘 -> 좌우도 모두 좁으면 후진, 아니면 더 열린 쪽으로 회전
            if (left < STOP_DIST && right < STOP_DIST) {
                changeState(BACKUP);
            } else {
                turnDir = chooseTurnDir(left, right);
                changeState(TURN);
            }
        } else if (frontL < SIDE_DIST) {
            // 왼쪽 앞이 가까움 -> 오른쪽으로 살짝 (오른쪽 바퀴를 느리게)
            drive(FORWARD_SPEED, FORWARD_SPEED - VEER_DELTA);
        } else if (frontR < SIDE_DIST) {
            // 오른쪽 앞이 가까움 -> 왼쪽으로 살짝 (왼쪽 바퀴를 느리게)
            drive(FORWARD_SPEED - VEER_DELTA, FORWARD_SPEED);
        } else {
            drive(FORWARD_SPEED, FORWARD_SPEED);
        }
        break;

    case TURN:
        // 방향은 회전을 시작할 때 한 번만 정한다 (좌우로 흔들리는 것 방지)
        drive(-turnDir * TURN_SPEED, turnDir * TURN_SPEED);

        if (front > CLEAR_DIST) {
            changeState(FORWARD);
        } else if (elapsed > TURN_TIMEOUT_MS) {
            changeState(BACKUP);
        }
        break;

    case BACKUP:
        if (back < STOP_DIST) {
            // 뒤도 막혀 있으면 후진하지 않고 바로 회전
            stopMotors();
            turnDir = chooseTurnDir(left, right);
            changeState(TURN);
        } else if (elapsed < BACKUP_TIME_MS) {
            drive(-BACKUP_SPEED, -BACKUP_SPEED);
        } else {
            turnDir = chooseTurnDir(left, right);
            changeState(TURN);
        }
        break;
    }
}


// ============================================================
// 8. 디버그 출력 (결과 분석 / README용)
// ============================================================

const char* stateName(State s) {
    switch (s) {
        case FORWARD: return "FORWARD";
        case TURN:    return "TURN";
        case BACKUP:  return "BACKUP";
    }
    return "?";
}

void printDebug() {
    Serial.print(stateName(state));
    if (state == TURN) {
        Serial.print(turnDir > 0 ? "(L)" : "(R)");
    }
    Serial.print(" | F:");   Serial.print(dbgFront);
    Serial.print(" FL:");    Serial.print(dbgFrontL);
    Serial.print(" FR:");    Serial.print(dbgFrontR);
    Serial.print(" | L:");   Serial.print(dbgLeft);
    Serial.print(" R:");     Serial.print(dbgRight);
    Serial.print(" B:");     Serial.println(dbgBack);
}


// ============================================================
// 9. setup / loop
// ============================================================

void setup() {
    Serial.begin(115200);
    Serial2.begin(230400, SERIAL_8N1, LIDAR_RX, -1);

    int motorPins[8] = { FR_IN1, FR_IN2, FL_IN1, FL_IN2, RR_IN1, RR_IN2, RL_IN1, RL_IN2 };
    for (int i = 0; i < 8; i++) {
        pinMode(motorPins[i], OUTPUT);
    }
    stopMotors();

#if MOTOR_CHECK
    delay(2000);
    motorCheck();
    while (true) {
        delay(1000);   // 확인 모드에서는 여기서 멈춘다
    }
#endif

    Serial.println("Waiting for LiDAR...");
}

void loop() {
    // (1) 버퍼에 패킷 하나(47바이트) 이상 쌓여 있을 때만 읽는다.
    //     LiDAR는 데이터를 계속 보내므로, 이렇게 하지 않으면 읽기만 하다가 판단을 못 할 수 있다.
    while (Serial2.available() >= 47) {
        if (readLidarPacket()) {
            lastPacketMs = millis();
        }
    }

    unsigned long now = millis();

    // (2) 안전 정지: 한 바퀴를 아직 못 받았거나, LiDAR가 끊겼으면 움직이지 않는다
    if (!scanReady || now - lastPacketMs > LIDAR_TIMEOUT_MS) {
        stopMotors();
        changeState(FORWARD);
        if (now - lastDebugMs >= 1000) {
            lastDebugMs = now;
            Serial.println(scanReady ? "LiDAR timeout -> stop" : "Waiting for LiDAR...");
        }
        return;
    }

    // (3) 일정 주기마다 판단
    if (now - lastControlMs >= CONTROL_PERIOD_MS) {
        lastControlMs = now;
        controlStep();
    }

    // (4) 상태 출력
    if (now - lastDebugMs >= DEBUG_PERIOD_MS) {
        lastDebugMs = now;
        printDebug();
    }
}
