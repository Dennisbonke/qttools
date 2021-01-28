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

/*
  jscodemarker.h
*/

#ifndef JSCODEMARKER_H
#define JSCODEMARKER_H

#include "qmlcodemarker.h"

QT_BEGIN_NAMESPACE

class JsCodeMarker : public QmlCodeMarker
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::JsCodeMarker)

public:
    JsCodeMarker();
    ~JsCodeMarker() override;

    bool recognizeCode(const QString &code) override;
    bool recognizeExtension(const QString &ext) override;
    bool recognizeLanguage(const QString &language) override;
    Atom::AtomType atomType() const override;

    virtual QString markedUpCode(const QString &code, const Node *relative,
                                 const Location &location) override;

private:
    QString addMarkUp(const QString &code, const Node *relative, const Location &location);
};

QT_END_NAMESPACE

#endif
