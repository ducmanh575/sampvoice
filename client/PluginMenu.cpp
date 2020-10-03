/*
    This is a SampVoice project file
    Developer: CyberMor <cyber.mor.2020@gmail.ru>

    See more here https://github.com/CyberMor/sampvoice

    Copyright (c) Daniel (CyberMor) 2020 All rights reserved
*/

#include "PluginMenu.h"

#include <cassert>

#include <samp/CChat.h>
#include <samp/CInput.h>
#include <samp/CScoreboard.h>
#include <util/ImGuiUtil.h>
#include <util/GameUtil.h>
#include <util/Logger.h>

#include "BlackList.h"
#include "SpeakerList.h"
#include "MicroIcon.h"
#include "Playback.h"
#include "Record.h"

bool PluginMenu::Init(IDirect3DDevice9* const pDevice,
                      D3DPRESENT_PARAMETERS* const pParameters,
                      const AddressesBase& const addrBase,
                      const Resource& const rShader,
                      const Resource& const rLogo,
                      const Resource& const rFont) noexcept
{
    assert(pDevice);
    assert(pParameters);

    if (PluginMenu::initStatus) return false;

    if (!ImGuiUtil::IsInited()) return false;

    try
    {
        const BYTE returnOpcode { 0xC3 };
        PluginMenu::openChatFuncPatch = MakePatch(addrBase.GetSampOpenChatFunc(),
            &returnOpcode, sizeof(returnOpcode), false);
        PluginMenu::openScoreboardFuncPatch = MakePatch(addrBase.GetSampOpenScoreboardFunc(),
            &returnOpcode, sizeof(returnOpcode), false);
        PluginMenu::switchModeFuncPatch = MakePatch(addrBase.GetSampSwitchModeFunc(),
            &returnOpcode, sizeof(returnOpcode), false);
    }
    catch (const std::exception& exception)
    {
        Logger::LogToFile("[sv:err:pluginmenu:init] : failed to create function patches");
        PluginMenu::switchModeFuncPatch.reset();
        PluginMenu::openScoreboardFuncPatch.reset();
        PluginMenu::openChatFuncPatch.reset();
        return false;
    }

    try
    {
        PluginMenu::blurEffect = MakeBlurEffect(pDevice, pParameters, rShader);
        PluginMenu::tLogo = MakeTexture(pDevice, rLogo);
    }
    catch (const std::exception& exception)
    {
        Logger::LogToFile("[sv:err:pluginmenu:init] : failed to create resources");
        PluginMenu::tLogo.reset();
        PluginMenu::blurEffect.reset();
        PluginMenu::switchModeFuncPatch.reset();
        PluginMenu::openScoreboardFuncPatch.reset();
        PluginMenu::openChatFuncPatch.reset();
        return false;
    }

    ImGui::StyleColorsClassic();

    if (float varWindowPaddingX { 0.f }, varWindowPaddingY { 0.f };
        Render::ConvertBaseXValueToScreenXValue(kBaseMenuPaddingX, varWindowPaddingX) &&
        Render::ConvertBaseYValueToScreenYValue(kBaseMenuPaddingY, varWindowPaddingY))
    {
        ImGui::GetStyle().WindowPadding = { varWindowPaddingX, varWindowPaddingY };
    }

    if (float varFramePaddingX { 0.f }, varFramePaddingY { 0.f };
        Render::ConvertBaseXValueToScreenXValue(kBaseMenuFramePaddingX, varFramePaddingX) &&
        Render::ConvertBaseYValueToScreenYValue(kBaseMenuFramePaddingY, varFramePaddingY))
    {
        ImGui::GetStyle().FramePadding = { varFramePaddingX, varFramePaddingY };
    }

    if (float varItemSpacingX { 0.f }, varItemSpacingY { 0.f };
        Render::ConvertBaseXValueToScreenXValue(kBaseMenuItemSpacingX, varItemSpacingX) &&
        Render::ConvertBaseYValueToScreenYValue(kBaseMenuItemSpacingY, varItemSpacingY))
    {
        ImGui::GetStyle().ItemSpacing = { varItemSpacingX, varItemSpacingY };
    }

    if (float varItemInnerSpacingX { 0.f }, varItemInnerSpacingY { 0.f };
        Render::ConvertBaseXValueToScreenXValue(kBaseMenuItemInnerSpacingX, varItemInnerSpacingX) &&
        Render::ConvertBaseYValueToScreenYValue(kBaseMenuItemInnerSpacingY, varItemInnerSpacingY))
    {
        ImGui::GetStyle().ItemInnerSpacing = { varItemInnerSpacingX, varItemInnerSpacingY };
    }

    if (float varRounding { 0.f }; Render::ConvertBaseXValueToScreenXValue(kBaseMenuRounding, varRounding))
    {
        ImGui::GetStyle().WindowRounding = 0.0f;
        ImGui::GetStyle().FrameRounding = varRounding;
        ImGui::GetStyle().ScrollbarRounding = varRounding;
        ImGui::GetStyle().ChildRounding = varRounding;
        ImGui::GetStyle().PopupRounding = varRounding;
        ImGui::GetStyle().GrabRounding = varRounding;
        ImGui::GetStyle().TabRounding = varRounding;
    }

    ImGui::GetStyle().WindowBorderSize = 0.0f;
    ImGui::GetStyle().FrameBorderSize = 0.0f;
    ImGui::GetStyle().ChildBorderSize = 0.0f;
    ImGui::GetStyle().PopupBorderSize = 0.0f;
    ImGui::GetStyle().TabBorderSize = 0.0f;
    ImGui::GetStyle().AntiAliasedFill = true;
    ImGui::GetStyle().AntiAliasedLines = true;
    ImGui::GetStyle().GrabMinSize = 20.0f;
    ImGui::GetStyle().ScrollbarSize = 10.0f;
    ImGui::GetStyle().Colors[ImGuiCol_Text] = { 0.94f, 0.94f, 0.94f, 1.f };
    ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = { 0.3f, 0.3f, 0.3f, 0.2f };
    ImGui::GetStyle().Colors[ImGuiCol_FrameBgHovered] = { 0.4f, 0.4f, 0.4f, 0.4f };
    ImGui::GetStyle().Colors[ImGuiCol_FrameBgActive] = { 0.5f, 0.5f, 0.5f, 0.6f };
    ImGui::GetStyle().Colors[ImGuiCol_Button] = { 0.3f, 0.3f, 0.3f, 0.2f };
    ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = { 0.4f, 0.4f, 0.4f, 0.4f };
    ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = { 0.5f, 0.5f, 0.5f, 0.6f };
    ImGui::GetStyle().Colors[ImGuiCol_SliderGrab] = { 0.3f, 0.3f, 0.3f, 0.2f };
    ImGui::GetStyle().Colors[ImGuiCol_SliderGrabActive] = { 0.5f, 0.5f, 0.5f, 0.6f };

    ImGui::GetIO().Fonts->AddFontDefault();

    float varFontSize { 0.f };

    if (!Render::ConvertBaseYValueToScreenYValue(kBaseFontTitleSize, varFontSize) ||
        !(PluginMenu::pTitleFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(rFont.GetDataPtr(),
            rFont.GetDataSize(), varFontSize, NULL, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic())))
    {
        Logger::LogToFile("[sv:err:pluginmenu:init] : failed to create title font");
        PluginMenu::tLogo.reset();
        PluginMenu::blurEffect.reset();
        PluginMenu::switchModeFuncPatch.reset();
        PluginMenu::openScoreboardFuncPatch.reset();
        PluginMenu::openChatFuncPatch.reset();
        return false;
    }

    if (!Render::ConvertBaseYValueToScreenYValue(kBaseFontTabSize, varFontSize) ||
        !(PluginMenu::pTabFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(rFont.GetDataPtr(),
            rFont.GetDataSize(), varFontSize, NULL, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic())))
    {
        Logger::LogToFile("[sv:err:pluginmenu:init] : failed to create tab font");
        delete PluginMenu::pTitleFont;
        PluginMenu::tLogo.reset();
        PluginMenu::blurEffect.reset();
        PluginMenu::switchModeFuncPatch.reset();
        PluginMenu::openScoreboardFuncPatch.reset();
        PluginMenu::openChatFuncPatch.reset();
        return false;
    }

    if (!Render::ConvertBaseYValueToScreenYValue(kBaseFontDescSize, varFontSize) ||
        !(PluginMenu::pDescFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(rFont.GetDataPtr(),
            rFont.GetDataSize(), varFontSize, NULL, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic())))
    {
        Logger::LogToFile("[sv:err:pluginmenu:init] : failed to create description font");
        delete PluginMenu::pTabFont;
        delete PluginMenu::pTitleFont;
        PluginMenu::tLogo.reset();
        PluginMenu::blurEffect.reset();
        PluginMenu::switchModeFuncPatch.reset();
        PluginMenu::openScoreboardFuncPatch.reset();
        PluginMenu::openChatFuncPatch.reset();
        return false;
    }

    if (!Render::ConvertBaseYValueToScreenYValue(kBaseFontSize, varFontSize) ||
        !(PluginMenu::pDefFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(rFont.GetDataPtr(),
            rFont.GetDataSize(), varFontSize, NULL, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic())))
    {
        Logger::LogToFile("[sv:err:pluginmenu:init] : failed to create default font");
        delete PluginMenu::pDescFont;
        delete PluginMenu::pTabFont;
        delete PluginMenu::pTitleFont;
        PluginMenu::tLogo.reset();
        PluginMenu::blurEffect.reset();
        PluginMenu::switchModeFuncPatch.reset();
        PluginMenu::openScoreboardFuncPatch.reset();
        PluginMenu::openChatFuncPatch.reset();
        return false;
    }

    PluginMenu::blurLevel = 0;
    PluginMenu::blurLevelDeviation = 0;
    PluginMenu::showStatus = false;

    PluginMenu::initStatus = true;
    PluginMenu::SyncOptions();

    return true;
}

