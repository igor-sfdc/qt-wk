/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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

#include "private/qdeclarativestateoperations_p.h"

#include <qdeclarative.h>
#include <qdeclarativecontext.h>
#include <qdeclarativeexpression.h>
#include <qdeclarativeinfo.h>
#include <qdeclarativeanchors_p_p.h>
#include <qdeclarativeitem_p.h>
#include <qdeclarativeguard_p.h>
#include <qdeclarativenullablevalue_p_p.h>
#include "private/qdeclarativecontext_p.h"
#include "private/qdeclarativeproperty_p.h"
#include "private/qdeclarativebinding_p.h"

#include <QtCore/qdebug.h>
#include <QtGui/qgraphicsitem.h>
#include <QtCore/qmath.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QDeclarativeParentChangePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativeParentChange)
public:
    QDeclarativeParentChangePrivate() : target(0), parent(0), origParent(0), origStackBefore(0),
        rewindParent(0), rewindStackBefore(0) {}

    QDeclarativeItem *target;
    QDeclarativeItem *parent;
    QDeclarativeGuard<QDeclarativeItem> origParent;
    QDeclarativeGuard<QDeclarativeItem> origStackBefore;
    QDeclarativeItem *rewindParent;
    QDeclarativeItem *rewindStackBefore;

    QDeclarativeNullableValue<qreal> x;
    QDeclarativeNullableValue<qreal> y;
    QDeclarativeNullableValue<qreal> width;
    QDeclarativeNullableValue<qreal> height;
    QDeclarativeNullableValue<qreal> scale;
    QDeclarativeNullableValue<qreal> rotation;

    void doChange(QDeclarativeItem *targetParent, QDeclarativeItem *stackBefore = 0);
};

void QDeclarativeParentChangePrivate::doChange(QDeclarativeItem *targetParent, QDeclarativeItem *stackBefore)
{
    if (targetParent && target && target->parentItem()) {
        Q_Q(QDeclarativeParentChange);
        bool ok;
        const QTransform &transform = target->parentItem()->itemTransform(targetParent, &ok);
        if (transform.type() >= QTransform::TxShear || !ok) {
            qmlInfo(q) << QDeclarativeParentChange::tr("Unable to preserve appearance under complex transform");
            ok = false;
        }

        qreal scale = 1;
        qreal rotation = 0;
        if (ok && transform.type() != QTransform::TxRotate) {
            if (transform.m11() == transform.m22())
                scale = transform.m11();
            else {
                qmlInfo(q) << QDeclarativeParentChange::tr("Unable to preserve appearance under non-uniform scale");
                ok = false;
            }
        } else if (ok && transform.type() == QTransform::TxRotate) {
            if (transform.m11() == transform.m22())
                scale = qSqrt(transform.m11()*transform.m11() + transform.m12()*transform.m12());
            else {
                qmlInfo(q) << QDeclarativeParentChange::tr("Unable to preserve appearance under non-uniform scale");
                ok = false;
            }

            if (scale != 0)
                rotation = atan2(transform.m12()/scale, transform.m11()/scale) * 180/M_PI;
            else {
                qmlInfo(q) << QDeclarativeParentChange::tr("Unable to preserve appearance under scale of 0");
                ok = false;
            }
        }

        const QPointF &point = transform.map(QPointF(target->x(),target->y()));
        qreal x = point.x();
        qreal y = point.y();

        // setParentItem will update the transformOriginPoint if needed
        target->setParentItem(targetParent);

        if (ok && target->transformOrigin() != QDeclarativeItem::TopLeft) {
            qreal tempxt = target->transformOriginPoint().x();
            qreal tempyt = target->transformOriginPoint().y();
            QTransform t;
            t.translate(-tempxt, -tempyt);
            t.rotate(rotation);
            t.scale(scale, scale);
            t.translate(tempxt, tempyt);
            const QPointF &offset = t.map(QPointF(0,0));
            x += offset.x();
            y += offset.y();
        }

        if (ok) {
            //qDebug() << x << y << rotation << scale;
            target->setX(x);
            target->setY(y);
            target->setRotation(target->rotation() + rotation);
            target->setScale(target->scale() * scale);
        }
    } else if (target) {
        target->setParentItem(targetParent);
    }

    //restore the original stack position.
    //### if stackBefore has also been reparented this won't work
    if (stackBefore)
        target->stackBefore(stackBefore);
}

/*!
    \preliminary
    \qmlclass ParentChange QDeclarativeParentChange
    \brief The ParentChange element allows you to reparent an Item in a state change.

    ParentChange reparents an item while preserving its visual appearance (position, size,
    rotation, and scale) on screen. You can then specify a transition to move/resize/rotate/scale
    the item to its final intended appearance.

    ParentChange can only preserve visual appearance if no complex transforms are involved.
    More specifically, it will not work if the transform property has been set for any
    items involved in the reparenting (i.e. items in the common ancestor tree
    for the original and new parent).

    You can specify at which point in a transition you want a ParentChange to occur by
    using a ParentAnimation.
*/


QDeclarativeParentChange::QDeclarativeParentChange(QObject *parent)
    : QDeclarativeStateOperation(*(new QDeclarativeParentChangePrivate), parent)
{
}

QDeclarativeParentChange::~QDeclarativeParentChange()
{
}

/*!
    \qmlproperty real ParentChange::x
    \qmlproperty real ParentChange::y
    \qmlproperty real ParentChange::width
    \qmlproperty real ParentChange::height
    \qmlproperty real ParentChange::scale
    \qmlproperty real ParentChange::rotation
    These properties hold the new position, size, scale, and rotation
    for the item in this state.
*/
qreal QDeclarativeParentChange::x() const
{
    Q_D(const QDeclarativeParentChange);
    return d->x.isNull ? qreal(0.) : d->x.value;
}

void QDeclarativeParentChange::setX(qreal x)
{
    Q_D(QDeclarativeParentChange);
    d->x = x;
}

bool QDeclarativeParentChange::xIsSet() const
{
    Q_D(const QDeclarativeParentChange);
    return d->x.isValid();
}

qreal QDeclarativeParentChange::y() const
{
    Q_D(const QDeclarativeParentChange);
    return d->y.isNull ? qreal(0.) : d->y.value;
}

