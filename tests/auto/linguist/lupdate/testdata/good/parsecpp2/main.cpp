/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

// IMPORTANT!!!! If you want to add testdata to this file, 
// always add it to the end in order to not change the linenumbers of translations!!!

// nothing here

// sickness: multi-\
line c++ comment } (with brace)

#define This is a closing brace } which was ignored
} // complain here

#define This is another \
    closing brace } which was ignored
} // complain here

#define This is another /* comment in } define */\
     something /* comment )
       spanning {multiple} lines */ \
    closing brace } which was ignored
} // complain here

#define This is another // comment in } define \
     something } comment
} // complain here



// Nested class in same file
class TopLevel {
    Q_OBJECT

    class Nested;
};

class TopLevel::Nested {
    void foo();
};

TopLevel::Nested::foo()
{
    TopLevel::tr("TopLevel");
}

// Nested class in other file
#include "main.h"

class TopLevel2::Nested {
    void foo();
};

TopLevel2::Nested::foo()
{
    TopLevel2::tr("TopLevel2");
}



namespace NameSpace {
class ToBeUsed;
}

// using statement before class definition
using NameSpace::ToBeUsed;

class NameSpace::ToBeUsed {
    Q_OBJECT
    void caller();
};

void ToBeUsed::caller()
{
    tr("NameSpace::ToBeUsed");
}