void PluginMenu::Free() noexcept
{
    PluginMenu::Hide();

    if (!PluginMenu::initStatus) return;

    PluginMenu::tLogo.reset();
    PluginMenu::blurEffect.reset();

    delete PluginMenu::pTitleFont;
    delete PluginMenu::pTabFont;
    delete PluginMenu::pDescFont;
    delete PluginMenu::pDefFont;

    PluginMenu::openChatFuncPatch.reset();
    PluginMenu::openScoreboardFuncPatch.reset();
    PluginMenu::switchModeFuncPatch.reset();

    PluginMenu::prevChatMode = SAMP::CChat::Normal;
    PluginMenu::blurLevelDeviation = 0.f;
    PluginMenu::blurLevel = 0.f;

    PluginMenu::bMicroMovement = false;

    PluginMenu::initStatus = false;
}

void PluginMenu::Render() noexcept
{
    if (!PluginMenu::initStatus) return;

    if (PluginMenu::blurLevel > 0.f)
    {
        PluginMenu::blurEffect->Render(PluginMenu::blurLevel);
    }

    if (!PluginMenu::showStatus) return;

    float vWindowWidth, vWindowHeight;

    if (!Render::ConvertBaseXValueToScreenXValue(kBaseMenuWidth, vWindowWidth)) return;
    if (!Render::ConvertBaseYValueToScreenYValue(kBaseMenuHeight, vWindowHeight)) return;

    float vTabWidth, vTabHeight;

    if (!Render::ConvertBaseXValueToScreenXValue(kBaseTabWidth, vTabWidth)) return;
    if (!Render::ConvertBaseYValueToScreenYValue(kBaseTabHeight, vTabHeight)) return;

    Samp::ToggleSampCursor(2);

    if (!ImGuiUtil::RenderBegin()) return;

    ImGui::SetNextWindowSize({ vWindowWidth, vWindowHeight });
    ImGui::SetNextWindowPosCenter();

    if (ImGui::Begin("configWindow", nullptr,
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoResize))
    {
        // Title rendering...
        // -------------------------------

        ImGui::PushFont(PluginMenu::pTitleFont);

        ImGui::Text(kTitleText);

        ImGui::SameLine(ImGui::GetWindowWidth() - (4 * ImGui::CalcTextSize(kTitleText).y + (vTabWidth -
            4 * ImGui::CalcTextSize(kTitleText).y) / 2.f + ImGui::GetStyle().WindowPadding.x));

        ImGui::Image(PluginMenu::tLogo->GetTexture(), ImVec2(4 * ImGui::CalcTextSize(kTitleText).y, ImGui::CalcTextSize(kTitleText).y));

        ImGui::PopFont();
        ImGui::NewLine();

        // Tabs rendering...
        // -------------------------------

        ImGui::PushFont(PluginMenu::pTabFont);

        if (ImGui::Button(kTab1TitleText, { vTabWidth, vTabHeight })) PluginMenu::iSelectedMenu = 0;
        ImGui::SameLine(ImGui::GetStyle().WindowPadding.x + vTabWidth + kBaseTabPadding);
        if (ImGui::Button(kTab2TitleText, { vTabWidth, vTabHeight })) PluginMenu::iSelectedMenu = 1;
        ImGui::SameLine(ImGui::GetStyle().WindowPadding.x + 2 * (vTabWidth + kBaseTabPadding));
        if (ImGui::Button(kTab3TitleText, { vTabWidth, vTabHeight })) PluginMenu::iSelectedMenu = 2;

        ImGui::PopFont();

        // Description rendering...
        // -------------------------------

        ImGui::PushFont(PluginMenu::pDefFont);

        switch (PluginMenu::iSelectedMenu)
        {
            case 0:
            {
                // Common settings rendering...
                // -------------------------------

                ImGui::NewLine();
                ImGui::PushFont(PluginMenu::pDescFont);
                ImGui::Text(kTab1Desc1TitleText);
                ImGui::Separator();
                ImGui::PopFont();
                ImGui::NewLine();

                if (ImGui::Checkbox(kTab1Desc1EnableSoundText, &PluginMenu::soundEnable))
                {
                    // Enabling/Disabling sound
                    Playback::SetSoundEnable(PluginMenu::soundEnable);
                }

                if (PluginMenu::soundEnable)
                {
                    if (ImGui::SliderInt(kTab1Desc1VolumeSoundText, &PluginMenu::soundVolume, 0, 100))
                    {
                        // Setting volume
                        Playback::SetSoundVolume(PluginMenu::soundVolume);
                    }
                }

                ImGui::NewLine();
                ImGui::PushFont(PluginMenu::pDescFont);
                ImGui::Text(kTab1Desc2TitleText);
                ImGui::Separator();
                ImGui::PopFont();
                ImGui::NewLine();

                if (ImGui::Checkbox(kTab1Desc2BalancerText, &PluginMenu::soundBalancer))
                {
                    // Enabling/Disabling sound balancer
                    Playback::SetSoundBalancer(PluginMenu::soundBalancer);
                }

                if (ImGui::Checkbox(kTab1Desc2FilterText, &PluginMenu::soundFilter))
                {
                    // Enabling/Disabling sound filter
                    Playback::SetSoundFilter(PluginMenu::soundFilter);
                }

                ImGui::NewLine();
                ImGui::PushFont(PluginMenu::pDescFont);
                ImGui::Text(kTab1Desc3TitleText);
                ImGui::Separator();
                ImGui::PopFont();
                ImGui::NewLine();

                if (ImGui::SliderFloat(kTab1Desc3SpeakerIconScaleText, &PluginMenu::speakerIconScale, 0.2f, 2.0f))
                {
                    SpeakerList::SetSpeakerIconScale(PluginMenu::speakerIconScale);
                }

                if (ImGui::SliderInt(kTab1Desc3SpeakerIconOffsetXText, &PluginMenu::speakerIconOffsetX, -500, 500))
                {
                    SpeakerList::SetSpeakerIconOffsetX(PluginMenu::speakerIconOffsetX);
                }

                if (ImGui::SliderInt(kTab1Desc3SpeakerIconOffsetYText, &PluginMenu::speakerIconOffsetY, -500, 500))
                {
                    SpeakerList::SetSpeakerIconOffsetY(PluginMenu::speakerIconOffsetY);
                }

                const auto rstBtnSize = ImGui::GetItemRectSize();

                ImGui::NewLine();
                ImGui::PushFont(PluginMenu::pDescFont);
                ImGui::Text(kTab1Desc4TitleText);
                ImGui::Separator();
                ImGui::PopFont();
                ImGui::NewLine();

                if (ImGui::Button(kTab1Desc4ConfigResetText, rstBtnSize))
                {
                    SpeakerList::ResetConfigs();
                    SpeakerList::SyncConfigs();
                    Playback::ResetConfigs();
                    Playback::SyncConfigs();
                    PluginMenu::SyncOptions();
                }
            } break;
            case 1:
            {
                // Micro settings rendering...
                // -------------------------------

                ImGui::NewLine();
                ImGui::PushFont(PluginMenu::pDescFont);
                ImGui::Text(kTab2Desc1TitleText);
                ImGui::Separator();
                ImGui::PopFont();
                ImGui::NewLine();

                const auto& devList = Record::GetDeviceNamesList();

                if (!devList.empty())
                {
                    if (ImGui::Checkbox(kTab2Desc1EnableMicroText, &PluginMenu::microEnable))
                    {
                        // Enabling/Disabling microphone
                        Record::SetMicroEnable(PluginMenu::microEnable);

                        if (!PluginMenu::microEnable && PluginMenu::bCheckDevice)
                            PluginMenu::bCheckDevice = false;
                    }

                    if (PluginMenu::microEnable)
                    {
                        if (ImGui::SliderInt(kTab2Desc1MicroVolumeText, &PluginMenu::microVolume, 0, 100))
                        {
                            // Setting volume micro
                            Record::SetMicroVolume(PluginMenu::microVolume);
                        }

                        if (ImGui::BeginCombo(kTab2Desc1DeviceNameText, devList[PluginMenu::deviceIndex].c_str()))
                        {
                            for (int i = 0; i < devList.size(); ++i)
                            {
                                if (ImGui::Selectable(devList[i].c_str(), i == PluginMenu::deviceIndex))
                                    Record::SetMicroDevice(PluginMenu::deviceIndex = i);

                                if (i == PluginMenu::deviceIndex)
                                    ImGui::SetItemDefaultFocus();
                            }

                            ImGui::EndCombo();
                        }

                        if (ImGui::Checkbox(kTab2Desc1CheckDeviceText, &PluginMenu::bCheckDevice))
                        {
                            if (PluginMenu::bCheckDevice) Record::StartChecking();
                            else Record::StopChecking();
                        }
                    }
                }
                else
                {
                    ImGui::TextDisabled(kTab2Desc3MicroNotFoundText);
                }

                ImGui::NewLine();
                ImGui::PushFont(PluginMenu::pDescFont);
                ImGui::Text(kTab2Desc2TitleText);
                ImGui::Separator();
                ImGui::PopFont();
                ImGui::NewLine();

                if (ImGui::SliderFloat(kTab2Desc2MicroIconScaleText, &PluginMenu::microIconScale, 0.2f, 2.0f))
                {
                    // Setting scale micro icon
                    MicroIcon::SetMicroIconScale(PluginMenu::microIconScale);
                }

                float screenWidth, screenHeight;

                if (Render::GetScreenSize(screenWidth, screenHeight))
                {
                    if (ImGui::SliderInt(kTab2Desc2MicroIconPositionXText, &PluginMenu::microIconPositionX, 0, screenWidth))
                        MicroIcon::SetMicroIconPositionX(PluginMenu::microIconPositionX);

                    if (ImGui::SliderInt(kTab2Desc2MicroIconPositionYText, &PluginMenu::microIconPositionY, 0, screenHeight))
                        MicroIcon::SetMicroIconPositionY(PluginMenu::microIconPositionY);
                }

                if (ImGui::Button(kTab2Desc2MicroIconMoveText, ImGui::GetItemRectSize()))
                    PluginMenu::bMicroMovement = true;

                const auto rstBtnSize = ImGui::GetItemRectSize();

                ImGui::NewLine();
                ImGui::PushFont(PluginMenu::pDescFont);
                ImGui::Text(kTab1Desc4TitleText);
                ImGui::Separator();
                ImGui::PopFont();
                ImGui::NewLine();

                if (ImGui::Button(kTab1Desc4ConfigResetText, rstBtnSize))
                {
                    MicroIcon::ResetConfigs();
                    MicroIcon::SyncConfigs();
                    Record::ResetConfigs();
                    Record::SyncConfigs();
                    PluginMenu::SyncOptions();
                }
            } break;
            case 2:
            {
                // Blacklist rendering...
                // -------------------------------

                ImGui::NewLine();
                ImGui::PushFont(PluginMenu::pDescFont);
                ImGui::Text(kTab3Desc1TitleText);
                ImGui::Separator();
                ImGui::PopFont();
                ImGui::NewLine();

                ImGui::PushItemWidth(-1);
                const ImVec2 oldCurPos = ImGui::GetCursorPos();
                ImGui::InputText("##label", PluginMenu::nBuffer, sizeof(PluginMenu::nBuffer));
                if (!ImGui::IsItemActive() && *PluginMenu::nBuffer == '\0')
                {
                    const ImVec2 newCurPos = ImGui::GetCursorPos();
                    const float inputTextHeight = newCurPos.y - oldCurPos.y;
                    ImGui::SetCursorPosX(oldCurPos.x + 10.f);
                    ImGui::SetCursorPosY(oldCurPos.y + (inputTextHeight - ImGui::CalcTextSize(kTab3Desc1InputPlaceholderText).y) / 2.f);
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 0.8f), kTab3Desc1InputPlaceholderText);
                    ImGui::SetCursorPos(newCurPos);
                } ImGui::PopItemWidth();
                ImGui::NewLine();
                ImGui::Separator();
                ImGui::NewLine();

                const float listWidth = (ImGui::GetWindowWidth() - 4.f * ImGui::GetStyle().WindowPadding.x) / 2.f;

                uint16_t iPlayerId;
                if (!_snscanf_s(nBuffer, sizeof(nBuffer), "%hu", &iPlayerId))
                    iPlayerId = 0xffffu;

                ImGui::PushItemWidth(listWidth);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.f);

                // Rendering all players list...
                // ---------------------------------------

                ImGui::BeginGroup();

                ImGui::Text(kTab3Desc2PlayerListText);
                ImGui::GetWindowDrawList()->AddLine(
                    ImGui::GetCursorScreenPos(), ImVec2(
                        ImGui::GetCursorScreenPos().x + listWidth,
                        ImGui::GetCursorScreenPos().y
                    ), 0xff808080
                );

                ImGui::NewLine();

                float listHeight = (ImGui::GetWindowHeight() - ImGui::GetCursorPosY()) - ImGui::GetStyle().WindowPadding.y;

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));

                if (ImGui::BeginChildFrame(1, ImVec2(listWidth, listHeight),
                                           ImGuiWindowFlags_NoCollapse |
                                           ImGuiWindowFlags_NoFocusOnAppearing |
                                           ImGuiWindowFlags_NoTitleBar |
                                           ImGuiWindowFlags_NoSavedSettings |
                                           ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoResize))
                {
                    for (uint16_t playerId = 0; playerId < MAX_PLAYERS; ++playerId)
                    {
                        const char* playerName { nullptr };

                        if (SAMP::pNetGame()->GetPlayerPool()->IsConnected(playerId) &&
                            (playerName = SAMP::pNetGame()->GetPlayerPool()->GetName(playerId)) &&
                            (*PluginMenu::nBuffer == '\0' || ((iPlayerId != 0xffffu) ? (playerId == iPlayerId) :
                                static_cast<bool>(strstr(playerName, PluginMenu::nBuffer)))) &&
                            !BlackList::IsPlayerBlocked(playerId))
                        {
                            ImGui::PushID(playerId);
                            const ImVec2 oldCurPos = ImGui::GetCursorPos();

                            if (ImGui::Button("##label", ImVec2(listWidth, ImGui::GetFontSize() + 2.f)))
                            {
                                // Add player black list
                                BlackList::LockPlayer(playerId);
                            }

                            ImGui::SetCursorPos(ImVec2(oldCurPos.x + 5.f, oldCurPos.y + 1.f));
                            if (auto stPlayer = SAMP::pNetGame()->GetPlayerPool()->GetPlayer(playerId))
                                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(htonl(stPlayer->GetColorAsRGBA() | 0xff000000)), "(%hu) %s", playerId, playerName);
                            else ImGui::Text("(%hu) %s", playerId, playerName);
                            ImGui::PopID();
                        }
                    }

                    ImGui::EndChildFrame();
                }

                ImGui::PopStyleVar();
                ImGui::PopStyleVar();

                ImGui::EndGroup();

                // ---------------------------------------

                ImGui::SameLine(listWidth + 3.f * ImGui::GetStyle().WindowPadding.x);

                // Rendering blocked players list...
                // ---------------------------------------

                ImGui::BeginGroup();

                ImGui::Text(kTab3Desc3BlackListText);
                ImGui::GetWindowDrawList()->AddLine(
                    ImGui::GetCursorScreenPos(), ImVec2(
                        ImGui::GetCursorScreenPos().x + listWidth,
                        ImGui::GetCursorScreenPos().y
                    ), 0xff808080
                );

                ImGui::NewLine();

                listHeight = (ImGui::GetWindowHeight() - ImGui::GetCursorPosY()) - ImGui::GetStyle().WindowPadding.y;

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.f, 0.f });
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });

                if (ImGui::BeginChildFrame(2, { listWidth, listHeight },
                                           ImGuiWindowFlags_NoCollapse |
                                           ImGuiWindowFlags_NoFocusOnAppearing |
                                           ImGuiWindowFlags_NoTitleBar |
                                           ImGuiWindowFlags_NoSavedSettings |
                                           ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoResize))
                {
                    const auto& blackList = BlackList::RequestBlackList();

                    for (const auto& playerInfo : blackList)
                    {
                        if (!(*nBuffer == '\0' || ((iPlayerId != 0xffffu) ? (playerInfo.playerId == iPlayerId) :
                            static_cast<bool>(strstr(playerInfo.playerName.c_str(), nBuffer))))) continue;

                        const ImVec2 oldCurPos = ImGui::GetCursorPos();
                        const ImVec2 oldCurScreenPos = ImGui::GetCursorScreenPos();

                        ImGui::PushID(&playerInfo);

                        if (ImGui::Button("##label", ImVec2(listWidth, ImGui::GetFontSize() + 2.f)))
                        {
                            // Remove player from black list
                            BlackList::UnlockPlayer(playerInfo.playerName);
                        }

                        ImGui::PopID();

                        ImGui::SetCursorPos(ImVec2(oldCurPos.x + 5.f, oldCurPos.y + 1.f));

                        if (playerInfo.playerId != SV::kNonePlayer)
                        {
                            if (auto stPlayer = SAMP::pNetGame()->GetPlayerPool()->GetPlayer(playerInfo.playerId))
                                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(htonl(stPlayer->GetColorAsRGBA())),
                                    "%s (%hu)", playerInfo.playerName.c_str(), playerInfo.playerId);
                            else ImGui::Text("%s (%hu)", playerInfo.playerName.c_str(), playerInfo.playerId);
                        }
                        else ImGui::TextDisabled("%s", playerInfo.playerName.c_str());

                        const ImVec2 cPos = ImVec2(
                            (oldCurScreenPos.x + listWidth) - (ImGui::GetFontSize() / 2.f + 2.f + ImGui::GetStyle().ScrollbarSize),
                            oldCurScreenPos.y + (ImGui::GetFontSize() / 2.f + 1.f)
                        );

                        if (playerInfo.playerId != SV::kNonePlayer)
                            ImGui::GetWindowDrawList()->AddCircleFilled(cPos, ImGui::GetFontSize() / 4.f, 0xff7dfe3f);
                        else ImGui::GetWindowDrawList()->AddCircle(cPos, ImGui::GetFontSize() / 4.f, 0xff808080);
                    }

                    ImGui::EndChildFrame();
                }

                ImGui::PopStyleVar();
                ImGui::PopStyleVar();

                ImGui::EndGroup();

                // ---------------------------------------

                ImGui::PopStyleVar();
                ImGui::PopItemWidth();
            } break;
        }

        ImGui::PopFont();
        ImGui::End();

        ImGuiUtil::RenderEnd();
    }
}

