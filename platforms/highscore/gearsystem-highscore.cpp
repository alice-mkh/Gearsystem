#include "gearsystem-highscore.h"

#include <gearsystem.h>
#include <math.h>

HsCore *core;

struct _GearsystemHsCore
{
  HsCore parent_instance;

  GearsystemCore *core;

  HsSoftwareContext *context;

  char *save_path;
  gboolean enable_fm_audio;

  gboolean enable_light_phaser;
};

static void gearsystem_game_gear_core_init (HsGameGearCoreInterface *iface);
static void gearsystem_master_system_core_init (HsMasterSystemCoreInterface *iface);
static void gearsystem_sg1000_core_init (HsSg1000CoreInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (GearsystemHsCore, gearsystem_hs_core, HS_TYPE_CORE,
                               G_IMPLEMENT_INTERFACE (HS_TYPE_GAME_GEAR_CORE, gearsystem_game_gear_core_init)
                               G_IMPLEMENT_INTERFACE (HS_TYPE_MASTER_SYSTEM_CORE, gearsystem_master_system_core_init)
                               G_IMPLEMENT_INTERFACE (HS_TYPE_SG1000_CORE, gearsystem_sg1000_core_init));

static inline void
load_save (GearsystemHsCore *self)
{
  std::ifstream ram_file (self->save_path, std::ofstream::in | std::ofstream::binary);

  MemoryRule *rule = self->core->GetMemory ()->GetCurrentRule ();

  rule->LoadRam (ram_file, rule->GetRamSize ());
}

static gboolean
gearsystem_hs_core_load_rom (HsCore      *core,
                             const char **rom_paths,
                             int          n_rom_paths,
                             const char  *save_path,
                             GError     **error)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);

  self->core = new GearsystemCore ();
  self->core->Init ();

  g_assert (n_rom_paths == 1);

  g_set_str (&self->save_path, save_path);

  if (!self->core->LoadROM (rom_paths[0])) {
    g_set_error (error, HS_CORE_ERROR, HS_CORE_ERROR_COULDNT_LOAD_ROM, "Couldn't load ROM");

    return FALSE;
  }

  load_save (self);

  self->context = hs_core_create_software_context (core,
                                                   GS_RESOLUTION_MAX_WIDTH_WITH_OVERSCAN,
                                                   GS_RESOLUTION_MAX_HEIGHT_WITH_OVERSCAN,
                                                   HS_PIXEL_FORMAT_R8G8B8X8);
  self->core->GetAudio ()->Mute (false);

  HsPlatform platform = hs_core_get_platform (core);
  self->core->GetAudio ()->DisableYM2413 (platform != HS_PLATFORM_MASTER_SYSTEM || !self->enable_fm_audio);

  self->core->GetVideo ()->SetOverscan (Video::Overscan::OverscanFull284);

  self->core->EnablePhaser (self->enable_light_phaser);
  self->core->EnablePaddle (false);

  self->core->SetGlassesConfig (GearsystemCore::GlassesConfig::GlassesRightEye);

  return TRUE;
}

static void
gearsystem_hs_core_reset (HsCore *core, gboolean hard)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);

  self->core->ResetROMPreservingRAM ();

  self->core->EnablePhaser (self->enable_light_phaser);
  self->core->EnablePaddle (false);
}

static void
gearsystem_hs_core_poll_input (HsCore *core, HsInputState *input_state)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);
  HsPlatform platform = hs_core_get_platform (core);

  if (platform == HS_PLATFORM_GAME_GEAR) {
    uint32_t buttons = input_state->game_gear.buttons;

    for (int btn = 0; btn < HS_GAME_GEAR_N_BUTTONS; btn++) {
      if (buttons & 1 << btn)
        self->core->KeyPressed (Joypad_1, (GS_Keys) btn);
      else
        self->core->KeyReleased (Joypad_1, (GS_Keys) btn);
    }

    return;
  }

  if (platform == HS_PLATFORM_MASTER_SYSTEM) {
    for (int player = 0; player < HS_MASTER_SYSTEM_MAX_PLAYERS; player++) {
      uint32_t buttons = input_state->master_system.pad_buttons[player];
      GS_Joypads joypad = (player == 0) ? Joypad_1 : Joypad_2;

      if (self->enable_light_phaser && player == 0)
        continue;

      for (int btn = 0; btn < HS_MASTER_SYSTEM_N_BUTTONS; btn++) {
        if (buttons & 1 << btn)
          self->core->KeyPressed (joypad, (GS_Keys) btn);
        else
          self->core->KeyReleased (joypad, (GS_Keys) btn);
      }
    }

    if (self->enable_light_phaser) {
      GS_RuntimeInfo runtime_info;
      self->core->GetRuntimeInfo (runtime_info);

      double x = input_state->master_system.light_phaser_x;
      double y = input_state->master_system.light_phaser_y;

      int width = runtime_info.screen_width;
      int height = runtime_info.screen_height;

      self->core->SetPhaser ((int) round (x * width), (int) round (y * height));

      if (input_state->master_system.light_phaser_fire)
        self->core->KeyPressed (Joypad_1, Key_1);
      else
        self->core->KeyReleased (Joypad_1, Key_1);
    }

    if (input_state->master_system.pause_button)
      self->core->KeyPressed (Joypad_1, Key_Start);
    else
      self->core->KeyReleased (Joypad_1, Key_Start);

    return;
  }

  if (platform == HS_PLATFORM_SG1000) {
    for (int player = 0; player < HS_SG1000_MAX_PLAYERS; player++) {
      uint32_t buttons = input_state->sg1000.pad_buttons[player];
      GS_Joypads joypad = (player == 0) ? Joypad_1 : Joypad_2;

      for (int btn = 0; btn < HS_SG1000_N_BUTTONS; btn++) {
        if (buttons & 1 << btn)
          self->core->KeyPressed (joypad, (GS_Keys) btn);
        else
          self->core->KeyReleased (joypad, (GS_Keys) btn);
      }
    }

    if (input_state->sg1000.pause_button)
      self->core->KeyPressed (Joypad_1, Key_Start);
    else
      self->core->KeyReleased (Joypad_1, Key_Start);

    return;
  }

  g_assert_not_reached ();
}

