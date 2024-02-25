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
		// �������
		HANDLE driver_handle;
		// Ŀ����̵�PID
		DWORD pid;
		// �Ƿ��Ѿ����ӵ�Ŀ�����
		bool attached;

		// ͨ����������ȡ����ID
		DWORD get_process_id(const wchar_t* process_name);
		// ��ȡָ��������ָ��ģ�����ʼ��ַ
		std::uintptr_t get_module_base(const DWORD pid, const wchar_t* module_name);

	public:
		driver();
		driver(const wchar_t* driver_path);
		driver(const wchar_t* driver_path, const wchar_t* process_name);
		driver(const wchar_t* driver_path, const DWORD pid);

		~driver();

		// ��ȡ�������
		bool setDriver(const wchar_t* driver_path);
		// ���ӵ�Ŀ����̣����������
		bool attach(const wchar_t* process_name);
		// ���ӵ�Ŀ����̣��������ID
		bool attach(const DWORD pid);

		// �����������
		const HANDLE _driver() const;
		// ���ظ��ӵĽ���ID
		const DWORD _pid() const;
		// �Ƿ��Ѿ����ӵ�Ŀ�����
		const bool isAttached() const;

		// ���ڴ棬����һ��uintptr_t���͵�ָ��
		template <typename T>
		T read_memory(const std::uintptr_t addr) {
			T temp = {};

			// ����ʼ�����������Ч��
			Request r;
			r.target = reinterpret_cast<PVOID>(addr);
			r.buffer = &temp;
			r.size = sizeof(T);

			DeviceIoControl(this->driver_handle, codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);

			return temp;
		}

		// д�ڴ棬����һ��uintptr_t���͵�ָ���һ����׼���͵�ֵ
		template <typename T>
		void write_memory(const std::uintptr_t addr, const T& value) {
			// ����ʼ�����������Ч��
			Request r;
			r.target = reinterpret_cast<PVOID>(addr);
			r.buffer = (PVOID)&value;
			r.size = sizeof(T);

			DeviceIoControl(this->driver_handle, codes::write, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
		}
	};
}  // namespace driver