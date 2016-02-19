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

#ifndef DEPRECATED_H
#define DEPRECATED_H

DC_BEGIN_DREEMCHEST

    template<class T>
    using StrongPtr = Ptr<T>;
    
    typedef StringHash::type strhash;

    template<typename T>
	class Hash : public std::map<strhash, T> {};

DC_END_DREEMCHEST

#include "Base/Preprocessor.h"
#include "Base/Classes.h"

#define DC_NOT_IMPLEMENTED  NIMBLE_NOT_IMPLEMENTED
#define DC_DEPRECATED       NIMBLE_DEPRECATED
#define DC_BREAK            NIMBLE_BREAK
#define DC_BREAK_IF         NIMBLE_BREAK_IF
#define DC_DECL_OVERRIDE    NIMBLE_OVERRIDE
#define DC_NEW              new

#endif  /*  !DEPRECATED_H   */