void QDeclarativeParentChange::setY(qreal y)
{
    Q_D(QDeclarativeParentChange);
    d->y = y;
}

bool QDeclarativeParentChange::yIsSet() const
{
    Q_D(const QDeclarativeParentChange);
    return d->y.isValid();
}

qreal QDeclarativeParentChange::width() const
{
    Q_D(const QDeclarativeParentChange);
    return d->width.isNull ? qreal(0.) : d->width.value;
}

void QDeclarativeParentChange::setWidth(qreal width)
{
    Q_D(QDeclarativeParentChange);
    d->width = width;
}

bool QDeclarativeParentChange::widthIsSet() const
{
    Q_D(const QDeclarativeParentChange);
    return d->width.isValid();
}

qreal QDeclarativeParentChange::height() const
{
    Q_D(const QDeclarativeParentChange);
    return d->height.isNull ? qreal(0.) : d->height.value;
}

void QDeclarativeParentChange::setHeight(qreal height)
{
    Q_D(QDeclarativeParentChange);
    d->height = height;
}

bool QDeclarativeParentChange::heightIsSet() const
{
    Q_D(const QDeclarativeParentChange);
    return d->height.isValid();
}

qreal QDeclarativeParentChange::scale() const
{
    Q_D(const QDeclarativeParentChange);
    return d->scale.isNull ? qreal(1.) : d->scale.value;
}

void QDeclarativeParentChange::setScale(qreal scale)
{
    Q_D(QDeclarativeParentChange);
    d->scale = scale;
}

bool QDeclarativeParentChange::scaleIsSet() const
{
    Q_D(const QDeclarativeParentChange);
    return d->scale.isValid();
}

qreal QDeclarativeParentChange::rotation() const
{
    Q_D(const QDeclarativeParentChange);
    return d->rotation.isNull ? qreal(0.) : d->rotation.value;
}

void QDeclarativeParentChange::setRotation(qreal rotation)
{
    Q_D(QDeclarativeParentChange);
    d->rotation = rotation;
}

bool QDeclarativeParentChange::rotationIsSet() const
{
    Q_D(const QDeclarativeParentChange);
    return d->rotation.isValid();
}

QDeclarativeItem *QDeclarativeParentChange::originalParent() const
{
    Q_D(const QDeclarativeParentChange);
    return d->origParent;
}

/*!
    \qmlproperty Item ParentChange::target
    This property holds the item to be reparented
*/

QDeclarativeItem *QDeclarativeParentChange::object() const
{
    Q_D(const QDeclarativeParentChange);
    return d->target;
}

void QDeclarativeParentChange::setObject(QDeclarativeItem *target)
{
    Q_D(QDeclarativeParentChange);
    d->target = target;
}

/*!
    \qmlproperty Item ParentChange::parent
    This property holds the new parent for the item in this state.
*/

QDeclarativeItem *QDeclarativeParentChange::parent() const
{
    Q_D(const QDeclarativeParentChange);
    return d->parent;
}

void QDeclarativeParentChange::setParent(QDeclarativeItem *parent)
{
    Q_D(QDeclarativeParentChange);
    d->parent = parent;
}

QDeclarativeStateOperation::ActionList QDeclarativeParentChange::actions()
{
    Q_D(QDeclarativeParentChange);
    if (!d->target || !d->parent)
        return ActionList();

    ActionList actions;

    QDeclarativeAction a;
    a.event = this;
    actions << a;

    if (d->x.isValid()) {
        QDeclarativeAction xa(d->target, QLatin1String("x"), x());
        actions << xa;
    }

    if (d->y.isValid()) {
        QDeclarativeAction ya(d->target, QLatin1String("y"), y());
        actions << ya;
    }

    if (d->scale.isValid()) {
        QDeclarativeAction sa(d->target, QLatin1String("scale"), scale());
        actions << sa;
    }

    if (d->rotation.isValid()) {
        QDeclarativeAction ra(d->target, QLatin1String("rotation"), rotation());
        actions << ra;
    }

    if (d->width.isValid()) {
        QDeclarativeAction wa(d->target, QLatin1String("width"), width());
        actions << wa;
    }

    if (d->height.isValid()) {
        QDeclarativeAction ha(d->target, QLatin1String("height"), height());
        actions << ha;
    }

    return actions;
}

class AccessibleFxItem : public QDeclarativeItem
{
    Q_OBJECT
    Q_DECLARE_PRIVATE_D(QGraphicsItem::d_ptr.data(), QDeclarativeItem)
public:
    int siblingIndex() {
        Q_D(QDeclarativeItem);
        return d->siblingIndex;
    }
};

void QDeclarativeParentChange::saveOriginals()
{
    Q_D(QDeclarativeParentChange);
    saveCurrentValues();
    d->origParent = d->rewindParent;
    d->origStackBefore = d->rewindStackBefore;
}

void QDeclarativeParentChange::copyOriginals(QDeclarativeActionEvent *other)
{
    Q_D(QDeclarativeParentChange);
    QDeclarativeParentChange *pc = static_cast<QDeclarativeParentChange*>(other);

    d->origParent = pc->d_func()->rewindParent;
    d->origStackBefore = pc->d_func()->rewindStackBefore;

    saveCurrentValues();
}

void QDeclarativeParentChange::execute(Reason)
{
    Q_D(QDeclarativeParentChange);
    d->doChange(d->parent);
}

bool QDeclarativeParentChange::isReversable()
{
    return true;
}

void QDeclarativeParentChange::reverse(Reason)
{
    Q_D(QDeclarativeParentChange);
    d->doChange(d->origParent, d->origStackBefore);
}

QString QDeclarativeParentChange::typeName() const
{
    return QLatin1String("ParentChange");
}

bool QDeclarativeParentChange::override(QDeclarativeActionEvent*other)
{
    Q_D(QDeclarativeParentChange);
    if (other->typeName() != QLatin1String("ParentChange"))
        return false;
    if (QDeclarativeParentChange *otherPC = static_cast<QDeclarativeParentChange*>(other))
        return (d->target == otherPC->object());
    return false;
}

