#include <stdint.h>
#include <algorithm>

#include <nds.h>
#include <nds/arm7/serial.h>

#include "arm7_rtcom_patch_uc11.h"
#include "rtcom.h"
#include "sys/_stdint.h"

// Delay (in swiDelay units) for each bit transfer
#define RTC_DELAY 48

// Pin defines on RTC_CR
#define CS_0 (1 << 6)
#define CS_1 ((1 << 6) | (1 << 2))
#define SCK_0 (1 << 5)
#define SCK_1 ((1 << 5) | (1 << 1))
#define SIO_0 (1 << 4)
#define SIO_1 ((1 << 4) | (1 << 0))
#define SIO_out (1 << 4)
#define SIO_in (1)

#define RTC_READ_112 0x6D
#define RTC_WRITE_112 0x6C

#define RTC_READ_113 0x6F
#define RTC_WRITE_113 0x6E

#define RTC_READ_DATE_AND_TIME 0x65
#define RTC_READ_HOUR_MINUTE_SECOND 0x67
#define RTC_READ_ALARM_TIME_1 0x69
#define RTC_READ_ALARM_TIME_2 0x6B

#define RTC_READ_COUNTER_EXT 0x71
#define RTC_READ_FOUT1_EXT 0x73
#define RTC_READ_FOUT2_EXT 0x75
#define RTC_READ_ALARM_DATE_1_EXT 0x79
#define RTC_READ_ALARM_DATE_2_EXT 0x7B

#define RTCOM_DATA_OUTPUT 0x027FF400

// an attempt to put global variables next to the code for easier memory control;
// be aware of the compiler error "unaligned opcodes detected in executable segment"
__attribute__((section(".text"))) static int RTCOM_STATE_TIMER = 0;

static Ipc_proto *ipc_proto = (Ipc_proto *)RTCOM_DATA_OUTPUT;
static bool fastMode = false;

static void waitByLoop(volatile int count) {
    // 1 loop = 10 cycles = ~0.3us ???
    while (--count > 0) {
    }
}

static void rtcTransferReversed(u8 *cmd, u32 cmdLen, u8 *result, u32 resultLen) {
    // Some games are sensitive to delays in the VBlank IRQ (the place where this code is called from)
    // to the point becoming literally unplayable (like graphics and timing glitches, etc).
    // To solve this problem we can reduce the following delays. As far as I know, they are completely unnecessary,
    // except one thing. The delays are needed when we're uploading code to Arm11 via RTCom,
    // otherwise there's a high chance of errors, the code may turn out to be corrupted during the transfer.
    // The worst part is these errors will be undetectable (normally they are supposed to be signaled by RTCom).
    // So use large delays to upload code into Arm11 (just once, at startup),
    // and then small delays (or none at all) to simply read the CPad stuff, etc each frame.
    int initDelay = 2;
    int bitTransferDelay = fastMode ? 1 : 14;

    // Raise CS
    RTC_CR8 = CS_0 | SCK_1 | SIO_1;
    waitByLoop(initDelay);
    RTC_CR8 = CS_1 | SCK_1 | SIO_1;
    waitByLoop(initDelay);

    // Write command byte (high bit first)
    u8 data = *cmd++;

    for (u32 bit = 0; bit < 8; bit++) {
        RTC_CR8 = CS_1 | SCK_0 | SIO_out | (data >> 7);
        waitByLoop(bitTransferDelay);

        RTC_CR8 = CS_1 | SCK_1 | SIO_out | (data >> 7);
        waitByLoop(bitTransferDelay);

        data <<= 1;
    }
    // Write parameter bytes (high bit first)
    for (; cmdLen > 1; cmdLen--) {
        data = *cmd++;
        for (u32 bit = 0; bit < 8; bit++) {
            RTC_CR8 = CS_1 | SCK_0 | SIO_out | (data >> 7);
            waitByLoop(bitTransferDelay);

            RTC_CR8 = CS_1 | SCK_1 | SIO_out | (data >> 7);
            waitByLoop(bitTransferDelay);

            data <<= 1;
        }
    }

    // Read result bytes (high bit first)
    for (; resultLen > 0; resultLen--) {
        data = 0;
        for (u32 bit = 0; bit < 8; bit++) {
            RTC_CR8 = CS_1 | SCK_0;
            waitByLoop(bitTransferDelay);

            RTC_CR8 = CS_1 | SCK_1;
            waitByLoop(bitTransferDelay);

            data <<= 1;
            if (RTC_CR8 & SIO_in)
                data |= 1;
        }
        *result++ = data;
    }

    // Finish up by dropping CS low
    waitByLoop(initDelay);
    RTC_CR8 = CS_0 | SCK_1;
    waitByLoop(initDelay);
}

