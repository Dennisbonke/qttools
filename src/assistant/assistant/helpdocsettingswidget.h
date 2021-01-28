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

#ifndef HELPDOCSETTINGSWIDGET_H
#define HELPDOCSETTINGSWIDGET_H

#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class HelpDocSettings;
class HelpDocSettingsWidgetPrivate;

class HelpDocSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    HelpDocSettingsWidget(QWidget *parent = nullptr);

    ~HelpDocSettingsWidget();

    void setDocSettings(const HelpDocSettings &settings);
    HelpDocSettings docSettings() const;

Q_SIGNALS:
    void docSettingsChanged(const HelpDocSettings &settings);

private:
    QScopedPointer<class HelpDocSettingsWidgetPrivate> d_ptr;
    Q_DECLARE_PRIVATE(HelpDocSettingsWidget)
    Q_DISABLE_COPY_MOVE(HelpDocSettingsWidget)
};

QT_END_NAMESPACE

#endif