void QDeclarativeParentChange::saveCurrentValues()
{
    Q_D(QDeclarativeParentChange);
    if (!d->target) {
        d->rewindParent = 0;
        d->rewindStackBefore = 0;
        return;
    }

    d->rewindParent = d->target->parentItem();
    d->rewindStackBefore = 0;

    if (!d->rewindParent)
        return;

    //try to determine the item's original stack position so we can restore it
    int siblingIndex = ((AccessibleFxItem*)d->target)->siblingIndex() + 1;
    QList<QGraphicsItem*> children = d->rewindParent->childItems();
    for (int i = 0; i < children.count(); ++i) {
        QDeclarativeItem *child = qobject_cast<QDeclarativeItem*>(children.at(i));
        if (!child)
            continue;
        if (((AccessibleFxItem*)child)->siblingIndex() == siblingIndex) {
            d->rewindStackBefore = child;
            break;
        }
    }
}

void QDeclarativeParentChange::rewind()
{
    Q_D(QDeclarativeParentChange);
    d->doChange(d->rewindParent, d->rewindStackBefore);
}

class QDeclarativeStateChangeScriptPrivate : public QObjectPrivate
{
public:
    QDeclarativeStateChangeScriptPrivate() {}

    QDeclarativeScriptString script;
    QString name;
};

/*!
    \qmlclass StateChangeScript QDeclarativeStateChangeScript
    \brief The StateChangeScript element allows you to run a script in a state.

    StateChangeScripts are run when entering the state. You can use
    ScriptAction to specify at which point in the transition
    you want the StateChangeScript to be run.

    \qml
    State {
        name "state1"
        StateChangeScript {
            name: "myScript"
            script: doStateStuff();
        }
        ...
    }
    ...
    Transition {
        to: "state1"
        SequentialAnimation {
            NumberAnimation { ... }
            ScriptAction { scriptName: "myScript" }
            NumberAnimation { ... }
        }
    }
    \endqml

    \sa ScriptAction
*/

QDeclarativeStateChangeScript::QDeclarativeStateChangeScript(QObject *parent)
: QDeclarativeStateOperation(*(new QDeclarativeStateChangeScriptPrivate), parent)
{
}

QDeclarativeStateChangeScript::~QDeclarativeStateChangeScript()
{
}

/*!
    \qmlproperty script StateChangeScript::script
    This property holds the script to run when the state is current.
*/
QDeclarativeScriptString QDeclarativeStateChangeScript::script() const
{
    Q_D(const QDeclarativeStateChangeScript);
    return d->script;
}

void QDeclarativeStateChangeScript::setScript(const QDeclarativeScriptString &s)
{
    Q_D(QDeclarativeStateChangeScript);
    d->script = s;
}

/*!
    \qmlproperty script StateChangeScript::script
    This property holds the name of the script. This name can be used by a
    ScriptAction to target a specific script.

    \sa ScriptAction::stateChangeScriptName
*/
QString QDeclarativeStateChangeScript::name() const
{
    Q_D(const QDeclarativeStateChangeScript);
    return d->name;
}

void QDeclarativeStateChangeScript::setName(const QString &n)
{
    Q_D(QDeclarativeStateChangeScript);
    d->name = n;
}

void QDeclarativeStateChangeScript::execute(Reason)
{
    Q_D(QDeclarativeStateChangeScript);
    const QString &script = d->script.script();
    if (!script.isEmpty()) {
        QDeclarativeExpression expr(d->script.context(), script, d->script.scopeObject());
        QDeclarativeData *ddata = QDeclarativeData::get(this);
        if (ddata && ddata->outerContext && !ddata->outerContext->url.isEmpty())
            expr.setSourceLocation(ddata->outerContext->url.toString(), ddata->lineNumber);
        expr.evaluate();
        if (expr.hasError())
            qmlInfo(this, expr.error());
    }
}

QDeclarativeStateChangeScript::ActionList QDeclarativeStateChangeScript::actions()
{
    ActionList rv;
    QDeclarativeAction a;
    a.event = this;
    rv << a;
    return rv;
}

QString QDeclarativeStateChangeScript::typeName() const
{
    return QLatin1String("StateChangeScript");
}

/*!
    \qmlclass AnchorChanges QDeclarativeAnchorChanges
    \brief The AnchorChanges element allows you to change the anchors of an item in a state.

    In the following example we change the top and bottom anchors of an item:
    \qml
    State {
        name: "reanchored"
        AnchorChanges {
            target: content;
            anchors.top: window.top;
            anchors.bottom: window.bottom
        }
    }
    \endqml

    AnchorChanges can be animated using AnchorAnimation.
    \qml
    //animate our anchor changes
    Transition {
        AnchorAnimation {}
    }
    \endqml

    For more information on anchors see \l {anchor-layout}{Anchor Layouts}.
*/

class QDeclarativeAnchorSetPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativeAnchorSet)
public:
    QDeclarativeAnchorSetPrivate()
      : usedAnchors(0), resetAnchors(0), fill(0),
        centerIn(0)/*, leftMargin(0), rightMargin(0), topMargin(0), bottomMargin(0),
        margins(0), vCenterOffset(0), hCenterOffset(0), baselineOffset(0)*/
    {
    }

    QDeclarativeAnchors::Anchors usedAnchors;
    QDeclarativeAnchors::Anchors resetAnchors;

    QDeclarativeItem *fill;
    QDeclarativeItem *centerIn;

    QDeclarativeScriptString leftScript;
    QDeclarativeScriptString rightScript;
    QDeclarativeScriptString topScript;
    QDeclarativeScriptString bottomScript;
    QDeclarativeScriptString hCenterScript;
    QDeclarativeScriptString vCenterScript;
    QDeclarativeScriptString baselineScript;

    /*qreal leftMargin;
    qreal rightMargin;
    qreal topMargin;
    qreal bottomMargin;
    qreal margins;
    qreal vCenterOffset;
    qreal hCenterOffset;
    qreal baselineOffset;*/
};

QDeclarativeAnchorSet::QDeclarativeAnchorSet(QObject *parent)
  : QObject(*new QDeclarativeAnchorSetPrivate, parent)
{
}

QDeclarativeAnchorSet::~QDeclarativeAnchorSet()
{
}

QDeclarativeScriptString QDeclarativeAnchorSet::top() const
{
    Q_D(const QDeclarativeAnchorSet);
    return d->topScript;
}

void QDeclarativeAnchorSet::setTop(const QDeclarativeScriptString &edge)
{
    Q_D(QDeclarativeAnchorSet);
    d->usedAnchors |= QDeclarativeAnchors::TopAnchor;
    d->topScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetTop();
}