static void
gearsystem_hs_core_run_frame (HsCore *core)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);
  u8 *video_buffer;
  int16_t audio_buffer[GS_AUDIO_BUFFER_SIZE];
  int n_audio_samples;

  video_buffer = (u8 *) hs_software_context_acquire_framebuffer (self->context);
  self->core->RunToVBlank (video_buffer, audio_buffer, &n_audio_samples);
  hs_software_context_release_framebuffer (self->context);

  GS_RuntimeInfo runtime_info;
  self->core->GetRuntimeInfo (runtime_info);

  int width = runtime_info.screen_width;
  int height = runtime_info.screen_height;
  HsRectangle area = HS_RECTANGLE_INIT (0, 0, width, height);

  hs_software_context_set_area (self->context, &area);
  hs_software_context_set_row_stride (self->context, width * hs_pixel_format_get_pixel_size (HS_PIXEL_FORMAT_R8G8B8X8));

  if (hs_core_get_platform (core) != HS_PLATFORM_GAME_GEAR) {
    bool is_224 = self->core->GetVideo()->IsExtendedMode224 ();
    bool pal = runtime_info.region == Region_PAL;

    HsBorder overscan;
    hs_border_init (&overscan,
                    GS_RESOLUTION_SMS_OVERSCAN_H_284_L,
                    (pal ? GS_RESOLUTION_SMS_OVERSCAN_V_PAL : GS_RESOLUTION_SMS_OVERSCAN_V) - (is_224 ? 16 : 0));

    hs_software_context_set_overscan (self->context, &overscan);
  }

  hs_core_play_samples (core, audio_buffer, n_audio_samples);
}

static void
gearsystem_hs_core_stop (HsCore *core)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);

  g_clear_pointer (&self->save_path, g_free);
  g_clear_object (&self->context);

  SafeDelete (self->core);
  self->core = NULL;
}

static gboolean
gearsystem_hs_core_reload_save (HsCore      *core,
                                const char  *save_path,
                                GError     **error)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);

  g_set_str (&self->save_path, save_path);

  load_save (self);

  return TRUE;
}

static gboolean
gearsystem_hs_core_sync_save (HsCore  *core,
                              GError **error)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);
  std::ofstream ram_file (self->save_path, std::ofstream::out | std::ofstream::binary);

  self->core->GetMemory ()->GetCurrentRule ()->SaveRam (ram_file);

  return TRUE;
}

static void
gearsystem_hs_core_save_state (HsCore          *core,
                               const char      *path,
                               HsStateCallback  callback)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);
  std::ofstream state_file (path, std::ofstream::out | std::ofstream::binary);
  size_t size;
  GError *error = NULL;

  if (!state_file.is_open ()) {
    g_set_error (&error, HS_CORE_ERROR, HS_CORE_ERROR_IO, "Failed to open state file");
    callback (core, &error);
    return;
  }

  if (!self->core->SaveState (state_file, size)) {
    state_file.close ();

    g_set_error (&error, HS_CORE_ERROR, HS_CORE_ERROR_INTERNAL, "Failed to save state");
    callback (core, &error);
    return;
  }

  state_file.close ();
  callback (core, NULL);
}

