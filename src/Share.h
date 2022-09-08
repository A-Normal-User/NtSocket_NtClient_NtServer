#pragma once
#include <winsock.h>
#include <windows.h>

typedef struct _IO_STATUS_BLOCK {
	union {
		NTSTATUS Status;
		PVOID64  Pointer;
	} DUMMYUNIONNAME;
	ULONG_PTR Information;
} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;

#if _WIN64
typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	ULONG Reserved;
	PCWSTR Buffer;
} UNICODE_STRING;
#elif _WIN32
typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PCWSTR Buffer;
} UNICODE_STRING;
#endif

typedef UNICODE_STRING* PUNICODE_STRING;
typedef const UNICODE_STRING* PCUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
	PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES* POBJECT_ATTRIBUTES;

#define OBJ_INHERIT                             0x00000002L
#define OBJ_PERMANENT                           0x00000010L
#define OBJ_EXCLUSIVE                           0x00000020L
#define OBJ_CASE_INSENSITIVE                    0x00000040L
#define OBJ_OPENIF                              0x00000080L
#define OBJ_OPENLINK                            0x00000100L
#define OBJ_KERNEL_HANDLE                       0x00000200L
#define OBJ_FORCE_ACCESS_CHECK                  0x00000400L
#define OBJ_VALID_ATTRIBUTES                    0x000007F2L
#define FILE_OPEN_IF                            0x00000003
#define STATUS_SUCCESS                          0x0
#define AF_INET6                                0x17
#define InitializeObjectAttributes(p,n,a,r,s) { \
    (p)->Length = sizeof(OBJECT_ATTRIBUTES);    \
    (p)->RootDirectory = (r);                   \
    (p)->Attributes = (a);                      \
    (p)->ObjectName = (n);                      \
    (p)->SecurityDescriptor = (s);              \
    (p)->SecurityQualityOfService = NULL;       \
}

typedef enum _EVENT_TYPE {
	NotificationEvent,
	SynchronizationEvent
} EVENT_TYPE;
#if _WIN64
typedef NTSTATUS(*NtDeviceIoControlFile)(
	HANDLE FileHandle,
	HANDLE Event,
	PVOID ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	ULONG IoControlCode,
	PVOID InputBuffer,
	ULONG InputBufferLength,
	PVOID OutputBuffer,
	ULONG OutputBufferLength
	);
typedef NTSTATUS(*NtCreateEvent)(
	PHANDLE EventHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	EVENT_TYPE EventType,
	BOOLEAN InitialState
	);
typedef NTSTATUS(*NtCreateFile)(PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	PLARGE_INTEGER AllocateSize,
	ULONG FileAttributes,
	ULONG ShareAccess,
	ULONG CreateDisposition,
	ULONG CreateOptions,
	PVOID EaBuffer,
	ULONG EaLength);
typedef NTSTATUS(*RtlIpv6StringToAddressExA) (
	PVOID   AddressString,
	byte* Address,
	PULONG  ScopeId,
	PUSHORT Port
	);
#elif _WIN32
typedef NTSTATUS(__stdcall *NtDeviceIoControlFile)(
	HANDLE FileHandle,
	HANDLE Event,
	PVOID ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	ULONG IoControlCode,
	PVOID InputBuffer,
	ULONG InputBufferLength,
	PVOID OutputBuffer,
	ULONG OutputBufferLength
	);
typedef NTSTATUS(__stdcall*NtCreateEvent)(
	PHANDLE EventHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	EVENT_TYPE EventType,
	BOOLEAN InitialState
	);
typedef NTSTATUS(__stdcall*NtCreateFile)(PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	PLARGE_INTEGER AllocateSize,
	ULONG FileAttributes,
	ULONG ShareAccess,
	ULONG CreateDisposition,
	ULONG CreateOptions,
	PVOID EaBuffer,
	ULONG EaLength);
typedef NTSTATUS(__stdcall*RtlIpv6StringToAddressExA) (
	PVOID   AddressString,
	byte* Address,
	PULONG  ScopeId,
	PUSHORT Port
	);
#endif 
/* AFD Packet Endpoint Flags */
#define AFD_ENDPOINT_CONNECTIONLESS	0x1
#define AFD_ENDPOINT_MESSAGE_ORIENTED	0x10
#define AFD_ENDPOINT_RAW		0x100
#define AFD_ENDPOINT_MULTIPOINT		0x1000
#define AFD_ENDPOINT_C_ROOT		0x10000
#define AFD_ENDPOINT_D_ROOT	        0x100000