static u8 readReg112() {
    u8 readCmd = RTC_READ_112;
    u8 readVal = 0;
    rtcTransferReversed(&readCmd, 1, &readVal, 1);
    return readVal;
}

static void writeReg112(u8 val) {
    u8 command[2] = {RTC_WRITE_112, val};
    rtcTransferReversed(command, 2, 0, 0);
}

static u8 readReg113() {
    u8 readCmd = RTC_READ_113;
    u8 readVal = 0;
    rtcTransferReversed(&readCmd, 1, &readVal, 1);
    return readVal;
}

static void writeReg113(u8 val) {
    u8 command[2] = {RTC_WRITE_113, val};
    rtcTransferReversed(command, 2, 0, 0);
}

u16 rtcom_beginComm() {
    u16 old_reg_rcnt = REG_RCNT;
    REG_IF = IRQ_NETWORK;
    REG_RCNT = 0x8100; // enable irq
    REG_IF = IRQ_NETWORK;
    return old_reg_rcnt;
}

void rtcom_endComm(u16 old_reg_rcnt) {
    REG_IF = IRQ_NETWORK;
    REG_RCNT = old_reg_rcnt;
}

u8 rtcom_getData() { return readReg112(); }

bool rtcom_waitStatus(u8 status) {
    // int timeout = 50000;
    int timeout = 2062500;
    do {
        // each iteration takes ~11 cycles

        if (!(REG_IF & IRQ_NETWORK))
            continue;

        REG_IF = IRQ_NETWORK;
        return status == readReg113();
    } while (--timeout);

    REG_IF = IRQ_NETWORK;
    return false;
}

void rtcom_requestAsync(u8 request) { writeReg113(request); }

void rtcom_requestAsync(u8 request, u8 param) {
    writeReg112(param);
    writeReg113(request);
}

bool rtcom_request(u8 request) {
    rtcom_requestAsync(request);
    return rtcom_waitAck();
}

bool rtcom_request(u8 request, u8 param) {
    rtcom_requestAsync(request, param);
    return rtcom_waitAck();
}

bool rtcom_requestKill() {
    rtcom_requestAsync(RTCOM_REQ_SYNC_KIL);
    return rtcom_waitReady();
}

void rtcom_signalDone() { writeReg113(RTCOM_STAT_DONE); }

bool rtcom_uploadUCode(const void *uCode, u32 length) {
    if (!rtcom_request(RTCOM_REQ_UPLOAD_UCODE, length & 0xFF))
        return false;

    if (!rtcom_requestNext((length >> 8) & 0xFF))
        return false;

    if (!rtcom_requestNext((length >> 16) & 0xFF))
        return false;

    if (!rtcom_requestNext((length >> 24) & 0xFF))
        return false;

    const u8 *pCode = (const u8 *)uCode;
    for (u32 i = 0; i < length; i++)
        if (!rtcom_requestNext(*pCode++))
            return false;

    // finish uploading
    rtcom_requestAsync(RTCOM_REQ_KEEPALIVE);
    if (!rtcom_waitDone())
        return false;

    // make it executable
    return rtcom_request(RTCOM_REQ_FINISH_UCODE);
}

bool rtcom_executeUCode(u8 param) { return rtcom_request(RTCOM_REQ_EXECUTE_UCODE, param); }

// ----------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------

void readTimer2(u8 *readVal, u32 readValLen) {
    u8 readCmd = RTC_READ_ALARM_TIME_2;
    rtcTransferReversed(&readCmd, 1, readVal, readValLen);
}

void Init_RTCom() {
    int savedIrq = enterCriticalSection();
    while (true) {
        u16 old_crtc = rtcom_beginComm();
        int trycount = 10;

        do {
            rtcom_requestAsync(1); // request communication from rtcom
            if (rtcom_waitStatus(RTCOM_STAT_DONE)) {
                break;
            }
        } while (--trycount);

        if (trycount) {
            trycount = 10;
            do {
                if (rtcom_uploadUCode(arm7_rtcom_patch_uc11, arm7_rtcom_patch_uc11_size)) {
                    break;
                }
            } while (--trycount);
        }

        rtcom_endComm(old_crtc);

        if (trycount > 0)
            break; // success

        // there is no point returning without success
    }
    leaveCriticalSection(savedIrq);
}

void Execute_Code_Async_via_RTCom(int param) {
    int savedIrq = enterCriticalSection();
    {
        // It may take relatively a long time to execute for Arm11
        // Don't wait for the answer, otherwise sound glitches might come up
        //  (if the Surround or Headphones audio modes are turned on).
        // Arm11 should release/kill the connection automatically
        u16 old_rcnt = rtcom_beginComm();
        rtcom_requestAsync(RTCOM_REQ_EXECUTE_UCODE, param);
        rtcom_endComm(old_rcnt);
    }
    leaveCriticalSection(savedIrq);
}

