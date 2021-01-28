/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Assistant of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QHELPSEARCHRESULTWIDGET_H
#define QHELPSEARCHRESULTWIDGET_H

#include <QtHelp/qhelpsearchengine.h>
#include <QtHelp/qhelp_global.h>

#include <QtCore/QUrl>
#include <QtCore/QPoint>
#include <QtCore/QObject>

#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class QHelpSearchResultWidgetPrivate;

class QHELP_EXPORT QHelpSearchResultWidget : public QWidget
{
    Q_OBJECT

public:
    ~QHelpSearchResultWidget() override;
    QUrl linkAt(const QPoint &point);

Q_SIGNALS:
    void requestShowLink(const QUrl &url);

private:
    friend class QHelpSearchEngine;

    QHelpSearchResultWidgetPrivate *d;
    QHelpSearchResultWidget(QHelpSearchEngine *engine);
    void changeEvent(QEvent *event) override;
};

QT_END_NAMESPACE

#endif  // QHELPSEARCHRESULTWIDGET_H