/* AFD TDI Query Flags */
#define AFD_ADDRESS_HANDLE      0x1L
#define AFD_CONNECTION_HANDLE   0x2L

/* AFD event bits */
#define AFD_EVENT_RECEIVE_BIT                   0
#define AFD_EVENT_OOB_RECEIVE_BIT               1
#define AFD_EVENT_SEND_BIT                      2
#define AFD_EVENT_DISCONNECT_BIT                3
#define AFD_EVENT_ABORT_BIT                     4
#define AFD_EVENT_CLOSE_BIT                     5
#define AFD_EVENT_CONNECT_BIT                   6
#define AFD_EVENT_ACCEPT_BIT                    7
#define AFD_EVENT_CONNECT_FAIL_BIT              8
#define AFD_EVENT_QOS_BIT                       9
#define AFD_EVENT_GROUP_QOS_BIT                 10
#define AFD_EVENT_ROUTING_INTERFACE_CHANGE_BIT  11
#define AFD_EVENT_ADDRESS_LIST_CHANGE_BIT       12
#define AFD_MAX_EVENT                           13
#define AFD_ALL_EVENTS                          ((1 << AFD_MAX_EVENT) - 1)

/* AFD Info Flags */
#define AFD_INFO_INLINING_MODE		0x01L
#define AFD_INFO_BLOCKING_MODE		0x02L
#define AFD_INFO_SENDS_IN_PROGRESS	0x04L
#define AFD_INFO_RECEIVE_WINDOW_SIZE	0x06L
#define AFD_INFO_SEND_WINDOW_SIZE	0x07L
#define AFD_INFO_GROUP_ID_TYPE	        0x10L
#define AFD_INFO_RECEIVE_CONTENT_SIZE   0x11L

/* AFD Share Flags */
#define AFD_SHARE_UNIQUE		0x0L
#define AFD_SHARE_REUSE			0x1L
#define AFD_SHARE_WILDCARD		0x2L
#define AFD_SHARE_EXCLUSIVE		0x3L

/* AFD Disconnect Flags */
#define AFD_DISCONNECT_SEND		0x01L
#define AFD_DISCONNECT_RECV		0x02L
#define AFD_DISCONNECT_ABORT		0x04L
#define AFD_DISCONNECT_DATAGRAM		0x08L

/* AFD Event Flags */
#define AFD_EVENT_RECEIVE                   (1 << AFD_EVENT_RECEIVE_BIT)
#define AFD_EVENT_OOB_RECEIVE               (1 << AFD_EVENT_OOB_RECEIVE_BIT)
#define AFD_EVENT_SEND                      (1 << AFD_EVENT_SEND_BIT)
#define AFD_EVENT_DISCONNECT                (1 << AFD_EVENT_DISCONNECT_BIT)
#define AFD_EVENT_ABORT                     (1 << AFD_EVENT_ABORT_BIT)
#define AFD_EVENT_CLOSE                     (1 << AFD_EVENT_CLOSE_BIT)
#define AFD_EVENT_CONNECT                   (1 << AFD_EVENT_CONNECT_BIT)
#define AFD_EVENT_ACCEPT                    (1 << AFD_EVENT_ACCEPT_BIT)
#define AFD_EVENT_CONNECT_FAIL              (1 << AFD_EVENT_CONNECT_FAIL_BIT)
#define AFD_EVENT_QOS                       (1 << AFD_EVENT_QOS_BIT)
#define AFD_EVENT_GROUP_QOS                 (1 << AFD_EVENT_GROUP_QOS_BIT)
#define AFD_EVENT_ROUTING_INTERFACE_CHANGE  (1 << AFD_EVENT_ROUTING_INTERFACE_CHANGE_BIT)
#define AFD_EVENT_ADDRESS_LIST_CHANGE       (1 << AFD_EVENT_ADDRESS_LIST_CHANGE_BIT)

/* AFD SEND/RECV Flags */
#define AFD_SKIP_FIO			0x1L
#define AFD_OVERLAPPED			0x2L
#define AFD_IMMEDIATE                   0x4L