void QDeclarativeAnchorSet::resetTop()
{
    Q_D(QDeclarativeAnchorSet);
    d->usedAnchors &= ~QDeclarativeAnchors::TopAnchor;
    d->topScript = QDeclarativeScriptString();
    d->resetAnchors |= QDeclarativeAnchors::TopAnchor;
}

QDeclarativeScriptString QDeclarativeAnchorSet::bottom() const
{
    Q_D(const QDeclarativeAnchorSet);
    return d->bottomScript;
}

void QDeclarativeAnchorSet::setBottom(const QDeclarativeScriptString &edge)
{
    Q_D(QDeclarativeAnchorSet);
    d->usedAnchors |= QDeclarativeAnchors::BottomAnchor;
    d->bottomScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetBottom();
}

void QDeclarativeAnchorSet::resetBottom()
{
    Q_D(QDeclarativeAnchorSet);
    d->usedAnchors &= ~QDeclarativeAnchors::BottomAnchor;
    d->bottomScript = QDeclarativeScriptString();
    d->resetAnchors |= QDeclarativeAnchors::BottomAnchor;
}

QDeclarativeScriptString QDeclarativeAnchorSet::verticalCenter() const
{
    Q_D(const QDeclarativeAnchorSet);
    return d->vCenterScript;
}

void QDeclarativeAnchorSet::setVerticalCenter(const QDeclarativeScriptString &edge)
{
    Q_D(QDeclarativeAnchorSet);
    d->usedAnchors |= QDeclarativeAnchors::VCenterAnchor;
    d->vCenterScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetVerticalCenter();
}

void QDeclarativeAnchorSet::resetVerticalCenter()
{
    Q_D(QDeclarativeAnchorSet);
    d->usedAnchors &= ~QDeclarativeAnchors::VCenterAnchor;
    d->vCenterScript = QDeclarativeScriptString();
    d->resetAnchors |= QDeclarativeAnchors::VCenterAnchor;
}

QDeclarativeScriptString QDeclarativeAnchorSet::baseline() const
{
    Q_D(const QDeclarativeAnchorSet);
    return d->baselineScript;
}

void QDeclarativeAnchorSet::setBaseline(const QDeclarativeScriptString &edge)
{
    Q_D(QDeclarativeAnchorSet);
    d->usedAnchors |= QDeclarativeAnchors::BaselineAnchor;
    d->baselineScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetBaseline();
}

void QDeclarativeAnchorSet::resetBaseline()
{
    Q_D(QDeclarativeAnchorSet);
    d->usedAnchors &= ~QDeclarativeAnchors::BaselineAnchor;
    d->baselineScript = QDeclarativeScriptString();
    d->resetAnchors |= QDeclarativeAnchors::BaselineAnchor;
}

QDeclarativeScriptString QDeclarativeAnchorSet::left() const
{
    Q_D(const QDeclarativeAnchorSet);
    return d->leftScript;
}

void QDeclarativeAnchorSet::setLeft(const QDeclarativeScriptString &edge)
{
    Q_D(QDeclarativeAnchorSet);
    d->usedAnchors |= QDeclarativeAnchors::LeftAnchor;
    d->leftScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetLeft();
}

void QDeclarativeAnchorSet::resetLeft()
{
    Q_D(QDeclarativeAnchorSet);
    d->usedAnchors &= ~QDeclarativeAnchors::LeftAnchor;
    d->leftScript = QDeclarativeScriptString();
    d->resetAnchors |= QDeclarativeAnchors::LeftAnchor;
}

QDeclarativeScriptString QDeclarativeAnchorSet::right() const
{
    Q_D(const QDeclarativeAnchorSet);
    return d->rightScript;
}

void QDeclarativeAnchorSet::setRight(const QDeclarativeScriptString &edge)
{
    Q_D(QDeclarativeAnchorSet);
    d->usedAnchors |= QDeclarativeAnchors::RightAnchor;
    d->rightScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetRight();
}

void QDeclarativeAnchorSet::resetRight()
{
    Q_D(QDeclarativeAnchorSet);
    d->usedAnchors &= ~QDeclarativeAnchors::RightAnchor;
    d->rightScript = QDeclarativeScriptString();
    d->resetAnchors |= QDeclarativeAnchors::RightAnchor;
}

QDeclarativeScriptString QDeclarativeAnchorSet::horizontalCenter() const
{
    Q_D(const QDeclarativeAnchorSet);
    return d->hCenterScript;
}

void QDeclarativeAnchorSet::setHorizontalCenter(const QDeclarativeScriptString &edge)
{
    Q_D(QDeclarativeAnchorSet);
    d->usedAnchors |= QDeclarativeAnchors::HCenterAnchor;
    d->hCenterScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetHorizontalCenter();
}

void QDeclarativeAnchorSet::resetHorizontalCenter()
{
    Q_D(QDeclarativeAnchorSet);
    d->usedAnchors &= ~QDeclarativeAnchors::HCenterAnchor;
    d->hCenterScript = QDeclarativeScriptString();
    d->resetAnchors |= QDeclarativeAnchors::HCenterAnchor;
}

QDeclarativeItem *QDeclarativeAnchorSet::fill() const
{
    Q_D(const QDeclarativeAnchorSet);
    return d->fill;
}

void QDeclarativeAnchorSet::setFill(QDeclarativeItem *f)
{
    Q_D(QDeclarativeAnchorSet);
    d->fill = f;
}

void QDeclarativeAnchorSet::resetFill()
{
    setFill(0);
}

QDeclarativeItem *QDeclarativeAnchorSet::centerIn() const
{
    Q_D(const QDeclarativeAnchorSet);
    return d->centerIn;
}

void QDeclarativeAnchorSet::setCenterIn(QDeclarativeItem* c)
{
    Q_D(QDeclarativeAnchorSet);
    d->centerIn = c;
}

void QDeclarativeAnchorSet::resetCenterIn()
{
    setCenterIn(0);
}


