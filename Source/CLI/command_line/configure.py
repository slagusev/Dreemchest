#################################################################################
#
# The MIT License (MIT)
#
# Copyright (c) 2015 Dmitry Sovetov
#
# https://github.com/dmsovetov
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
#################################################################################

import os
import sys
import cmake
import command_line
import env


class PlatformConfigurationCommand(cmake.Command):
    """A base class for all platform configuration commands"""

    def __init__(self, platform, parser, generators, rendering_backend, toolchain=None):
        """Constructs a platform configuration command"""

        # Load active environment
        self._env = env.load()

        cmake.Command.__init__(self, parser, generators, prefix_path=os.path.join(self.env.dependencies, platform))

        command_line.add_component(parser, 'pch', description='generate a build system that uses precompiled headers.', default=False)
        command_line.add_component(parser, 'composer', description='do not build the Composer tool.', default=False)
        command_line.add_component(parser, 'relight', description='do not build the Relight lightmapping library.', default=False)
        command_line.add_component(parser, 'tests', description='do not build unit tests.', default=False)
        command_line.add_component(parser, 'examples', description='do not build examples.')
        command_line.add_component(parser, 'sound', description='build with no sound support.')
        command_line.add_component(parser, 'renderer', description='build with no rendering support.', options=rendering_backend)

        # Add third party libraries options
        self._libraries = ['TIFF', 'jsoncpp', 'Zlib', 'Box2D', 'GTest', 'PNG', 'Lua', 'Ogg', 'Vorbis', 'OpenAL', 'cURL']

        for lib in self._libraries:
            command_line.add_library(parser, lib)

        self._toolchain = toolchain

        parser.set_defaults(function=self.configure)

    @property
    def env(self):
        """Returns a loaded environment configuration"""
        return self._env

    def configure(self, options):
        """Performs basic build system configuration"""
        pass

    def _prepare(self, options):
        """Generates a command line arguments from an input options"""

        # Generate platform independent CMake arguments from passed options
        parameters = dict(
            DC_USE_PCH=cmake.disable_option(options.no_pch),
            DC_COMPOSER_ENABLED=cmake.disable_option(options.no_composer or options.no_qt),
            DC_OPENGL_ENABLED=cmake.disable_option(options.no_renderer),
            DC_SOUND_ENABLED=cmake.disable_option(options.no_sound),
            DC_WITH_RELIGHT=cmake.disable_option(options.no_relight),
            DC_BUILD_TESTS=cmake.disable_option(options.no_tests),
            DC_BUILD_EXAMPLES=cmake.disable_option(options.no_examples),
            CMAKE_CXX_STANDARD=options.cpp,
            DC_QT_SUPPORT=('disabled' if not hasattr(options, 'no_qt') or options.no_qt else options.qt).capitalize(),
            CMAKE_PREFIX_PATH=os.path.abspath(options.prefix_path),
        )

        # Set the CMAKE_TOOLCHAIN_FILE variable
        if self._toolchain:
            parameters['CMAKE_TOOLCHAIN_FILE'] = self._toolchain

        # Generate CMake arguments from added libraries
        for lib in self._libraries:
            cmake.library_option(parameters, lib, options)

        if hasattr(options, 'identifier'):
            parameters['MACOSX_BUNDLE_GUI_IDENTIFIER'] = options.identifier

        if hasattr(options, 'codesign'):
            parameters['IOS_CODESIGN_IDENTITY'] = '"%s"' % ' '.join(options.codesign)

        if hasattr(options, 'api'):
            parameters['ANDROID_NATIVE_API_LEVEL'] = 'android-%s' % options.api

        return parameters


class DesktopConfigureCommand(PlatformConfigurationCommand):
    """Performs desktop OS build system configuration"""

    def __init__(self, platform, parser, generators, rendering_backend):
        """Constructs desktop configuration command"""

        PlatformConfigurationCommand.__init__(self, platform, parser, generators, rendering_backend)

        # Add Qt library option
        command_line.add_system_library(parser, 'Qt', versions=['auto', 'qt4', 'qt5'])

        # Add third party libraries
        self._libraries.append('FBX')
        command_line.add_library(parser, 'FBX', is_bundled=False)


class WindowsConfigureCommand(DesktopConfigureCommand):
    """Performs Windows build system configuration"""

    def __init__(self, parser):
        """Constructs Windows configuration command"""

        DesktopConfigureCommand.__init__(self, 'Windows', parser, ['Visual Studio 12'], ['opengl', 'direct3d9', 'direct3d12'])

    def configure(self, options):
        """Performs basic build system configuration"""

        # Perform basic build configuration
        cmake_parameters = self._prepare(options)

        # Invoke a CMake command to generate a build system
        install_path = os.path.join(self.env.prebuilt, 'Windows')
        prefix_path = os.path.join(self.env.dependencies, 'Windows')
        cm = cmake.CMake(self.env.cmake, prefix_path=prefix_path, install_prefix=install_path)
        cm.configure('Visual Studio 12', options.source, options.output, cmake_parameters)


