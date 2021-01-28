/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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
****************************************************************************/

#ifndef QDOCCOMMANDLINEPARSER_H
#define QDOCCOMMANDLINEPARSER_H

#include <QtCore/qcommandlineparser.h>

QT_BEGIN_NAMESPACE

struct QDocCommandLineParser : public QCommandLineParser
{
    QDocCommandLineParser();
    void process(const QStringList &arguments);

    QCommandLineOption defineOption, dependsOption, highlightingOption;
    QCommandLineOption showInternalOption, redirectDocumentationToDevNullOption;
    QCommandLineOption noExamplesOption, indexDirOption, installDirOption;
    QCommandLineOption obsoleteLinksOption, outputDirOption, outputFormatOption;
    QCommandLineOption noLinkErrorsOption, autoLinkErrorsOption, debugOption;
    QCommandLineOption prepareOption, generateOption, logProgressOption;
    QCommandLineOption singleExecOption, writeQaPagesOption;
    QCommandLineOption includePathOption, includePathSystemOption, frameworkOption;
    QCommandLineOption timestampsOption, useDocBookExtensions;
};

QT_END_NAMESPACE

#endif // QDOCCOMMANDLINEPARSER_H