class QDeclarativeAnchorChangesPrivate : public QObjectPrivate
{
public:
    QDeclarativeAnchorChangesPrivate()
        : target(0), anchorSet(new QDeclarativeAnchorSet),
          leftBinding(0), rightBinding(0), hCenterBinding(0),
          topBinding(0), bottomBinding(0), vCenterBinding(0), baselineBinding(0),
          origLeftBinding(0), origRightBinding(0), origHCenterBinding(0),
          origTopBinding(0), origBottomBinding(0), origVCenterBinding(0),
          origBaselineBinding(0)
    {

    }
    ~QDeclarativeAnchorChangesPrivate() { delete anchorSet; }

    QDeclarativeItem *target;
    QDeclarativeAnchorSet *anchorSet;

    QDeclarativeBinding *leftBinding;
    QDeclarativeBinding *rightBinding;
    QDeclarativeBinding *hCenterBinding;
    QDeclarativeBinding *topBinding;
    QDeclarativeBinding *bottomBinding;
    QDeclarativeBinding *vCenterBinding;
    QDeclarativeBinding *baselineBinding;

    QDeclarativeAbstractBinding *origLeftBinding;
    QDeclarativeAbstractBinding *origRightBinding;
    QDeclarativeAbstractBinding *origHCenterBinding;
    QDeclarativeAbstractBinding *origTopBinding;
    QDeclarativeAbstractBinding *origBottomBinding;
    QDeclarativeAbstractBinding *origVCenterBinding;
    QDeclarativeAbstractBinding *origBaselineBinding;

    QDeclarativeAnchorLine rewindLeft;
    QDeclarativeAnchorLine rewindRight;
    QDeclarativeAnchorLine rewindHCenter;
    QDeclarativeAnchorLine rewindTop;
    QDeclarativeAnchorLine rewindBottom;
    QDeclarativeAnchorLine rewindVCenter;
    QDeclarativeAnchorLine rewindBaseline;

    qreal fromX;
    qreal fromY;
    qreal fromWidth;
    qreal fromHeight;

    qreal toX;
    qreal toY;
    qreal toWidth;
    qreal toHeight;

    qreal rewindX;
    qreal rewindY;
    qreal rewindWidth;
    qreal rewindHeight;

    bool applyOrigLeft;
    bool applyOrigRight;
    bool applyOrigHCenter;
    bool applyOrigTop;
    bool applyOrigBottom;
    bool applyOrigVCenter;
    bool applyOrigBaseline;

    QList<QDeclarativeAbstractBinding*> oldBindings;

    QDeclarativeProperty leftProp;
    QDeclarativeProperty rightProp;
    QDeclarativeProperty hCenterProp;
    QDeclarativeProperty topProp;
    QDeclarativeProperty bottomProp;
    QDeclarativeProperty vCenterProp;
    QDeclarativeProperty baselineProp;
};

/*!
    \qmlproperty Item AnchorChanges::target
    This property holds the Item whose anchors will change
*/

QDeclarativeAnchorChanges::QDeclarativeAnchorChanges(QObject *parent)
 : QDeclarativeStateOperation(*(new QDeclarativeAnchorChangesPrivate), parent)
{
}

QDeclarativeAnchorChanges::~QDeclarativeAnchorChanges()
{
}

QDeclarativeAnchorChanges::ActionList QDeclarativeAnchorChanges::actions()
{
    Q_D(QDeclarativeAnchorChanges);
    d->leftBinding = d->rightBinding = d->hCenterBinding = d->topBinding
                   = d->bottomBinding = d->vCenterBinding = d->baselineBinding = 0;

    d->leftProp = QDeclarativeProperty(d->target, QLatin1String("anchors.left"));
    d->rightProp = QDeclarativeProperty(d->target, QLatin1String("anchors.right"));
    d->hCenterProp = QDeclarativeProperty(d->target, QLatin1String("anchors.horizontalCenter"));
    d->topProp = QDeclarativeProperty(d->target, QLatin1String("anchors.top"));
    d->bottomProp = QDeclarativeProperty(d->target, QLatin1String("anchors.bottom"));
    d->vCenterProp = QDeclarativeProperty(d->target, QLatin1String("anchors.verticalCenter"));
    d->baselineProp = QDeclarativeProperty(d->target, QLatin1String("anchors.baseline"));

    if (d->anchorSet->d_func()->usedAnchors & QDeclarativeAnchors::LeftAnchor) {
        d->leftBinding = new QDeclarativeBinding(d->anchorSet->d_func()->leftScript.script(), d->target, qmlContext(this));
        d->leftBinding->setTarget(d->leftProp);
    }
    if (d->anchorSet->d_func()->usedAnchors & QDeclarativeAnchors::RightAnchor) {
        d->rightBinding = new QDeclarativeBinding(d->anchorSet->d_func()->rightScript.script(), d->target, qmlContext(this));
        d->rightBinding->setTarget(d->rightProp);
    }
    if (d->anchorSet->d_func()->usedAnchors & QDeclarativeAnchors::HCenterAnchor) {
        d->hCenterBinding = new QDeclarativeBinding(d->anchorSet->d_func()->hCenterScript.script(), d->target, qmlContext(this));
        d->hCenterBinding->setTarget(d->hCenterProp);
    }
    if (d->anchorSet->d_func()->usedAnchors & QDeclarativeAnchors::TopAnchor) {
        d->topBinding = new QDeclarativeBinding(d->anchorSet->d_func()->topScript.script(), d->target, qmlContext(this));
        d->topBinding->setTarget(d->topProp);
    }
    if (d->anchorSet->d_func()->usedAnchors & QDeclarativeAnchors::BottomAnchor) {
        d->bottomBinding = new QDeclarativeBinding(d->anchorSet->d_func()->bottomScript.script(), d->target, qmlContext(this));
        d->bottomBinding->setTarget(d->bottomProp);
    }
    if (d->anchorSet->d_func()->usedAnchors & QDeclarativeAnchors::VCenterAnchor) {
        d->vCenterBinding = new QDeclarativeBinding(d->anchorSet->d_func()->vCenterScript.script(), d->target, qmlContext(this));
        d->vCenterBinding->setTarget(d->vCenterProp);
    }
    if (d->anchorSet->d_func()->usedAnchors & QDeclarativeAnchors::BaselineAnchor) {
        d->baselineBinding = new QDeclarativeBinding(d->anchorSet->d_func()->baselineScript.script(), d->target, qmlContext(this));
        d->baselineBinding->setTarget(d->baselineProp);
    }

    QDeclarativeAction a;
    a.event = this;
    return ActionList() << a;
}