void PluginMenu::Update() noexcept
{
    if (PluginMenu::blurLevelDeviation != 0.f)
    {
        PluginMenu::blurLevel += PluginMenu::blurLevelDeviation;

        if (PluginMenu::blurLevel > 100.f)
        {
            PluginMenu::blurLevelDeviation = 0.f;
            PluginMenu::blurLevel = 100.f;
        }
        else if (PluginMenu::blurLevel < 0.f)
        {
            PluginMenu::blurLevelDeviation = 0.f;
            PluginMenu::blurLevel = 0.f;
        }
    }

    if (PluginMenu::bMicroMovement)
    {
        static bool movementInitStatus { false };
        static int oldMousePosX { 0 }, oldMousePosY { 0 };

        if (!movementInitStatus)
        {
            PluginMenu::Hide();

            oldMousePosX = MicroIcon::GetMicroIconPositionX();
            oldMousePosY = MicroIcon::GetMicroIconPositionY();

            movementInitStatus = true;
        }

        Samp::ToggleSampCursor(2);

        const ImVec2 mousePosition = ImGui::GetMousePos();

        MicroIcon::SetMicroIconPosition(mousePosition.x, mousePosition.y);

        if (GameUtil::IsKeyDown(0x01) || GameUtil::IsKeyDown(0x02) || GameUtil::IsKeyDown(0x1B))
        {
            if (GameUtil::IsKeyDown(0x02) || GameUtil::IsKeyDown(0x1B))
            {
                MicroIcon::SetMicroIconPosition(oldMousePosX, oldMousePosY);
            }

            movementInitStatus = false;
            PluginMenu::bMicroMovement = false;

            PluginMenu::Show();
        }
    }
}

