#include "gearsystem-highscore.h"

#include <gearsystem.h>

struct _GearsystemHsCore
{
  HsCore parent_instance;

  GearsystemCore *core;

  HsSoftwareContext *context;

  char *save_location;
};

static void gearsystem_game_gear_core_init (HsGameGearCoreInterface *iface);
static void gearsystem_master_system_core_init (HsMasterSystemCoreInterface *iface);
static void gearsystem_sg1000_core_init (HsSg1000CoreInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (GearsystemHsCore, gearsystem_hs_core, HS_TYPE_CORE,
                               G_IMPLEMENT_INTERFACE (HS_TYPE_GAME_GEAR_CORE, gearsystem_game_gear_core_init)
                               G_IMPLEMENT_INTERFACE (HS_TYPE_MASTER_SYSTEM_CORE, gearsystem_master_system_core_init)
                               G_IMPLEMENT_INTERFACE (HS_TYPE_SG1000_CORE, gearsystem_sg1000_core_init));

static gboolean
gearsystem_hs_core_start (HsCore     *core,
                          const char  *rom_path,
                          const char  *save_location,
                          GError     **error)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);
  GS_RuntimeInfo runtime_info;

  g_set_str (&self->save_location, save_location);

  if (!self->core->LoadROM (rom_path)) {
    g_set_error (error, HS_CORE_ERROR, HS_CORE_ERROR_COULDNT_LOAD_ROM, "Couldn't load ROM");

    return FALSE;
  }

  self->context = hs_core_create_software_context (core,
                                                   GS_RESOLUTION_MAX_WIDTH,
                                                   GS_RESOLUTION_MAX_HEIGHT,
                                                   HS_PIXEL_FORMAT_RGB888);

  self->core->GetRuntimeInfo (runtime_info);

  int width = runtime_info.screen_width;
  int height = runtime_info.screen_height;
  HsRectangle area = HS_RECTANGLE_INIT (0, 0, width, height);

  hs_software_context_set_area (self->context, &area);
  hs_software_context_set_row_stride (self->context,
                                      width * hs_pixel_format_get_pixel_size (HS_PIXEL_FORMAT_RGB888));

  return TRUE;
}

static void
gearsystem_hs_core_reset (HsCore *core)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);

  self->core->ResetROM ();
}

static void
gearsystem_hs_core_run_frame (HsCore *core)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);
  u8 *video_buffer = (u8 *) hs_software_context_get_framebuffer (self->context);
  int16_t audio_buffer[GS_AUDIO_BUFFER_SIZE];
  int n_audio_samples;

  self->core->RunToVBlank (video_buffer, audio_buffer, &n_audio_samples);

  hs_core_play_samples (core, audio_buffer, n_audio_samples);
}

static void
gearsystem_hs_core_stop (HsCore *core)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);

  g_clear_pointer (&self->save_location, g_free);
  g_clear_object (&self->context);
}

static gboolean
gearsystem_hs_core_save_data (HsCore  *core,
                              GError **error)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);
  g_autoptr (GFile) file = g_file_new_for_path (self->save_location);

  guchar *save_data = self->core->GetMemory ()->GetCurrentRule ()->GetRamBanks ();
  size_t save_size = self->core->GetMemory ()->GetCurrentRule ()->GetRamSize ();

  if (save_data == NULL)
    return g_file_replace_contents (file, "", 0, NULL, FALSE,
                                    G_FILE_CREATE_NONE, NULL, NULL, error);

  return g_file_replace_contents (file, (char *) save_data, save_size, NULL, FALSE,
                                  G_FILE_CREATE_NONE, NULL, NULL, error);
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

  if (hs_core_get_platform (core) == HS_PLATFORM_GAME_GEAR)
    return (double) 4 / (double) 3;

  self->core->GetRuntimeInfo (runtime_info);

  int width = runtime_info.screen_width;
  int height = runtime_info.screen_height;

  return (double) width / (double) height;
}

static double
gearsystem_hs_core_get_sample_rate (HsCore *core)
{
  return 44100;
}

static void
gearsystem_hs_core_finalize (GObject *object)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (object);

  SafeDelete (self->core);

  G_OBJECT_CLASS (gearsystem_hs_core_parent_class)->finalize (object);
}