QDeclarativeAnchorSet *QDeclarativeAnchorChanges::anchors()
{
    Q_D(QDeclarativeAnchorChanges);
    return d->anchorSet;
}

QDeclarativeItem *QDeclarativeAnchorChanges::object() const
{
    Q_D(const QDeclarativeAnchorChanges);
    return d->target;
}

void QDeclarativeAnchorChanges::setObject(QDeclarativeItem *target)
{
    Q_D(QDeclarativeAnchorChanges);
    d->target = target;
}

/*!
    \qmlproperty AnchorLine AnchorChanges::anchors.left
    \qmlproperty AnchorLine AnchorChanges::anchors.right
    \qmlproperty AnchorLine AnchorChanges::anchors.horizontalCenter
    \qmlproperty AnchorLine AnchorChanges::anchors.top
    \qmlproperty AnchorLine AnchorChanges::anchors.bottom
    \qmlproperty AnchorLine AnchorChanges::anchors.verticalCenter
    \qmlproperty AnchorLine AnchorChanges::anchors.baseline

    These properties change the respective anchors of the item.

    To reset an anchor you can assign \c undefined:
    \qml
    AnchorChanges {
        target: myItem
        left: undefined          //remove myItem's left anchor
        right: otherItem.right
    }
    \endqml
*/

void QDeclarativeAnchorChanges::execute(Reason reason)
{
    Q_D(QDeclarativeAnchorChanges);
    if (!d->target)
        return;

    //incorporate any needed "reverts"
    if (d->applyOrigLeft) {
        if (!d->origLeftBinding)
            d->target->anchors()->resetLeft();
        QDeclarativePropertyPrivate::setBinding(d->leftProp, d->origLeftBinding);
    }
    if (d->applyOrigRight) {
        if (!d->origRightBinding)
            d->target->anchors()->resetRight();
        QDeclarativePropertyPrivate::setBinding(d->rightProp, d->origRightBinding);
    }
    if (d->applyOrigHCenter) {
        if (!d->origHCenterBinding)
            d->target->anchors()->resetHorizontalCenter();
        QDeclarativePropertyPrivate::setBinding(d->hCenterProp, d->origHCenterBinding);
    }
    if (d->applyOrigTop) {
        if (!d->origTopBinding)
            d->target->anchors()->resetTop();
        QDeclarativePropertyPrivate::setBinding(d->topProp, d->origTopBinding);
    }
    if (d->applyOrigBottom) {
        if (!d->origBottomBinding)
            d->target->anchors()->resetBottom();
        QDeclarativePropertyPrivate::setBinding(d->bottomProp, d->origBottomBinding);
    }
    if (d->applyOrigVCenter) {
        if (!d->origVCenterBinding)
            d->target->anchors()->resetVerticalCenter();
        QDeclarativePropertyPrivate::setBinding(d->vCenterProp, d->origVCenterBinding);
    }
    if (d->applyOrigBaseline) {
        if (!d->origBaselineBinding)
            d->target->anchors()->resetBaseline();
        QDeclarativePropertyPrivate::setBinding(d->baselineProp, d->origBaselineBinding);
    }

    //destroy old bindings
    if (reason == ActualChange) {
        for (int i = 0; i < d->oldBindings.size(); ++i) {
            QDeclarativeAbstractBinding *binding = d->oldBindings.at(i);
            if (binding)
                binding->destroy();
        }
        d->oldBindings.clear();
    }

    //reset any anchors that have been specified as "undefined"
    if (d->anchorSet->d_func()->resetAnchors & QDeclarativeAnchors::LeftAnchor) {
        d->target->anchors()->resetLeft();
        QDeclarativePropertyPrivate::setBinding(d->leftProp, 0);
    }
    if (d->anchorSet->d_func()->resetAnchors & QDeclarativeAnchors::RightAnchor) {
        d->target->anchors()->resetRight();
        QDeclarativePropertyPrivate::setBinding(d->rightProp, 0);
    }
    if (d->anchorSet->d_func()->resetAnchors & QDeclarativeAnchors::HCenterAnchor) {
        d->target->anchors()->resetHorizontalCenter();
        QDeclarativePropertyPrivate::setBinding(d->hCenterProp, 0);
    }
    if (d->anchorSet->d_func()->resetAnchors & QDeclarativeAnchors::TopAnchor) {
        d->target->anchors()->resetTop();
        QDeclarativePropertyPrivate::setBinding(d->topProp, 0);
    }
    if (d->anchorSet->d_func()->resetAnchors & QDeclarativeAnchors::BottomAnchor) {
        d->target->anchors()->resetBottom();
        QDeclarativePropertyPrivate::setBinding(d->bottomProp, 0);
    }
    if (d->anchorSet->d_func()->resetAnchors & QDeclarativeAnchors::VCenterAnchor) {
        d->target->anchors()->resetVerticalCenter();
        QDeclarativePropertyPrivate::setBinding(d->vCenterProp, 0);
    }
    if (d->anchorSet->d_func()->resetAnchors & QDeclarativeAnchors::BaselineAnchor) {
        d->target->anchors()->resetBaseline();
        QDeclarativePropertyPrivate::setBinding(d->baselineProp, 0);
    }

    //set any anchors that have been specified
    if (d->leftBinding)
        QDeclarativePropertyPrivate::setBinding(d->leftBinding->property(), d->leftBinding);
    if (d->rightBinding)
        QDeclarativePropertyPrivate::setBinding(d->rightBinding->property(), d->rightBinding);
    if (d->hCenterBinding)
        QDeclarativePropertyPrivate::setBinding(d->hCenterBinding->property(), d->hCenterBinding);
    if (d->topBinding)
        QDeclarativePropertyPrivate::setBinding(d->topBinding->property(), d->topBinding);
    if (d->bottomBinding)
        QDeclarativePropertyPrivate::setBinding(d->bottomBinding->property(), d->bottomBinding);
    if (d->vCenterBinding)
        QDeclarativePropertyPrivate::setBinding(d->vCenterBinding->property(), d->vCenterBinding);
    if (d->baselineBinding)
        QDeclarativePropertyPrivate::setBinding(d->baselineBinding->property(), d->baselineBinding);
}

bool QDeclarativeAnchorChanges::isReversable()
{
    return true;
}

