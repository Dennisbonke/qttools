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

#ifndef HELPDOCSETTINGS_H
#define HELPDOCSETTINGS_H

#include <QtCore/QSharedDataPointer>

QT_BEGIN_NAMESPACE

class QVersionNumber;
class QHelpEngineCore;
class HelpDocSettingsPrivate;

class HelpDocSettings final
{
public:
    HelpDocSettings();
    HelpDocSettings(const HelpDocSettings &other);
    HelpDocSettings(HelpDocSettings &&other);
    ~HelpDocSettings();

    HelpDocSettings &operator=(const HelpDocSettings &other);
    HelpDocSettings &operator=(HelpDocSettings &&other);

    void swap(HelpDocSettings &other) noexcept
    { d.swap(other.d); }

    bool addDocumentation(const QString &fileName);
    bool removeDocumentation(const QString &namespaceName);
    QString namespaceName(const QString &fileName) const;
    QStringList components() const;
    QList<QVersionNumber> versions() const;
    QStringList namespaces() const;
    QMap<QString, QString> namespaceToFileName() const;

    static HelpDocSettings readSettings(QHelpEngineCore *helpEngine);
    static bool applySettings(QHelpEngineCore *helpEngine, const HelpDocSettings &settings);

private:
    QSharedDataPointer<HelpDocSettingsPrivate> d;
};

QT_END_NAMESPACE

#endif // HELPDOCSETTINGS_H
