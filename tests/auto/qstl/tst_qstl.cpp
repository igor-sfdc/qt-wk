/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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


#include <QtTest/QtTest>

#include <qstring.h>

#include <ostream>
#include <sstream>

class tst_QStl: public QObject
{
    Q_OBJECT

private slots:
    void streaming_data();
    void streaming();

    void concatenate();
};


static inline std::ostream &operator<<(std::ostream &out, const QString &string)
{
    out << string.toLocal8Bit().constData();
    return out;
}

void tst_QStl::streaming_data()
{
    QTest::addColumn<QString>("str");

    QTest::newRow("hello") << "hello";
    QTest::newRow("empty") << "";
}

void tst_QStl::streaming()
{
    QFETCH(QString, str);

    std::ostringstream buf;
    buf << str;

    std::string result = buf.str();

    QCOMPARE(QString::fromLatin1(result.data()), str);
}

void tst_QStl::concatenate()
{
    std::ostringstream buf;
    buf << QLatin1String("Hello ") << QLatin1String("World");

    QCOMPARE(QString::fromLatin1(buf.str().data()), QString("Hello World"));
}


QTEST_APPLESS_MAIN(tst_QStl)
#include "tst_qstl.moc"