bool PluginMenu::Show() noexcept
{
    if (!PluginMenu::initStatus) return false;

    if (GameUtil::IsMenuActive()) return false;
    if (PluginMenu::showStatus) return false;

    PluginMenu::openChatFuncPatch->Enable();
    PluginMenu::openScoreboardFuncPatch->Enable();
    PluginMenu::switchModeFuncPatch->Enable();

    PluginMenu::blurLevelDeviation = kBlurLevelIncrement;

    if (const auto pChat = SAMP::pChat())
    {
        PluginMenu::prevChatMode = pChat->m_nMode;
        pChat->m_nMode = SAMP::CChat::Off;
    }

    if (const auto pScoreboard = SAMP::pScoreboard();
        pScoreboard && pScoreboard->m_bIsEnabled)
    {
        pScoreboard->m_bIsEnabled = FALSE;
        pScoreboard->Close(true);
    }

    if (const auto pInputBox = SAMP::pInputBox();
        pInputBox && pInputBox->m_bEnabled)
    {
        pInputBox->m_bEnabled = FALSE;
        pInputBox->m_szInput[0] = '\0';
        pInputBox->Close();
    }

    PluginMenu::SyncOptions();
    PluginMenu::showStatus = true;

    return true;
}

bool PluginMenu::IsShowed() noexcept
{
    return PluginMenu::showStatus;
}

