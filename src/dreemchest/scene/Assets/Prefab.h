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

#ifndef __DC_Scene_Prefab_H__
#define __DC_Scene_Prefab_H__

#include "Assets.h"

DC_BEGIN_DREEMCHEST

namespace Scene {

	//! Prefab scene asset.
	class Prefab : public Asset {
	public:

								ClassEnableTypeInfoSuper( Prefab, Asset )

								//! Constructs Prefab instance.
								Prefab( AssetBundle* bundle = NULL, const String& uuid = String(), const String& name = String(), u16 width = 0, u16 height = 0 );
	};

} // namespace Scene

DC_END_DREEMCHEST

#endif    /*    !__DC_Scene___DC_Scene_Prefab_H___H__    */