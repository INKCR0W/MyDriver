/*
*  ��������������
*�����ߩ��������ߩ�
*��              �� ��
*������������������
*�����ש������ס���
*��              ��
*���������ߡ�������
*��              ��
*������������������
*�����������������ޱ���		THE MYTHICAL BEAST BLESS
*��������������������BUG��	THE CODE WITHOUT BUGS!
*����������������������
*������              �ǩ�
*������              ����
*���������������ש�����
*���������ϩϡ����ϩ�
*���������ߩ������ߩ� ��
*/


#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <thread>
#include <cmath>

#include "header/client.dll.hpp"
#include "header/offsets.hpp"
#include "header/driver.hpp"
#include "header/buttons.hpp"
#include "header/structs.hpp"

using namespace cs2_dumper;

constexpr float M_PI = 3.14159265358979;


template <typename T>
void Log(T val) {
	std::cout << val << "\n";
}

Vec3 clampAngles(Vec3 angles) {
	if (angles.x > 89.0f && angles.x <= 180.0f) {
		angles.x = 89.0f;
	}

	if (angles.x > 180.0f) {
		angles.x -= 360.0f;
	}

	if (angles.x < -89.0f) {
		angles.x = -89.0f;
	}

	if (angles.y > 180.0f) {
		angles.y -= 360.0f;
	}

	if (angles.y < -180.0f) {
		angles.y += 360.0f;
	}

	angles.z = 0;

	return angles;
}

static void print_error_info(const int error_code) {
	const auto error = GetLastError();

	switch (error_code)
	{
	case driver::error_codes::ACCESS:
		break;

	case driver::error_codes::GET_DRIVER_ERROR:
		std::cout << "[-] Failed to create driver handler: " << error << "\n";
		break;

	case driver::error_codes::GET_PROCESSID_ERROR:
		std::cout << "[-] Failed to find cs2 process: " << error << "\n";
		break;

	default:
		std::cout << "[-] Unknown error: " << error << "\n";
		break;
	}
}


