#include <ntddk.h>
#include <windef.h>
#include <stdlib.h> 



//【1】定义符号链接，一般来说修改为驱动的名字即可
#define DEVICE_NAME   L"\\Device\\KrnlHW64"
#define LINK_NAME   L"\\DosDevices\\KrnlHW64"
#define LINK_GLOBAL_NAME  L"\\DosDevices\\Global\\KrnlHW64" 

//【2】定义驱动功能号和名字，提供接口给应用程序调用
#define IOCTL_IO_TEST  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SAY_HELLO  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS) 

//【3】驱动卸载的处理例程
VOID DriverUnload(PDRIVER_OBJECT pDriverObj) {
	UNICODE_STRING strLink;
	DbgPrint(L"[KrnlHW64]DriverUnload\n");
	RtlInitUnicodeString(&strLink, LINK_NAME);
	IoDeleteSymbolicLink(&strLink);
	IoDeleteDevice(pDriverObj->DeviceObject);
	//析构驱动
}
//【4】IRP_MJ_CREATE对应的处理例程，一般不用管它
NTSTATUS DispatchCreate(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	//DbgPrint(L"[KrnlHW64]DispatchCreate\n");
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//【5】IRP_MJ_CLOSE对应的处理例程，一般不用管它
NTSTATUS DispatchClose(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	//DbgPrint(L"[KrnlHW64]DispatchClose\n");
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//【6】IRP_MJ_DEVICE_CONTROL对应的处理例程，驱动最重要的函数之一，一般走正常途径调 用驱动功能的程序，都会经过这个函数
NTSTATUS DispatchIoctl(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;  PIO_STACK_LOCATION pIrpStack;  ULONG uIoControlCode;  PVOID pIoBuffer;  ULONG uInSize;  ULONG uOutSize;  DbgPrint("[KrnlHW64]DispatchIoctl\n");  pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	//控制码
	uIoControlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
	//输入输出缓冲区
	pIoBuffer = pIrp->AssociatedIrp.SystemBuffer;
	//输入区域大小
	uInSize = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	//输出区域大小 
	uOutSize = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	switch (uIoControlCode)
	{
	case IOCTL_IO_TEST:
	{
		DWORD dw = 0;
		//获得输入的内容
		memcpy(&dw, pIoBuffer, sizeof(DWORD));
		//使用输入的内容
		dw++;
		//输出处理的结果 
		memcpy(pIoBuffer, &dw, sizeof(DWORD));
		//处理成功，返回非STATUS_SUCCESS会让DeviveIoControl返回失败 
		status = STATUS_SUCCESS;
		break;
	}
	case IOCTL_SAY_HELLO:
	{
		DbgPrint("[KrnlHW64]IOCTL_SAY_HELLO\n");
		status = STATUS_SUCCESS;
		break;
	}
	default:
		status = STATUS_SUCCESS;
		break;
	}
	if (status == STATUS_SUCCESS)
		pIrp->IoStatus.Information = uOutSize;
	else
		pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}

//【7】驱动加载的处理例程，里面进行了驱动的初始化工作
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING pRegistryString)
{
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING ustrLinkName;
	UNICODE_STRING ustrDevName;
	PDEVICE_OBJECT pDevObj;
	//初始化驱动例程
	pDriverObj->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
	pDriverObj->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
	pDriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;
	pDriverObj->DriverUnload = DriverUnload;
	//创建驱动设备
	RtlInitUnicodeString(&ustrDevName, DEVICE_NAME);
	status = IoCreateDevice(pDriverObj, 0, &ustrDevName, FILE_DEVICE_UNKNOWN, 0, FALSE, &pDevObj);
	if (!NT_SUCCESS(status))
		return status;
	if (IoIsWdmVersionAvailable(1, 0x10))
		RtlInitUnicodeString(&ustrLinkName, LINK_GLOBAL_NAME);
	else
		RtlInitUnicodeString(&ustrLinkName, LINK_NAME);
	//创建符号链接
	status = IoCreateSymbolicLink(&ustrLinkName, &ustrDevName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
		return status;
	}
	//走到这里驱动实际上已经初始化完成，下面添加的是功能初始化的代码
	DbgPrint("[KrnlHW64]DriverEntry\n");
	//-------初始化项目
	return STATUS_SUCCESS;
}