#define TDI_SEND_EXPEDITED            ((USHORT)0x0020) // TSDU is/was urgent/expedited.
#define TDI_SEND_PARTIAL              ((USHORT)0x0040) // TSDU is/was terminated by an EOR.
#define TDI_SEND_NO_RESPONSE_EXPECTED ((USHORT)0x0080) // HINT: no back traffic expected.
#define TDI_SEND_NON_BLOCKING         ((USHORT)0x0100) // don't block if no buffer space in protocol
#define TDI_SEND_AND_DISCONNECT       ((USHORT)0x0200) // Piggy back disconnect to remote and do not
													   // indicate disconnect from remote
/* IOCTL Generation */
#define FSCTL_AFD_BASE                  FILE_DEVICE_NETWORK
#define _AFD_CONTROL_CODE(Operation,Method) \
  ((FSCTL_AFD_BASE)<<12 | (Operation<<2) | Method)

/* AFD Commands */
#define AFD_BIND			0
#define AFD_CONNECT			1
#define AFD_START_LISTEN		2
#define AFD_WAIT_FOR_LISTEN		3
#define AFD_ACCEPT			4
#define AFD_RECV			5
#define AFD_RECV_DATAGRAM		6
#define AFD_SEND			7
#define AFD_SEND_DATAGRAM		8
#define AFD_SELECT			9
#define AFD_DISCONNECT			10
#define AFD_GET_SOCK_NAME		11
#define AFD_GET_PEER_NAME               12
#define AFD_GET_TDI_HANDLES		13
#define AFD_SET_INFO			14
#define AFD_GET_CONTEXT_SIZE		15
#define AFD_GET_CONTEXT			16
#define AFD_SET_CONTEXT			17
#define AFD_SET_CONNECT_DATA		18
#define AFD_SET_CONNECT_OPTIONS		19
#define AFD_SET_DISCONNECT_DATA		20
#define AFD_SET_DISCONNECT_OPTIONS	21
#define AFD_GET_CONNECT_DATA		22
#define AFD_GET_CONNECT_OPTIONS		23
#define AFD_GET_DISCONNECT_DATA		24
#define AFD_GET_DISCONNECT_OPTIONS	25
#define AFD_SET_CONNECT_DATA_SIZE       26
#define AFD_SET_CONNECT_OPTIONS_SIZE    27
#define AFD_SET_DISCONNECT_DATA_SIZE    28
#define AFD_SET_DISCONNECT_OPTIONS_SIZE 29
#define AFD_GET_INFO			30
#define AFD_EVENT_SELECT		33
#define AFD_ENUM_NETWORK_EVENTS         34
#define AFD_DEFER_ACCEPT		35
#define AFD_GET_PENDING_CONNECT_DATA	41
#define AFD_VALIDATE_GROUP		42

/* AFD IOCTLs */

#define IOCTL_AFD_BIND \
  _AFD_CONTROL_CODE(AFD_BIND, METHOD_NEITHER)
#define IOCTL_AFD_CONNECT \
  _AFD_CONTROL_CODE(AFD_CONNECT, METHOD_NEITHER)
#define IOCTL_AFD_START_LISTEN \
  _AFD_CONTROL_CODE(AFD_START_LISTEN, METHOD_NEITHER)
#define IOCTL_AFD_WAIT_FOR_LISTEN \
  _AFD_CONTROL_CODE(AFD_WAIT_FOR_LISTEN, METHOD_BUFFERED )
#define IOCTL_AFD_ACCEPT \
  _AFD_CONTROL_CODE(AFD_ACCEPT, METHOD_BUFFERED )
#define IOCTL_AFD_RECV \
  _AFD_CONTROL_CODE(AFD_RECV, METHOD_NEITHER)
#define IOCTL_AFD_RECV_DATAGRAM \
  _AFD_CONTROL_CODE(AFD_RECV_DATAGRAM, METHOD_NEITHER)
#define IOCTL_AFD_SEND \
  _AFD_CONTROL_CODE(AFD_SEND, METHOD_NEITHER)
#define IOCTL_AFD_SEND_DATAGRAM \
  _AFD_CONTROL_CODE(AFD_SEND_DATAGRAM, METHOD_NEITHER)
#define IOCTL_AFD_SELECT \
  _AFD_CONTROL_CODE(AFD_SELECT, METHOD_BUFFERED )
#define IOCTL_AFD_DISCONNECT \
  _AFD_CONTROL_CODE(AFD_DISCONNECT, METHOD_NEITHER)
#define IOCTL_AFD_GET_SOCK_NAME \
  _AFD_CONTROL_CODE(AFD_GET_SOCK_NAME, METHOD_NEITHER)