static void
gearsystem_hs_core_class_init (GearsystemHsCoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  HsCoreClass *core_class = HS_CORE_CLASS (klass);

  object_class->finalize = gearsystem_hs_core_finalize;

  core_class->start = gearsystem_hs_core_start;
  core_class->reset = gearsystem_hs_core_reset;
  core_class->run_frame = gearsystem_hs_core_run_frame;
  core_class->stop = gearsystem_hs_core_stop;

  core_class->save_data = gearsystem_hs_core_save_data;

  core_class->save_state = gearsystem_hs_core_save_state;
  core_class->load_state = gearsystem_hs_core_load_state;

  core_class->get_frame_rate = gearsystem_hs_core_get_frame_rate;
  core_class->get_aspect_ratio = gearsystem_hs_core_get_aspect_ratio;

  core_class->get_sample_rate = gearsystem_hs_core_get_sample_rate;
}

static void
gearsystem_hs_core_init (GearsystemHsCore *self)
{
  self->core = new GearsystemCore ();
  self->core->Init ();
}

static void
gearsystem_game_gear_core_button_pressed (HsGameGearCore *core, HsGameGearButton button)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);

  self->core->KeyPressed (Joypad_1, (GS_Keys) button);
}

static void
gearsystem_game_gear_core_button_released (HsGameGearCore *core, HsGameGearButton button)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);

  self->core->KeyReleased (Joypad_1, (GS_Keys) button);
}

static void
gearsystem_game_gear_core_init (HsGameGearCoreInterface *iface)
{
  iface->button_pressed = gearsystem_game_gear_core_button_pressed;
  iface->button_released = gearsystem_game_gear_core_button_released;
}

static void
gearsystem_master_system_core_button_pressed (HsMasterSystemCore *core, uint port, HsMasterSystemButton button)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);

  if (port == 0)
    self->core->KeyPressed (Joypad_1, (GS_Keys) button);
  else
    self->core->KeyPressed (Joypad_2, (GS_Keys) button);
}

static void
gearsystem_master_system_core_button_released (HsMasterSystemCore *core, uint port, HsMasterSystemButton button)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);

  if (port == 0)
    self->core->KeyReleased (Joypad_1, (GS_Keys) button);
  else
    self->core->KeyReleased (Joypad_2, (GS_Keys) button);
}

static void
gearsystem_master_system_core_pause_pressed (HsMasterSystemCore *core)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);

  self->core->KeyPressed (Joypad_1, Key_Start);
}

static void
gearsystem_master_system_core_pause_released (HsMasterSystemCore *core)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);

  self->core->KeyReleased (Joypad_1, Key_Start);
}

static void
gearsystem_master_system_core_init (HsMasterSystemCoreInterface *iface)
{
  iface->button_pressed = gearsystem_master_system_core_button_pressed;
  iface->button_released = gearsystem_master_system_core_button_released;
  iface->pause_pressed = gearsystem_master_system_core_pause_pressed;
  iface->pause_released = gearsystem_master_system_core_pause_released;
}

static void
gearsystem_sg1000_core_button_pressed (HsSg1000Core *core, uint port, HsSg1000Button button)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);

  if (port == 0)
    self->core->KeyPressed (Joypad_1, (GS_Keys) button);
  else
    self->core->KeyPressed (Joypad_2, (GS_Keys) button);
}

static void
gearsystem_sg1000_core_button_released (HsSg1000Core *core, uint port, HsSg1000Button button)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);

  if (port == 0)
    self->core->KeyReleased (Joypad_1, (GS_Keys) button);
  else
    self->core->KeyReleased (Joypad_2, (GS_Keys) button);
}

static void
gearsystem_sg1000_core_pause_pressed (HsSg1000Core *core)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);

  self->core->KeyPressed (Joypad_1, Key_Start);
}

static void
gearsystem_sg1000_core_pause_released (HsSg1000Core *core)
{
  GearsystemHsCore *self = GEARSYSTEM_HS_CORE (core);

  self->core->KeyReleased (Joypad_1, Key_Start);
}

static void
gearsystem_sg1000_core_init (HsSg1000CoreInterface *iface)
{
  iface->button_pressed = gearsystem_sg1000_core_button_pressed;
  iface->button_released = gearsystem_sg1000_core_button_released;
  iface->pause_pressed = gearsystem_sg1000_core_pause_pressed;
  iface->pause_released = gearsystem_sg1000_core_pause_released;
}

GType
hs_get_core_type (void)
{
  return GEARSYSTEM_TYPE_HS_CORE;
}