void QDeclarativeAnchorChanges::reverse(Reason reason)
{
    Q_D(QDeclarativeAnchorChanges);
    if (!d->target)
        return;

    //reset any anchors set by the state
    if (d->leftBinding) {
        d->target->anchors()->resetLeft();
        QDeclarativePropertyPrivate::setBinding(d->leftBinding->property(), 0);
        if (reason == ActualChange) {
            d->leftBinding->destroy(); d->leftBinding = 0;
        }
    }
    if (d->rightBinding) {
        d->target->anchors()->resetRight();
        QDeclarativePropertyPrivate::setBinding(d->rightBinding->property(), 0);
        if (reason == ActualChange) {
            d->rightBinding->destroy(); d->rightBinding = 0;
        }
    }
    if (d->hCenterBinding) {
        d->target->anchors()->resetHorizontalCenter();
        QDeclarativePropertyPrivate::setBinding(d->hCenterBinding->property(), 0);
        if (reason == ActualChange) {
            d->hCenterBinding->destroy(); d->hCenterBinding = 0;
        }
    }
    if (d->topBinding) {
        d->target->anchors()->resetTop();
        QDeclarativePropertyPrivate::setBinding(d->topBinding->property(), 0);
        if (reason == ActualChange) {
            d->topBinding->destroy(); d->topBinding = 0;
        }
    }
    if (d->bottomBinding) {
        d->target->anchors()->resetBottom();
        QDeclarativePropertyPrivate::setBinding(d->bottomBinding->property(), 0);
        if (reason == ActualChange) {
            d->bottomBinding->destroy(); d->bottomBinding = 0;
        }
    }
    if (d->vCenterBinding) {
        d->target->anchors()->resetVerticalCenter();
        QDeclarativePropertyPrivate::setBinding(d->vCenterBinding->property(), 0);
        if (reason == ActualChange) {
            d->vCenterBinding->destroy(); d->vCenterBinding = 0;
        }
    }
    if (d->baselineBinding) {
        d->target->anchors()->resetBaseline();
        QDeclarativePropertyPrivate::setBinding(d->baselineBinding->property(), 0);
        if (reason == ActualChange) {
            d->baselineBinding->destroy(); d->baselineBinding = 0;
        }
    }

    //restore previous anchors
    if (d->origLeftBinding)
        QDeclarativePropertyPrivate::setBinding(d->leftProp, d->origLeftBinding);
    if (d->origRightBinding)
        QDeclarativePropertyPrivate::setBinding(d->rightProp, d->origRightBinding);
    if (d->origHCenterBinding)
        QDeclarativePropertyPrivate::setBinding(d->hCenterProp, d->origHCenterBinding);
    if (d->origTopBinding)
        QDeclarativePropertyPrivate::setBinding(d->topProp, d->origTopBinding);
    if (d->origBottomBinding)
        QDeclarativePropertyPrivate::setBinding(d->bottomProp, d->origBottomBinding);
    if (d->origVCenterBinding)
        QDeclarativePropertyPrivate::setBinding(d->vCenterProp, d->origVCenterBinding);
    if (d->origBaselineBinding)
        QDeclarativePropertyPrivate::setBinding(d->baselineProp, d->origBaselineBinding);
}

QString QDeclarativeAnchorChanges::typeName() const
{
    return QLatin1String("AnchorChanges");
}

QList<QDeclarativeAction> QDeclarativeAnchorChanges::additionalActions()
{
    Q_D(QDeclarativeAnchorChanges);
    QList<QDeclarativeAction> extra;

    if (d->target) {
        QDeclarativeAction a;
        if (d->fromX != d->toX) {
            a.property = QDeclarativeProperty(d->target, QLatin1String("x"));
            a.toValue = d->toX;
            extra << a;
        }
        if (d->fromY != d->toY) {
            a.property = QDeclarativeProperty(d->target, QLatin1String("y"));
            a.toValue = d->toY;
            extra << a;
        }
        if (d->fromWidth != d->toWidth) {
            a.property = QDeclarativeProperty(d->target, QLatin1String("width"));
            a.toValue = d->toWidth;
            extra << a;
        }
        if (d->fromHeight != d->toHeight) {
            a.property = QDeclarativeProperty(d->target, QLatin1String("height"));
            a.toValue = d->toHeight;
            extra << a;
        }
    }

    return extra;
}

bool QDeclarativeAnchorChanges::changesBindings()
{
    return true;
}

void QDeclarativeAnchorChanges::saveOriginals()
{
    Q_D(QDeclarativeAnchorChanges);
    if (!d->target)
        return;

    d->origLeftBinding = QDeclarativePropertyPrivate::binding(d->leftProp);
    d->origRightBinding = QDeclarativePropertyPrivate::binding(d->rightProp);
    d->origHCenterBinding = QDeclarativePropertyPrivate::binding(d->hCenterProp);
    d->origTopBinding = QDeclarativePropertyPrivate::binding(d->topProp);
    d->origBottomBinding = QDeclarativePropertyPrivate::binding(d->bottomProp);
    d->origVCenterBinding = QDeclarativePropertyPrivate::binding(d->vCenterProp);
    d->origBaselineBinding = QDeclarativePropertyPrivate::binding(d->baselineProp);

    d->applyOrigLeft = d->applyOrigRight = d->applyOrigHCenter = d->applyOrigTop
      = d->applyOrigBottom = d->applyOrigVCenter = d->applyOrigBaseline = false;

    saveCurrentValues();
}

