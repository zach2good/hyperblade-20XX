#include "ui/scenes.h"
#include "app/gameapp.h"
#include "app/window.h"
#include "common/path.h"
#include "engine/cameraobject.h"
#include "engine/cameraprojectionstate.h"
#include "engine/engineobject.h"
#include "engine/framepacing.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"
#include "globals.h"
#include "graphics/gdi.h"
#include "graphics/meshblock.h"
#include "graphics/meshbone.h"
#include "graphics/presenter.h"
#include "graphics/renderer.h"
#include "graphics/transparentqueue.h"
#include "math/mathops.h"
#include "physics/collisionellipsoid.h"
#include "sound/audiomgr.h"
#include "ui/debugmenu.h"
#include "ui/originalui.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>
#include <imgui.h>
#include "stb_image.h"

namespace
{

using Scenes::Scene;

// The scene on screen, and the one asked for, which Frame switches to outside any ImGui frame.
Scene s_scene = Scene::None;
Scene s_next  = Scene::None;

// A scene's window in the Windows 95 look, where closing the window closes the scene. End it with
// ImGui::End.
void BeginSceneWindow(const char* pszTitle, ImGuiWindowFlags flags)
{
    ImGui::Begin(pszTitle, nullptr, flags | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);

    if (!OriginalUI::DrawWindowFrame(pszTitle))
    {
        Scenes::Open(Scene::None);
    }
}

bool ContainsNoCase(const std::string& strText, const char* pszPart)
{
    const auto it = std::search(
        strText.begin(),
        strText.end(),
        pszPart,
        pszPart + std::strlen(pszPart),
        [](char a, char b)
        {
            return SDL_tolower(static_cast<unsigned char>(a)) == SDL_tolower(static_cast<unsigned char>(b));
        });

    return it != strText.end();
}

// Model viewer. It shows one of the engine's objects alone, with the main camera circling it, and
// puts the camera back afterwards so a match carries on from where it was.

struct Orbit
{
    float flYaw   = 0.6f;
    float flPitch = 0.25f;
    float flZoom  = 1.0f;
};

// By slot, which outlives the objects a new match builds.
int   s_nModelSlot = kSlotDromer0;
Orbit s_orbit;
bool  s_bTurning    = true;
float s_flTurnSpeed = 0.35f;
bool  s_bDragging   = false;

bool HasModel(const EngineObject* pObject)
{
    return pObject != nullptr && pObject->m_pBones != nullptr && pObject->GetBoneCount() > 0;
}

// The slots of every object with a model, in slot order. The drome is not in the object list.
std::vector<int> ModelSlots(GameEngine& engine)
{
    auto slots = std::vector<int>();

    for (short i = 0; i < engine.m_nObjectCount; i++)
    {
        if (HasModel(engine.m_ppObjects[i]))
        {
            slots.push_back(engine.m_ppObjects[i]->GetSlot());
        }
    }

    if (HasModel(engine.m_ppSlots[kSlotDrome]))
    {
        slots.push_back(kSlotDrome);
    }

    std::sort(slots.begin(), slots.end());

    return slots;
}

struct Bounds
{
    Vec3  vCentre;
    float flRadius;
};

// The box round every vertex of the object's shown pieces, as posed. The engine's own bounding
// sphere sits too low and too small to frame a model by.
Bounds MeasureObject(const EngineObject& object)
{
    constexpr auto kFar = 1.0e9f;

    auto vMin = Vec3{ kFar, kFar, kFar };
    auto vMax = Vec3{ -kFar, -kFar, -kFar };

    for (short i = 0; i < object.GetBoneCount(); i++)
    {
        const auto& bone = object.m_pBones[i];

        if (bone.IsBound() == 0 || bone.IsHidden() != 0 || bone.m_pMesh == nullptr)
        {
            continue;
        }

        const auto& a         = bone.m_world.m_a;
        const auto* pVertices = bone.m_pMesh->Vertices();

        for (short n = 0; n < bone.m_pMesh->nVertexCount; n++)
        {
            const auto& v = pVertices[n];

            const auto vWorld = Vec3{ a[0][0] * v.x + a[0][1] * v.y + a[0][2] * v.z + a[0][3],
                                      a[1][0] * v.x + a[1][1] * v.y + a[1][2] * v.z + a[1][3],
                                      a[2][0] * v.x + a[2][1] * v.y + a[2][2] * v.z + a[2][3] };

            vMin = Vec3{ (std::min)(vMin.x, vWorld.x), (std::min)(vMin.y, vWorld.y), (std::min)(vMin.z, vWorld.z) };
            vMax = Vec3{ (std::max)(vMax.x, vWorld.x), (std::max)(vMax.y, vWorld.y), (std::max)(vMax.z, vWorld.z) };
        }
    }

    if (vMin.x > vMax.x)
    {
        return Bounds{ Vec3{ 0.0f, 0.0f, 0.0f }, 10.0f };
    }

    const auto vHalf = Vec3{ (vMax.x - vMin.x) * 0.5f, (vMax.y - vMin.y) * 0.5f, (vMax.z - vMin.z) * 0.5f };

    return Bounds{ Vec3{ vMin.x + vHalf.x, vMin.y + vHalf.y, vMin.z + vHalf.z }, (std::max)(std::sqrt(vHalf.x * vHalf.x + vHalf.y * vHalf.y + vHalf.z * vHalf.z), 1.0f) };
}

Vec3 Cross(const Vec3& a, const Vec3& b)
{
    return Vec3{ a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

// The way the object stands. Dromers on the drome's curved walls lean away from the world's z, and
// the engine's bounding centre sits along this axis from the object's position.
Vec3 UpAxis(EngineObject& object)
{
    auto vCentre = Vec3{ 0.0f, 0.0f, 0.0f };

    object.ComputeWorldBoundingCentre(&vCentre);

    const auto* const pPos = object.GetPositionPtr();

    auto vUp = Vec3{ vCentre.x - pPos->x, vCentre.y - pPos->y, vCentre.z - pPos->z };

    if (!Math::Vec3Normalize(&vUp))
    {
        return Vec3{ 0.0f, 0.0f, 1.0f };
    }

    return vUp;
}

// The camera's view from the eye along the direction, built as CameraObject::ViewIt builds it.
void LookFrom(CameraObject& camera, const Vec3& vEye, const Vec3& vDir, const Vec3& vWorldUp)
{
    auto vRight = Cross(vDir, vWorldUp);

    Math::Vec3Normalize(&vRight);

    auto vUp = Vec3{ vRight.y * vDir.z - vRight.z * vDir.y, vRight.z * vDir.x - vRight.x * vDir.z, vRight.x * vDir.y - vRight.y * vDir.x };

    Math::Vec3Normalize(&vUp);

    const float aBasis[4][4] = {
        { vRight.x, vDir.x, vUp.x, 0.0f },
        { vRight.y, vDir.y, vUp.y, 0.0f },
        { vRight.z, vDir.z, vUp.z, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f },
    };

    std::memcpy(camera.m_pRotB->m_a, aBasis, sizeof(aBasis));

    camera.m_pRotB->ToTransposed();
    camera.m_pProjection->Multiply(camera.m_pView.get(), camera.m_pRotB.get());
    camera.m_pView->SetEyePoint(&vEye);
}

void DrawModel()
{
    Renderer::ClearFrame();

    auto* const pEngine = g_pGameEngine.get();

    if (pEngine == nullptr)
    {
        return;
    }

    auto* const pCamera = pEngine->GetCamera(kCameraMainSlot);
    auto* const pObject = pEngine->GetSlot(s_nModelSlot);

    if (pCamera == nullptr || pCamera->m_pView == nullptr || !HasModel(pObject))
    {
        return;
    }

    auto& view = *pCamera->m_pView;

    float aSavedView[4][4];

    std::memcpy(aSavedView, view.m_a, sizeof(aSavedView));

    const auto vSavedEye       = view.m_vEye;
    const auto vSavedTranslate = view.m_vTranslate;

    pObject->ApplyRenderSlotPose();

    const auto bounds = MeasureObject(*pObject);

    const auto flTanHalfFov = [&]
    {
        if (view.m_flTanHalfFov > 0.0f)
        {
            return view.m_flTanHalfFov;
        }

        return 0.5f;
    }();

    // A sphere fits the view at its radius over the sine of the half angle, and the margin keeps
    // its near side, which perspective makes larger, inside the frame.
    const auto flSinHalfFov = flTanHalfFov / std::sqrt(1.0f + flTanHalfFov * flTanHalfFov);
    const auto flDistance   = bounds.flRadius / flSinHalfFov * 1.25f * s_orbit.flZoom;
    // Yaw turns round the object's up axis and pitch tilts towards it, from two axes across it.
    const auto vUp = UpAxis(*pObject);

    auto vAcross = Cross(vUp, Vec3{ 1.0f, 0.0f, 0.0f });

    if (!Math::Vec3Normalize(&vAcross))
    {
        vAcross = Vec3{ 0.0f, 1.0f, 0.0f };
    }

    const auto vAlong = Cross(vUp, vAcross);

    const auto flCosPitch = std::cos(s_orbit.flPitch);
    const auto flSinPitch = std::sin(s_orbit.flPitch);
    const auto flCosYaw   = flCosPitch * std::cos(s_orbit.flYaw);
    const auto flSinYaw   = flCosPitch * std::sin(s_orbit.flYaw);

    const auto vDir = Vec3{ -(flCosYaw * vAcross.x + flSinYaw * vAlong.x + flSinPitch * vUp.x),
                            -(flCosYaw * vAcross.y + flSinYaw * vAlong.y + flSinPitch * vUp.y),
                            -(flCosYaw * vAcross.z + flSinYaw * vAlong.z + flSinPitch * vUp.z) };
    const auto vEye = Vec3{ bounds.vCentre.x - vDir.x * flDistance, bounds.vCentre.y - vDir.y * flDistance, bounds.vCentre.z - vDir.z * flDistance };

    LookFrom(*pCamera, vEye, vDir, vUp);

    // Only the viewed object is visible, so only its shadow is drawn.
    auto visible = std::vector<char>(pEngine->m_nObjectCount);

    for (short i = 0; i < pEngine->m_nObjectCount; i++)
    {
        visible[i] = pEngine->m_ppObjects[i]->IsVisible();
        pEngine->m_ppObjects[i]->SetVisible(0);
    }

    pObject->SetVisible(1);

    g_pScene = pCamera;
    Renderer::SetCameraTransformAndViewport(pCamera->layout);

    g_nTransparentCount = 0;

    // The drome draws through its own pass rather than the object queue.
    if (s_nModelSlot == kSlotDrome)
    {
        Renderer::DrawAllMeshes();
    }
    else
    {
        pObject->UpdateLODAndPushComponents(pCamera);
    }

    pCamera->DrawShadowAndTransparentPass(&g_arenaEllipsoid);

    for (short i = 0; i < pEngine->m_nObjectCount; i++)
    {
        pEngine->m_ppObjects[i]->SetVisible(visible[i]);
    }

    std::memcpy(view.m_a, aSavedView, sizeof(aSavedView));
    view.m_vEye       = vSavedEye;
    view.m_vTranslate = vSavedTranslate;
}

// Dragging on the model turns it and the wheel moves in and out, wherever no window is in the way.
void SteerOrbit()
{
    auto& io = ImGui::GetIO();

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.WantCaptureMouse)
    {
        s_bDragging = true;
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        s_bDragging = false;
    }

    if (s_bDragging)
    {
        s_orbit.flYaw -= io.MouseDelta.x * 0.01f;
        s_orbit.flPitch = std::clamp(s_orbit.flPitch + io.MouseDelta.y * 0.01f, -1.4f, 1.4f);
    }
    else if (s_bTurning)
    {
        s_orbit.flYaw += s_flTurnSpeed * io.DeltaTime;
    }

    if (!io.WantCaptureMouse && io.MouseWheel != 0.0f)
    {
        s_orbit.flZoom = std::clamp(s_orbit.flZoom * std::pow(0.9f, io.MouseWheel), 0.2f, 5.0f);
    }
}

void ModelViewerWindow()
{
    SteerOrbit();

    ImGui::SetNextWindowPos(ImVec2(16.0f, 40.0f), ImGuiCond_FirstUseEver);
    BeginSceneWindow("Model viewer", ImGuiWindowFlags_AlwaysAutoResize);

    auto* const pEngine = g_pGameEngine.get();

    if (pEngine == nullptr)
    {
        ImGui::TextDisabled("No objects are loaded.");
        ImGui::End();

        return;
    }

    OriginalUI::SeparatorText("Object");

    if (ImGui::BeginListBox("##objects", ImVec2(300.0f, 200.0f)))
    {
        for (const auto nSlot : ModelSlots(*pEngine))
        {
            const auto strLabel = DebugMenu::SlotName(nSlot) + "##" + std::to_string(nSlot);

            if (OriginalUI::Selectable(strLabel.c_str(), nSlot == s_nModelSlot))
            {
                s_nModelSlot = nSlot;
                s_orbit      = Orbit();
            }
        }

        ImGui::EndListBox();
    }

    OriginalUI::SeparatorText("View");
    OriginalUI::Checkbox("Turn gently", &s_bTurning);
    ImGui::SetNextItemWidth(160.0f);
    ImGui::SliderFloat("Turn speed", &s_flTurnSpeed, 0.05f, 2.0f, "%.2f");

    if (OriginalUI::Button("Reset view"))
    {
        s_orbit = Orbit();
    }

    ImGui::TextDisabled("Drag the model to turn it; the wheel zooms.");

    OriginalUI::SeparatorText("Renderer");
    DebugMenu::RendererControls();

    ImGui::End();
}

// Sound player. It lists every sound SNDFILES.LST names, by its id, and the music tracks ripped to
// Data/Music.

auto s_soundFilter = std::array<char, 64>{};
auto s_soundOnDisk = std::vector<char>();

// SNDFILES.LST names some sounds the game shipped without.
bool SoundOnDisk(const AudioMgr& audio, int nSound)
{
    if (s_soundOnDisk.empty())
    {
        for (int i = 0; i < audio.pEntries->m_nSoundCount; i++)
        {
            const auto strPath = std::string(g_gameApp.m_strExeDir.m_str.m_pchData) + "Sounds\\" + audio.pEntries->m_apSounds[i]->m_strName + ".wav";

            std::error_code error;

            s_soundOnDisk.push_back(static_cast<char>(std::filesystem::exists(Path::Native(strPath.c_str()), error)));
        }
    }

    return nSound < static_cast<int>(s_soundOnDisk.size()) && s_soundOnDisk[nSound] != 0;
}

void SoundPlayerWindow()
{
    ImGui::SetNextWindowPos(ImVec2(16.0f, 40.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(640.0f, 600.0f), ImGuiCond_FirstUseEver);
    BeginSceneWindow("Sound player", ImGuiWindowFlags_None);

    auto* const pAudio = g_pAudioMgr.get();

    if (pAudio == nullptr || pAudio->pEntries == nullptr)
    {
        ImGui::TextDisabled("No sounds are loaded.");
        ImGui::End();

        return;
    }

    if (pAudio->wNoSound != 0)
    {
        ImGui::TextColored(ImVec4(0.5f, 0.0f, 0.0f, 1.0f), "Sound is off: started with -s, or no audio device opened.");
    }

    OriginalUI::SeparatorText("Music");

    const auto& anTracks = AudioMgr::MusicTracks();

    if (anTracks.empty())
    {
        ImGui::TextDisabled("No tracks are ripped to Data/Music.");
    }

    for (size_t i = 0; i < anTracks.size(); i++)
    {
        if (i % 6 != 0)
        {
            ImGui::SameLine();
        }

        const auto strLabel = "Track " + std::to_string(anTracks[i]);

        if (OriginalUI::Button(strLabel.c_str()))
        {
            pAudio->PlayCDTrack(anTracks[i]);
        }
    }

    if (OriginalUI::Button("Stop music"))
    {
        pAudio->StopCD();
    }

    OriginalUI::SeparatorText("Sounds");

    ImGui::SetNextItemWidth(220.0f);
    ImGui::InputTextWithHint("##filter", "Filter by name", s_soundFilter.data(), s_soundFilter.size());
    ImGui::SameLine();

    if (OriginalUI::Button("Stop all"))
    {
        pAudio->StopAllSfx();
    }

    ImGui::TextDisabled("Click a sound to play it.");

    constexpr auto kFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY;

    if (!ImGui::BeginTable("sounds", 5, kFlags, ImVec2(0.0f, ImGui::GetContentRegionAvail().y)))
    {
        ImGui::End();

        return;
    }

    for (const auto* pszHeading : { "Id", "Name", "Length", "Group", "State" })
    {
        ImGui::TableSetupColumn(pszHeading);
    }

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableHeadersRow();

    for (int i = 0; i < pAudio->pEntries->m_nSoundCount; i++)
    {
        const auto& sound = *pAudio->pEntries->m_apSounds[i];

        if (!ContainsNoCase(sound.m_strName, s_soundFilter.data()))
        {
            continue;
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        const auto strID = std::to_string(i);

        if (OriginalUI::Selectable(strID.c_str(), false, ImGuiSelectableFlags_SpanAllColumns))
        {
            pAudio->PlaySpecific(i);
        }

        ImGui::TableNextColumn();
        ImGui::TextUnformatted(sound.m_strName.c_str());
        ImGui::TableNextColumn();
        ImGui::Text("%.2f s", sound.m_nDurationMs / 1000.0f);
        ImGui::TableNextColumn();
        ImGui::Text("%d", sound.m_nGroup);
        ImGui::TableNextColumn();

        if (!SoundOnDisk(*pAudio, i))
        {
            ImGui::TextDisabled("No file");
        }
        else if (pAudio->IsPlaying(i) != 0)
        {
            ImGui::TextColored(ImVec4(0.0f, 0.5f, 0.0f, 1.0f), "Playing");
        }
    }

    ImGui::EndTable();
    ImGui::End();
}

// Image browser. The menu art in ASSETS8 is MDM1-packed, so it goes through the game's own reader;
// the model textures are plain bitmaps.

enum class ImageSource
{
    Assets,
    Textures,
};

struct ImageFolder
{
    const char*              pszLabel;
    std::vector<std::string> names;
    bool                     bRead = false;
};

auto s_imageFolders = std::array<ImageFolder, 2>{ ImageFolder{ "Menu art (ASSETS8)" }, ImageFolder{ "Model textures (TEXTURES)" } };
auto s_imageSource  = ImageSource::Assets;
auto s_imageFilter  = std::array<char, 64>{};
auto s_strImage     = std::string();
int  s_nImageWidth  = 0;
int  s_nImageHeight = 0;
int  s_nImageZoom   = 0;

SDL_Texture* s_pImage = nullptr;

std::string FolderPath(ImageSource source)
{
    if (source == ImageSource::Assets)
    {
        return g_gameApp.m_strAssetsPath.m_str.m_pchData;
    }

    return std::string(g_gameApp.m_strExeDir.m_str.m_pchData) + "Textures";
}

ImageFolder& Folder(ImageSource source)
{
    auto& folder = s_imageFolders[static_cast<size_t>(source)];

    if (folder.bRead)
    {
        return folder;
    }

    folder.bRead = true;

    std::error_code error;

    for (const auto& entry : std::filesystem::directory_iterator(Path::Native(FolderPath(source).c_str()), error))
    {
        if (SDL_strcasecmp(entry.path().extension().string().c_str(), ".bmp") == 0)
        {
            folder.names.push_back(entry.path().stem().string());
        }
    }

    std::sort(
        folder.names.begin(),
        folder.names.end(),
        [](const std::string& a, const std::string& b)
        {
            return SDL_strcasecmp(a.c_str(), b.c_str()) < 0;
        });

    return folder;
}

void ForgetImage()
{
    if (s_pImage != nullptr)
    {
        SDL_DestroyTexture(s_pImage);
        s_pImage = nullptr;
    }

    s_strImage.clear();
}

void UploadImage(SDL_PixelFormat format, int nWidth, int nHeight, const void* pPixels)
{
    auto* const pRenderer = SDL_GetRenderer(Window::Native().As<SDL_Window>());

    if (pRenderer == nullptr)
    {
        return;
    }

    s_pImage = SDL_CreateTexture(pRenderer, format, SDL_TEXTUREACCESS_STATIC, nWidth, nHeight);

    if (s_pImage == nullptr)
    {
        return;
    }

    SDL_SetTextureScaleMode(s_pImage, SDL_SCALEMODE_NEAREST);
    SDL_UpdateTexture(s_pImage, nullptr, pPixels, nWidth * 4);

    s_nImageWidth  = nWidth;
    s_nImageHeight = nHeight;
}

void ShowImage(const std::string& strName)
{
    ForgetImage();

    s_strImage = strName;

    if (s_imageSource == ImageSource::Assets)
    {
        auto* const pBitmap = GDI::LoadBitmapWithPalette(strName.c_str(), nullptr);

        if (pBitmap != nullptr)
        {
            UploadImage(SDL_PIXELFORMAT_XRGB8888, pBitmap->nWidth, pBitmap->nHeight, pBitmap->pixels.data());
            GDI::SafeDeleteBitmap(pBitmap);
        }

        return;
    }

    const auto strPath = FolderPath(s_imageSource) + "\\" + strName + ".bmp";

    int   nWidth    = 0;
    int   nHeight   = 0;
    int   nChannels = 0;
    auto* pPixels   = stbi_load(Path::Native(strPath.c_str()).c_str(), &nWidth, &nHeight, &nChannels, 4);

    if (pPixels != nullptr)
    {
        UploadImage(SDL_PIXELFORMAT_RGBA32, nWidth, nHeight, pPixels);
        stbi_image_free(pPixels);
    }
}

void ImageBrowserWindow()
{
    ImGui::SetNextWindowPos(ImVec2(16.0f, 40.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(960.0f, 640.0f), ImGuiCond_FirstUseEver);
    BeginSceneWindow("Image browser", ImGuiWindowFlags_None);

    ImGui::BeginChild("##list", ImVec2(300.0f, 0.0f));

    for (const auto source : { ImageSource::Assets, ImageSource::Textures })
    {
        if (OriginalUI::RadioButton(s_imageFolders[static_cast<size_t>(source)].pszLabel, s_imageSource == source))
        {
            s_imageSource = source;
            ForgetImage();
        }
    }

    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##filter", "Filter by name", s_imageFilter.data(), s_imageFilter.size());

    const auto& folder = Folder(s_imageSource);

    auto shown = std::vector<const std::string*>();

    for (const auto& strName : folder.names)
    {
        if (ContainsNoCase(strName, s_imageFilter.data()))
        {
            shown.push_back(&strName);
        }
    }

    // Up and down step through the list while no field has the keyboard.
    if (!ImGui::GetIO().WantTextInput && !shown.empty())
    {
        const auto it    = std::find_if(shown.begin(),
                                        shown.end(),
                                        [](const std::string* pName)
                                        {
                                         return *pName == s_strImage;
                                        });
        const auto nAt   = static_cast<int>(it - shown.begin());
        const auto nLast = static_cast<int>(shown.size()) - 1;

        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
        {
            ShowImage(*shown[(std::min)(nAt + 1, nLast)]);
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && nAt > 0)
        {
            ShowImage(*shown[(std::min)(nAt - 1, nLast)]);
        }
    }

    if (ImGui::BeginListBox("##images", ImVec2(-1.0f, -1.0f)))
    {
        if (folder.names.empty())
        {
            ImGui::TextDisabled("No images in %s.", FolderPath(s_imageSource).c_str());
        }

        for (const auto* pName : shown)
        {
            const auto bSelected = *pName == s_strImage;

            if (OriginalUI::Selectable(pName->c_str(), bSelected))
            {
                ShowImage(*pName);
            }

            if (bSelected && ImGui::IsWindowAppearing())
            {
                ImGui::SetScrollHereY();
            }
        }

        ImGui::EndListBox();
    }

    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("##preview");

    if (s_strImage.empty())
    {
        ImGui::TextDisabled("%zu images. Pick one, or step through them with the arrow keys.", folder.names.size());
    }
    else if (s_pImage == nullptr)
    {
        ImGui::Text("%s did not load.", s_strImage.c_str());
    }
    else
    {
        ImGui::Text("%s, %d by %d", s_strImage.c_str(), s_nImageWidth, s_nImageHeight);
    }

    for (const auto& [nZoom, pszLabel] : { std::pair{ 0, "Fit" }, std::pair{ 1, "1x" }, std::pair{ 2, "2x" }, std::pair{ 4, "4x" } })
    {
        if (nZoom != 0)
        {
            ImGui::SameLine();
        }

        if (OriginalUI::RadioButton(pszLabel, s_nImageZoom == nZoom))
        {
            s_nImageZoom = nZoom;
        }
    }

    ImGui::BeginChild("##image", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);

    if (s_pImage != nullptr)
    {
        const auto size = [&]
        {
            if (s_nImageZoom != 0)
            {
                return ImVec2(static_cast<float>(s_nImageWidth * s_nImageZoom), static_cast<float>(s_nImageHeight * s_nImageZoom));
            }

            const auto avail   = ImGui::GetContentRegionAvail();
            const auto flScale = (std::min)(avail.x / s_nImageWidth, avail.y / s_nImageHeight);

            return ImVec2(s_nImageWidth * flScale, s_nImageHeight * flScale);
        }();

        ImGui::Image(ImTextureRef(reinterpret_cast<ImTextureID>(s_pImage)), size);
    }

    ImGui::EndChild();
    ImGui::EndChild();
    ImGui::End();
}

} // namespace

void Scenes::Open(Scene scene)
{
    s_next = scene;
}

Scenes::Scene Scenes::Current()
{
    return s_next;
}

bool Scenes::Frame()
{
    if (s_next != s_scene)
    {
        const auto previous = s_scene;

        s_scene = s_next;

        if (s_scene != Scene::ImageBrowser)
        {
            ForgetImage();
        }

        // A match would otherwise try to catch up on the time the scene was open, and a menu only
        // repaints when it changes.
        if (s_scene == Scene::None && previous != Scene::None)
        {
            FramePacing::Reset();

            if (g_gameApp.m_nEngineState != kEstateMatchLive)
            {
                Window::PresentMenu();
            }

            return true;
        }
    }

    if (s_scene == Scene::None)
    {
        return false;
    }

    if (s_scene == Scene::ModelViewer)
    {
        DrawModel();
        Renderer::PresentFrame();

        return true;
    }

    // The Windows 95 desktop's teal.
    Presenter::Clear(0, 128, 128);

    return true;
}

void Scenes::DrawWindows()
{
    if (s_scene == Scene::None)
    {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        Open(Scene::None);

        return;
    }

    const auto theme = OriginalUI::Theme();

    // Fields and lists take a sunken edge from ImGui; the other controls draw their own.
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

    switch (s_scene)
    {
        case Scene::ModelViewer:
            ModelViewerWindow();
            break;

        case Scene::SoundPlayer:
            SoundPlayerWindow();
            break;

        case Scene::ImageBrowser:
            ImageBrowserWindow();
            break;

        case Scene::None:
            break;
    }

    ImGui::PopStyleVar();
}
