project('gearsystem-highscore', ['c', 'cpp'],
  version : '3.8.1',
  default_options : [
    'warning_level=3',
    'cpp_std=c++11'
  ]
)

version = meson.project_version()
commit = run_command('git', 'describe', '--always', '--dirty', '--match', '').stdout().strip()

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
  '../../src/JanggunMemoryRule.cpp',
  '../../src/JumboDahjeeMemoryRule.cpp',
  '../../src/Korean0000XORFFMemoryRule.cpp',
  '../../src/Korean2000XOR1FMemoryRule.cpp',
  '../../src/KoreanBFFCMemoryRule.cpp',
  '../../src/KoreanFFF3FFFCMemoryRule.cpp',
  '../../src/KoreanFFFEMemoryRule.cpp',
  '../../src/KoreanFFFFHiComMemoryRule.cpp',
  '../../src/KoreanMDFFF5MemoryRule.cpp',
  '../../src/KoreanMemoryRule.cpp',
  '../../src/KoreanMSX32KB2000MemoryRule.cpp',
  '../../src/KoreanMSX8KB0300MemoryRule.cpp',
  '../../src/KoreanMSXSMS8000MemoryRule.cpp',
  '../../src/KoreanSMS32KB2000MemoryRule.cpp',
  '../../src/Memory.cpp',
  '../../src/MemoryRule.cpp',
  '../../src/Multi4PAKAllActionMemoryRule.cpp',
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
  '../../src/YM2413.cpp',
  '../../src/audio/Blip_Buffer.cpp',
  '../../src/audio/Sms_Apu.cpp',
  '../../src/audio/Stereo_Buffer.cpp',
  '../../src/audio/emu2413/emu2413.c',
  '../../src/miniz/miniz.c',
]

gearsystem_deps = [
  dependency('highscore-1'),
  dependency('gio-2.0'),
]

cores_dir = get_option('libdir') / 'highscore' / 'cores'

gearsystem_shared_args = [
  '-DGEARSYSTEM_DISABLE_DISASSEMBLER',
  '-DEMULATOR_BUILD="@0@"'.format(version),
  '-fvisibility=hidden',
  '-D__HIGHSCORE__',
]

gearsystem_c_args = [
  gearsystem_shared_args,
  '-Wno-calloc-transposed-args',
]

gearsystem_cpp_args = [
  gearsystem_shared_args,
  '-DGEARSYSTEM_DISABLE_DISASSEMBLER',
  '-Wno-pedantic',
  '-Wno-stringop-overflow',
  '-Wno-unused-parameter',
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
  name_prefix: '',
  install_dir : cores_dir,
  install : true
)

descriptor = configuration_data()
descriptor.set('version', version)
descriptor.set('commit', commit)
configure_file(
  input: 'gearsystem.highscore.in',
  output: 'gearsystem.highscore',
  configuration: descriptor,
  install_dir: cores_dir,
  install: true
)