void QDeclarativeAnchorChanges::copyOriginals(QDeclarativeActionEvent *other)
{
    Q_D(QDeclarativeAnchorChanges);
    QDeclarativeAnchorChanges *ac = static_cast<QDeclarativeAnchorChanges*>(other);
    QDeclarativeAnchorChangesPrivate *acp = ac->d_func();

    QDeclarativeAnchors::Anchors combined = acp->anchorSet->d_func()->usedAnchors |
                                            acp->anchorSet->d_func()->resetAnchors;

    //probably also need to revert some things
    d->applyOrigLeft = (combined & QDeclarativeAnchors::LeftAnchor);
    d->applyOrigRight = (combined & QDeclarativeAnchors::RightAnchor);
    d->applyOrigHCenter = (combined & QDeclarativeAnchors::HCenterAnchor);
    d->applyOrigTop = (combined & QDeclarativeAnchors::TopAnchor);
    d->applyOrigBottom = (combined & QDeclarativeAnchors::BottomAnchor);
    d->applyOrigVCenter = (combined & QDeclarativeAnchors::VCenterAnchor);
    d->applyOrigBaseline = (combined & QDeclarativeAnchors::BaselineAnchor);

    d->origLeftBinding = acp->origLeftBinding;
    d->origRightBinding = acp->origRightBinding;
    d->origHCenterBinding = acp->origHCenterBinding;
    d->origTopBinding = acp->origTopBinding;
    d->origBottomBinding = acp->origBottomBinding;
    d->origVCenterBinding = acp->origVCenterBinding;
    d->origBaselineBinding = acp->origBaselineBinding;

    d->oldBindings.clear();
    d->oldBindings << acp->leftBinding << acp->rightBinding << acp->hCenterBinding
                << acp->topBinding << acp->bottomBinding << acp->baselineBinding;

    saveCurrentValues();
}

void QDeclarativeAnchorChanges::clearBindings()
{
    Q_D(QDeclarativeAnchorChanges);
    if (!d->target)
        return;

    d->fromX = d->target->x();
    d->fromY = d->target->y();
    d->fromWidth = d->target->width();
    d->fromHeight = d->target->height();

    //reset any anchors with corresponding reverts
    //reset any anchors that have been specified as "undefined"
    //reset any anchors that we'll be setting in the state
    QDeclarativeAnchors::Anchors combined = d->anchorSet->d_func()->resetAnchors |
                                            d->anchorSet->d_func()->usedAnchors;
    if (d->applyOrigLeft || (combined & QDeclarativeAnchors::LeftAnchor)) {
        d->target->anchors()->resetLeft();
        QDeclarativePropertyPrivate::setBinding(d->leftProp, 0);
    }
    if (d->applyOrigRight || (combined & QDeclarativeAnchors::RightAnchor)) {
        d->target->anchors()->resetRight();
        QDeclarativePropertyPrivate::setBinding(d->rightProp, 0);
    }
    if (d->applyOrigHCenter || (combined & QDeclarativeAnchors::HCenterAnchor)) {
        d->target->anchors()->resetHorizontalCenter();
        QDeclarativePropertyPrivate::setBinding(d->hCenterProp, 0);
    }
    if (d->applyOrigTop || (combined & QDeclarativeAnchors::TopAnchor)) {
        d->target->anchors()->resetTop();
        QDeclarativePropertyPrivate::setBinding(d->topProp, 0);
    }
    if (d->applyOrigBottom || (combined & QDeclarativeAnchors::BottomAnchor)) {
        d->target->anchors()->resetBottom();
        QDeclarativePropertyPrivate::setBinding(d->bottomProp, 0);
    }
    if (d->applyOrigVCenter || (combined & QDeclarativeAnchors::VCenterAnchor)) {
        d->target->anchors()->resetVerticalCenter();
        QDeclarativePropertyPrivate::setBinding(d->vCenterProp, 0);
    }
    if (d->applyOrigBaseline || (combined & QDeclarativeAnchors::BaselineAnchor)) {
        d->target->anchors()->resetBaseline();
        QDeclarativePropertyPrivate::setBinding(d->baselineProp, 0);
    }
}

bool QDeclarativeAnchorChanges::override(QDeclarativeActionEvent*other)
{
    if (other->typeName() != QLatin1String("AnchorChanges"))
        return false;
    if (static_cast<QDeclarativeActionEvent*>(this) == other)
        return true;
    if (static_cast<QDeclarativeAnchorChanges*>(other)->object() == object())
        return true;
    return false;
}

void QDeclarativeAnchorChanges::rewind()
{
    Q_D(QDeclarativeAnchorChanges);
    if (!d->target)
        return;

    //restore previous anchors
    if (d->rewindLeft.anchorLine != QDeclarativeAnchorLine::Invalid)
        d->target->anchors()->setLeft(d->rewindLeft);
    if (d->rewindRight.anchorLine != QDeclarativeAnchorLine::Invalid)
        d->target->anchors()->setRight(d->rewindRight);
    if (d->rewindHCenter.anchorLine != QDeclarativeAnchorLine::Invalid)
        d->target->anchors()->setHorizontalCenter(d->rewindHCenter);
    if (d->rewindTop.anchorLine != QDeclarativeAnchorLine::Invalid)
        d->target->anchors()->setTop(d->rewindTop);
    if (d->rewindBottom.anchorLine != QDeclarativeAnchorLine::Invalid)
        d->target->anchors()->setBottom(d->rewindBottom);
    if (d->rewindVCenter.anchorLine != QDeclarativeAnchorLine::Invalid)
        d->target->anchors()->setVerticalCenter(d->rewindVCenter);
    if (d->rewindBaseline.anchorLine != QDeclarativeAnchorLine::Invalid)
        d->target->anchors()->setBaseline(d->rewindBaseline);

    d->target->setX(d->rewindX);
    d->target->setY(d->rewindY);
    d->target->setWidth(d->rewindWidth);
    d->target->setHeight(d->rewindHeight);
}

void QDeclarativeAnchorChanges::saveCurrentValues()
{
    Q_D(QDeclarativeAnchorChanges);
    if (!d->target)
        return;

    d->rewindLeft = d->target->anchors()->left();
    d->rewindRight = d->target->anchors()->right();
    d->rewindHCenter = d->target->anchors()->horizontalCenter();
    d->rewindTop = d->target->anchors()->top();
    d->rewindBottom = d->target->anchors()->bottom();
    d->rewindVCenter = d->target->anchors()->verticalCenter();
    d->rewindBaseline = d->target->anchors()->baseline();

    d->rewindX = d->target->x();
    d->rewindY = d->target->y();
    d->rewindWidth = d->target->width();
    d->rewindHeight = d->target->height();
}

void QDeclarativeAnchorChanges::saveTargetValues()
{
    Q_D(QDeclarativeAnchorChanges);
    if (!d->target)
        return;

    d->toX = d->target->x();
    d->toY = d->target->y();
    d->toWidth = d->target->width();
    d->toHeight = d->target->height();
}

#include <qdeclarativestateoperations.moc>
#include <moc_qdeclarativestateoperations_p.cpp>

QT_END_NAMESPACE