#define DATAFLOW
#define TIMER
#ifdef DATAFLOW
static u32 *dataflow = (u32 *)(RTCOM_DATA_OUTPUT + 8);
static u8 bo = 0;
#endif
#ifdef TIMER
static u32 *timing = (u32 *)(RTCOM_DATA_OUTPUT + 12);
static u8 bo2 = 0;
void stop_timer() {
	TIMER2_CR = 0;
	TIMER3_CR = 0;
	timing[bo2] = (TIMER3_DATA << 16 | TIMER2_DATA);
	bo2 += 2;
}
void start_timer() {
	TIMER3_DATA = 0;
	TIMER2_DATA = 0;
	TIMER3_CR = TIMER_ENABLE | TIMER_CASCADE;
	TIMER2_CR = TIMER_ENABLE;
}
#endif
static u8 *arm11_buffer = (u8 *)(RTCOM_DATA_OUTPUT + 16);
static void Ir_service() {
	if (ipc_proto->flags != 0xF)
		return;

	int savedIrq = enterCriticalSection();
	u8 tc = 0;
	u16 old_crtc = rtcom_beginComm();
	switch (ipc_proto->opcode) {
		// Send data
		case 1:
#ifdef TIMER
			start_timer();
#endif
			rtcom_executeUCode(1);
			rtcom_requestNext(ipc_proto->size);
			if (ipc_proto->size <= 16) {
				for (u8 i = 0; i < ipc_proto->size; i++) {
#ifdef TIMER
					// Next byte will trigger data transfer
					if (i == ipc_proto->size - 1) {
						stop_timer();
						start_timer();
					}
#endif
					rtcom_requestNext(ipc_proto->data[i]);
				}
			} else {
				for (u8 i = 0; i < ipc_proto->size; i++) {
#ifdef TIMER
					if (i == ipc_proto->size - 1) {
						stop_timer();
						start_timer();
					}
#endif
					do {
						rtcom_requestNext(ipc_proto->data[i]);
					} while (rtcom_getData());
				}
			}
#ifdef TIMER
			stop_timer();
#endif
#ifdef DATAFLOW
			dataflow[bo] = ipc_proto->size << 8 | (ipc_proto->data[0] ^ 0xAA);
			bo += 2;
#endif
			break;
		// Recv data
		case 2:
			rtcom_executeUCode(2);
			tc = rtcom_getData();
			for (u8 i = 0; i < tc; i += 3) {
				rtcom_requestNext();
				ipc_proto->data[i++] = rtcom_getData();
				/* readValLen = tc - i; */
				readTimer2((u8 *)(ipc_proto->data + i), 3);
				/* if (readValLen) { */
				/* 	readValLen = readValLen < 3 ? readValLen : 3; */
				/* 	readTimer2((u8 *)(ipc_proto->data + i), readValLen); */
				/* 	i += readValLen; */
				/* } */
			}
			ipc_proto->size = tc;
#ifdef DATAFLOW
			if (tc) {
				dataflow[bo] = ipc_proto->size << 8 | ipc_proto->data[0];
				bo += 2;
			}
#endif
			break;
		// beginComm and endComm
		case 3:
		//case 4: TODO rimettere a posto qua
			rtcom_executeUCode(ipc_proto->opcode);
#ifdef DATAFLOW
			bo = 0;
#endif
#ifdef TIMER
			bo2 = 0;
#endif
			break;
		case 4:
			rtcom_executeUCode(ipc_proto->opcode);
			for (u8 i = 0; i < 136; i++) {
				rtcom_requestNext();
				arm11_buffer[i] = rtcom_getData();
			}
			break;
	}
	rtcom_endComm(old_crtc);
	// TEST
	rtcom_requestKill();
	rtcom_requestAsync(RTCOM_STAT_DONE);
	// TEST
	leaveCriticalSection(savedIrq);

	ipc_proto->flags = 0xF0;
}

__attribute__((target("arm"))) void Update_RTCom() {
    // Steps: Wait; Upload the code to Arm11;
    //        Wait; Start the IR service handler;
    enum RtcomStateTime {
        Start = 0,
        UploadCode = 200,
		IrBeginComm = UploadCode + 50,
        ReadyToRead = IrBeginComm + 50,
    };

    switch (RTCOM_STATE_TIMER) {
    case UploadCode:
        Init_RTCom();
        RTCOM_STATE_TIMER += 1;
        break;
	case IrBeginComm:
		// TODO Cambiare sta cosa hookandolo all'Init????
		Execute_Code_Async_via_RTCom(3);
		fastMode = true;
		RTCOM_STATE_TIMER += 1;
		break;
    case ReadyToRead:
		Ir_service();
        break;
    default:
        RTCOM_STATE_TIMER += 1;
        break;
    }
}
