/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Assistant of the Qt Toolkit.
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

#ifndef BOOKMARKITEM_H
#define BOOKMARKITEM_H

#include <QtCore/QVariant>
#include <QtCore/QVector>

QT_BEGIN_NAMESPACE

enum {
    UserRoleUrl = Qt::UserRole + 50,
    UserRoleFolder = Qt::UserRole + 100,
    UserRoleExpanded = Qt::UserRole + 150
};

typedef QVector<QVariant> DataVector;

class BookmarkItem
{
public:
    BookmarkItem(const DataVector &data, BookmarkItem *parent = 0);
    ~BookmarkItem();

    BookmarkItem *parent() const;
    void setParent(BookmarkItem *parent);

    void addChild(BookmarkItem *child);
    BookmarkItem *child(int number) const;

    int childCount() const;
    int childNumber() const;

    QVariant data(int column) const;
    void setData(const DataVector &data);
    bool setData(int column, const QVariant &value);

    bool insertChildren(bool isFolder, int position, int count);
    bool removeChildren(int position, int count);

    void dumpTree(int indent) const;

private:
    DataVector m_data;

    BookmarkItem *m_parent;
    QList<BookmarkItem*> m_children;
};

QT_END_NAMESPACE

#endif // BOOKMARKITEM_H