int main() {
	using driver::driver;

	driver myDriver(L"\\\\.\\BabyDriver", L"cs2.exe");

	if (!myDriver.isAttached()) {
		print_error_info(myDriver.getError());
		std::cin.get();
		return 1;
	}

	std::cout << "[+] Attachment successful.\n";

	if (const uintptr_t client = myDriver.get_module_base(L"client.dll"); client != 0) {
		std::cout << "[+] Client.dll found, address : 0x" << std::hex << std::uppercase << client << "\n";
		std::cout << "[+] Press END to stop this process.\n";

		while (true) {
			// Sleep(1);
			if (GetAsyncKeyState(VK_END))
				break;

			const uintptr_t game_rules = myDriver.read_memory<uintptr_t>(client + offsets::client_dll::dwGameRules);
			
			if (!myDriver.read_memory<bool>(game_rules + schemas::client_dll::C_CSGameRules::m_bHasMatchStarted))
				continue;


			const auto local_player_pawn = myDriver.read_memory<uintptr_t>(client + offsets::client_dll::dwLocalPlayerPawn);
			const auto local_player_controller = myDriver.read_memory<uintptr_t>(client + offsets::client_dll::dwLocalPlayerController);
			const auto view_angle_addr = client + offsets::client_dll::dwViewAngles;

			if (local_player_pawn == 0 || view_angle_addr == 0)
				continue;

			Vec3 local_pos = myDriver.read_memory<Vec3>(local_player_pawn + schemas::client_dll::C_CSPlayerPawnBase::m_vecLastClipCameraPos);

			const auto entity_list = myDriver.read_memory<uintptr_t>(client + offsets::client_dll::dwEntityList);
			

			float nearest = 999999999.f;
			unsigned long long target = 0;
			Vec3 opp_pos = {};
			Vec2 target_angle = {};

			Vec2 view_angle = myDriver.read_memory<Vec2>(view_angle_addr);

			int local_player_controller_index = 0;

			for (unsigned long long i = 0; i < 64; ++i) {
				const uintptr_t entry_address = myDriver.read_memory<uintptr_t>(entity_list + (0x8ULL * (i & 0x7FFF) >> 9) + 0x10);
				const uintptr_t entry_controller = myDriver.read_memory<uintptr_t>(entry_address + 0x78ULL * (i & 0x1FF));

				if (entry_controller == local_player_controller) {
					local_player_controller_index = i;
					continue;
				}

				if (!myDriver.read_memory<bool>(entry_controller + schemas::client_dll::CCSPlayerController::m_bPawnIsAlive))
					continue;

				const uint32_t C_CSPlayerPawn = myDriver.read_memory<std::uint32_t>(entry_controller + schemas::client_dll::CCSPlayerController::m_hPlayerPawn);
				const uintptr_t pawn_entry_address = myDriver.read_memory<uintptr_t>(entity_list + 0x8ULL * ((C_CSPlayerPawn & 0x7FFF) >> 9) + 0x10);
				const uintptr_t entry_pawn = myDriver.read_memory<uintptr_t>(pawn_entry_address + 0x78ULL * (C_CSPlayerPawn & 0x1FF));

				if (myDriver.read_memory<int>(entry_pawn + schemas::client_dll::C_BaseEntity::m_iHealth) < 0)
					continue;

				// glow
				myDriver.write_memory<float>(entry_pawn + schemas::client_dll::C_CSPlayerPawnBase::m_flDetectedByEnemySensorTime, 100000.f);
				// glow end


				int local_spotted = myDriver.read_memory<int>(local_player_pawn + schemas::client_dll::C_CSPlayerPawnBase::m_entitySpottedState + schemas::client_dll::EntitySpottedState_t::m_bSpottedByMask);
				int target_spotted = myDriver.read_memory<int>(entry_pawn + schemas::client_dll::C_CSPlayerPawnBase::m_entitySpottedState + schemas::client_dll::EntitySpottedState_t::m_bSpottedByMask);

				if (!(target_spotted & (1ULL << entry_pawn) || (local_player_pawn & (1ULL << i))))
					continue;

				const uintptr_t game_scene_node = myDriver.read_memory<uintptr_t>(entry_pawn + schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
				const uintptr_t bone_array_address = myDriver.read_memory<uintptr_t>(game_scene_node + schemas::client_dll::CSkeletonInstance::m_modelState + schemas::client_dll::CGameSceneNode::m_vecOrigin);

				//std::string pawn_name;
				//const uintptr_t pawn_name_address = myDriver.read_memory<uintptr_t>(entry_controller + schemas::client_dll::CCSPlayerController::m_sSanitizedPlayerName);
				//if (pawn_name_address) {
				//	char buf[260] = {};
				//	myDriver.read_memory_size(pawn_name_address, buf, 260);
				//	pawn_name = std::string(buf);
				//}
				//else {
				//	pawn_name = "unknow";
				//}
				// Log(pawn_name);


				Vec3 head_pos = myDriver.read_memory<Vec3>(bone_array_address + 6ULL * 32);

				float distance = 0;

				Vec3 OppPos;

				OppPos = head_pos - local_pos;
				distance = sqrt(pow(OppPos.x, 2) + pow(OppPos.y, 2));

				float yaw = 0;
				float pitch = 0;

				yaw = atan2(OppPos.y, OppPos.x) * 180 / M_PI;
				pitch = -atan2(OppPos.z, distance) * 180 / M_PI;

				if (yaw < -180)
					yaw += 360;
				else if (yaw > 180)
					yaw -= 360;

				if (pitch < -89)
					pitch = -89;
				else if (pitch > 89)
					pitch = 89;

				if (std::abs(yaw - view_angle.y) > 50 || std::abs(pitch - view_angle.x) > 50)
					continue;

				if (distance < nearest) {
					nearest = distance;
					target_angle = { pitch, yaw };
				}
			}

			//static float roll = 0.f;
			//static bool right = true;


			if (GetAsyncKeyState(VK_XBUTTON1) && nearest != 999999999.f) {
				//if (right)
				//	roll += 0.1;
				//else
				//	roll -= 0.1;

				//if (roll >= 50)
				//	right = false;
				//else if (roll <= -50)
				//	right = true;


				//if (ShotsFired > RCSBullet)
				//{
				//	Vec2 PunchAngle;
				//	if (AimPunchCache.Count <= 0 && AimPunchCache.Count > 0xFFFF)
				//		return;
				//	PunchAngle = driver.readv<Vec2>(AimPunchCache.Data + (AimPunchCache.Count - 1) * sizeof(Vector3));

				//	Yaw = Yaw - PunchAngle.y * RCSScale.x;
				//	Pitch = Pitch - PunchAngle.x * RCSScale.y;
				//}

				Vec2 aim_angle = (target_angle - view_angle) / 50;
				//myDriver.write_memory(view_angle_addr, view_angle + aim_angle);


				static Vec2 old_punch = {};

				C_UTL_VECTOR aim_punch_cache = myDriver.read_memory<C_UTL_VECTOR>(local_player_pawn + schemas::client_dll::C_CSPlayerPawn::m_aimPunchCache);

				Vec2 aim_punch_angle = myDriver.read_memory<Vec2>(aim_punch_cache.data + (aim_punch_cache.count - 1) * sizeof(Vec3));

				if (myDriver.read_memory<int>(local_player_pawn + schemas::client_dll::C_CSPlayerPawnBase::m_iShotsFired) > 1) {
					aim_angle.x = old_punch.x - aim_punch_angle.x * 2.f;
					aim_angle.y = old_punch.y - aim_punch_angle.y * 2.f;
				}

				old_punch.x = aim_punch_angle.x * 2.f;
				old_punch.y = aim_punch_angle.y * 2.f;



				myDriver.write_memory(view_angle_addr, view_angle + aim_angle);
			}

		}
	}
	else {
		std::cout << "[-] Failed to find client.dll " << GetLastError() << "\n";
	}



	std::cout << "[*] Process stopped. Press any key to close this window.\n";
	std::cin.get();
	return 0;
}