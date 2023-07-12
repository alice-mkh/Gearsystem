project('gearsystem-highscore', ['c', 'cpp'],
  version : '0.1',
  default_options : [
    'warning_level=3',
    'cpp_std=c++11'
  ]
)

gearsystem_source = [
  'gearsystem-highscore.cpp',
  '../../src/Audio.cpp',
  '../../src/BootromMemoryRule.cpp',
  '../../src/Cartridge.cpp',
  '../../src/CodemastersMemoryRule.cpp',
  '../../src/GameGearIOPorts.cpp',
  '../../src/GearsystemCore.cpp',
  '../../src/Input.cpp',
  '../../src/JanggunMemoryRule.cpp',
  '../../src/KoreanMemoryRule.cpp',
  '../../src/Memory.cpp',
  '../../src/MemoryRule.cpp',
  '../../src/MSXMemoryRule.cpp',
  '../../src/opcodes_cb.cpp',
  '../../src/opcodes.cpp',
  '../../src/opcodes_ed.cpp',
  '../../src/Processor.cpp',
  '../../src/RomOnlyMemoryRule.cpp',
  '../../src/SegaMemoryRule.cpp',
  '../../src/SG1000MemoryRule.cpp',
  '../../src/SmsIOPorts.cpp',
  '../../src/Video.cpp',
  '../../src/audio/Blip_Buffer.cpp',
  '../../src/audio/Effects_Buffer.cpp',
  '../../src/audio/Multi_Buffer.cpp',
  '../../src/audio/Sms_Apu.cpp',
]

gearsystem_deps = [
  dependency('highscore-1'),
  dependency('gio-2.0'),
]

cores_dir = get_option('libdir') / 'highscore' / 'cores'

gearsystem_c_args = [
  '-DGEARSYSTEM_DISABLE_DISASSEMBLER',
]

gearsystem_cpp_args = [
  '-DGEARSYSTEM_DISABLE_DISASSEMBLER',
]

gearsystem_include_dirs = [
  include_directories('.'),
  include_directories('../../src'),
]

shared_library('gearsystem-highscore',
  gearsystem_source,
  dependencies : gearsystem_deps,
  c_args : gearsystem_c_args,
  cpp_args : gearsystem_cpp_args,
  include_directories : gearsystem_include_dirs,
  install_dir : cores_dir,
  install : true
)

install_data('gearsystem.highscore',
  install_dir : cores_dir,
)