static void
gearsystem_hs_core_load_state (HsCore          *core,
                               const char      *path,
                               HsStateCallback  callback)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);
  std::ifstream state_file (path, std::ifstream::in | std::ifstream::binary);
  GError *error = NULL;

  if (!state_file.is_open ()) {
    g_set_error (&error, HS_CORE_ERROR, HS_CORE_ERROR_IO, "Failed to open state file");
    callback (core, &error);
    return;
  }

  if (!self->core->LoadState (state_file)) {
    state_file.close ();

    g_set_error (&error, HS_CORE_ERROR, HS_CORE_ERROR_INTERNAL, "Failed to load state");
    callback (core, &error);
    return;
  }

  state_file.close ();
  callback (core, NULL);
}

static double
gearsystem_hs_core_get_frame_rate (HsCore *core)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);
  GS_RuntimeInfo runtime_info;

  self->core->GetRuntimeInfo (runtime_info);

  return runtime_info.region == Region_NTSC ? 60.0 : 50.0;
}

static double
gearsystem_hs_core_get_aspect_ratio (HsCore *core)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);
  GS_RuntimeInfo runtime_info;

  self->core->GetRuntimeInfo (runtime_info);

  int width = runtime_info.screen_width;
  int height = runtime_info.screen_height;
  double par;

  if (hs_core_get_platform (core) == HS_PLATFORM_GAME_GEAR)
    par = 6.0 / 5.0;
  else if (runtime_info.region == Region_PAL)
    par = 2950000.0 / 2128137.0;
  else
    par = 8.0 / 7.0;

  return par * (double) width / (double) height;
}

static double
gearsystem_hs_core_get_sample_rate (HsCore *core)
{
  return 44100;
}

static HsRegion
gearsystem_hs_core_get_region (HsCore *core)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);
  GS_RuntimeInfo runtime_info;

  if (hs_core_get_platform (core) == HS_PLATFORM_GAME_GEAR)
    return HS_REGION_UNKNOWN;

  self->core->GetRuntimeInfo (runtime_info);

  if (runtime_info.region == Region_PAL)
    return HS_REGION_PAL;
  else
    return HS_REGION_NTSC;
}

static void
gearsystem_hs_core_finalize (GObject *object)
{
  core = NULL;

  G_OBJECT_CLASS (gearsystem_hs_core_parent_class)->finalize (object);
}

static void
gearsystem_hs_core_class_init (GearsystemHsCoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  HsCoreClass *core_class = HS_CORE_CLASS (klass);

  object_class->finalize = gearsystem_hs_core_finalize;

  core_class->load_rom = gearsystem_hs_core_load_rom;
  core_class->reset = gearsystem_hs_core_reset;
  core_class->poll_input = gearsystem_hs_core_poll_input;
  core_class->run_frame = gearsystem_hs_core_run_frame;
  core_class->stop = gearsystem_hs_core_stop;

  core_class->reload_save = gearsystem_hs_core_reload_save;
  core_class->sync_save = gearsystem_hs_core_sync_save;

  core_class->save_state = gearsystem_hs_core_save_state;
  core_class->load_state = gearsystem_hs_core_load_state;

  core_class->get_frame_rate = gearsystem_hs_core_get_frame_rate;
  core_class->get_aspect_ratio = gearsystem_hs_core_get_aspect_ratio;

  core_class->get_sample_rate = gearsystem_hs_core_get_sample_rate;

  core_class->get_region = gearsystem_hs_core_get_region;
}

static void
gearsystem_hs_core_init (GearsystemHsCore *self)
{
  g_assert (!core);

  core = HS_CORE (self);
}

static void
gearsystem_game_gear_core_init (HsGameGearCoreInterface *iface)
{
}

static void
gearsystem_master_system_core_set_enable_fm_audio (HsMasterSystemCore *core,
                                                   gboolean            enable_fm_audio)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);

  self->enable_fm_audio = enable_fm_audio;

  self->core->GetAudio ()->DisableYM2413 (!self->enable_fm_audio);
}

static void
gearsystem_master_system_core_set_enable_light_phaser (HsMasterSystemCore *core,
                                                       gboolean            enable_light_phaser)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);

  self->enable_light_phaser = enable_light_phaser;

  self->core->EnablePhaser (enable_light_phaser);
}

static void
gearsystem_master_system_core_init (HsMasterSystemCoreInterface *iface)
{
  iface->set_enable_fm_audio = gearsystem_master_system_core_set_enable_fm_audio;
  iface->set_enable_light_phaser = gearsystem_master_system_core_set_enable_light_phaser;
}

static void
gearsystem_sg1000_core_init (HsSg1000CoreInterface *iface)
{
}

GType
hs_get_core_type (void)
{
  return GEARSYSTEM_TYPE_HS_CORE;
}

void
gearsystem_hs_log (const char *message)
{
  hs_core_log_literal (core, HS_LOG_INFO, message);
}
