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

#ifndef JSONGENERATOR
#define JSONGENERATOR

#include "logging.h"
#include "package.h"

#include <QtCore/qtextstream.h>
#include <QtCore/qvector.h>

namespace JsonGenerator {

void generate(QTextStream &out, const QVector<Package> &packages, LogLevel logLevel);

} // namespace JsonGenerator

#endif // JSONGENERATOR
