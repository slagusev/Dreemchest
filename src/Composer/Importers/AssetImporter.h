/**************************************************************************

 The MIT License (MIT)

 Copyright (c) 2015 Dmitry Sovetov

 https://github.com/dmsovetov

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 **************************************************************************/

#ifndef __DC_Composer_AssetImporter_H__
#define __DC_Composer_AssetImporter_H__

#include "../Composer.h"
#include "../IFileSystem.h"

DC_BEGIN_COMPOSER

namespace Importers {

	//! Base class for all asset importers.
	class AssetImporter : public RefCounted {
	public:

		//! Imports an asset to project cache.
		virtual bool			import( IFileSystemWPtr fs, const Asset& asset, const io::Path& path ) = 0;

	protected:

		//! Opens the binary stream for writing.
		virtual io::StreamPtr	openWriteStream( const io::Path& path ) const { return io::DiskFileSystem().openFile( path, io::BinaryWriteStream ); }

		//! Opens the binary stream for reading.
		virtual io::StreamPtr	openReadStream( const io::Path& path ) const { return io::DiskFileSystem().openFile( path, io::BinaryReadStream ); }
	};

} // namespace Importers

DC_END_COMPOSER

#endif	/*	!__DC_Composer_AssetImporter_H__	*/