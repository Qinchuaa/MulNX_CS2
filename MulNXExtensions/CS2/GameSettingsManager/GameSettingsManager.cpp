#include "GameSettingsManager.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>

bool GameSettingsManager::UINodeFunc(MulNXUINode* ThisNode) {
    if (ImGui::Button("一键修复数字切人bug")) {
        this->AL3D->ExecuteCommand("unbind 1");
        this->AL3D->ExecuteCommand("unbind 2");
        this->AL3D->ExecuteCommand("unbind 3");
        this->AL3D->ExecuteCommand("unbind 4");
        this->AL3D->ExecuteCommand("unbind 5");
        this->AL3D->ExecuteCommand("unbind 6");
        this->AL3D->ExecuteCommand("unbind 7");
        this->AL3D->ExecuteCommand("unbind 8");
        this->AL3D->ExecuteCommand("unbind 9");
        this->AL3D->ExecuteCommand("unbind 0");


        this->AL3D->ExecuteCommand("bind 1 spec_player 1");
        this->AL3D->ExecuteCommand("bind 2 spec_player 2");
        this->AL3D->ExecuteCommand("bind 3 spec_player 3");
        this->AL3D->ExecuteCommand("bind 4 spec_player 4");
        this->AL3D->ExecuteCommand("bind 5 spec_player 5");
        this->AL3D->ExecuteCommand("bind 6 spec_player 6");
        this->AL3D->ExecuteCommand("bind 7 spec_player 7");
        this->AL3D->ExecuteCommand("bind 8 spec_player 8");
        this->AL3D->ExecuteCommand("bind 9 spec_player 9");
        this->AL3D->ExecuteCommand("bind 0 spec_player 10");
    }

    ImGui::Checkbox("作弊模式", this->sv_cheats);

    this->SettingGraphFloat("游戏速度", this->GameSettings.host_timescale, 0.001f, 10.000f);

    if (ImGui::CollapsingHeader("画面设置")) {
        this->SettingGraphInt("FPS上限", this->GameSettings.fps_max, 0, 1000);

        if (ImGui::TreeNode("UI设置")) {
            ImGui::Checkbox("显示HUD", this->GameSettings.cl_drawhud);
            ImGui::Checkbox("只渲染击杀信息", this->GameSettings.cl_draw_only_deathnotices);
            ImGui::Checkbox("强制雷达渲染", this->GameSettings.cl_drawhud_force_radar);
            ImGui::SliderInt("展示FPS", this->GameSettings.cl_showfps, 0, 3, "%d");
            ImGui::SliderInt("展示Tick", this->GameSettings.cl_showtick, 0, 3, "%d");
            ImGui::SliderInt("TrueView控制", this->GameSettings.cl_trueview_show_status, 0, 2);
            if (ImGui::Button("切换Demo进度条UI显示")) {
                this->AL3D->ExecuteCommand("demoui");
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("渲染设置")) {
            ImGui::SliderInt("X光", this->GameSettings.ScreenSettings.spec_show_xray, 0, 100);
            static bool bESP = this->GameSettings.ScreenSettings.ESPBox.load();
            ImGui::Checkbox("方框透视", &bESP);
            this->GameSettings.ScreenSettings.ESPBox = bESP;
            if (ImGui::TreeNode("景深控制")) {

                ImGui::Checkbox("启用景深", this->dof.r_dof_override);

                bool DOFChange = false;
                DOFChange |= ImGui::SliderFloat("聚焦距离", &this->dof.FocusDistance, 0, 5000);
                DOFChange |= ImGui::SliderFloat("清晰半径", &this->dof.CrispRadius, 0, 5000);
                DOFChange |= ImGui::SliderFloat("模糊距离", &this->dof.BlurDistance, 0, 5000);
                if (DOFChange) {
                    MulNX::Math::DOFParam Param;
                    MulNX::Math::CalculateDOFParameters(this->dof.FocusDistance, this->dof.CrispRadius, this->dof.BlurDistance, Param);

                    *this->dof.r_dof_override_near_blurry = Param.NearBlurry;// 近模糊
                    *this->dof.r_dof_override_near_crisp = Param.NearCrisp;// 近清晰
                    *this->dof.r_dof_override_far_crisp = Param.FarCrisp;// 远清晰
                    *this->dof.r_dof_override_far_blurry = Param.FarBlurry;// 远模糊
                }

                ImGui::Separator();

                ImGui::SliderFloat("r_dof_override_far_blurry", this->dof.r_dof_override_far_blurry, 0, 5000);
                ImGui::SliderFloat("r_dof_override_far_crisp", this->dof.r_dof_override_far_crisp, 0, 5000);
                ImGui::SliderFloat("r_dof_override_near_crisp", this->dof.r_dof_override_near_crisp, 0, 5000);
                ImGui::SliderFloat("r_dof_override_near_blurry", this->dof.r_dof_override_near_blurry, 0, 5000);

                ImGui::SliderFloat("r_dof_override_tilt_to_ground", this->dof.r_dof_override_tilt_to_ground, 0, 5000);

                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
    }

    if (ImGui::CollapsingHeader("声音设置")) {
        ImGui::Checkbox("游戏窗口失去焦点时静音", this->GameSettings.SoundSettings.snd_mute_losefocus);

        if (ImGui::TreeNode("音乐设置")) {
            ImGui::SliderFloat("主菜单音量", this->GameSettings.SoundSettings.snd_menumusic_volume, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("回合开始音量", this->GameSettings.SoundSettings.snd_roundstart_volume, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("回合开始行动音量", this->GameSettings.SoundSettings.snd_roundaction_volume, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("回合结束音量", this->GameSettings.SoundSettings.snd_roundend_volume, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("MVP音量", this->GameSettings.SoundSettings.snd_mvp_volume, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("炸弹/人质音量", this->GameSettings.SoundSettings.snd_mapobjective_volume, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("十秒警告音量", this->GameSettings.SoundSettings.snd_tensecondwarning_volume, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("死亡视角音量", this->GameSettings.SoundSettings.snd_deathcamera_volume, 0.0f, 1.0f, "%.2f");
            ImGui::Checkbox("当双方团队成员都存活时关闭MVP音乐", this->GameSettings.SoundSettings.snd_mute_mvp_music_live_players);

            ImGui::TreePop();
        }
        if (ImGui::TreeNode("音效设置")) {

            ImGui::TreePop();
        }
    }

    return true;
}

bool GameSettingsManager::Init() {
	this->CS = this->Core->ModuleManager()->FindModule<CSController>("CSController");
	C_ConVarSystem& CVarSystem = this->CS->GetCvarSystem();

    this->GameSettings.ScreenSettings.spec_show_xray = CVarSystem.GetCvar("spec_show_xray")->GetPtr<int>();
    
    this->dof.r_dof_override = CVarSystem.GetCvar("r_dof_override")->GetPtr<bool>();
	this->dof.r_dof_override_far_blurry = CVarSystem.GetCvar("r_dof_override_far_blurry")->GetPtr<float>();
	this->dof.r_dof_override_far_crisp = CVarSystem.GetCvar("r_dof_override_far_crisp")->GetPtr<float>();
	this->dof.r_dof_override_near_blurry = CVarSystem.GetCvar("r_dof_override_near_blurry")->GetPtr<float>();
	this->dof.r_dof_override_near_crisp = CVarSystem.GetCvar("r_dof_override_near_crisp")->GetPtr<float>();
	this->dof.r_dof_override_tilt_to_ground = CVarSystem.GetCvar("r_dof_override_tilt_to_ground")->GetPtr<float>();

	this->GameSettings.cl_drawhud = CVarSystem.GetCvar("cl_drawhud")->GetPtr<bool>();
	this->GameSettings.cl_draw_only_deathnotices = CVarSystem.GetCvar("cl_draw_only_deathnotices")->GetPtr<bool>();
	this->GameSettings.cl_drawhud_force_radar = CVarSystem.GetCvar("cl_drawhud_force_radar")->GetPtr<bool>();
	this->GameSettings.cl_showfps = CVarSystem.GetCvar("cl_showfps")->GetPtr<int>();
	this->GameSettings.cl_showtick = CVarSystem.GetCvar("cl_showtick")->GetPtr<int>();
	this->GameSettings.cl_trueview_show_status = CVarSystem.GetCvar("cl_trueview_show_status")->GetPtr<int>();
	this->GameSettings.host_timescale = CVarSystem.GetCvar("host_timescale")->GetPtr<float>();
	this->GameSettings.fps_max = CVarSystem.GetCvar("fps_max")->GetPtr<int>();

    this->sv_cheats = CVarSystem.GetCvar("sv_cheats")->GetPtr<bool>();

    this->GameSettings.SoundSettings.snd_mute_losefocus = CVarSystem.GetCvar("snd_mute_losefocus")->GetPtr<bool>();
    this->GameSettings.SoundSettings.snd_menumusic_volume = CVarSystem.GetCvar("snd_menumusic_volume")->GetPtr<float>();
    this->GameSettings.SoundSettings.snd_roundstart_volume = CVarSystem.GetCvar("snd_roundstart_volume")->GetPtr<float>();
    this->GameSettings.SoundSettings.snd_roundaction_volume = CVarSystem.GetCvar("snd_roundaction_volume")->GetPtr<float>();
    this->GameSettings.SoundSettings.snd_roundend_volume = CVarSystem.GetCvar("snd_roundend_volume")->GetPtr<float>();
    this->GameSettings.SoundSettings.snd_mvp_volume = CVarSystem.GetCvar("snd_mvp_volume")->GetPtr<float>();
    this->GameSettings.SoundSettings.snd_mapobjective_volume = CVarSystem.GetCvar("snd_mapobjective_volume")->GetPtr<float>();
    this->GameSettings.SoundSettings.snd_tensecondwarning_volume = CVarSystem.GetCvar("snd_tensecondwarning_volume")->GetPtr<float>();
    this->GameSettings.SoundSettings.snd_deathcamera_volume = CVarSystem.GetCvar("snd_deathcamera_volume")->GetPtr<float>();
    this->GameSettings.SoundSettings.snd_mute_mvp_music_live_players = CVarSystem.GetCvar("snd_mute_mvp_music_live_players")->GetPtr<bool>();

    this->NeedUINode = true;
    return true;
}

void GameSettingsManager::SettingGraphFloat(const char* Label, float* V, const float& min, const float& max) {
	ImGui::SliderFloat(Label, V, min, max, "%.3f");
	if (ImGui::Button("-1"))*V -= 1; ImGui::SameLine();
	if (ImGui::Button("-0.1"))*V -= 0.1; ImGui::SameLine();
	if (ImGui::Button("-0.01"))*V -= 0.01; ImGui::SameLine();
	if (ImGui::Button("-0.001"))*V -= 0.001; ImGui::SameLine();
	if (ImGui::Button("+0.001"))*V += 0.001; ImGui::SameLine();
	if (ImGui::Button("+0.01"))*V += 0.01; ImGui::SameLine();
	if (ImGui::Button("+0.1"))*V += 0.1; ImGui::SameLine();
	if (ImGui::Button("+1"))*V += 1;

	if (ImGui::Button("0.1"))*V = 0.1; ImGui::SameLine();
	if (ImGui::Button("0.25"))*V = 0.25; ImGui::SameLine();
	if (ImGui::Button("0.5"))*V = 0.5; ImGui::SameLine();
	if (ImGui::Button("1"))*V = 1; ImGui::SameLine();
	if (ImGui::Button("2"))*V = 2; ImGui::SameLine();
	if (ImGui::Button("3"))*V = 3;
}

void GameSettingsManager::SettingGraphInt(const char* Label, int* V, const int& min, const int& max) {
	ImGui::SliderInt(Label, V, min, max);
	if (ImGui::Button("-10"))*V -= 10; ImGui::SameLine();
	if (ImGui::Button("-5"))*V -= 5; ImGui::SameLine();
	if (ImGui::Button("-1"))*V -= 1; ImGui::SameLine();
	if (ImGui::Button("+1"))*V += 1; ImGui::SameLine();
	if (ImGui::Button("+5"))*V += 5; ImGui::SameLine();
	if (ImGui::Button("+10"))*V += 10;

	if (ImGui::Button("0"))*V = 0; ImGui::SameLine();
	if (ImGui::Button("30"))*V = 30; ImGui::SameLine();
	if (ImGui::Button("60"))*V = 60; ImGui::SameLine();
	if (ImGui::Button("90"))*V = 90; ImGui::SameLine();
	if (ImGui::Button("120"))*V = 120; ImGui::SameLine();
	if (ImGui::Button("144"))*V = 144; ImGui::SameLine();
	if (ImGui::Button("200"))*V = 200; ImGui::SameLine();
	if (ImGui::Button("240"))*V = 240;
}

void GameSettingsManager::VirtualMain() {

	if (this->GameSettings.ScreenSettings.ESPBox) {
		this->ESPDraw();
	}
}


void GameSettingsManager::ESPDraw() {
	for (int i = 1;  i <= 10; ++i) {
		const DirectX::XMFLOAT3 EyePos3D = this->AL3D->GetPlayerMsg(i).EyePosition;
		const DirectX::XMFLOAT3 OriginPos3D = this->AL3D->GetPlayerMsg(i).Position;

		DirectX::XMFLOAT2 EyePos2D{};
		DirectX::XMFLOAT2 OriginPos2D{};

		MulNX::Math::XMWorldToScreen(EyePos3D, EyePos2D, this->AL3D->GetViewMatrix(), this->AL3D->GetWinWidth(), this->AL3D->GetWinHeight());
		MulNX::Math::XMWorldToScreen(OriginPos3D, OriginPos2D, this->AL3D->GetViewMatrix(), this->AL3D->GetWinWidth(), this->AL3D->GetWinHeight());

		const float hight{ ::abs(EyePos2D.y - OriginPos2D.y) * 1.25f };
		const float width{ hight / 2.f };
		const float x = EyePos2D.x - (width / 2.f);
		const float y = EyePos2D.y - (width / 2.5f);

		ImGui::GetBackgroundDrawList()->AddRect(ImVec2(x, y), ImVec2(x + width, y + hight), ImColor(0, 255, 0, 255), 0.0f, 0, 1.5f);
	}

	return;
}