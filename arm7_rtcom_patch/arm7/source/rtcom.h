#pragma once

// Arm11 answers
#define RTCOM_STAT_READY        0x00
#define RTCOM_STAT_ACK          0x80
#define RTCOM_STAT_DONE         0x82

// Arm7 requests
#define RTCOM_REQ_KIL           0x00
#define RTCOM_REQ_UPLOAD_UCODE  0x41
#define RTCOM_REQ_FINISH_UCODE  0x42
#define RTCOM_REQ_EXECUTE_UCODE 0x44
#define RTCOM_REQ_NEXT          0x81
#define RTCOM_REQ_KEEPALIVE     0x83
#define RTCOM_REQ_SYNC_KIL      0xFE

typedef struct {
	vu8 opcode;
	vu8 flags;
	vu16 size;
	vu8 *data;
} Ipc_proto;

u16 rtcom_beginComm();
void rtcom_endComm(u16 old_reg_rcnt);

u8 rtcom_getData();

bool rtcom_waitStatus(u8 status);
#define rtcom_waitReady()               rtcom_waitStatus(RTCOM_STAT_READY)
#define rtcom_waitAck()                 rtcom_waitStatus(RTCOM_STAT_ACK)
#define rtcom_waitDone()                rtcom_waitStatus(RTCOM_STAT_DONE)

void rtcom_requestAsync(u8 request);
void rtcom_requestAsync(u8 request, u8 param);
bool rtcom_request(u8 request);
bool rtcom_request(u8 request, u8 param);

static inline void rtcom_requestNextAsync() { rtcom_requestAsync(RTCOM_REQ_NEXT); }
static inline void rtcom_requestNextAsync(u8 param) { rtcom_requestAsync(RTCOM_REQ_NEXT, param); }
static inline bool rtcom_requestNext() { return rtcom_request(RTCOM_REQ_NEXT); }
static inline bool rtcom_requestNext(u8 param) { return rtcom_request(RTCOM_REQ_NEXT, param); }

static inline void rtcom_requestKillAsync() { rtcom_requestAsync(RTCOM_REQ_KIL); }
bool rtcom_requestKill();

void rtcom_signalDone();

bool rtcom_uploadUCode(const void* uCode, u32 length);
bool rtcom_executeUCode(u8 param);

// ----------------------------------------------------------------------------------

void Init_RTCom();
void Update_RTCom();