#define IOCTL_AFD_GET_PEER_NAME \
  _AFD_CONTROL_CODE(AFD_GET_PEER_NAME, METHOD_NEITHER)
#define IOCTL_AFD_GET_TDI_HANDLES \
  _AFD_CONTROL_CODE(AFD_GET_TDI_HANDLES, METHOD_NEITHER)
#define IOCTL_AFD_SET_INFO \
  _AFD_CONTROL_CODE(AFD_SET_INFO, METHOD_NEITHER)
#define IOCTL_AFD_GET_CONTEXT_SIZE \
  _AFD_CONTROL_CODE(AFD_GET_CONTEXT_SIZE, METHOD_NEITHER)
#define IOCTL_AFD_GET_CONTEXT \
  _AFD_CONTROL_CODE(AFD_GET_CONTEXT, METHOD_NEITHER)
#define IOCTL_AFD_SET_CONTEXT \
  _AFD_CONTROL_CODE(AFD_SET_CONTEXT, METHOD_NEITHER)
#define IOCTL_AFD_SET_CONNECT_DATA \
  _AFD_CONTROL_CODE(AFD_SET_CONNECT_DATA, METHOD_NEITHER)
#define IOCTL_AFD_SET_CONNECT_OPTIONS \
  _AFD_CONTROL_CODE(AFD_SET_CONNECT_OPTIONS, METHOD_NEITHER)
#define IOCTL_AFD_SET_DISCONNECT_DATA \
  _AFD_CONTROL_CODE(AFD_SET_DISCONNECT_DATA, METHOD_NEITHER)
#define IOCTL_AFD_SET_DISCONNECT_OPTIONS \
  _AFD_CONTROL_CODE(AFD_SET_DISCONNECT_OPTIONS, METHOD_NEITHER)
#define IOCTL_AFD_GET_CONNECT_DATA \
  _AFD_CONTROL_CODE(AFD_GET_CONNECT_DATA, METHOD_NEITHER)
#define IOCTL_AFD_GET_CONNECT_OPTIONS \
  _AFD_CONTROL_CODE(AFD_GET_CONNECT_OPTIONS, METHOD_NEITHER)
#define IOCTL_AFD_GET_DISCONNECT_DATA \
  _AFD_CONTROL_CODE(AFD_GET_DISCONNECT_DATA, METHOD_NEITHER)
#define IOCTL_AFD_GET_DISCONNECT_OPTIONS \
  _AFD_CONTROL_CODE(AFD_GET_DISCONNECT_OPTIONS, METHOD_NEITHER)
#define IOCTL_AFD_SET_CONNECT_DATA_SIZE \
  _AFD_CONTROL_CODE(AFD_SET_CONNECT_DATA_SIZE, METHOD_NEITHER)
#define IOCTL_AFD_SET_CONNECT_OPTIONS_SIZE \
  _AFD_CONTROL_CODE(AFD_SET_CONNECT_OPTIONS_SIZE, METHOD_NEITHER)
#define IOCTL_AFD_SET_DISCONNECT_DATA_SIZE \
  _AFD_CONTROL_CODE(AFD_SET_DISCONNECT_DATA_SIZE, METHOD_NEITHER)
#define IOCTL_AFD_SET_DISCONNECT_OPTIONS_SIZE \
  _AFD_CONTROL_CODE(AFD_SET_DISCONNECT_OPTIONS_SIZE, METHOD_NEITHER)
#define IOCTL_AFD_GET_INFO \
  _AFD_CONTROL_CODE(AFD_GET_INFO, METHOD_NEITHER)
#define IOCTL_AFD_EVENT_SELECT \
  _AFD_CONTROL_CODE(AFD_EVENT_SELECT, METHOD_NEITHER)
#define IOCTL_AFD_DEFER_ACCEPT \
  _AFD_CONTROL_CODE(AFD_DEFER_ACCEPT, METHOD_NEITHER)
#define IOCTL_AFD_GET_PENDING_CONNECT_DATA \
  _AFD_CONTROL_CODE(AFD_GET_PENDING_CONNECT_DATA, METHOD_NEITHER)
#define IOCTL_AFD_ENUM_NETWORK_EVENTS \
  _AFD_CONTROL_CODE(AFD_ENUM_NETWORK_EVENTS, METHOD_NEITHER)
#define IOCTL_AFD_VALIDATE_GROUP \
  _AFD_CONTROL_CODE(AFD_VALIDATE_GROUP, METHOD_NEITHER)
