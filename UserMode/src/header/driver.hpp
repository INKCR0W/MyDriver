#pragma once

#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

namespace driver {
	namespace codes {
		// Used to setup the driver.
		constexpr ULONG attach =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x666, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

		// Read process memory.
		constexpr ULONG read =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x777, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

		// Write process memory.
		constexpr ULONG write =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x888, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	}  // namespace codes

	// Shared between user mode & kernel mode.
	struct Request {
		HANDLE process_id;

		PVOID target;
		PVOID buffer;

		SIZE_T size;
		SIZE_T return_size;
	};

	bool attach_to_process(HANDLE driver_handle, const DWORD pid) {
		Request r;
		r.process_id = reinterpret_cast<HANDLE>(pid);

		return DeviceIoControl(driver_handle, codes::attach, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
	}




	class driver {
	private:
		// 驱动句柄
		HANDLE driver_handle;
		// 目标进程的PID
		DWORD pid;
		// 是否已经附加到目标进程
		bool attached;

		// 通过进程名获取进程ID
		DWORD get_process_id(const wchar_t* process_name);
		// 获取指定进程里指定模块的起始地址
		std::uintptr_t get_module_base(const DWORD pid, const wchar_t* module_name);

	public:
		driver();
		driver(const wchar_t* driver_path);
		driver(const wchar_t* driver_path, const wchar_t* process_name);
		driver(const wchar_t* driver_path, const DWORD pid);

		~driver();

		// 获取驱动句柄
		bool setDriver(const wchar_t* driver_path);
		// 附加到目标进程，传入进程名
		bool attach(const wchar_t* process_name);
		// 附加到目标进程，传入进程ID
		bool attach(const DWORD pid);

		// 返回驱动句柄
		const HANDLE _driver() const;
		// 返回附加的进程ID
		const DWORD _pid() const;
		// 是否已经附加到目标进程
		const bool isAttached() const;

		// 读内存，传入一个uintptr_t类型的指针
		template <typename T>
		T read_memory(const std::uintptr_t addr) {
			T temp = {};

			// 不初始化，提高运行效率
			Request r;
			r.target = reinterpret_cast<PVOID>(addr);
			r.buffer = &temp;
			r.size = sizeof(T);

			DeviceIoControl(this->driver_handle, codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);

			return temp;
		}

		// 写内存，传入一个uintptr_t类型的指针和一个标准类型的值
		template <typename T>
		void write_memory(const std::uintptr_t addr, const T& value) {
			// 不初始化，提高运行效率
			Request r;
			r.target = reinterpret_cast<PVOID>(addr);
			r.buffer = (PVOID)&value;
			r.size = sizeof(T);

			DeviceIoControl(this->driver_handle, codes::write, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
		}
	};
}  // namespace driver