void PluginMenu::Hide() noexcept
{
    if (!PluginMenu::initStatus) return;

    if (!PluginMenu::showStatus) return;

    Samp::ToggleSampCursor(0);

    PluginMenu::blurLevelDeviation = kBlurLevelDecrement;

    if (const auto pChat = SAMP::pChat())
    {
        pChat->m_nMode = PluginMenu::prevChatMode;
    }

    PluginMenu::switchModeFuncPatch->Disable();
    PluginMenu::openScoreboardFuncPatch->Disable();
    PluginMenu::openChatFuncPatch->Disable();

    ImGui::SetWindowFocus(nullptr);

    *PluginMenu::nBuffer = '\0';
    PluginMenu::bCheckDevice = false;
    Record::StopChecking();

    PluginMenu::showStatus = false;
}

LRESULT PluginMenu::OnWndMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    if (!PluginMenu::initStatus) return FALSE;

    if (uMsg == WM_KEYDOWN && (uint8_t)(wParam) == 0x7A)
    {
        if (!PluginMenu::Show()) PluginMenu::Hide();
        return TRUE;
    }

    if (!PluginMenu::showStatus && !PluginMenu::bMicroMovement)
        return FALSE;

    if (uMsg == WM_KEYDOWN && (uint8_t)(wParam) == 0x1B)
    {
        PluginMenu::Hide();
        return TRUE;
    }

    return ImGuiUtil::OnWndMessage(hWnd, uMsg, wParam, lParam);
}

