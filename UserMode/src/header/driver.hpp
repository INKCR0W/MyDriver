#pragma once

#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

namespace driver {
	namespace codes {
		// Used to setup the driver.
		constexpr ULONG attach =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x676, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

		// Read process memory.
		constexpr ULONG read =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x678, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

		// Write process memory.
		constexpr ULONG write =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x679, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	}  // namespace codes

	// Shared between user mode & kernel mode.
	struct Request {
		HANDLE process_id;

		PVOID target;
		PVOID buffer;

		SIZE_T size;
		SIZE_T return_size;
	};

	namespace error_codes {
		static const int ACCESS = 0x00;
		static const int GET_DRIVER_ERROR = 0x01;
		static const int GET_PROCESSID_ERROR = 0x02;
	}  // namespace error_codes


	class driver {
	private:
		// �������
		HANDLE driver_handle;
		// Ŀ����̵�PID
		DWORD pid;
		// �Ƿ��Ѿ����ӵ�Ŀ�����
		bool attached;
		// ������Ϣ
		int error_code;

		// ͨ����������ȡ����ID
		DWORD get_process_id(const wchar_t* process_name);

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
		// ��ȡ���һ�δ���ı��� ��namespace driver::error_codes��
		const int getError() const;

		// ��ȡĿ�������ָ��ģ�����ʼ��ַ
		const std::uintptr_t get_module_base(const wchar_t* module_name) const;

		// ���ڴ棬����һ��uintptr_t���͵�ָ��
		template <typename T>
		T read_memory(const std::uintptr_t addr) {
			T temp = {};

			Request r = {
				nullptr,
				reinterpret_cast<PVOID>(addr),
				&temp,
				sizeof(T),
				0
			};

			DeviceIoControl(this->driver_handle, codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);

			return temp;
		}

		// д�ڴ棬����һ��uintptr_t���͵�ָ���һ����׼���͵�ֵ
		template <typename T>
		void write_memory(const std::uintptr_t addr, const T& value) {
			Request r = {
				nullptr,
				reinterpret_cast<PVOID>(addr),
				(PVOID)& value,
				sizeof(T),
				0
			};

			DeviceIoControl(this->driver_handle, codes::write, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
		}
	};
}  // namespace driver