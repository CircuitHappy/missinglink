flags = [
    '-x',
    'c++',
    '-DLINK_PLATFORM_LINUX=1',
    '-DLINK_PLATFORM_UNIX=1',
    '-Isrc',
    '-isystem',
    'vendor/link/include',
    '-isystem',
    'vendor/link/cmake_include/../modules/asio-standalone/asio/include',
    '-Wall',
    '-Os',
    '-Wno-psabi',
    '-std=gnu++11'
]

def FlagsForFile(filename):
    return {'flags': flags}