void PluginMenu::SyncOptions() noexcept
{
    PluginMenu::soundEnable = Playback::GetSoundEnable();
    PluginMenu::soundVolume = Playback::GetSoundVolume();
    PluginMenu::soundBalancer = Playback::GetSoundBalancer();
    PluginMenu::soundFilter = Playback::GetSoundFilter();

    PluginMenu::speakerIconScale = SpeakerList::GetSpeakerIconScale();
    PluginMenu::speakerIconOffsetX = SpeakerList::GetSpeakerIconOffsetX();
    PluginMenu::speakerIconOffsetY = SpeakerList::GetSpeakerIconOffsetY();

    PluginMenu::microEnable = Record::GetMicroEnable();
    PluginMenu::microVolume = Record::GetMicroVolume();
    PluginMenu::deviceIndex = Record::GetMicroDevice();

    PluginMenu::microIconScale = MicroIcon::GetMicroIconScale();
    PluginMenu::microIconPositionX = MicroIcon::GetMicroIconPositionX();
    PluginMenu::microIconPositionY = MicroIcon::GetMicroIconPositionY();
    PluginMenu::microIconColor = MicroIcon::GetMicroIconColor();
    PluginMenu::microIconAngle = MicroIcon::GetMicroIconAngle();
}

bool PluginMenu::initStatus { false };
bool PluginMenu::showStatus { false };

