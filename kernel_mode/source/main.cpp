#include <ntifs.h>

extern "C" {
	NTKERNELAPI NTSTATUS IoCreateDriver(PUNICODE_STRING DriverName,
		PDRIVER_INITIALIZE InitializationFunction);

	NTKERNELAPI NTSTATUS MmCopyVirtualMemory(PEPROCESS SourceProcess, PVOID SourceAddress,
											 PEPROCESS TargetProcess, PVOID TargetAddress,
											 SIZE_T bufferSize, KPROCESSOR_MODE PreviousMode,
											 PSIZE_T ReturnSize);
}

namespace driver
{
	namespace codes
	{
		constexpr ULONG attach =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x696, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		
		constexpr ULONG read =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x697, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		
		constexpr ULONG write =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x698, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	}

	struct Request {
		HANDLE process_id;

		PVOID target;
		PVOID buffer;

		SIZE_T size;
		SIZE_T return_size;
	};

	NTSTATUS create(PDEVICE_OBJECT device_object, PIRP irp)
	{
		UNREFERENCED_PARAMETER(device_object);

		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return irp->IoStatus.Status;
	}

	NTSTATUS close(PDEVICE_OBJECT device_object, PIRP irp)
	{
		UNREFERENCED_PARAMETER(device_object);

		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return irp->IoStatus.Status;
	}

	NTSTATUS device_control(PDEVICE_OBJECT device_object, PIRP irp)
	{
		UNREFERENCED_PARAMETER(device_object);
		
		DbgPrint("[+] Controle de dispositivo chamado.\n");

		NTSTATUS status = STATUS_UNSUCCESSFUL;

		// Precisamos disso para determinar qual codigo foi passado
		PIO_STACK_LOCATION stack_irp = IoGetCurrentIrpStackLocation(irp);

		auto request = reinterpret_cast<Request*>(irp->AssociatedIrp.SystemBuffer);

		if (stack_irp == nullptr || request == nullptr)
		{
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return status;
		}

		static PEPROCESS target_process = nullptr;

		const ULONG control_code = stack_irp->Parameters.DeviceIoControl.IoControlCode;

		switch (control_code)
		{
		case codes::attach:
			status = PsLookupProcessByProcessId(request->process_id, &target_process);

			break;

		case codes::read:
			if (target_process != nullptr)
				status = MmCopyVirtualMemory(
					target_process, request->target,
					PsGetCurrentProcess(), request->buffer,
					request->size, KernelMode, &request->return_size
				);

			break;

		case codes::write:
			if (target_process != nullptr)
				status = MmCopyVirtualMemory(
					PsGetCurrentProcess(), request->buffer,
					target_process, request->target,
					request->size, KernelMode, &request->return_size
				);

			break;

		default:
			break;
		}

		irp->IoStatus.Status = status;
		irp->IoStatus.Information = sizeof(Request);

		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return irp->IoStatus.Status;
	}
}

void driver_unload(PDRIVER_OBJECT driver_object)
{
	DbgPrint("[+] Descarregamento do driver chamado. Limpando...\n");

	UNICODE_STRING symbolic_link = {};
	RtlInitUnicodeString(&symbolic_link, L"\\DosDevices\\KernelDriver2024");
	IoDeleteSymbolicLink(&symbolic_link);

	if (driver_object->DeviceObject != nullptr) {
		IoDeleteDevice(driver_object->DeviceObject);
	}
	DbgPrint("[+] Driver descarregado com sucesso.\n");
}




NTSTATUS driver_main(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path) {
	UNREFERENCED_PARAMETER(registry_path);

	UNICODE_STRING device_name = {};
	RtlInitUnicodeString(&device_name, L"\\Device\\KernelDriver2024");

	PDEVICE_OBJECT device_object = nullptr;
	NTSTATUS status = IoCreateDevice(driver_object, 0, &device_name, FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN, FALSE, &device_object);

	if (status != STATUS_SUCCESS) {
		DbgPrint("[-] Falha ao criar device do driver. Status: 0x%X\n", status);
		return status;
	}

	DbgPrint("[+] Device do driver criado com sucesso.\n");

	UNICODE_STRING symbolic_link = {};
	RtlInitUnicodeString(&symbolic_link, L"\\DosDevices\\KernelDriver2024");

	status = IoCreateSymbolicLink(&symbolic_link, &device_name);
	if (status != STATUS_SUCCESS)
	{
		DbgPrint("[-] Falha ao estabelecer Symbolic Link. Status: 0x%X\n", status);

		IoDeleteDevice(device_object);
		return status;
	}

	DbgPrint("[+] Symbolic Link estabelecido com sucesso.\n");

	SetFlag(device_object->Flags, DO_BUFFERED_IO);

	driver_object->MajorFunction[IRP_MJ_CREATE] = driver::create;
	driver_object->MajorFunction[IRP_MJ_CLOSE] = driver::close;
	driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver::device_control;

	ClearFlag(device_object->Flags, DO_DEVICE_INITIALIZING);

	driver_object->DriverUnload = driver_unload;

	DbgPrint("[+] Driver inicializado com sucesso.\n");

	return STATUS_SUCCESS; 
}
NTSTATUS DriverEntry()
{
	DbgPrint("[+] Driver Kernel-Mode IOCTL carregado.\n");

	UNICODE_STRING driver_name = {};
	RtlInitUnicodeString(&driver_name, L"\\Driver\\IOCTLKernelCheat");

	return IoCreateDriver(&driver_name, &driver_main);
}