class MacOSConfigureCommand(DesktopConfigureCommand):
    """Performs MacOS build system configuration"""

    def __init__(self, parser):
        """Constructs MacOS configuration command"""

        DesktopConfigureCommand.__init__(self, 'macOS', parser, ['Xcode'], ['opengl'])

    def configure(self, options):
        """Performs MacOS build system configuration"""

        # Perform basic build configuration
        cmake_parameters = self._prepare(options)

        # Invoke a CMake command to generate a build system
        install_path = os.path.join(self.env.prebuilt, 'macOS')
        prefix_path = os.path.join(self.env.dependencies, 'macOS')
        cm = cmake.CMake(self.env.cmake, prefix_path=prefix_path, install_prefix=install_path)
        cm.configure('Xcode', options.source, options.output, cmake_parameters)


class IOSConfigureCommand(PlatformConfigurationCommand):
    """Performs iOS build system configuration"""

    def __init__(self, parser):
        """Constructs iOS configuration command"""

        PlatformConfigurationCommand.__init__(self,
                                              'iOS',
                                              parser,
                                              ['Xcode'], ['opengl'],
                                              toolchain=env.load().ios_toolchain)

        parser.add_argument('--identifier', help='a bundle identifier to be used.')
        parser.add_argument('--codesign', nargs='+', help='code sign identity name')

    def configure(self, options):
        """Performs iOS build system configuration"""

        # Perform basic build configuration
        cmake_parameters = self._prepare(options)

        # Invoke a CMake command to generate a build system
        install_path = os.path.join(self.env.prebuilt, 'iOS')
        prefix_path = os.path.join(self.env.dependencies, 'iOS')
        cm = cmake.CMake(self.env.cmake, prefix_path=prefix_path, install_prefix=install_path)
        cm.configure('Xcode', options.source, options.output, cmake_parameters)


class AndroidConfigureCommand(PlatformConfigurationCommand):
    def __init__(self, parser):
        """Constructs Android configuration command"""

        PlatformConfigurationCommand.__init__(self,
                                              'Android',
                                              parser,
                                              ['Unix Makefiles'],
                                              ['opengl'],
                                              toolchain=env.load().android_toolchain)

    def configure(self, options):
        """Performs iOS build system configuration"""

        # Perform basic build configuration
        cmake_parameters = self._prepare(options)
        cmake_parameters['ANDROID_NATIVE_API_LEVEL'] = 'android-24'
        cmake_parameters['ANDROID_NDK'] = self.env.android_ndk

        # Invoke a CMake command to generate a build system
        install_path = os.path.join(self.env.prebuilt, 'Android')
        prefix_path = os.path.join(self.env.dependencies, 'Android')
        cm = cmake.CMake(self.env.cmake, prefix_path=prefix_path, install_prefix=install_path)
        cm.configure('Unix Makefiles', options.source, options.output, cmake_parameters)


class EmscriptenConfigureCommand(PlatformConfigurationCommand):
    def __init__(self, parser):
        """Constructs Emscripten configuration command"""

        PlatformConfigurationCommand.__init__(self,
                                              'Emscripten',
                                              parser,
                                              ['Unix Makefiles'],
                                              ['opengl'],
                                              toolchain=env.load().emscripten_toolchain)

    def configure(self, options):
        """Performs iOS build system configuration"""

        # Perform basic build configuration
        cmake_parameters = self._prepare(options)
        cmake_parameters['EMSCRIPTEN_ROOT_PATH'] = self.env.emscripten

        # Invoke a CMake command to generate a build system
        install_path = os.path.join(self.env.prebuilt, 'Emscripten')
        prefix_path = os.path.join(self.env.dependencies, 'Emscripten')
        cm = cmake.CMake(self.env.cmake, prefix_path=prefix_path, install_prefix=install_path)
        cm.configure('Unix Makefiles', options.source, options.output, cmake_parameters)


class Command(command_line.Tool):
    """A command line tool to configure a build system for a specified target system"""

    def __init__(self, parser):
        """Constructs a configure command line tool"""

        command_line.Tool.__init__(self, parser, 'available platforms')

        self._add_command('android', AndroidConfigureCommand)
        self._add_command('emscripten', EmscriptenConfigureCommand)
        if os.name == 'nt':
            self._add_command('windows', WindowsConfigureCommand)
        if sys.platform == 'darwin':
            self._add_command('macos', MacOSConfigureCommand)
            self._add_command('ios', IOSConfigureCommand)