float PluginMenu::blurLevel { 0.f };
float PluginMenu::blurLevelDeviation { 0.f };
BlurEffectPtr PluginMenu::blurEffect { nullptr };

TexturePtr PluginMenu::tLogo { nullptr };

ImFont* PluginMenu::pTitleFont { nullptr };
ImFont* PluginMenu::pTabFont { nullptr };
ImFont* PluginMenu::pDescFont { nullptr };
ImFont* PluginMenu::pDefFont { nullptr };

Memory::PatchPtr PluginMenu::openChatFuncPatch { nullptr };
Memory::PatchPtr PluginMenu::openScoreboardFuncPatch { nullptr };
Memory::PatchPtr PluginMenu::switchModeFuncPatch { nullptr };

int PluginMenu::prevChatMode { 0 };

bool PluginMenu::soundEnable { false };
int PluginMenu::soundVolume { 0 };
bool PluginMenu::soundBalancer { false };
bool PluginMenu::soundFilter { false };
float PluginMenu::speakerIconScale { 0.f };
int PluginMenu::speakerIconOffsetX { 0 };
int PluginMenu::speakerIconOffsetY { 0 };

bool PluginMenu::microEnable { false };
int PluginMenu::microVolume { 0 };
int PluginMenu::deviceIndex { 0 };
float PluginMenu::microIconScale { 0.f };
int PluginMenu::microIconPositionX { 0 };
int PluginMenu::microIconPositionY { 0 };
D3DCOLOR PluginMenu::microIconColor { 0 };
float PluginMenu::microIconAngle { 0.f };

int PluginMenu::iSelectedMenu { 0 };
bool PluginMenu::bCheckDevice { false };
bool PluginMenu::bMicroMovement { false };
char PluginMenu::nBuffer[64] {};
